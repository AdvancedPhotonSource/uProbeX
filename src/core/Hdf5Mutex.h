/*-----------------------------------------------------------------------------
 * Copyright (c) 2026, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef HDF5_MUTEX_H
#define HDF5_MUTEX_H

#include <mutex>

namespace hdf5
{

// The HDF5 library shipped with uProbeX is built without H5_HAVE_THREADSAFE.
// Use one process-wide recursive mutex at every application entry point into
// HDF5. Recursive locking is required because model loaders call lower-level
// helpers that may also protect their own HDF5 operations.
inline std::recursive_mutex& mutex()
{
    static std::recursive_mutex instance;
    return instance;
}

using Lock = std::lock_guard<std::recursive_mutex>;

}

#endif
