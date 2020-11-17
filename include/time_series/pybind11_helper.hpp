#pragma once

#include <type_traits>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include "time_series/internal/specialized_classes.hpp"
#include "time_series/multiprocess_time_series.hpp"
#include "time_series/time_series.hpp"

namespace time_series
{

  /**
   * adds to the python module m a class called TimeSeries
   * which is of typedef time_series::MultiprocessTimeSeries<T>
   */
  template<typename T>
  void create_multiprocesses_python_bindings(pybind11::module &m);

  /**
   * adds to the python module m a class called TimeSeries
   * which is of typedef time_series::TimeSeries<T>
   */
  template<typename T>
  void create_python_bindings(pybind11::module &m);

  
  #include "pybind11_helper.hxx"
  
}
