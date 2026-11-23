/*-----------------------------------------------------------------------------
 * Copyright (c) 2021, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

//---------------------------------------------------------------------------
#include "core/GlobalThreadPool.h"

std::mutex Global_Thread_Pool::_mutex;

Global_Thread_Pool* Global_Thread_Pool::_this_inst(nullptr);
//---------------------------------------------------------------------------

// hardware_concurrency() returns unsigned and is explicitly allowed to return 0 when the
// value cannot be computed, which is the normal case in a container or a restricted
// sandbox. "hardware_concurrency() - 1" then wraps to 4294967295, and ThreadPool's
// constructor loops that many times with no clamp of its own, so the first use of the
// singleton spawns threads until std::thread throws std::system_error or the process is
// killed. On a genuine single core machine the subtraction instead gives a pool of 0
// workers, where every enqueued future simply never completes.
static size_t global_pool_thread_count()
{
    const unsigned int cores = std::thread::hardware_concurrency();
    if (cores <= 2)
    {
        return 1;
    }
    return static_cast<size_t>(cores - 1);
}

Global_Thread_Pool::Global_Thread_Pool() : ThreadPool(global_pool_thread_count())
{

}

//-----------------------------------------------------------------------------

// Declared in the header but previously never defined, so any future "delete" of the
// singleton would have failed to link. It is still never called - inst() leaks the
// instance deliberately so the pool outlives every job that might still reference it.
Global_Thread_Pool::~Global_Thread_Pool()
{

}

//-----------------------------------------------------------------------------

Global_Thread_Pool* Global_Thread_Pool::inst()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_this_inst == nullptr)
    {
        _this_inst = new Global_Thread_Pool();
    }
    return _this_inst;
}



//---------------------------------------------------------------------------
