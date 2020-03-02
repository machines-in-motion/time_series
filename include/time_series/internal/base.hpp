// Copyright (c) 2019 Max Planck Gesellschaft
// Vincent Berenz

#pragma once

#include <chrono>
#include <cmath>

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
    TimeSeriesBase(Index start_timeindex = 0);
    Index newest_timeindex();
    Index count_appended_elements();
    Index oldest_timeindex();
    T newest_element();
    T operator[](const Index &timeindex);
    Timestamp timestamp_ms(const Index &timeindex);
    Timestamp timestamp_s(const Index &timeindex);
    bool wait_for_timeindex(const Index &timeindex,
                            const double &max_duration_s =
                                std::numeric_limits<double>::quiet_NaN());
    size_t length();
    size_t max_length();
    bool has_changed_since_tag();
    void tag(const Index &timeindex);
    void append(const T &element);

protected:
    // in case of multiprocesses: will be used to keep
    // indexes values aligned for all instances
    virtual void read_indexes() = 0;
    virtual void write_indexes() = 0;

    Index start_timeindex_;
    Index oldest_timeindex_;
    Index newest_timeindex_;
    Index tagged_timeindex_;

protected:
    // see specialized_classes.hpp for
    // implementations depending on P
    // (SINGLEPROCESS or MULTIPROCESS)
    std::shared_ptr<Mutex<P> > mutex_ptr_;
    std::shared_ptr<ConditionVariable<P> > condition_ptr_;
    std::shared_ptr<Vector<P, T> > history_elements_ptr_;
    std::shared_ptr<Vector<P, Timestamp> > history_timestamps_ptr_;
};

#include "base.hxx"
}
}
