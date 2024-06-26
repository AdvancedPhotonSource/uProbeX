/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef RAW_MODEL_H
#define RAW_MODEL_H

//---------------------------------------------------------------------------

#include <QString>
#include <QObject>
#include <unordered_map>
#include <vector>
#include "io/file/mda_io.h"

//---------------------------------------------------------------------------

/**
 * @brief Model for raw mda files
 */
class RAW_Model : public QObject
{

    Q_OBJECT

public:

    /**
    * Constructor
    */
    RAW_Model();

    /**
    * Destructor
    */
    ~RAW_Model();

    bool load(QString directory, QString filename);

    bool load(QString filename);

    QString getPath() { return _path; }

    QString getFilePath(){return _filepath;}

	unsigned int getNumIntegratedSpectra();

	data_struct::Spectra<double>* getIntegratedSpectra(unsigned int det);

    void getDims(Eigen::Index& rows, Eigen::Index& cols) { rows = _scan_info.meta_info.requested_rows; cols = _scan_info.meta_info.requested_cols; }

	data_struct::Scan_Info<double>* getScanInfo();

    void setParamOverride(int idx, data_struct::Params_Override<double>* params) { if (params != nullptr) { _fit_params_override_dict[idx] = params; } }

    data_struct::Params_Override<double>* getParamOverride(int idx);

    data_struct::Params_Override<double>* getParamOverrideOrAvg(int idx);

    std::vector<unsigned int> getDetectorKeys();

private:

    data_struct::Scan_Info<double> _scan_info;

    std::map<int, data_struct::Params_Override<double>*> _fit_params_override_dict;

    std::unordered_map<unsigned int, data_struct::Spectra<double>> _integrated_spectra_map;

    QString _filepath;

    QString _path;

    std::unordered_map<std::string, data_struct::ArrayTr<double> > _csv_data;
};


//---------------------------------------------------------------------------

#endif /* RAW_MODEL_H_ */

//---------------------------------------------------------------------------
