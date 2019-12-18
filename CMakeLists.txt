cmake_minimum_required(VERSION 2.8.3)

project(time_series)

# required to use std::shared_ptr in code and to link the python bindings
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lrt -Wl,--no-as-needed")
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
endif()

###########
# imports #
###########

find_package(catkin REQUIRED COMPONENTS
  mpi_cmake_modules
  shared_memory
  real_time_tools
)
search_for_eigen()
search_for_boost()
search_for_pthread()

# This macro sets the C++ preprocessor flags "XENOMAI", "RT_PREEMPT", or
# "UBUNTU" according to the current operating system.
DEFINE_OS()


############
# includes #
############

include_directories(
    include
    ${Boost_INCLUDE_DIRS}
    ${Eigen_INCLUDE_DIRS}
    ${catkin_INCLUDE_DIRS}
)

##################
# catkin exports #
##################


catkin_package(
  LIBRARIES time_series
  INCLUDE_DIRS include
  CATKIN_DEPENDS mpi_cmake_modules shared_memory 
)

###########
# library #
###########

add_library(time_series
  src/time_series.cpp)
target_link_libraries(time_series
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  ${catkin_LIBRARIES}
  )
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(time_series
    -lrt
    -pthread
    )
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#########
# demos #
#########

add_executable(demo_time_series
  demos/demo_time_series.cpp)
target_link_libraries(demo_time_series
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  ${catkin_LIBRARIES}
  time_series
)
add_executable(demo_multiprocess_write
  demos/demo_multiprocess_write.cpp)
target_link_libraries(demo_multiprocess_write
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  ${catkin_LIBRARIES}
  time_series
)

add_executable(demo_multiprocess_read
  demos/demo_multiprocess_read.cpp)
target_link_libraries(demo_multiprocess_read
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  ${catkin_LIBRARIES}
  time_series
)

##############
# unit tests #
##############

catkin_add_gtest(test_time_series
  tests/main.cpp
  tests/basic_unit_tests.cpp
  tests/parallel_unit_tests.cpp
  )
if (TARGET test_time_series)
  target_link_libraries(test_time_series
    ${CMAKE_THREAD_LIBS_INIT}
    ${catkin_LIBRARIES}
    ${Boost_LIBRARIES}
    time_series
    )
endif()

##########################
# building documentation #
##########################

build_doxygen_documentation()

####################################
# formatting according to standard #
####################################

format_code()
