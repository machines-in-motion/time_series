/**
 * @file demo_time_series.cpp
 * @author Vincent Berenz
 * @copyright Copyright (c) 2021, Max Planck Gesellschaft.
 *
 * @brief usage of the snapshot method of time series
 *
 */

#include "time_series/time_series.hpp"

/**
 * @brief adds some items to a time series and print a snapshot
 */
void run()
{
    time_series::TimeSeries<int> ts(100);

    for (int i = 0; i < 110; i++)
    {
        ts.append(i + 100);
    }

    std::vector<std::tuple<int, time_series::Index, time_series::Timestamp>>
        snap = ts.snapshot();
    std::size_t size = snap.size();

    // the two items of snap contains the values of the
    // newest and oldest elements
    time_series::Index newest = std::get<1>(snap[size-2]);
    time_series::Index oldest = std::get<1>(snap[size-1]);

    for (int i = 0; i < 100; i++)
    {
        std::tuple<int, time_series::Index, time_series::Timestamp> item =
            snap[i];
        int value = std::get<0>(item);
        time_series::Index timeindex = std::get<1>(item);
        time_series::Timestamp stamp = std::get<2>(item);
        if (timeindex == newest)
        {
            std::cout << "(newest) \t";
        }
        else if (timeindex == oldest)
        {
            std::cout << "(oldest) \t";
        }
        else
        {
            std::cout << "         \t";
        }
        std::cout << "data structure index: " << i << "\t|\t"
                  << "value(timeseries): " << ts[timeindex] << "\t"
                  << "value(snapshot): " << value << "\t"
                  << "timeindex: " << timeindex << "\t"
                  << "timestamp: " << stamp << "\n";
    }
}

int main()
{
    run();
    return 0;
}
