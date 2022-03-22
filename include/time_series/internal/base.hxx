// Copyright (c) 2019 Max Planck Gesellschaft
// Vincent Berenz

template <typename P, typename T>
void TimeSeriesBase<P, T>::throw_if_sigint_received() const
{
    // only throw if throw_on_sigint_ is true
    if (throw_on_sigint_ &&
        signal_handler::SignalHandler::has_received_sigint())
    {
        throw signal_handler::ReceivedSignal(SIGINT);
    }
}

template <typename P, typename T>
TimeSeriesBase<P, T>::TimeSeriesBase(Index start_timeindex,
                                     bool throw_on_sigint)
    : empty_(true), is_destructor_called_(false)
{
    start_timeindex_ = start_timeindex;
    oldest_timeindex_ = start_timeindex_;
    newest_timeindex_ = oldest_timeindex_ - 1;

    tagged_timeindex_ = newest_timeindex_;

    throw_on_sigint_ = throw_on_sigint;
    if (throw_on_sigint)
    {
        signal_handler::SignalHandler::initialize();
        signal_monitor_thread_ =
            std::thread(&TimeSeriesBase::monitor_signal, this);
    }
}

template <typename P, typename T>
TimeSeriesBase<P, T>::TimeSeriesBase(TimeSeriesBase<P, T>&& other) noexcept
    : start_timeindex_(other.start_timeindex_),
      oldest_timeindex_(other.oldest_timeindex_),
      newest_timeindex_(other.newest_timeindex_),
      tagged_timeindex_(other.tagged_timeindex_),
      empty_(other.empty_),
      is_destructor_called_(false)
{
    mutex_ptr_ = std::move(other.mutex_ptr_);
    condition_ptr_ = std::move(other.condition_ptr_);
    history_elements_ptr_ = std::move(other.history_elements_ptr_);
    history_timestamps_ptr_ = std::move(other.history_timestamps_ptr_);
    signal_monitor_thread_ = std::move(other.signal_monitor_thread_);
}

template <typename P, typename T>
TimeSeriesBase<P, T>::~TimeSeriesBase()
{
    is_destructor_called_ = true;
    if (signal_monitor_thread_.joinable())
    {
        signal_monitor_thread_.join();
    }
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
Index TimeSeriesBase<P, T>::tagged_timeindex() const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return tagged_timeindex_;
}

template <typename P, typename T>
bool TimeSeriesBase<P, T>::has_changed_since_tag() const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return tagged_timeindex_ != newest_timeindex_;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::newest_timeindex(bool wait) const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (wait)
    {
        while (newest_timeindex_ < oldest_timeindex_)
        {
            throw_if_sigint_received();

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
Index TimeSeriesBase<P, T>::count_appended_elements() const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return newest_timeindex_ - start_timeindex_ + 1;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::oldest_timeindex(bool wait) const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (wait)
    {
        while (newest_timeindex_ < oldest_timeindex_)
        {
            throw_if_sigint_received();

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
T TimeSeriesBase<P, T>::newest_element() const
{
    Index timeindex = newest_timeindex();
    return (*this)[timeindex];
}

template <typename P, typename T>
T TimeSeriesBase<P, T>::operator[](const Index& timeindex) const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument("you tried to access time_series element " +
                                    std::to_string(timeindex) +
                                    " which is too old (oldest in buffer is " +
                                    std::to_string(oldest_timeindex_) + ").");
    }

    while (newest_timeindex_ < timeindex)
    {
        throw_if_sigint_received();

        condition_ptr_->wait(lock);
        read_indexes();
    }

    T element;
    this->history_elements_ptr_->get(
        timeindex % this->history_elements_ptr_->size(), element);

    return element;
}

template <typename P, typename T>
Timestamp TimeSeriesBase<P, T>::timestamp_ms(const Index& timeindex) const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument("you tried to access time_series element " +
                                    std::to_string(timeindex) +
                                    " which is too old (oldest in buffer is " +
                                    std::to_string(oldest_timeindex_) + ").");
    }

    while (newest_timeindex_ < timeindex)
    {
        throw_if_sigint_received();

        condition_ptr_->wait(lock);
        read_indexes();
    }

    Timestamp timestamp;
    this->history_timestamps_ptr_->get(
        timeindex % this->history_timestamps_ptr_->size(), timestamp);

    return timestamp;
}

template <typename P, typename T>
Timestamp TimeSeriesBase<P, T>::timestamp_s(const Index& timeindex) const
{
    return timestamp_ms(timeindex) / 1000.;
}

template <typename P, typename T>
bool TimeSeriesBase<P, T>::wait_for_timeindex(
    const Index& timeindex, const double& max_duration_s) const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument("you tried to access time_series element " +
                                    std::to_string(timeindex) +
                                    " which is too old (oldest in buffer is " +
                                    std::to_string(oldest_timeindex_) + ").");
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
            throw_if_sigint_received();
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
size_t TimeSeriesBase<P, T>::length() const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return newest_timeindex_ - oldest_timeindex_ + 1;
}

template <typename P, typename T>
size_t TimeSeriesBase<P, T>::max_length() const
{
    Lock<P> lock(*this->mutex_ptr_);
    read_indexes();
    return this->history_elements_ptr_->size();
}

template <typename P, typename T>
bool TimeSeriesBase<P, T>::is_empty() const
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
    constexpr double SLEEP_DURATION_MS = 100;

    // We need to copy the pointer here because otherwize the system segfaults.
    // I am suspecting that the derefencing of the smart_pointer is not
    // thread safe.
    std::shared_ptr<ConditionVariable<P> > local_condition_ptr = condition_ptr_;
    while (!local_condition_ptr)
    {
        local_condition_ptr = condition_ptr_;
        real_time_tools::Timer::sleep_ms(SLEEP_DURATION_MS);
    }

    while (!signal_handler::SignalHandler::has_received_sigint() &&
           !is_destructor_called_)
    {
        real_time_tools::Timer::sleep_ms(SLEEP_DURATION_MS);
    }
    // Notify to release locks that could otherwise prevent the application from
    // terminating.
    local_condition_ptr->notify_all();
}
