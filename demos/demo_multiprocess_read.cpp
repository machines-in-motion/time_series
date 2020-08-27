/**
 * @file demo_multiprocess_read.cpp
 * @author Vincent Berenz
 * @copyright Copyright (c) 2019, Max Planck Gesellschaft.
 *
 * @brief Read data from a shared timed series.
 * This demo does nothing until demo_multiprocess_write
 * is started. ctrl+c for exit.
 *
 */

#include "shared_memory/demos/item.hpp"
#include "time_series/multiprocess_time_series.hpp"

#define SEGMENT_ID "demo_time_series_multiprocess"
#define TIMESERIES_SIZE 100

typedef time_series::MultiprocessTimeSeries<shared_memory::Item<10>> TimeSeries;

/**
 * @brief read (and print) items written by demo_multiprocess_write
 */
void run()
{
    // in case previous run did not exit cleanly
    time_series::clear_memory(SEGMENT_ID);

    // warning: any "demo_multiprocess_write" demo running
    // may hang or crash when this process exit
    TimeSeries ts = TimeSeries::create_leader(SEGMENT_ID, TIMESERIES_SIZE);

    while (true)
    {
        time_series::Index index = ts.newest_timeindex();
        shared_memory::Item<10> item = ts[index];
        item.compact_print();
        std::cout << "\n";
        ts.wait_for_timeindex(index + 1);
    }
}

int main()
{
    run();
    return 0;
}
