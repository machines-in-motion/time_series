// Copyright (c) 2019 Max Planck Gesellschaft
// Vincent Berenz

#pragma once

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

#include "signal_handler/exceptions.hpp"
#include "signal_handler/signal_handler.hpp"

#include "time_series/interface.hpp"
#include "time_series/internal/specialized_classes.hpp"

#include "real_time_tools/timer.hpp"

namespace time_series
{
namespace internal
{
// implement all the code common to multithread time_series
// and multiprocesses time_series

// P will be expected to be SINGLEPROCESS or MULTIPROCESS
// (types defined in specialized_classes.hpp)

template <typename P, typename T = int>
class TimeSeriesBase : public TimeSeriesInterface<T>
{
public:
    /**
     * @brief Constructor.
     *
     * @param start_timeindex  Time index of the first element.
     * @param throw_on_sigint  If true, a signal_handler::ReceivedSignal
     *     exception is thrown when a SIGINT signal is received while waiting in
     *     one of the getter methods.
     */
    TimeSeriesBase(Index start_timeindex = 0, bool throw_on_sigint = true);
    TimeSeriesBase(TimeSeriesBase<P, T> &&other) noexcept;
    ~TimeSeriesBase();
    Index newest_timeindex(bool wait = true) const;
    Index count_appended_elements() const;
    Index oldest_timeindex(bool wait = false) const;
    T newest_element() const;
    T operator[](const Index &timeindex) const;
    Timestamp timestamp_ms(const Index &timeindex) const;
    Timestamp timestamp_s(const Index &timeindex) const;
    bool wait_for_timeindex(const Index &timeindex,
                            const double &max_duration_s =
                                std::numeric_limits<double>::quiet_NaN()) const;
    size_t length() const;
    size_t max_length() const;
    bool has_changed_since_tag() const;
    void tag(const Index &timeindex);
    Index tagged_timeindex() const;
    void append(const T &element);
    bool is_empty() const;

protected:
    // in case of multiprocesses: will be used to keep
    // indexes values aligned for all instances
    virtual void read_indexes() const = 0;
    virtual void write_indexes() = 0;

    mutable Index start_timeindex_;
    mutable Index oldest_timeindex_;
    mutable Index newest_timeindex_;
    mutable Index tagged_timeindex_;

protected:
    // non shared variable. initialized at true,
    // and switched to false when an element is observed
    // in the time series. Used only in the "is_empty" method.
    mutable bool empty_;

protected:
    // see specialized_classes.hpp for
    // implementations depending on P
    // (SINGLEPROCESS or MULTIPROCESS)
    std::shared_ptr<Mutex<P> > mutex_ptr_;
    std::shared_ptr<ConditionVariable<P> > condition_ptr_;
    std::shared_ptr<Vector<P, T> > history_elements_ptr_;
    std::shared_ptr<Vector<P, Timestamp> > history_timestamps_ptr_;

private:
    std::thread signal_monitor_thread_;
    //! Set to true in destructor to indicate thread that it should terminate.
    std::atomic<bool> is_destructor_called_;

    //! If true an exception is thrown if a SIGINT is received while waiting in
    //! one of the methods.
    bool throw_on_sigint_;

    /**
     * @brief Monitors if SIGINT was received and releases the lock if yes.
     *
     * Loops until SIGINT was received.  When this happens, the condition_ptr_
     * lock is released to prevent the lock from blocking application shut down.
     */
    void monitor_signal();

protected:
    //! @brief Throw a ReceivedSignal exception if SIGINT was received.
    void throw_if_sigint_received() const;
};

#include "base.hxx"
}  // namespace internal
}  // namespace time_series
