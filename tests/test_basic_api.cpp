
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

TEST(time_series_ut, multi_processes_get_max_length)
{
    clear_memory(SEGMENT_ID);
    {
        MultiprocessTimeSeries<int> ts1(SEGMENT_ID, 100, true);
        size_t s = MultiprocessTimeSeries<int>::get_max_length(SEGMENT_ID);
        ASSERT_EQ(s, (size_t)100);
    }
    {
        MultiprocessTimeSeries<int> ts1(SEGMENT_ID, 200, true);
        size_t s = MultiprocessTimeSeries<int>::get_max_length(SEGMENT_ID);
        ASSERT_EQ(s, (size_t)200);
    }
}

TEST(time_series_ut, multi_processes_get_start_timeindex)
{
    clear_memory(SEGMENT_ID);
    {
        MultiprocessTimeSeries<int> ts1(SEGMENT_ID, 100, true, 25);
        Index index =
            MultiprocessTimeSeries<Index>::get_start_timeindex(SEGMENT_ID);
        ASSERT_EQ(index, 25);
    }
    {
        MultiprocessTimeSeries<int> ts1(SEGMENT_ID, 200, true, 32);
        Index index =
            MultiprocessTimeSeries<Index>::get_start_timeindex(SEGMENT_ID);
        ASSERT_EQ(index, 32);
    }
}

TEST(time_series_ut, factories)
{
    clear_memory(SEGMENT_ID);
    typedef MultiprocessTimeSeries<double> Mt;
    size_t max_length = 100;
    Index start_timeindex = 25;
    Mt leader = Mt::create_leader(SEGMENT_ID, max_length, start_timeindex);
    Mt follower1 = Mt::create_follower(SEGMENT_ID);
    Mt follower2 = Mt::create_follower(SEGMENT_ID);
    ASSERT_EQ(follower1.max_length(), max_length);
    ASSERT_EQ(follower2.max_length(), max_length);
    leader.append(1.0);
    ASSERT_EQ(follower1.newest_timeindex(), start_timeindex);
    ASSERT_EQ(follower2.newest_timeindex(), start_timeindex);
}


TEST(time_series_ut, serialized_multi_processes)
{
    clear_memory(SEGMENT_ID);

    typedef MultiprocessTimeSeries<Type> Mpt;

    Mpt ts1 = Mpt::create_leader(SEGMENT_ID, 100);
    Mpt ts2 = Mpt::create_follower(SEGMENT_ID);

    Type type1;
    ts1.append(type1);
    Index index1 = ts1.newest_timeindex();
    Index index2 = ts2.newest_timeindex();
    ASSERT_EQ(index1, index2);
    Type type2 = ts2[index2];
    ASSERT_TRUE(type1==type2);
}


TEST(time_series_ut, get_raw)
{
    clear_memory(SEGMENT_ID);
    typedef MultiprocessTimeSeries<Type> Mpt;
    Mpt ts1 = Mpt::create_leader(SEGMENT_ID, 100);
    Mpt ts2 = Mpt::create_follower(SEGMENT_ID);
    Type type1;
    ts1.append(type1);
    Index index2 = ts2.newest_timeindex();
    std::string serialized = ts2.get_raw(index2);
    shared_memory::Serializer<Type> serializer;
    Type type2;
    serializer.deserialize(serialized, type2);
    ASSERT_TRUE(type1==type2);
}

TEST(time_series_ut, full_round)
{
    clear_memory(SEGMENT_ID);

    typedef MultiprocessTimeSeries<Type> Mpt;

    Mpt ts1 = Mpt::create_leader(SEGMENT_ID, 100);
    Mpt ts2 = Mpt::create_follower(SEGMENT_ID);

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
    ASSERT_TRUE(type1==type2);
}

void *add_element(void *args)
{
    TimeSeries<int> &ts = *static_cast<TimeSeries<int> *>(args);
    usleep(1000);
    ts.append(20);
    return nullptr;
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

TEST(time_series_ut, newest_index_no_wait)
{
    TimeSeries<int> ts(100);
    time_series::Index index = ts.newest_timeindex(false);
    ASSERT_EQ(index, -1);
}


void *add_element_mp(void *)
{
    typedef MultiprocessTimeSeries<Type> Mpt;
    Mpt ts1 = Mpt::create_follower(SEGMENT_ID);
    usleep(2000);
    Type t;
    t.set(5, 10, 20.0);
    ts1.append(t);
    return nullptr;
}

TEST(time_series_ut, multiprocesses_newest_element)
{
    clear_memory(SEGMENT_ID);
    typedef MultiprocessTimeSeries<Type> Mpt;
    Mpt ts = Mpt::create_leader(SEGMENT_ID, 100);
    RealTimeThread thread;
    thread.create_realtime_thread(&add_element_mp);
    Type t = ts.newest_element();
    ASSERT_EQ(t.get(5, 10), 20.0);
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
    typedef MultiprocessTimeSeries<int> Mpt;
    Mpt ts = Mpt::create_follower(SEGMENT_ID);
    Index target_index = 10;
    int c = 0;
    ts.append(c);
    c++;
    while (ts.newest_timeindex() != (target_index + 2))
    {
        ts.append(c);
        c++;
        usleep(200);
    }
    return nullptr;
}



TEST(time_series_ut, wait_for_time_index)
{
    clear_memory(SEGMENT_ID);
    typedef MultiprocessTimeSeries<int> Mpt;
    Mpt ts = Mpt::create_leader(SEGMENT_ID, 100);
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
    Index tagged_index = ts.tagged_timeindex();
    ASSERT_EQ(tagged_index, index);
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

TEST(time_series_ut, empty)
{
    TimeSeries<int> ts(100);
    ASSERT_TRUE(ts.is_empty());
    ts.append(10);
    ASSERT_FALSE(ts.is_empty());
}

TEST(time_series_ut, multi_processes_empty)
{
    clear_memory(SEGMENT_ID);
    typedef MultiprocessTimeSeries<int> Mpt;
    Mpt ts1 = Mpt::create_leader(SEGMENT_ID, 100);
    Mpt ts2 = Mpt::create_follower(SEGMENT_ID);
    ASSERT_TRUE(ts1.is_empty());
    ASSERT_TRUE(ts2.is_empty());
    ts1.append(10);
    ASSERT_FALSE(ts1.is_empty());
    ASSERT_FALSE(ts2.is_empty());
}

