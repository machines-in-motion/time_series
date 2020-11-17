

namespace internal
{

  template<typename TS>
  void __create_python_bindings(pybind11::module &m)
  {
    m.def("create_leader",&TS::create_leader);
    m.def("create_follower",&TS::create_follower);
    pybind11::class_<TS>(m,"TimeSeries")
      .def("newest_timeindex",&TS::newest_timeindex)
      .def("count_appended_elements",&TS::count_appended_elements)
      .def("oldest_timeindex",&TS::oldest_timeindex)
      .def("newest_element",&TS::newest_element)
      .def("timestamp_ms",&TS::timestamp_ms)
      .def("timestamp_s",&TS::timestamp_s)
      .def("wait_for_timeindex",&TS::wait_for_timeindex)
      .def("length",&TS::length)
      .def("max_length",&TS::max_length)
      .def("has_changed_since_tag",&TS::has_changed_since_tag)
      .def("tag",&TS::tag)
      .def("tagged_timeindex",&TS::tagged_timeindex)
      .def("append",&TS::append)
      .def("is_empty",&TS::is_empty)
      .def("get",[](TS& ts,time_series::Index index)
	   {
	     return ts[index];
	   });
  }


  template<typename P,
	   typename T>
  void _create_python_bindings(pybind11::module &m)
  {
  
    if constexpr (std::is_same<P,SingleProcess>::value)
      {
	typedef time_series::TimeSeries<T> TS;
	__create_python_bindings<TS>(m);
      }
    else
      {
	typedef time_series::MultiprocessTimeSeries<T> TS;
	__create_python_bindings<TS>(m);
      }

  }

}

template<typename T>
void create_multiprocesses_python_bindings(pybind11::module &m)
{
  internal::_create_python_bindings<internal::MultiProcesses,T>(m);
}

template<typename T>
void create_python_bindings(pybind11::module &m)
{
  internal::_create_python_bindings<internal::SingleProcess,T>(m);
}
