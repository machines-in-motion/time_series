#include <gtest/gtest.h>
#include <atomic>

#include "real_time_tools/mutex.hpp"
#include "real_time_tools/thread.hpp"
#include "real_time_tools/timer.hpp"

#include "time_series/multiprocess_time_series.hpp"
#include "time_series/time_series.hpp"

// declares and defines class Type, which
// instances will be used as elements of the time_series
#include "ut_type.hpp"

/******************************************************************************/
// Preparation

#define TIME_SERIES_MAX_SIZE 200
#define SEGMENT_ID "parallel_time_series_unittests"

std::shared_ptr<time_series::TimeSeriesInterface<Type> >
construct_a_time_series(const bool& multiprocess)
{
    std::shared_ptr<time_series::TimeSeriesInterface<Type> > ts;
    if (multiprocess)
    {
        ts = std::make_shared<time_series::MultiprocessTimeSeries<Type> >(
            SEGMENT_ID, TIME_SERIES_MAX_SIZE, true /*clear_on_destruction*/);
    }
    else
    {
        ts = std::make_shared<time_series::TimeSeries<Type> >(
            TIME_SERIES_MAX_SIZE);
    }
    return ts;
}

/******************************************************************************/
// Unit tests

TEST(parallel_time_series, monitor_signal_thread)
{
    // Init task.
    std::shared_ptr<time_series::TimeSeriesInterface<Type> > ts =
        construct_a_time_series(/* multiprocess = */ false);

    // Do a task that hangs.
    rt_printf("Notify the sigint to fire.\n");
    signal_handler::SignalHandler::signal_handler(SIGINT);

    // Test the behavior.
    try
    {
        (*ts)[time_series::Index(TIME_SERIES_MAX_SIZE / 2)];
        FAIL() << "Expected an instance of 'signal_handler::ReceivedSignal'.";
    }
    catch (signal_handler::ReceivedSignal const& err)
    {
        EXPECT_TRUE(std::string(err.what()).find("Received signal SIGINT") !=
                    std::string::npos);
    }
    catch (...)
    {
        FAIL() << "Expected an instance of 'signal_handler::ReceivedSignal'.";
    }

    // No hang, success!
    EXPECT_TRUE(true);

    // Reset the signal handler
    signal_handler::SignalHandler::reset();
}

// below : multiprocess does not indicate processes will be spawned
// instead of threads. It means the multiprocesses version of the TimeSeries
// API will be used, i.e. separated instances of TimeSeries communicating
// via shared memory (as opposed to thread sharing a common instance of
// TimeSeries)

TEST(parallel_time_series, monitor_signal_multiprocess)
{
}
