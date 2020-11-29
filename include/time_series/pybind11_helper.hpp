#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <type_traits>
#include "time_series/internal/specialized_classes.hpp"
#include "time_series/multiprocess_time_series.hpp"
#include "time_series/time_series.hpp"

namespace time_series
{
/**
 * adds to the python module m a class called TimeSeries
 * which is of typedef time_series::MultiprocessTimeSeries<T>
 */
template <typename T>
void create_multiprocesses_python_bindings(pybind11::module& m,
                                           const std::string& classname);

/**
 * adds to the python module m a class called TimeSeries
 * which is of typedef time_series::TimeSeries<T>
 */
template <typename T>
void create_python_bindings(pybind11::module& m, const std::string& classname);

#include "pybind11_helper.hxx"

}  // namespace time_series
