/**
 * @file
 * @brief Small application to compare time delay of single- and multi-process
 *        time series.
 *
 * One thread/process writes the current time stamp to the time series at a
 * fixed rate.  The other one reads it, computes the elapsed time and prints
 * some simple analysis on this when finished.
 *
 * Expects a "mode" argument which has to be one of the following values:
 *
 * - "single": Using single-process time series and running sender and receiver
 *   in separate real-time threads.
 * - "multi_write": Using multi-process time series and running only the sender
 *   which writes to the time series (using a real-time thread).
 * - "multi_read": Using multi-process time series and running only the receiver
 *   which reads from the time series (using a real-time thread).  Also performs
 *   the analysis in the end.
 *
 * @copyright Copyright (c) 2020, Max Planck Gesellschaft.
 */

#include <fstream>
#include <iostream>

#include <Eigen/Eigen>
#include <limits>
#include <real_time_tools/process_manager.hpp>
#include <real_time_tools/thread.hpp>
#include <time_series/multiprocess_time_series.hpp>
#include <time_series/time_series.hpp>


//#define ADD_EIGEN_PAYLOAD 1


const unsigned int NUM_STEPS = 100000;


namespace cereal
{
template <class Archive, class Derived>
inline typename std::enable_if<
    traits::is_output_serializable<BinaryData<typename Derived::Scalar>,
                                   Archive>::value,
    void>::type
save(Archive& archive, const Eigen::PlainObjectBase<Derived>& object)
{
    typedef Eigen::PlainObjectBase<Derived> ArrT;

    // only add dimensions to the serialized data when they are dynamic
    if (ArrT::RowsAtCompileTime == Eigen::Dynamic)
    {
        archive(object.rows());
    }
    if (ArrT::ColsAtCompileTime == Eigen::Dynamic)
    {
        archive(object.cols());
    }

    archive(binary_data(object.data(),
                        object.size() * sizeof(typename Derived::Scalar)));
}

template <class Archive, class Derived>
inline typename std::enable_if<
    traits::is_input_serializable<BinaryData<typename Derived::Scalar>,
                                  Archive>::value,
    void>::type
load(Archive& archive, Eigen::PlainObjectBase<Derived>& object)
{
    typedef Eigen::PlainObjectBase<Derived> ArrT;

    Eigen::Index rows = ArrT::RowsAtCompileTime, cols = ArrT::ColsAtCompileTime;
    // information about dimensions are only serialized for dynamic-size types
    if (rows == Eigen::Dynamic)
    {
        archive(rows);
    }
    if (cols == Eigen::Dynamic)
    {
        archive(cols);
    }

    object.resize(rows, cols);
    archive(binary_data(object.data(),
                        static_cast<std::size_t>(
                            rows * cols * sizeof(typename Derived::Scalar))));
}
}  // namespace cereal


typedef Eigen::Matrix<double, 9, 1> Vector;

#ifdef ADD_EIGEN_PAYLOAD
struct Payload
{
    //! Timestamp set just before adding to time series
    double timestamp;

    //! Some random payload data, to have a comparable message size to the
    //! TriFinger robot observations.
    Vector data1;
    Vector data2;
    Vector data3;

    template <class Archive>
    void serialize(Archive &archive)
    {
        archive(timestamp, data1, data2, data3);
    }
};
#else
typedef double Payload;
#endif

typedef time_series::TimeSeriesInterface<Payload> TimeSeriesInterface;

std::array<double, NUM_STEPS> g_delays;
//Eigen::Array<double, NUM_STEPS, 1> g_delays;

/**
 * @brief Write time stamps to the time series
 */
void *send(void *args)
{
    real_time_tools::set_cpu_dma_latency(0);

#ifdef ADD_EIGEN_PAYLOAD
    std::cout << "Add Eigen vectors to payload" << std::endl;
#else
    std::cout << "Send raw timestamp" << std::endl;
#endif

    // wait a bit to ensure the other loop is waiting
    real_time_tools::Timer::sleep_sec(1);

    Payload payload;

    TimeSeriesInterface &ts = *static_cast<TimeSeriesInterface *>(args);
    std::cout << "start transmitting" << std::endl;
    for (unsigned int i = 0; i < NUM_STEPS; i++)
    {
#ifdef ADD_EIGEN_PAYLOAD
        payload.data1 = Vector::Random();
        payload.data2 = Vector::Random();
        payload.data3 = Vector::Random();
        payload.timestamp = real_time_tools::Timer::get_current_time_sec();
#else
        payload = real_time_tools::Timer::get_current_time_sec();
#endif
        ts.append(payload);
        // TODO verify that the sleep here is long enough
        real_time_tools::Timer::sleep_sec(0.001);
    }
    // indicate end by sending a NaN
#ifdef ADD_EIGEN_PAYLOAD
    payload.timestamp = std::numeric_limits<double>::quiet_NaN();
#else
    payload = std::numeric_limits<double>::quiet_NaN();
#endif

    ts.append(payload);
    return nullptr;
}

void *receive(void *args)
{
    real_time_tools::set_cpu_dma_latency(0);

    TimeSeriesInterface &ts = *static_cast<TimeSeriesInterface *>(args);

    size_t t = 0;
    std::cout << "ready for receiving" << std::endl;
    while (true)
    {
        Payload payload = ts[t];
        double now = real_time_tools::Timer::get_current_time_sec();

#ifdef ADD_EIGEN_PAYLOAD
        double send_time = payload.timestamp;
#else
        double send_time = payload;
#endif

        if (std::isnan(send_time))
        {
            break;
        }

        g_delays[t] = now - send_time;
        t++;
    }
    return nullptr;
}

enum Mode
{
    SINGLE,
    MULTI_WRITE,
    MULTI_READ
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Expect on of the following values as argument: single, "
                     "multi_write, multi_read"
                  << std::endl;
        return -1;
    }

    Mode mode;
    if (strcmp(argv[1], "single") == 0)
    {
        std::cout << "Single-process mode." << std::endl;
        mode = SINGLE;
    }
    else if (strcmp(argv[1], "multi_write") == 0)
    {
        std::cout << "Multi-process write mode." << std::endl;
        mode = MULTI_WRITE;
    }
    else if (strcmp(argv[1], "multi_read") == 0)
    {
        std::cout << "Multi-process read mode." << std::endl;
        mode = MULTI_READ;
    }
    else
    {
        std::cerr << "Invalid argument" << std::endl;
        return -1;
    }

    real_time_tools::RealTimeThread thread_send, thread_receive;

    switch (mode)
    {
        case SINGLE:
        {
            time_series::TimeSeries<Payload> ts(100);

            thread_receive.create_realtime_thread(&receive, &ts);
            thread_send.create_realtime_thread(&send, &ts);

            thread_receive.join();
            thread_send.join();
        }
        break;

        case MULTI_WRITE:
        {
            time_series::MultiprocessTimeSeries<Payload> ts(
                "measure_delay", 100, false);

            thread_send.create_realtime_thread(&send, &ts);

            thread_send.join();
        }
        break;

        case MULTI_READ:
        {
            time_series::clear_memory("measure_delay");
            time_series::MultiprocessTimeSeries<Payload> ts(
                "measure_delay", 100, true);

            thread_receive.create_realtime_thread(&receive, &ts);

            thread_receive.join();
        }
        break;

        default:
            // do nothing
            break;
    }

    if (mode == SINGLE || mode == MULTI_READ)
    {
        double mean = 0;
        double min = g_delays[0];
        double max = g_delays[0];
        for (size_t i = 0; i < g_delays.size(); i++)
        {
            mean += g_delays[i];
            if (g_delays[i] < min) {
                min = g_delays[i];
            }
            if (g_delays[i] > max) {
                max = g_delays[i];
            }
        }
        mean /= g_delays.size();

        std::cout << "Mean delay: " << mean << std::endl;
        std::cout << "Min. delay: " << min << std::endl;
        std::cout << "Max. delay: " << max << std::endl;

        // Dump g_delays to a file, then we can analyse and plot in Python.
        std::ofstream output_file;
        output_file.open("/tmp/time_series_delays");
        std::ostream_iterator<double> stream_iter(output_file, "\n");
        std::copy(g_delays.begin(), g_delays.end(), stream_iter);
        output_file.close();
    }

    return 0;
}

