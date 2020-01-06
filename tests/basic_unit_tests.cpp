
#include <gtest/gtest.h>
#include <eigen3/Eigen/Core>

#include "time_series/multiprocess_time_series.hpp"
#include "time_series/time_series.hpp"

#include "real_time_tools/mutex.hpp"
#include "real_time_tools/thread.hpp"
#include "real_time_tools/timer.hpp"

// class Type, used as elements of some time_series
#include "ut_type.hpp"

#define SEGMENT_ID "basic_time_series_unittests"

using namespace real_time_tools;
using namespace time_series;

TEST(time_series_ut, basic)
{
    TimeSeries<int> ts1(100);
    ts1.append(10);
    Index index = ts1.newest_timeindex();
    int value = ts1[index];
    ASSERT_EQ(value, 10);
}

TEST(time_series_ut, basic_multi_processes)
{
    clear_memory(SEGMENT_ID);
    MultiprocessTimeSeries<int> ts1(SEGMENT_ID, 100, true);
    MultiprocessTimeSeries<int> ts2(SEGMENT_ID, 100, false);
    ts1.append(10);
    Index index1 = ts1.newest_timeindex();
    Index index2 = ts2.newest_timeindex();
    ASSERT_EQ(index1, index2);
    int value = ts2[index2];
    ASSERT_EQ(value, 10);
    ts1.append(20);
    ts1.append(30);
    index1 = ts1.newest_timeindex();
    index2 = ts2.newest_timeindex();
    ASSERT_EQ(index1, index2);
    value = ts2[index2];
    ASSERT_EQ(value, 30);
}

TEST(time_series_ut, serialized_multi_processes)
{
    clear_memory(SEGMENT_ID);

    MultiprocessTimeSeries<Type> ts1(SEGMENT_ID, 100, true);
    MultiprocessTimeSeries<Type> ts2(SEGMENT_ID, 100, false);

    Type type1;
    ts1.append(type1);
    Index index1 = ts1.newest_timeindex();
    Index index2 = ts2.newest_timeindex();
    ASSERT_EQ(index1, index2);

    Type type2 = ts2[index2];
    ASSERT_EQ(type1, type2);
}

TEST(time_series_ut, full_round)
{
    clear_memory(SEGMENT_ID);

    MultiprocessTimeSeries<Type> ts1(SEGMENT_ID, 100, true);
    MultiprocessTimeSeries<Type> ts2(SEGMENT_ID, 100, false);
    for (int i = 0; i < 101; i++)
    {
        Type type;
        ts1.append(type);
    }

    Index index1 = ts1.newest_timeindex();
    Index index2 = ts2.newest_timeindex();
    ASSERT_EQ(index1, index2);

    Type type1 = ts1[index1];
    Type type2 = ts2[index2];
    ASSERT_EQ(type1, type2);
}

void *add_element(void *args)
{
    TimeSeries<int> &ts = *static_cast<TimeSeries<int> *>(args);
    usleep(1000);
    ts.append(20);
}

TEST(time_series_ut, basic_newest_element)
{
    TimeSeries<int> ts(100);
    RealTimeThread thread;
    thread.create_realtime_thread(&add_element, &ts);
    int value = ts.newest_element();
    ASSERT_EQ(value, 20);
    thread.join();
    ts.append(30);
    value = ts.newest_element();
    ASSERT_EQ(value, 30);
    RealTimeThread thread2;
    thread2.create_realtime_thread(&add_element, &ts);
    usleep(3000);
    value = ts.newest_element();
    ASSERT_EQ(value, 20);
    thread2.join();
}

void *add_element_mp(void *)
{
    bool clear_on_destruction = false;
    MultiprocessTimeSeries<int> ts(SEGMENT_ID, 100, clear_on_destruction);
    usleep(2000);
    ts.append(20);
}

TEST(time_series_ut, multiprocesses_newest_element)
{
    clear_memory(SEGMENT_ID);
    bool clear_on_destruction = true;
    MultiprocessTimeSeries<int> ts(SEGMENT_ID, 100, clear_on_destruction);
    RealTimeThread thread;
    thread.create_realtime_thread(&add_element_mp);
    int value = ts.newest_element();
    ASSERT_EQ(value, 20);
    thread.join();
}

TEST(time_series_ut, count_appended_elements)
{
    TimeSeries<int> ts(100);
    for (int i = 0; i < 205; i++)
    {
        ts.append(i);
    }
    int count = ts.count_appended_elements();
    ASSERT_EQ(count, 205);
}

void *to_time_index(void *)
{
    bool clear_on_destruction = false;
    MultiprocessTimeSeries<int> ts(SEGMENT_ID, 100, clear_on_destruction);
    Index target_index = 10;
    Index index = 0;
    int c = 0;
    ts.append(c);
    c++;
    while (ts.newest_timeindex() != (target_index + 2))
    {
        ts.append(c);
        c++;
        usleep(200);
    }
}

TEST(time_series_ut, wait_for_time_index)
{
    clear_memory(SEGMENT_ID);
    bool clear_on_destruction = true;
    MultiprocessTimeSeries<int> ts(SEGMENT_ID, 100, clear_on_destruction);
    RealTimeThread thread;
    thread.create_realtime_thread(&to_time_index);
    Index target_index = 10;
    ts.wait_for_timeindex(10, 1);
    int value = ts[target_index];
    ASSERT_EQ(value, 10);
    thread.join();
}

TEST(time_series_ut, tag)
{
    TimeSeries<int> ts(100);
    ts.append(10);
    Index index = ts.newest_timeindex();
    ts.tag(index);
    bool changed = ts.has_changed_since_tag();
    ASSERT_FALSE(changed);
    ts.append(20);
    changed = ts.has_changed_since_tag();
    ASSERT_TRUE(changed);
}

TEST(time_series_ut, timestamps)
{
    TimeSeries<int> ts(100);
    ts.append(10);
    Index index = ts.newest_timeindex();
    Timestamp stamp_ms = ts.timestamp_ms(index);
    Timestamp stamp_s = ts.timestamp_s(index);
    ASSERT_EQ(stamp_ms, stamp_s * 1000);
    usleep(1000);
    ts.append(10);
    Timestamp stamp_ms2 = ts.timestamp_ms(index);
    ASSERT_LT(stamp_ms2, stamp_ms + 1);
}
