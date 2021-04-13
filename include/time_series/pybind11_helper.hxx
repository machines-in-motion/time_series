

namespace internal
{
template <typename TS>
void __create_python_bindings(pybind11::module& m, const std::string& classname)
{
    // dev note: this will be binding over shared ptr of the time series,
    // rather than direct bindings over the class. For some unclear reason.
    // bindings directly over the class result in segfault in python.
    // (maybe some issue with the move copy of the shared memory condition
    // variable)

    pybind11::class_<TS, std::shared_ptr<TS>>(m, classname.c_str())
        .def("newest_timeindex",
             &TS::newest_timeindex,
             pybind11::arg("wait") = true)
        .def("count_appended_elements", &TS::count_appended_elements)
        .def("oldest_timeindex",
             &TS::oldest_timeindex,
             pybind11::arg("wait") = true)
        .def("newest_element", &TS::newest_element)
        .def("timestamp_ms", &TS::timestamp_ms)
        .def("timestamp_s", &TS::timestamp_s)
        .def("wait_for_timeindex",
             &TS::wait_for_timeindex,
             pybind11::arg("timeindex"),
             pybind11::arg("max_duration_s") =
                 std::numeric_limits<double>::quiet_NaN())
        .def("length", &TS::length)
        .def("max_length", &TS::max_length)
        .def("has_changed_since_tag", &TS::has_changed_since_tag)
        .def("tag", &TS::tag)
        .def("tagged_timeindex", &TS::tagged_timeindex)
        .def("append", &TS::append)
        .def("is_empty", &TS::is_empty)
        .def("get", [](TS& ts, time_series::Index index) { return ts[index]; });
}

template <typename P, typename T>
void _create_python_bindings(pybind11::module& m, const std::string& classname)
{
    if constexpr (std::is_same<P, SingleProcess>::value)
    {
        typedef time_series::TimeSeries<T> TS;
        __create_python_bindings<TS>(m, classname);
    }
    else
    {
        typedef time_series::MultiprocessTimeSeries<T> TS;
        __create_python_bindings<TS>(m, classname);

        std::string leader = std::string("create_leader_") + classname;
        std::string follower = std::string("create_follower_") + classname;
        m.def(leader.c_str(), &TS::create_leader_ptr);
        m.def(follower.c_str(), &TS::create_follower_ptr);
        m.def("clear_memory", &time_series::clear_memory);
    }
}

}  // namespace internal

template <typename T>
void create_multiprocesses_python_bindings(pybind11::module& m,
                                           const std::string& classname)
{
    internal::_create_python_bindings<internal::MultiProcesses, T>(m,
                                                                   classname);
}

template <typename T>
void create_python_bindings(pybind11::module& m, const std::string& classname)
{
    internal::_create_python_bindings<internal::SingleProcess, T>(m, classname);
}
