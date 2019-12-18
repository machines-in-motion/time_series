

template <typename P, typename T>
TimeSeriesBase<P, T>::TimeSeriesBase(size_t max_length, Index start_timeindex)
{
    start_timeindex_ = start_timeindex;
    oldest_timeindex_ = start_timeindex_;
    newest_timeindex_ = oldest_timeindex_ - 1;

    tagged_timeindex_ = newest_timeindex_;
}

template <typename P, typename T>
void TimeSeriesBase<P, T>::tag(const Index& timeindex)
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    tagged_timeindex_ = timeindex;
    write_indexes();
}

template <typename P, typename T>
bool TimeSeriesBase<P, T>::has_changed_since_tag()
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    return tagged_timeindex_ != newest_timeindex_;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::newest_timeindex()
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    while (newest_timeindex_ < oldest_timeindex_)
    {
        conditionPtr_->wait(lock);
        read_indexes();
    }
    return newest_timeindex_;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::count_appended_elements()
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    return newest_timeindex_ - start_timeindex_ + 1;
}

template <typename P, typename T>
Index TimeSeriesBase<P, T>::oldest_timeindex()
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    while (newest_timeindex_ < oldest_timeindex_)
    {
        conditionPtr_->wait(lock);
        read_indexes();
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
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument(
            "you tried to access time_series element which is too old.");
    }

    while (newest_timeindex_ < timeindex)
    {
        conditionPtr_->wait(lock);
        read_indexes();
    }

    T element;
    this->history_elementsPtr_->get(
        timeindex % this->history_elementsPtr_->size(), element);

    return element;
}

template <typename P, typename T>
Timestamp TimeSeriesBase<P, T>::timestamp_ms(const Index& timeindex)
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    if (timeindex < oldest_timeindex_)
    {
        throw std::invalid_argument(
            "you tried to access time_series element which is too old.");
    }

    while (newest_timeindex_ < timeindex)
    {
        conditionPtr_->wait(lock);
        read_indexes();
    }

    Timestamp timestamp;
    this->history_timestampsPtr_->get(
        timeindex % this->history_timestampsPtr_->size(), timestamp);

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
    Lock<P> lock(*this->mutexPtr_);
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
            bool notified = conditionPtr_->wait_for(lock, max_duration_s);
            if (!notified)
            {
                return false;
            }
        }
        else
        {
            conditionPtr_->wait(lock);
        }
        read_indexes();
    }
    return true;
}

template <typename P, typename T>
void TimeSeriesBase<P, T>::append(const T& element)
{
    {
        Lock<P> lock(*this->mutexPtr_);

        // std::cout << "time_series append:";
        // element.print();

        read_indexes();
        newest_timeindex_++;
        if (newest_timeindex_ - oldest_timeindex_ + 1 >
            static_cast<long>(this->history_elementsPtr_->size()))
        {
            oldest_timeindex_++;
        }
        Index history_index =
            newest_timeindex_ % this->history_elementsPtr_->size();
        this->history_elementsPtr_->set(history_index, element);
        this->history_timestampsPtr_->set(
            history_index, real_time_tools::Timer::get_current_time_ms());
        write_indexes();
    }
    conditionPtr_->notify_all();
}

template <typename P, typename T>
size_t TimeSeriesBase<P, T>::length()
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    return newest_timeindex_ - oldest_timeindex_ + 1;
}

template <typename P, typename T>
size_t TimeSeriesBase<P, T>::max_length()
{
    Lock<P> lock(*this->mutexPtr_);
    read_indexes();
    return this->history_elementsPtr_->size();
}
