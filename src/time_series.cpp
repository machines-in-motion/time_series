#include "time_series/multiprocesses_time_series.hpp"

namespace time_series
{
void clear_memory(std::string segment_id)
{
    shared_memory::clear_array(segment_id + internal::shm_indexes);
    shared_memory::clear_array(segment_id + internal::shm_elements);
    shared_memory::clear_array(segment_id + internal::shm_timestamps);
    // shared memory wiped on destruction
    shared_memory::Mutex(segment_id + internal::shm_mutex, true);
    shared_memory::ConditionVariable(
        segment_id + internal::shm_condition_variable, true);
}
}
