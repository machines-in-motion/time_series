/**
 * @file demo_time_series.cpp
 * @author Vincent Berenz
 * @copyright Copyright (c) 2019, Max Planck Gesellshaft.
 *
 * @brief basic usage of time series
 *
 */

#include "real_time_tools/thread.hpp"
#include "time_series/time_series.hpp"

bool RUNNING = true;

/**
 * @brief Write values to the time series
 */
void *producer(void *args)
{
    time_series::TimeSeries<int> &ts =
        *static_cast<time_series::TimeSeries<int> *>(args);
    for (int i = 0; i < 20; i++)
    {
        ts.append(i);
        real_time_tools::Timer::sleep_ms(100);
    }
    RUNNING = false;
    return nullptr;
}

/**
 * @brief Read and display the values from the time series
 */
void run()
{
    time_series::TimeSeries<int> ts(100);

    real_time_tools::RealTimeThread thread;
    thread.create_realtime_thread(&producer, &ts);

    while (RUNNING)
    {
        int value = ts.newest_element();
        std::cout << "-> " << value << "\n";
    }

    thread.join();
}

int main()
{
    run();
    return 0;
}
