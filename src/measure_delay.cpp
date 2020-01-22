/**
 * @file
 * @brief Small application to compare time delay of single- and multi-process
 *        time series.
 * @copyright Copyright (c) 2020, Max Planck Gesellschaft.
 */

#include <Eigen/Eigen>
#include <limits>
#include <real_time_tools/thread.hpp>
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

    time_series::TimeSeries<double> &ts =
        *static_cast<time_series::TimeSeries<double> *>(args);
    std::cout << "start transmitting" << std::endl;
    for (unsigned int i = 0; i < NUM_STEPS; i++)
    {
        double now = real_time_tools::Timer::get_current_time_sec();
        ts.append(now);
        real_time_tools::Timer::sleep_until_sec(now + 0.001);
    }
    // indicate end by sending a NaN
    ts.append(std::numeric_limits<double>::quiet_NaN());
    return nullptr;
}

void *receive(void *args)
{
    time_series::TimeSeries<double> &ts =
        *static_cast<time_series::TimeSeries<double> *>(args);

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

/**
 * @brief Read and display the values from the time series
 */
void run()
{
    time_series::TimeSeries<double> ts(100);

    real_time_tools::RealTimeThread thread_send, thread_receive;

    thread_receive.create_realtime_thread(&receive, &ts);
    thread_send.create_realtime_thread(&send, &ts);

    thread_receive.join();
    thread_send.join();

    // for (size_t i = 0; i < NUM_STEPS; i++) {
    //    std::cout << g_delays[i] << std::endl;
    //}
    double std_dev = std::sqrt((g_delays - g_delays.mean()).square().sum() /
                               (g_delays.size() - 1));
    std::cout << "Mean delay: " << g_delays.mean() << std::endl;
    std::cout << "Min. delay: " << g_delays.minCoeff() << std::endl;
    std::cout << "Max. delay: " << g_delays.maxCoeff() << std::endl;
    std::cout << "std dev:    " << std_dev << std::endl;
}

int main()
{
    run();
    return 0;
}

