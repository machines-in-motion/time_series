// Copyright (c) 2019 Max Planck Gesellschaft
// Vincent Berenz

// throws an exception if SIGINT is received
#define TS_THROW_IF_SIGINT                                    \
    if (signal_handler::SignalHandler::has_received_sigint()) \
    {                                                         \
        throw signal_handler::ReceivedSignal(SIGINT);         \
    }

template <typename P, typename T>
TimeSeriesBase<P, T>::TimeSeriesBase(Index start_timeindex) : empty_(true)
{
    start_timeindex_ = start_timeindex;
    oldest_timeindex_ = start_timeindex_;
    newest_timeindex_ = oldest_timeindex_ - 1;

    tagged_timeindex_ = newest_timeindex_;

    signal_handler::SignalHandler::initialize();
    signal_monitor_thread_ = std::thread(&TimeSeriesBase::monitor_signal, this);
}

template <typename P, typename T>
void TimeSeriesBase<P, T>::tag(const Index& timeindex)
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    tagged_timeindex_ = timeindex;
    write_indexes();
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::tagged_timeindex()
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return tagged_timeindex_;
}

template <typename P, typename T>
bool TimeSeriesBase<P, T>::has_changed_since_tag()
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return tagged_timeindex_ != newest_timeindex_;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::newest_timeindex(bool wait)
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (wait)
    {
        while (newest_timeindex_ < oldest_timeindex_)
        {
            TS_THROW_IF_SIGINT;

            condition_ptr_->wait(lock);
            read_indexes();
        }
    }
    else
    {
        if (newest_timeindex_ < oldest_timeindex_)
        {
            return EMPTY;
        }
    }
    return newest_timeindex_;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::count_appended_elements()
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return newest_timeindex_ - start_timeindex_ + 1;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::oldest_timeindex(bool wait)
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (wait)
    {
        TS_THROW_IF_SIGINT;

        while (newest_timeindex_ < oldest_timeindex_)
        {
            condition_ptr_->wait(lock);
            read_indexes();
        }
    }
    else
    {
        if (newest_timeindex_ < oldest_timeindex_)
        {
            return EMPTY;
        }
    }

    return oldest_timeindex_;
}

template <typename P, typename T>
T TimeSeriesBase<P, T>::newest_element()
{
    Index timeindex = newest_timeindex();
    return (*this)[timeindex];
}

template <typename P, typename T>
T TimeSeriesBase<P, T>::operator[](const Index& timeindex)
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument(
            "you tried to access time_series element which is too old.");
    }

    while (newest_timeindex_ < timeindex)
    {
        TS_THROW_IF_SIGINT;

        condition_ptr_->wait(lock);
        read_indexes();
    }

    T element;
    this->history_elements_ptr_->get(
        timeindex % this->history_elements_ptr_->size(), element);

    return element;
}

template <typename P, typename T>
Timestamp TimeSeriesBase<P, T>::timestamp_ms(const Index& timeindex)
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument(
            "you tried to access time_series element which is too old.");
    }

    while (newest_timeindex_ < timeindex)
    {
        TS_THROW_IF_SIGINT;

        condition_ptr_->wait(lock);
        read_indexes();
    }

    Timestamp timestamp;
    this->history_timestamps_ptr_->get(
        timeindex % this->history_timestamps_ptr_->size(), timestamp);

    return timestamp;
}

template <typename P, typename T>
Timestamp TimeSeriesBase<P, T>::timestamp_s(const Index& timeindex)
{
    return timestamp_ms(timeindex) / 1000.;
}

template <typename P, typename T>
bool TimeSeriesBase<P, T>::wait_for_timeindex(const Index& timeindex,
                                              const double& max_duration_s)
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument(
            "you tried to access time_series element which is too old.");
    }

    while (newest_timeindex_ < timeindex)
    {
        if (std::isfinite(max_duration_s))
        {
            bool notified = condition_ptr_->wait_for(lock, max_duration_s);
            if (!notified ||
                signal_handler::SignalHandler::has_received_sigint())
            {
                return false;
            }
        }
        else
        {
            TS_THROW_IF_SIGINT;
            condition_ptr_->wait(lock);
        }
        read_indexes();
    }
    return true;
}

template <typename P, typename T>
void TimeSeriesBase<P, T>::append(const T& element)
{
    {
        Lock<P> lock(*this->mutex_ptr_);

        // std::cout << "time_series append:";
        // element.print();

        read_indexes();
        newest_timeindex_++;
        if (newest_timeindex_ - oldest_timeindex_ + 1 >
            static_cast<Index>(this->history_elements_ptr_->size()))
        {
            oldest_timeindex_++;
        }
        Index history_index =
            newest_timeindex_ % this->history_elements_ptr_->size();
        this->history_elements_ptr_->set(history_index, element);
        this->history_timestamps_ptr_->set(
            history_index, real_time_tools::Timer::get_current_time_ms());
        write_indexes();
    }
    condition_ptr_->notify_all();
}

template <typename P, typename T>
size_t TimeSeriesBase<P, T>::length()
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return newest_timeindex_ - oldest_timeindex_ + 1;
}

template <typename P, typename T>
size_t TimeSeriesBase<P, T>::max_length()
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return this->history_elements_ptr_->size();
}

template <typename P, typename T>
bool TimeSeriesBase<P, T>::is_empty()
{
    if (!empty_)
    {
        return false;
    }
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (newest_timeindex_ < oldest_timeindex_)
    {
        return true;
    }
    empty_ = false;
    return false;
}

template <typename P, typename T>
void TimeSeriesBase<P, T>::monitor_signal()
{
    while (!signal_handler::SignalHandler::has_received_sigint())
    {
        real_time_tools::Timer::sleep_ms(10);
    }
    condition_ptr_->notify_all();
}
