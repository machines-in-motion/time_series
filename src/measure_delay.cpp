/**
 * @file
 * @brief Small application to compare time delay of single- and multi-process
 *        time series.
 *
 * One thread/process writes the current time stamp to the time series at a
 * fixed rate.  The other one reads it, computes the elapsed time and prints
 * some simple analysis on this when finished.
 *
 * Expects a "mode" argument which has to be one of the following values:
 *
 * - "single": Using single-process time series and running sender and receiver
 *   in separate real-time threads.
 * - "multi_write": Using multi-process time series and running only the sender
 *   which writes to the time series (using a real-time thread).
 * - "multi_read": Using multi-process time series and running only the receiver
 *   which reads from the time series (using a real-time thread).  Also performs
 *   the analysis in the end.
 *
 * @copyright Copyright (c) 2020, Max Planck Gesellschaft.
 */

#include <Eigen/Eigen>
#include <limits>
#include <real_time_tools/thread.hpp>
#include <time_series/multiprocess_time_series.hpp>
#include <time_series/time_series.hpp>

const unsigned int NUM_STEPS = 10000;
Eigen::Array<double, NUM_STEPS, 1> g_delays;

/**
 * @brief Write time stamps to the time series
 */
void *send(void *args)
{
    // wait a bit to ensure the other loop is waiting
    real_time_tools::Timer::sleep_sec(1);

    time_series::TimeSeriesInterface<double> &ts =
        *static_cast<time_series::TimeSeriesInterface<double> *>(args);
    std::cout << "start transmitting" << std::endl;
    for (unsigned int i = 0; i < NUM_STEPS; i++)
    {
        ts.append(real_time_tools::Timer::get_current_time_sec());
        // TODO verify that the sleep here is long enough
        real_time_tools::Timer::sleep_sec(0.001);
    }
    // indicate end by sending a NaN
    ts.append(std::numeric_limits<double>::quiet_NaN());
    return nullptr;
}

void *receive(void *args)
{
    time_series::TimeSeriesInterface<double> &ts =
        *static_cast<time_series::TimeSeriesInterface<double> *>(args);

    size_t t = 0;
    std::cout << "ready for receiving" << std::endl;
    while (true)
    {
        double send_time = ts[t];
        double now = real_time_tools::Timer::get_current_time_sec();
        if (std::isnan(send_time))
        {
            break;
        }

        g_delays[t] = now - send_time;
        t++;
    }
    return nullptr;
}

enum Mode
{
    SINGLE,
    MULTI_WRITE,
    MULTI_READ
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Expect on of the following values as argument: single, "
                     "multi_write, multi_read"
                  << std::endl;
        return -1;
    }

    Mode mode;
    if (strcmp(argv[1], "single") == 0)
    {
        std::cout << "Single-process mode." << std::endl;
        mode = SINGLE;
    }
    else if (strcmp(argv[1], "multi_write") == 0)
    {
        std::cout << "Multi-process write mode." << std::endl;
        mode = MULTI_WRITE;
    }
    else if (strcmp(argv[1], "multi_read") == 0)
    {
        std::cout << "Multi-process read mode." << std::endl;
        mode = MULTI_READ;
    }
    else
    {
        std::cerr << "Invalid argument" << std::endl;
        return -1;
    }

    real_time_tools::RealTimeThread thread_send, thread_receive;

    switch (mode)
    {
        case SINGLE:
        {
            time_series::TimeSeries<double> ts(100);

            thread_receive.create_realtime_thread(&receive, &ts);
            thread_send.create_realtime_thread(&send, &ts);

            thread_receive.join();
            thread_send.join();
        }
        break;

        case MULTI_WRITE:
        {
            time_series::MultiprocessTimeSeries<double> ts(
                "measure_delay", 100, false);

            thread_send.create_realtime_thread(&send, &ts);

            thread_send.join();
        }
        break;

        case MULTI_READ:
        {
            time_series::clear_memory("measure_delay");
            time_series::MultiprocessTimeSeries<double> ts(
                "measure_delay", 100, true);

            thread_receive.create_realtime_thread(&receive, &ts);

            thread_receive.join();
        }
        break;

        default:
            // do nothing
            break;
    }

    if (mode == SINGLE || mode == MULTI_READ)
    {
        // analyse measured delays
        double std_dev = std::sqrt((g_delays - g_delays.mean()).square().sum() /
                                   (g_delays.size() - 1));
        std::cout << "Mean delay: " << g_delays.mean() << std::endl;
        std::cout << "Min. delay: " << g_delays.minCoeff() << std::endl;
        std::cout << "Max. delay: " << g_delays.maxCoeff() << std::endl;
        std::cout << "std dev:    " << std_dev << std::endl;
    }

    return 0;
}

