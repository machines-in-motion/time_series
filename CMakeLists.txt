cmake_minimum_required(VERSION 3.10.2)
project(time_series VERSION 2.0.0)

# Using C++17
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# stop build on first error
string(APPEND CMAKE_CXX_FLAGS " -Wfatal-errors")

#
# Dependencies
#
find_package(mpi_cmake_modules REQUIRED)
find_package(shared_memory REQUIRED)
find_package(real_time_tools REQUIRED)
find_package(signal_handler REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system thread)
find_package(Threads REQUIRED)

set(all_targets)

#
# library
#
add_library(${PROJECT_NAME} SHARED src/multiprocess_time_series.cpp)
# Add the include dependencies
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>)
# Link the dependencies
target_link_libraries(${PROJECT_NAME} Boost::boost Boost::filesystem
                      Boost::system Boost::thread)
target_link_libraries(${PROJECT_NAME} Threads::Threads)
target_link_libraries(${PROJECT_NAME} shared_memory::shared_memory)
target_link_libraries(${PROJECT_NAME} real_time_tools::real_time_tools)
target_link_libraries(${PROJECT_NAME} signal_handler::signal_handler)
target_link_libraries(${PROJECT_NAME} Eigen3::Eigen)
# For the installation
list(APPEND all_targets ${PROJECT_NAME})

#
# demos #
#
add_executable(time_series_demo demos/demo_time_series.cpp)
target_link_libraries(time_series_demo ${PROJECT_NAME})
list(APPEND all_targets time_series_demo)

add_executable(time_series_demo_multiprocess_write
               demos/demo_multiprocess_write.cpp)
target_link_libraries(time_series_demo_multiprocess_write ${PROJECT_NAME})
list(APPEND all_targets time_series_demo_multiprocess_write)

add_executable(time_series_demo_multiprocess_read
               demos/demo_multiprocess_read.cpp)
target_link_libraries(time_series_demo_multiprocess_read ${PROJECT_NAME})
list(APPEND all_targets time_series_demo_multiprocess_read)

#
# Add unit tests.
#
include(CTest)
if(BUILD_TESTING)
  # C++ unit-tests framework used.
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
  # Config folder
  get_filename_component(CONFIG_PATH config ABSOLUTE)
  # create the executable
  add_executable(
    test_time_series
    tests/main.cpp tests/test_basic_api.cpp tests/test_monitor_signal.cpp
    tests/test_parallel_execution.cpp)
  # link to the created librairies and its dependencies
  target_link_libraries(test_time_series ${PROJECT_NAME} GTest::gtest)
  # declare the test as gtest
  gtest_add_tests(TARGET test_time_series)
endif()

#
# building documentation
#
# add_documentation()

#
# Install the package
#

# install the include directory
install(DIRECTORY include/ DESTINATION include)

# command to install the library and binaries
install(
  TARGETS ${all_targets}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include)

# Export this package as a cmake package.
generate_cmake_package()
