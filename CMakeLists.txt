cmake_minimum_required(VERSION 3.10.2)
project(time_series)

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

# depend on ament macros
find_package(ament_cmake REQUIRED)

find_package(mpi_cmake_modules REQUIRED)
find_package(shared_memory REQUIRED)
find_package(real_time_tools REQUIRED)
find_package(signal_handler REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system thread)
find_package(Threads REQUIRED)

ament_export_dependencies(shared_memory real_time_tools signal_handler Eigen3
                          Boost Threads)

set(all_targets)
set(all_target_exports)

#
# library
#
add_library(${PROJECT_NAME} SHARED src/multiprocess_time_series.cpp)
# Add the include dependencies
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>)
# Link the dependencies
ament_target_dependencies(${PROJECT_NAME} Boost)
ament_target_dependencies(${PROJECT_NAME} Threads)
target_link_libraries(${PROJECT_NAME} shared_memory::shared_memory)
target_link_libraries(${PROJECT_NAME} real_time_tools::real_time_tools)
target_link_libraries(${PROJECT_NAME} signal_handler::signal_handler)
target_link_libraries(${PROJECT_NAME} Eigen3::Eigen)
# Export the target.
ament_export_interfaces(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
# For the installation
list(APPEND all_targets ${PROJECT_NAME})
list(APPEND all_target_exports export_${PROJECT_NAME})

#
# demos #
#
add_executable(demo_time_series demos/demo_time_series.cpp)
target_link_libraries(demo_time_series ${PROJECT_NAME})
list(APPEND all_targets demo_time_series)

add_executable(demo_multiprocess_write demos/demo_multiprocess_write.cpp)
target_link_libraries(demo_multiprocess_write ${PROJECT_NAME})
list(APPEND all_targets demo_multiprocess_write)

add_executable(demo_multiprocess_read demos/demo_multiprocess_read.cpp)
target_link_libraries(demo_multiprocess_read ${PROJECT_NAME})
list(APPEND all_targets demo_multiprocess_read)

#
# Add unit tests.
#
if(BUILD_TESTING)
  find_package(ament_cmake_gtest)

  # Set a general config folder path for all tests
  set(TEST_CONFIG_PATH ${PROJECT_SOURCE_DIR}/tests/config/)

  ament_add_gtest(
    test_time_series tests/main.cpp tests/test_basic_api.cpp
    tests/test_monitor_signal.cpp tests/test_parallel_execution.cpp)

  # Link the dependencies to it.
  target_link_libraries(test_time_series ${PROJECT_NAME})
  target_include_directories(
    test_time_series
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include> SYSTEM
    PUBLIC ${PYTHON_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
endif()

#
# Install the package
#
install(DIRECTORY include/ DESTINATION include)

install(
  TARGETS ${all_targets}
  EXPORT ${all_target_exports}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME} # to be seen by ros2 run
  INCLUDES
  DESTINATION include)

#
# Building documentation.
#
add_documentation()

#
# Export as an ament_package
#
ament_package()
