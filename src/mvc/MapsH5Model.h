/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef MAPS_H5_MODEL_H
#define MAPS_H5_MODEL_H

/*---------------------------------------------------------------------------*/

#include <QString>
#include <hdf5.h>

/*---------------------------------------------------------------------------*/

/**
 * @brief Model for Maps analyzed hdf5 files
 */
class MapsH5Model
{

public:

    /**
    * Constructor
    */
    MapsH5Model();

    /**
    * Destructor
    */
    ~MapsH5Model();

    bool load(QString filepath);

    bool is_loaded() {return _is_loaded;}

protected:

    bool _load_version_9(hid_t maps_grp_id);

    bool _load_version_10(hid_t maps_grp_id);

private:

    bool _is_loaded;

};


/*---------------------------------------------------------------------------*/

#endif /* MapsH5Model_H_ */

/*---------------------------------------------------------------------------*/
