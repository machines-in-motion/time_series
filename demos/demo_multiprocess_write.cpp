/**
 * @file demo_multiprocess_write.cpp
 * @author Vincent Berenz
 * @copyright Copyright (c) 2019, Max Planck Gesellschaft.
 *
 * @brief Write data into a shared timed series. The demo
 * demo_multiprocess_read is expected to be already running
 * when this demo is started. Infinite hanging may occur otherwhise
 *
 */

#include "shared_memory/demos/item.hpp"
#include "time_series/multiprocess_time_series.hpp"

#define SEGMENT_ID "demo_time_series_multiprocess"

typedef time_series::MultiprocessTimeSeries<shared_memory::Item<10>> TIMESERIES;

void run()
{
    // because the instance created by demo_process_read
    // will do this
    bool clean_on_destruction = false;

    size_t max_length = TIMESERIES::get_max_length(SEGMENT_ID);

    TIMESERIES ts(SEGMENT_ID, max_length, clean_on_destruction);

    for (int i = 0; i < 100; i++)
    {
        // Item implements a serialize function, which is required
        // for the usage of multiprocess time series.
        shared_memory::Item<10> item(i);
        item.compact_print();
        std::cout << "\n";
        ts.append(item);
        real_time_tools::Timer::sleep_ms(400);
    }
}

int main()
{
    run();
    return 0;
}
