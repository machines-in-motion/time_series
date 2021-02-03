/**
 * @file multiprocesses_time_series.hpp
 * @author Vincent Berenz
 * license License BSD-3-Clause
 * @copyright Copyright (c) 2019, Max Planck Gesellschaft.
 */

#pragma once

// virtual class specifying all functions
// a time_series class should implement
// Defines also Index and Timestamp
#include "time_series/interface.hpp"

// all common code to TimeSeries and
// multiprocesses TimeSeries
#include "time_series/internal/base.hpp"

// Mutex, ConditionVariable, Lock and Vector
// use different implementation for TimeSeries and
// MultiprocessTimeSeries. Those are defined there.
#include "time_series/internal/specialized_classes.hpp"

namespace time_series
{
// various shared memory segments are created based on the
// segment_id provided by the user. Here are the corresponding
// suffixes
namespace internal
{
static const std::string shm_indexes("_indexes");
static const std::string shm_elements("_elements");
static const std::string shm_timestamps("_timestamps");
static const std::string shm_mutex("_mutex");
static const std::string shm_condition_variable("_condition_variable");
}  // namespace internal

/**
 * Multiprocess Time Series. Several instances hosted
 * by different processes, if pointing
 * to the same shared memory segment (as specified by the segment_id),
 * may read/write from the same underlying time series.
 */
template <typename T = int>
class MultiprocessTimeSeries
    : public internal::TimeSeriesBase<internal::MultiProcesses, T>
{
public:
    /**
     * @deprecated uses the factory functions create_leader or
     *             create_follower
     * @brief create a new instance pointing to the specified shared
     * memory segment
     * @param segment_id the id of the segment to point to
     * @param max_length max number of elements in the time series
     * @param leader if true, the shared memory segment will initialize
     * the shared time series, and wiped the related shared memory on
     * destruction.
     * Instantiating a  first MultiprocessTimeSeries with leader set to false
     * will result in undefined behavior. When the leader instance is destroyed,
     * other instances are pointing to the shared segment may crash or hang.
     */
    MultiprocessTimeSeries(std::string segment_id,
                           size_t max_length,
                           bool leader = true,
                           Index start_timeindex = 0)
        : internal::TimeSeriesBase<internal::MultiProcesses, T>(
              start_timeindex),
          indexes_(segment_id + internal::shm_indexes, 4, leader, false)
    {
        this->mutex_ptr_ =
            std::make_shared<internal::Mutex<internal::MultiProcesses>>(
                segment_id + internal::shm_mutex, leader);
        this->condition_ptr_ = std::make_shared<
            internal::ConditionVariable<internal::MultiProcesses>>(
            segment_id + internal::shm_condition_variable, leader);
        this->history_elements_ptr_ =
            std::make_shared<internal::Vector<internal::MultiProcesses, T>>(
                max_length, segment_id + internal::shm_elements, leader);
        this->history_timestamps_ptr_ = std::make_shared<
            internal::Vector<internal::MultiProcesses, Timestamp>>(
            max_length, segment_id + internal::shm_timestamps, leader);
        if (leader)
        {
            write_indexes();
        }
        if (leader)
        {
            // sharing the max_length in the shared memory
            // (follower can query size for proper construction)
            shared_memory::set<size_t>(segment_id, "max_length", max_length);
            shared_memory::set<Index>(
                segment_id, "start_timeindex", start_timeindex);
        }
    }

    MultiprocessTimeSeries(MultiprocessTimeSeries<T>&& other) noexcept
        : internal::TimeSeriesBase<internal::MultiProcesses, T>(
              std::forward<MultiprocessTimeSeries<T>>(other)),
          indexes_(other.indexes_)
    {
    }

    /**
     * returns the max length used by a leading MultiprocessTimeSeries
     * of the corresponding segment_id
     */
    static size_t get_max_length(const std::string& segment_id)
    {
        size_t s;
        shared_memory::get<size_t>(segment_id, "max_length", s);
        return s;
    }

    /**
     * returns the start index used by a leading MultiprocessTimeSeries
     * of the corresponding segment_id
     */
    static Index get_start_timeindex(const std::string& segment_id)
    {
        Index index;
        shared_memory::get<Index>(segment_id, "start_timeindex", index);
        return index;
    }

    /**
     * returns a leader instance of MultiprocessTimeSeries<T>
     * @param segment_id the id of the segment to point to
     * @param max_length max number of elements in the time series
     */
    static MultiprocessTimeSeries<T> create_leader(
        const std::string& segment_id,
        size_t max_length,
        Index start_timeindex = 0)
    {
        bool leader = true;
        return MultiprocessTimeSeries<T>(
            segment_id, max_length, leader, start_timeindex);
    }

    //! @brief same as create_leader but returning a shared_ptr.
    static std::shared_ptr<MultiprocessTimeSeries<T>> create_leader_ptr(
        const std::string& segment_id,
        size_t max_length,
        Index start_timeindex = 0)
    {
        bool leader = true;
        return std::make_shared<MultiprocessTimeSeries<T>>(
            segment_id, max_length, leader, start_timeindex);
    }

    /**
     * returns a follower instance of MultiprocessTimeSeries<T>.
     * An follower instance should be created only if a leader
     * instance has been created first. A std::runtime_error will
     * be thrown otherwise.
     * @param segment_id the id of the segment to point to
     */
    static MultiprocessTimeSeries<T> create_follower(
        const std::string& segment_id)
    {
        bool leader = false;
        Index start_timeindex;
        size_t max_length;
        get_max_length_and_start_index_from_leader(
            segment_id, &max_length, &start_timeindex);

        return MultiprocessTimeSeries<T>(
            segment_id, max_length, leader, start_timeindex);
    }

    //! @brief same as create_follower but returning a shared_ptr.
    static std::shared_ptr<MultiprocessTimeSeries<T>> create_follower_ptr(
        const std::string& segment_id)
    {
        bool leader = false;
        Index start_timeindex;
        size_t max_length;
        get_max_length_and_start_index_from_leader(
            segment_id, &max_length, &start_timeindex);

        return std::make_shared<MultiprocessTimeSeries<T>>(
            segment_id, max_length, leader, start_timeindex);
    }

    /**
     * similar to the random access operator, but does not deserialized the
     * accessed element. If the element is of a fundamental type (or an array
     * of), an std::logic_error is thrown.
     */
    std::string get_raw(const Index& timeindex)
    {
        internal::Lock<internal::MultiProcesses> lock(*this->mutex_ptr_);
        read_indexes();
        if (timeindex < this->oldest_timeindex_)
        {
            throw std::invalid_argument(
                "you tried to access time_series element which is too old.");
        }

        while (this->newest_timeindex_ < timeindex)
        {
            this->throw_if_sigint_received();

            this->condition_ptr_->wait(lock);
            read_indexes();
        }

        return this->history_elements_ptr_->get_serialized(
            timeindex % this->history_elements_ptr_->size());
    }

protected:
    void read_indexes() const
    {
        indexes_.get(0, this->start_timeindex_);
        indexes_.get(1, this->oldest_timeindex_);
        indexes_.get(2, this->newest_timeindex_);
        indexes_.get(3, this->tagged_timeindex_);
    }

    void write_indexes()
    {
        indexes_.set(0, this->start_timeindex_);
        indexes_.set(1, this->oldest_timeindex_);
        indexes_.set(2, this->newest_timeindex_);
        indexes_.set(3, this->tagged_timeindex_);
    }

    /**
     * @brief Load length and start index from leader.
     *
     * Assumes that a leader time series is already running and providing this
     * information in the shared memory.
     *
     * @param[in]  segment_id The id of the segment to point to.
     * @param[out] max_length The max. length of the time series.
     * @param[out] start_timeindex
     * @throws std::runtime_error If the data cannot be read from the specified
     *     shared memory segment.
     */
    static void get_max_length_and_start_index_from_leader(
        const std::string& segment_id,
        size_t* max_length,
        Index* start_timeindex)
    {
        try
        {
            *start_timeindex =
                MultiprocessTimeSeries::get_start_timeindex(segment_id);
            *max_length = MultiprocessTimeSeries::get_max_length(segment_id);
        }
        catch (shared_memory::Unexpected_size_exception& e)
        {
            std::stringstream stream;
            stream << "failing to create follower multiprocess_time_series "
                      "with segment_id "
                   << segment_id << ": "
                   << "a corresponding leader should be started first";
            throw std::runtime_error(stream.str());
        }
    }

    mutable shared_memory::array<Index> indexes_;
};

/**
 * Wipe out the corresponding shared memory.
 * Useful if no instances of MultiprocessTimeSeries cleared
 * the memory on destruction.
 * Reusing the segment id of a non-wiped shared memory may result
 * in the newly created instance to hang.
 */
void clear_memory(std::string segment_id);
}  // namespace time_series
