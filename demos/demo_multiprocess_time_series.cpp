/**
 * @file demo_multiprocess_time_series.cpp
 * @author Vincent Berenz
 * @copyright Copyright (c) 2019, Max Planck Gesellschaft.
 *
 * @brief basic usage of multiprocess time series
 *
 */

#include "real_time_tools/thread.hpp"
#include "time_series/multiprocess_time_series.hpp"

#define SEGMENT_ID "demo_multiprocess_time_series"

bool g_running = true;

/**
 * @brief Write values to the time series
 */
THREAD_FUNCTION_RETURN_TYPE producer(void*)
{

  bool leader = false; // because not created first
  time_series::MultiprocessTimeSeries<int> ts(SEGMENT_ID,
					      100,
					      leader);
  for (int i = 0; i < 20; i++)
    {
      ts.append(i);
      real_time_tools::Timer::sleep_ms(100);
    }
  g_running = false;
  return THREAD_FUNCTION_RETURN_VALUE;
}

/**
 * @brief Read and display the values from the time series
 */
void run()
{

  bool leader = true; // because created first, in charge of
                      // initializing and destroying the shared
                      // memory segment
  time_series::MultiprocessTimeSeries<int> ts(SEGMENT_ID,
					      100,
					      leader);

  real_time_tools::RealTimeThread thread;
  thread.create_realtime_thread(&producer, &ts);

  while (g_running)
    {
      time_series::Index t = ts.newest_timeindex();
      int value = ts[t];
      std::cout << "-> " << value << "\n";
    }
  
  thread.join();
}

int main()
{
    run();
    return 0;
}
