// Copyright (c) 2019 Max Planck Gesellschaft
// Vincent Berenz

#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>

#include "shared_memory/array.hpp"
#include "shared_memory/condition_variable.hpp"
#include "shared_memory/lock.hpp"
#include "shared_memory/mutex.hpp"

namespace time_series
{
namespace internal
{
typedef std::integral_constant<int, 0> SingleProcess;
typedef std::integral_constant<int, 1> MultiProcesses;

// ------- Mutex ------- //

template <typename P>
class Mutex
{
};

template <>
class Mutex<SingleProcess>
{
public:
    Mutex()
    {
    }
    std::mutex mutex;
};

template <>
class Mutex<MultiProcesses>
{
public:
    Mutex(std::string mutex_id, bool clean_on_destruction = false)
        : mutex(mutex_id, clean_on_destruction)
    {
    }
    shared_memory::Mutex mutex;
};

// ------- Lock ------- //

template <typename P>
class Lock
{
};

template <>
class Lock<SingleProcess>
{
public:
    Lock(Mutex<SingleProcess> &mutex) : lock(mutex.mutex)
    {
    }
    std::unique_lock<std::mutex> lock;
};

template <>
class Lock<MultiProcesses>
{
public:
    Lock(Mutex<MultiProcesses> &mutex) : lock(mutex.mutex)
    {
    }
    shared_memory::Lock lock;
};

// ------- Condition variable ------- //

template <typename P>
class ConditionVariable
{
};

template <>
class ConditionVariable<SingleProcess>
{
public:
    ~ConditionVariable()
    {
        condition.notify_all();
    }
    void notify_all()
    {
        condition.notify_all();
    }
    void wait(Lock<SingleProcess> &lock)
    {
        condition.wait(lock.lock);
    }
    bool wait_for(Lock<SingleProcess> &lock, double max_duration_s)
    {
        std::chrono::duration<double> chrono_duration(max_duration_s);
        std::cv_status status = condition.wait_for(lock.lock, chrono_duration);
        return !(status == std::cv_status::timeout);
    }
    std::condition_variable condition;
};

template <>
class ConditionVariable<MultiProcesses>
{
public:
    ConditionVariable(std::string object_id, bool clear_on_destruction)
        : condition(object_id, clear_on_destruction)
    {
    }
    ~ConditionVariable()
    {
        condition.notify_all();
    }
    void notify_all()
    {
        condition.notify_all();
    }
    void wait(Lock<MultiProcesses> &lock)
    {
        condition.wait(lock.lock);
    }
    bool wait_for(Lock<MultiProcesses> &lock, double max_duration_s)
    {
        long wait_time = static_cast<long>(max_duration_s * 1e6);
        return condition.timed_wait(lock.lock, wait_time);
    }
    shared_memory::ConditionVariable condition;
};

// -------- items containers -------- //

template <typename P, typename T>
class Vector
{
};

// single process

template <typename T>
class Vector<SingleProcess, T>
{
public:
    Vector(std::size_t size) : v_(size)
    {
    }
    std::size_t size() const
    {
        return v_.size();
    }
    void get(int index, T &t)
    {
        t = v_[index];
    }
    std::string get_serialized(int index)
    {
        throw std::logic_error(
            "function not implemented for non multiprocess time series");
    }
    void set(int index, const T &t)
    {
        v_[index] = t;
    }

private:
    std::vector<T> v_;
};

// multi-processes

template <typename T>
class Vector<MultiProcesses, T>
{
public:
    Vector(std::size_t size,
           std::string segment_id,
           bool clear_on_destruction = true)
        : a_(segment_id, size, clear_on_destruction)
    {
    }
    std::size_t size() const
    {
        return a_.size();
    }
    void get(int index, T &t)
    {
        a_.get(index, t);
    }
    std::string get_serialized(int index)
    {
        return a_.get_serialized(index);
    }
    void set(int index, const T &t)
    {
        a_.set(index, t);
    }

private:
    shared_memory::array<T> a_;
};
}  // namespace internal
}  // namespace time_series
