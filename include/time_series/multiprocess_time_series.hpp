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
            std::make_shared<internal::Mutex<internal::MultiProcesses> >(
                segment_id + internal::shm_mutex, leader);
        this->condition_ptr_ = std::make_shared<
            internal::ConditionVariable<internal::MultiProcesses> >(
            segment_id + internal::shm_condition_variable, leader);
        this->history_elements_ptr_ =
            std::make_shared<internal::Vector<internal::MultiProcesses, T> >(
                max_length, segment_id + internal::shm_elements, leader);
        this->history_timestamps_ptr_ = std::make_shared<
            internal::Vector<internal::MultiProcesses, Timestamp> >(
            max_length, segment_id + internal::shm_timestamps, leader);
        if (leader)
        {
            write_indexes();
        }
    }

protected:
    void read_indexes()
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

    shared_memory::array<Index> indexes_;
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
