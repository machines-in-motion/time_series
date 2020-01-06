#include <gtest/gtest.h>

#include "time_series/multiprocess_time_series.hpp"
#include "time_series/time_series.hpp"

#include "real_time_tools/mutex.hpp"
#include "real_time_tools/thread.hpp"
#include "real_time_tools/timer.hpp"

// declares and defines class Type, which
// instances will be used as elements of the time_series
#include "ut_type.hpp"

#define SEGMENT_ID "parallel_time_series_unittests"

#define NB_INPUT_DATA 600
#define NB_OUTPUT_DATA 5
#define TIMESERIES_LENGTH 500
#define TIMESERIES_LENGTH_SLOW 100
#define SLEEP_MS_SLOW 1.0

using namespace real_time_tools;
using namespace time_series;

/**
 * @brief This class contain the thread data
 */
class ThreadData
{
public:
    /**
     * @brief Construct a new ThreadData object
     *
     * @param time_series_length is the size of the each buffer
     */
    ThreadData(TimeSeriesInterface<Type>* tsi)
        : length_(NB_INPUT_DATA),
          n_outputs_(NB_OUTPUT_DATA),
          time_series_(tsi),
          inputs_(length_),
          outputs_(n_outputs_),
          output_indices_(n_outputs_)
    {
        for (int i = 0; i < n_outputs_; i++)
        {
            outputs_[i] = std::vector<Type>(length_);
        }
    }
    ThreadData() : ThreadData(nullptr)
    {
    }
    /** @brief is the number of input data*/
    size_t length_;
    /** @brief is the number of output data*/
    size_t n_outputs_;
    /** @brief Is the input data buffer*/
    std::vector<Type> inputs_;
    /** @brief Is the mutex that provide safe access to the ouputs*/
    real_time_tools::RealTimeMutex outputs_mutex_;
    /** @brief The output data buffer*/
    std::vector<std::vector<Type>> outputs_;
    /** @brief The thread safe time series data buffer*/
    TimeSeriesInterface<Type>* time_series_;
    /** @brief is the indices of the buffer to be used in each threads*/
    std::vector<size_t> output_indices_;
};

/**
 * @brief This class is used to extract the data from a thread
 */
class OutputThreadData
{
public:
    /** @brief data_ This is a pointer to the data buffer */
    ThreadData* data_;
    /** @brief output_index_ This is the index in the data buffer */
    size_t output_index_;
    /** @brief slow down the writting thread */
    bool slow_;
    /** @brief id of the thread */
    int id;
};

/**
 * @brief This class gives the pointer to the data buffer to the thread and
 * define the speed of the thread.
 */
class InputThreadData
{
public:
    /** @brief Data buffer */
    ThreadData* data_;
    /** @brief slow down the writting thread */
    bool slow_;
};

size_t get_time_series_length(bool slow)
{
    if (slow)
    {
        return TIMESERIES_LENGTH_SLOW;
    }
    else
    {
        return TIMESERIES_LENGTH;
    }
}

void* input_to_time_series(void* void_ptr)
{
    InputThreadData& thread_data = *static_cast<InputThreadData*>(void_ptr);
    size_t length = thread_data.data_->length_;
    std::vector<Type>& inputs = thread_data.data_->inputs_;

    // cf comment in output_to_time_series function
    TimeSeriesInterface<Type>* time_series = thread_data.data_->time_series_;
    bool multiprocesses = false;
    if (time_series == nullptr)
    {
        multiprocesses = true;
        bool clear_on_destruction = false;
        time_series = new MultiprocessTimeSeries<Type>(
            SEGMENT_ID,
            get_time_series_length(thread_data.slow_),
            clear_on_destruction);
    }

    for (size_t i = 0; i < length; i++)
    {
        time_series->append(inputs[i]);
        if (thread_data.slow_)
        {
            real_time_tools::Timer::sleep_ms(SLEEP_MS_SLOW);
        }
        usleep(1);
    }

    if (multiprocesses)
    {
        delete time_series;
    }

    return nullptr;
}

void* time_series_to_output(void* void_ptr)
{
    OutputThreadData& thread_data = *static_cast<OutputThreadData*>(void_ptr);

    int id = thread_data.id;

    // if single process, the time_series is a member of OutputThreadData.data,
    // and shared accross threads.
    // if multiprocesses, each thread creates it own class of TimeSeries,
    // each pointing to a common shared memory
    bool multiprocesses = false;
    TimeSeriesInterface<Type>* time_series = thread_data.data_->time_series_;
    if (time_series == nullptr)
    {
        multiprocesses = true;
        bool clear_on_destruction = false;
        time_series = new MultiprocessTimeSeries<Type>(
            SEGMENT_ID,
            get_time_series_length(thread_data.slow_),
            clear_on_destruction);
    }

    size_t length = thread_data.data_->length_;
    size_t output_index = thread_data.output_index_;
    std::vector<std::vector<Type>>& outputs = thread_data.data_->outputs_;
    real_time_tools::RealTimeMutex& ouputs_mutex =
        thread_data.data_->outputs_mutex_;

    for (size_t i = 0; i < length; i++)
    {
        Type element;
        Index timeindex = i;
        element = (*time_series)[timeindex];
        ouputs_mutex.lock();
        outputs[output_index][i] = element;
        ouputs_mutex.unlock();
    }

    if (multiprocesses)
    {
        delete time_series;
    }

    return nullptr;
}

bool test_parallel_time_series_history(bool slow, bool multiprocesses)
{
    // for multiprocesses:  this time series used to setup indexes
    // and cleanup shared memory at the end of the test.
    // (clear_on_destruction is true)
    TimeSeriesInterface<Type>* master_time_series = nullptr;
    if (multiprocesses)
    {
        bool clear_on_destruction = true;
        master_time_series = new MultiprocessTimeSeries<Type>(
            SEGMENT_ID, get_time_series_length(slow), clear_on_destruction);
    }

    ThreadData* data = nullptr;
    TimeSeriesInterface<Type>* time_series = nullptr;

    size_t time_series_length = get_time_series_length(slow);

    // if not multiprocesses, creating here the time_series
    // that will be shared accross each thread.
    // (for multiprocesses time_series, each thread will
    // create its own instance)
    if (!multiprocesses)
    {
        time_series = new TimeSeries<Type>(time_series_length);
        data = new ThreadData(time_series);
    }
    else
    {
        data = new ThreadData();
    }

    std::vector<OutputThreadData> output_data(data->n_outputs_);
    InputThreadData input_data;
    input_data.data_ = data;
    input_data.slow_ = slow;

    std::vector<RealTimeThread> threads(data->n_outputs_ + 1);
    for (size_t i = 0; i < data->output_indices_.size(); i++)
    {
        // We provide the thread with all the infos needed
        output_data[i].id = i;
        output_data[i].data_ = data;
        output_data[i].output_index_ = i;
        output_data[i].slow_ = slow;
        // We start the consumers thread
        threads[i].create_realtime_thread(&time_series_to_output,
                                          &output_data[i]);
    }

    usleep(1000);

    // We start the feeding thread
    threads.back().create_realtime_thread(&input_to_time_series, &input_data);

    // wait for all the thread to finish
    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }

    // check that the outputs written by the individual threads
    // correspond to the input.
    for (size_t i = 0; i < data->n_outputs_; i++)
    {
        EXPECT_TRUE(data->inputs_ == data->outputs_[i]);
    }

    // sanity check
    data->inputs_[0].set(0, 0, 33.);
    EXPECT_FALSE(data->inputs_ == data->outputs_[0]);

    // cleanup
    if (time_series != nullptr)
    {
        delete time_series;
    }
    if (data != nullptr)
    {
        delete data;
    }
    if (master_time_series != nullptr)
    {
        delete master_time_series;
    }
    return true;
}

TEST(parallel_time_series, full_history)
{
    bool slow = false;
    bool multiprocess = false;
    bool res = test_parallel_time_series_history(slow, multiprocess);
    EXPECT_TRUE(res);
}

TEST(parallel_time_series, partial_history)
{
    bool slow = true;
    bool multiprocess = false;
    bool res = test_parallel_time_series_history(slow, multiprocess);
    EXPECT_TRUE(res);
}

// below : multiprocess does not indicate processes will be spawned
// instead of threads. It means the multiprocesses version of the TimeSeries
// API will be used, i.e. separated instances of TimeSeries communicating
// via shared memory (as opposed to thread sharing a common instance of
// TimeSeries)

TEST(parallel_time_series, multiprocesses_full_history)
{
    clear_memory(SEGMENT_ID);
    bool slow = false;
    bool multiprocess = true;
    bool res = test_parallel_time_series_history(slow, multiprocess);
    EXPECT_TRUE(res);
    clear_memory(SEGMENT_ID);
}

TEST(parallel_time_series, multiprocesses_partial_history)
{
    clear_memory(SEGMENT_ID);
    bool slow = true;
    bool multiprocess = true;
    bool res = test_parallel_time_series_history(slow, multiprocess);
    EXPECT_TRUE(res);
    clear_memory(SEGMENT_ID);
}
