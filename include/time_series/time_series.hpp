/**
 * @file time_series.hpp
 * @author Vincent Berenz
 * license License BSD-3-Clause
 * @copyright Copyright (c) 2019, Max Planck Gesellschaft.
 */

#pragma once

#include <chrono>
#include <cmath>

#include "real_time_tools/timer.hpp"

// virtual class specifying all functions
// a time_series class should implement
// Defines also Index and Timestamp
#include "time_series/interface.hpp"

// all common code to TimeSeries and
// multiprocesses TimeSeries
#include "time_series/internal/base.hpp"

// Mutex, ConditionVariable, Lock and Vector
// use different implementation for TimeSeries and
// MultiprocessesTimeSeries. Those are defined there.
#include "time_series/internal/specialized_classes.hpp"

namespace time_series
{
/**
 * @brief Threadsafe time series
 */
template <typename T = int>
class TimeSeries : public internal::TimeSeriesBase<internal::SingleProcess, T>
{
public:
    TimeSeries(size_t max_length,
               Index start_timeindex = 0,
               bool throw_on_sigint = true)
        : internal::TimeSeriesBase<internal::SingleProcess, T>(start_timeindex,
                                                               throw_on_sigint)
    {
        this->mutex_ptr_ =
            std::make_shared<internal::Mutex<internal::SingleProcess> >();
        this->condition_ptr_ = std::make_shared<
            internal::ConditionVariable<internal::SingleProcess> >();
        this->history_elements_ptr_ =
            std::make_shared<internal::Vector<internal::SingleProcess, T> >(
                max_length);
        this->history_timestamps_ptr_ = std::make_shared<
            internal::Vector<internal::SingleProcess, Timestamp> >(max_length);
    }

protected:
    void read_indexes() const
    {
    }
    void write_indexes()
    {
    }
};
}  // namespace time_series
