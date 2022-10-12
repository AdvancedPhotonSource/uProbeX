/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef MAPS_WORKSPACE_MODEL_H
#define MAPS_WORKSPACE_MODEL_H

/*---------------------------------------------------------------------------*/

#include <QString>
#include "RawH5Model.h"
#include "MapsH5Model.h"
#include "TIFF_Model.h"
#include "SWSModel.h"
#include "RAW_Model.h"
#include "io/file/hl_file_io.h"
#include <map>
#include <QDir>

/*---------------------------------------------------------------------------*/

typedef std::function<bool(const QFileInfo fileInfo)> Check_Func_Def;


bool check_raw_mda(QFileInfo fileInfo);

bool check_raw_h5(QFileInfo fileInfo);

bool check_vlm(QFileInfo fileInfo);

bool check_imgdat_h5(QFileInfo fileInfo);

bool check_roi(QFileInfo fileInfo);

bool check_region_link(QFileInfo fileInfo);

/**
 * @brief Model
 */
class MapsWorkspaceModel : public QObject
{

    Q_OBJECT

public:

    /**
    * Constructor
    */
    MapsWorkspaceModel();

    /**
    * Destructor
    */
    ~MapsWorkspaceModel();

    void load(QString filepath);

    void unload();

    //bool is_loaded() {return _is_loaded;}

    QString get_directory_name();

    MapsH5Model* get_MapsH5_Model(QString name);

    RAW_Model* get_RAW_Model(QString name);

    VLM_Model* get_VLM_Model(QString name);

    void unload_H5_Model(QString name);

    void unload_RAW_Model(QString name);

    void unload_VLM_Model(QString name);

    data_struct::Fit_Parameters<double>* getFitParameters(int idx);

    data_struct::Fit_Element_Map_Dict<double>* getElementToFit(int idx);

    data_struct::Params_Override<double>* getParamOverride(int idx);

    const map<QString, QFileInfo>& get_hdf5_file_list() { return _h5_fileinfo_list; }

    const map<QString, QFileInfo>& get_raw_file_list() { return _raw_fileinfo_list; }

    const map<QString, QFileInfo>& get_vlm_file_list() { return _vlm_fileinfo_list; }

    vector<QString> get_loaded_raw_names();

    vector<QString> get_loaded_h5_names();

    vector<QString> get_loaded_vlm_names();

    QDir get_directory() { return *_dir; }

    bool load_v9_rois(QString fname, MapsH5Model* model);

signals:

    void doneLoadingMDA();

    void doneLoadingRAW();

    void doneLoadingVLM();

    void doneLoadingImgDat();

    void doneLoading();

    void doneUnloading();

    void newFitParamsFileLoaded(int);

public slots:

    void reload_raw();

    void reload_analyzed();

    void reload_vlm();

    void reload_roi();

    void reload_region_link();

protected:

    bool _load_fit_params();

    void _load_region_links(QString name, MapsH5Model* model);

    //bool _get_filesnames_in_directory(QString sub_dir_name, QList <QString> suffex, map<QString, QFileInfo> *fileinfo_list, Check_Func_Def chk_func);

    std::map<QString, MapsH5Model*> _h5_models;
    std::map<QString, RAW_Model*> _raw_models;
    std::map<QString, VLM_Model*> _vlm_models;

    std::map<int, data_struct::Params_Override<double>> _fit_params_override_dict;

private:

    QList <QString> _all_h5_suffex;
    QList <QString> _mda_suffex;
    QList <QString> _raw_suffex;
    QList <QString> _vlm_suffex;
    QList <QString> _all_roi_suffex;
    QList <QString> _all_region_links_suffex;

    map<QString, QFileInfo> _h5_fileinfo_list;
    map<QString, QFileInfo> _raw_fileinfo_list;
    map<QString, QFileInfo> _vlm_fileinfo_list;
    map<QString, QFileInfo> _roi_fileinfo_list;
    map<QString, QFileInfo> _region_links_fileinfo_list;

    QDir* _dir;

    bool _is_vlm_loaded;
    bool _is_raw_loaded;
    bool _is_fit_params_loaded;
    bool _is_imgdat_loaded;
    bool _is_rois_loaded;
};

bool get_filesnames_in_directory(QDir dir, QString sub_dir_name, QList <QString> suffex, map<QString, QFileInfo>* fileinfo_list, Check_Func_Def chk_func);

/*---------------------------------------------------------------------------*/

#endif /* MapsWorkspaceModel_H_ */

/*---------------------------------------------------------------------------*/
