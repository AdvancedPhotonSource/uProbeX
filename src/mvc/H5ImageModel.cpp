/*-----------------------------------------------------------------------------
 * Copyright (c) 2025, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/H5ImageModel.h>
#include "core/Hdf5Mutex.h"

#include <limits>

namespace
{

bool read_first_float(hid_t dataset_id, float& value)
{
    hid_t file_space_id = H5Dget_space(dataset_id);
    if (file_space_id < 0)
    {
        return false;
    }

    const int rank = H5Sget_simple_extent_ndims(file_space_id);
    if (rank < 0)
    {
        H5Sclose(file_space_id);
        return false;
    }

    hid_t memory_space_id = H5S_ALL;
    bool selected = true;
    if (rank > 0)
    {
        std::vector<hsize_t> dims(static_cast<size_t>(rank));
        std::vector<hsize_t> offset(static_cast<size_t>(rank), 0);
        std::vector<hsize_t> count(static_cast<size_t>(rank), 1);
        selected = H5Sget_simple_extent_dims(file_space_id, dims.data(), nullptr) >= 0;
        for (hsize_t dim : dims)
        {
            selected = selected && dim > 0;
        }
        if (selected)
        {
            selected = H5Sselect_hyperslab(file_space_id, H5S_SELECT_SET,
                                            offset.data(), nullptr, count.data(), nullptr) >= 0;
        }
        if (selected)
        {
            memory_space_id = H5Screate_simple(rank, count.data(), nullptr);
            selected = memory_space_id >= 0;
        }
    }

    const bool read = selected
        && H5Dread(dataset_id, H5T_NATIVE_FLOAT, memory_space_id, file_space_id,
                   H5P_DEFAULT, &value) >= 0;

    if (memory_space_id != H5S_ALL)
    {
        H5Sclose(memory_space_id);
    }
    H5Sclose(file_space_id);
    return read;
}

}

/*----------------src/mvc/H5ImageModel.cpp \-----------------------------------------------------------*/

H5ImageModel::H5ImageModel() : QObject()
{

    _filepath = "";

}

//---------------------------------------------------------------------------

H5ImageModel::~H5ImageModel()
{

}

//---------------------------------------------------------------------------

bool H5ImageModel::load(QString filepath)
{
    hdf5::Lock hdf5_lock(hdf5::mutex());

    _filepath = filepath;
    _data.clear();
    _data_dims_in[0] = 0;
    _data_dims_in[1] = 0;
    _data_dims_in[2] = 0;
    _x_axis = 0.0f;
    _y_axis = 0.0f;

    hid_t file_id = H5Fopen(filepath.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if(file_id < 0)
    {
        logE<<"Error opening file "<<filepath.toStdString() << "\n";
        return false;
    }

    hid_t data_id = H5Dopen(file_id, STR_DATA_PATH.c_str(), H5P_DEFAULT);
    if(data_id < 0)
    {
        logE<<"Error opening path "<<STR_DATA_PATH << "\n";
        H5Fclose(file_id);
        return false;
    }

    hid_t x_id = H5Dopen(file_id, STR_MOTOR_X_PATH.c_str(), H5P_DEFAULT);
    if(x_id < 0)
    {
        logE<<"Error opening path "<<STR_MOTOR_X_PATH << "\n";
        H5Dclose(data_id);
        H5Fclose(file_id);
        return false;
    }
    
    hid_t y_id = H5Dopen(file_id, STR_MOTOR_Y_PATH.c_str(), H5P_DEFAULT);
    if(y_id < 0)
    {
        logE<<"Error opening path "<<STR_MOTOR_Y_PATH << "\n";
        H5Dclose(x_id);
        H5Dclose(data_id);
        H5Fclose(file_id);
        return false;
    }

    hid_t data_space_id = H5Dget_space(data_id);
    bool loaded = data_space_id >= 0;
    int rank = loaded ? H5Sget_simple_extent_ndims(data_space_id) : -1;
    loaded = loaded && (rank == 2 || rank == 3);

    hsize_t dims[3] = {1, 0, 0};
    if (loaded)
    {
        loaded = H5Sget_simple_extent_dims(data_space_id, dims, nullptr) >= 0;
    }

    const hsize_t height = rank == 2 ? dims[0] : dims[1];
    const hsize_t width = rank == 2 ? dims[1] : dims[2];
    const size_t max_size = _data.max_size();
    loaded = loaded && height > 0 && width > 0
        && height <= static_cast<hsize_t>(max_size)
        && width <= static_cast<hsize_t>(max_size / static_cast<size_t>(height));

    hid_t memory_space_id = H5S_ALL;
    if (loaded)
    {
        _data_dims_in[0] = 1;
        _data_dims_in[1] = height;
        _data_dims_in[2] = width;
        _data.resize(static_cast<size_t>(height * width));

        if (rank == 3)
        {
            const hsize_t offset[3] = {0, 0, 0};
            const hsize_t count[3] = {1, height, width};
            loaded = H5Sselect_hyperslab(data_space_id, H5S_SELECT_SET, offset,
                                          nullptr, count, nullptr) >= 0;
            if (loaded)
            {
                memory_space_id = H5Screate_simple(3, count, nullptr);
                loaded = memory_space_id >= 0;
            }
        }

        if (loaded)
        {
            loaded = H5Dread(data_id, H5T_NATIVE_UCHAR, memory_space_id, data_space_id,
                             H5P_DEFAULT, _data.data()) >= 0;
        }
    }

    if (memory_space_id != H5S_ALL)
    {
        H5Sclose(memory_space_id);
    }
    if (data_space_id >= 0)
    {
        H5Sclose(data_space_id);
    }

    if (loaded)
    {
        loaded = read_first_float(x_id, _x_axis) && read_first_float(y_id, _y_axis);
    }

    H5Dclose(x_id);
    H5Dclose(y_id);
    H5Dclose(data_id);
    H5Fclose(file_id);

    if (false == loaded)
    {
        _data.clear();
        _data_dims_in[0] = 0;
        _data_dims_in[1] = 0;
        _data_dims_in[2] = 0;
        logW << "Could not read HDF5 image or motor coordinates\n";
    }
    return loaded;
}

//---------------------------------------------------------------------------

QPixmap H5ImageModel::gen_pixmap()
{
    if (_data.empty() || _data_dims_in[1] == 0 || _data_dims_in[2] == 0
        || _data_dims_in[1] > static_cast<hsize_t>(std::numeric_limits<int>::max())
        || _data_dims_in[2] > static_cast<hsize_t>(std::numeric_limits<int>::max()))
    {
        return QPixmap();
    }
    QImage image(_data.data(), static_cast<int>(_data_dims_in[2]),
                 static_cast<int>(_data_dims_in[1]), QImage::Format_Grayscale8);
    return QPixmap::fromImage(image.convertToFormat(QImage::Format_RGB32));
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
