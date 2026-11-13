/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/MapsWorkspaceModel.h>
#include <QEventLoop>
#include <QPointer>
#include "core/GlobalThreadPool.h"
#include <QRegularExpression>
#include "io/file/hdf5_io.h"
#include "preferences/Preferences.h"
#include "TIFF_Model.h"
#include "PNG_Model.h"
#include "SWSModel.h"

//#include "io/file/aps/aps_roi.h"
//                                          confocal,  emd,          gsecars   gsecars
std::vector<std::string> raw_h5_groups = {"2D Scan", "/Data/Image", "xrmmap", "xrfmap" };

//---------------------------------------------------------------------------

bool check_raw_mda(QFileInfo fileInfo)
{
    int is_good = io::file::mda_get_multiplied_dims(fileInfo.filePath().toStdString());
    if (is_good > 0)
        return true;
    return false;
}

//---------------------------------------------------------------------------

bool check_raw_h5(QFileInfo fileInfo)
{

    hid_t fid = H5Fopen(fileInfo.filePath().toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (fid < 0)
        return false;

    for (const auto& itr : raw_h5_groups)
    {
        hid_t gid = H5Gopen(fid, itr.c_str(), H5P_DEFAULT);
        if (gid > -1)
        {
            H5Gclose(gid);
            H5Fclose(fid);
            return true;
        }
    }

    return false;

}

//---------------------------------------------------------------------------

bool check_region_link(QFileInfo fileInfo)
{
    // TODO:
    return true;

}


//---------------------------------------------------------------------------

bool check_roi(QFileInfo fileInfo)
{
    // TODO:
    return true;

}

//---------------------------------------------------------------------------

bool check_vlm(QFileInfo fileInfo)
{
    // TODO:
    return true;

}

//---------------------------------------------------------------------------

bool check_imgdat_h5(QFileInfo fileInfo)
{

    hid_t fid = H5Fopen(fileInfo.filePath().toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (fid < 0)
        return false;

    hid_t gid = H5Gopen(fid, "MAPS", H5P_DEFAULT);
    if (gid > -1)
    {
        H5Gclose(gid);
        H5Fclose(fid);
        return true;
    }

    return false;

}


/*----------------src/mvc/MapsWorkspaceModel.cpp \-----------------------------------------------------------*/

MapsWorkspaceModel::MapsWorkspaceModel() : QObject()
{

    _is_fit_params_loaded = false;
    _is_imgdat_loaded = false;
    _is_rois_loaded = false;
    _is_vlm_loaded = false;
    _is_raw_loaded = false;

    _all_h5_suffex.append("h5");
    _all_h5_suffex.append("hdf5");

    _mda_suffex.append("mda");
    _mda_suffex.append("mca");
    _mda_suffex.append("h5");
    for (int i = 0; i < MAX_DETECTORS; i++)
    {
        _all_h5_suffex.append(QString("h5") + QString::number(i));
        _mda_suffex.append(QString("mca") + QString::number(i));
    }
    
    _raw_suffex.append("h5");
    _raw_suffex.append("hdf");
    _raw_suffex.append("hdf5");
    _raw_suffex.append("emd");

    _vlm_suffex.append("sws");
    _vlm_suffex.append("tif");
    _vlm_suffex.append("tiff");
    _vlm_suffex.append("png");

    _all_roi_suffex.append("r0i");

    _all_region_links_suffex.append("tif");

    _load_poll_timer.setInterval(100);
    connect(&_load_poll_timer, &QTimer::timeout, this, &MapsWorkspaceModel::_poll_load_jobs);
}

//---------------------------------------------------------------------------

MapsWorkspaceModel::~MapsWorkspaceModel()
{
    // The pool jobs write into members of this object, so they have to be finished
    // before any of it is torn down.
    _wait_for_load_jobs();

	_h5_models.clear();
	_raw_models.clear();
	_vlm_models.clear();
}

//---------------------------------------------------------------------------

// This used to block until every directory scan had finished, spinning on
// QCoreApplication::processEvents() to keep the window painting. That pumped the
// whole event queue from inside load(): closing the workspace there runs
// ImageStackControlWidget's close (WA_DeleteOnClose), which emits controllerClosed
// and makes uProbeX::mapsControllerClosed delete the MapsWorkspaceController - and
// with it this model, whose load() was still on the stack. No null check can fix
// that, so load() no longer waits at all. It hands the scans to the pool, returns,
// and a timer collects the results from the real event loop.
void MapsWorkspaceModel::load(QString filepath)
{
    // A previous load may still be in flight; its jobs write into the members this
    // call is about to reuse.
    _wait_for_load_jobs();

    try
    {
        std::vector<std::regex> ignore_dir_list = { std::regex("img.dat.*"), std::regex("mda.*"), std::regex("output.*"), std::regex("rois"), std::regex("flyXRF"), std::regex("XRF"), std::regex("[vV][lL][mM]") };

        _dir = std::make_unique<QDir>(filepath);
        if (!_dir->exists())
        {
            qWarning("Cannot find the example directory");
            return;
        }

        // get list of directories in the root
        std::string dataset_dir = filepath.toStdString();

        // Jobs 0, 1 and 6 used to emplace into _raw_fileinfo_list from several threads
        // at once. std::map insertion is not thread safe and they rebalance the same
        // tree, and on top of that get_raw_file_list() handed the GUI a reference to
        // that map while they were running. Each job now fills its own map and
        // _poll_load_jobs() merges the result in when the job reports done, so
        // _raw_fileinfo_list is only ever touched on the GUI thread.
        _raw_scan_fileinfo_scratch.clear();
        _mda_fileinfo_scratch.clear();
        _esrf_fileinfo_scratch.clear();

        // if check for esrf
        if (Preferences::inst()->getValue(STR_SEARCH_SUB_DIR_FOR_DATASETS).toBool())
        {
            // find_all_dirs() reads no File_Scan state, but inst() is not thread safe,
            // so the singleton is resolved here on the GUI thread.
            io::file::File_Scan* file_scan = io::file::File_Scan::inst();
            QDir dir = *_dir;
            QList<QString> raw_suffex = _raw_suffex;
            std::map<QString, QFileInfo>* scratch = &_esrf_fileinfo_scratch;
            _load_job_queue[6] = Global_Thread_Pool::inst()->enqueue(
                [file_scan, dataset_dir, ignore_dir_list, dir, raw_suffex, scratch]() mutable -> bool
                {
                    std::vector<std::string> root_dir_list = file_scan->find_all_dirs(dataset_dir + DIR_END_CHAR, ignore_dir_list, false);
                    for (const auto& itr : root_dir_list)
                    {
                        get_filesnames_in_directory(dir, QString(itr.c_str()), raw_suffex, scratch, check_raw_h5, true);
                    }
                    return true;
                });
        }

        _load_job_queue[0] = Global_Thread_Pool::inst()->enqueue(get_filesnames_in_directory, *_dir, ".", _raw_suffex, &_raw_scan_fileinfo_scratch, check_raw_h5, false);
        _load_job_queue[1] = Global_Thread_Pool::inst()->enqueue(get_filesnames_in_directory, *_dir, "mda", _mda_suffex, &_mda_fileinfo_scratch, check_raw_mda, false);
        _load_job_queue[2] = Global_Thread_Pool::inst()->enqueue(get_filesnames_in_directory, *_dir, "vlm", _vlm_suffex, &_vlm_fileinfo_list, check_vlm, false);
        _load_job_queue[3] = Global_Thread_Pool::inst()->enqueue(get_filesnames_in_directory, *_dir, "img.dat", _all_h5_suffex, &_h5_fileinfo_list, check_imgdat_h5, false);
        _load_job_queue[4] = Global_Thread_Pool::inst()->enqueue(get_filesnames_in_directory, *_dir, "rois", _all_roi_suffex, &_roi_fileinfo_list, check_roi, false);
        _load_job_queue[5] = Global_Thread_Pool::inst()->enqueue(get_filesnames_in_directory, *_dir, "VLM/region_links", _all_region_links_suffex, &_region_links_fileinfo_list, check_region_link, false);

        _is_fit_params_loaded = load_fit_params();

        _load_poll_timer.start();
    }
    catch (std::string& s)
    {
        throw s;
    }
    catch (...)
    {
        throw std::string("Failed to open Maps Analyzed dataset!");
    }

}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::_poll_load_jobs()
{
    std::vector<int> finished;
    for (auto& itr : _load_job_queue)
    {
        // Zero timeout: this runs on the GUI thread from a timer, so it must not
        // block. Jobs that are not ready are picked up on a later tick.
        if (itr.second.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        {
            continue;
        }

        finished.push_back(itr.first);

        // future::get() rethrows whatever the job threw, and this is a timer slot:
        // anything escaping here unwinds into the event loop and calls
        // std::terminate. The old code made the same get() calls, but from inside
        // load(), which had a catch-all around it.
        bool job_result = false;
        try
        {
            job_result = itr.second.get();
        }
        catch (...)
        {
            logE << "Directory scan job " << itr.first << " failed\n";
        }

        // Only bookkeeping here. The signals go out below, once every map this tick
        // touches is consistent and the finished jobs are off the queue.
        switch (itr.first)
        {
        case 0:
            _is_raw_loaded = job_result;
            // The job is finished, so this merge and every reader of
            // _raw_fileinfo_list are now on the same thread.
            _raw_fileinfo_list.insert(_raw_scan_fileinfo_scratch.begin(), _raw_scan_fileinfo_scratch.end());
            break;
        case 1:
            _is_raw_loaded = job_result;
            _raw_fileinfo_list.insert(_mda_fileinfo_scratch.begin(), _mda_fileinfo_scratch.end());
            break;
        case 2:
            _is_vlm_loaded = job_result;
            break;
        case 3:
            _is_imgdat_loaded = job_result;
            break;
        case 4:
            _is_rois_loaded = job_result;
            break;
        case 6:
            _raw_fileinfo_list.insert(_esrf_fileinfo_scratch.begin(), _esrf_fileinfo_scratch.end());
            break;
        default:
            break;
        }
    }

    for (const auto& itr : finished)
    {
        _load_job_queue.erase(itr);
    }

    const bool all_done = (_load_job_queue.size() == 0);
    if (all_done)
    {
        // Every dataset has finished loading, so the roi's can be updated.
        _load_poll_timer.stop();
        _roi_files_to_num();
    }

    // A slot on any of these can tear the workspace down, which deletes this model.
    // The queue is already drained above, so the destructor's wait is a no-op and
    // the only thing left to get wrong is touching "this" afterwards.
    QPointer<MapsWorkspaceModel> alive(this);
    for (const auto& itr : finished)
    {
        switch (itr)
        {
        case 0:
        case 6:
            emit doneLoadingRAW();
            break;
        case 1:
            emit doneLoadingMDA();
            break;
        case 2:
            emit doneLoadingVLM();
            break;
        case 3:
            emit doneLoadingImgDat();
            break;
        default:
            break;
        }
        if (alive.isNull())
        {
            return;
        }
    }

    if (all_done)
    {
        emit doneLoadingROIS();
        if (alive.isNull())
        {
            return;
        }
        emit doneLoading();
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::_wait_for_load_jobs()
{
    _load_poll_timer.stop();
    if (_load_job_queue.size() == 0)
    {
        return;
    }

    // Blocking, and deliberately without processEvents(): the point of this call is
    // that no event may be delivered while the jobs still hold pointers into this
    // object. It is only reached when a load is restarted or the model is destroyed
    // mid-scan.
    for (auto& itr : _load_job_queue)
    {
        try
        {
            static_cast<void>(itr.second.get());
        }
        catch (...)
        {
            // A job that threw cannot stop us from waiting for the rest.
        }
    }
    _load_job_queue.clear();
}

//---------------------------------------------------------------------------

// Each of these rescans a list that a load() job may still be filling, so they all
// have to let the initial load finish first. Refresh is a button, so it can be
// pressed while the workspace is still loading.

void MapsWorkspaceModel::reload_raw()
{
    _wait_for_load_jobs();
    _is_raw_loaded = get_filesnames_in_directory(*_dir, "mda", _mda_suffex, &_raw_fileinfo_list, check_raw_mda, false);
    emit doneLoadingRAW();
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::reload_analyzed()
{
    _wait_for_load_jobs();
    _is_imgdat_loaded = get_filesnames_in_directory(*_dir, "img.dat", _all_h5_suffex, &_h5_fileinfo_list, check_imgdat_h5, false);
    emit doneLoadingImgDat();
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::reload_vlm()
{
    _wait_for_load_jobs();
    _is_vlm_loaded = get_filesnames_in_directory(*_dir, "vlm", _vlm_suffex, &_vlm_fileinfo_list, check_vlm, false);
    emit doneLoadingVLM();
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::reload_roi()
{
    _wait_for_load_jobs();
    _roi_fileinfo_list.clear();
    get_filesnames_in_directory(*_dir, "rois", _all_roi_suffex, &_roi_fileinfo_list, check_roi, false);
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::reload_region_link()
{
    _wait_for_load_jobs();
    _region_links_fileinfo_list.clear();
    get_filesnames_in_directory(*_dir, "VLM/region_links", _all_region_links_suffex, &_region_links_fileinfo_list, check_region_link, false);
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::unload()
{
    // The in flight scans write into the maps cleared below.
    _wait_for_load_jobs();

    _h5_models.clear();
    _raw_models.clear();
    _vlm_models.clear();

    _fit_params_override_dict.clear();
    _raw_fileinfo_list.clear();
    _raw_scan_fileinfo_scratch.clear();
    _mda_fileinfo_scratch.clear();
    _esrf_fileinfo_scratch.clear();
    _vlm_fileinfo_list.clear();
    _h5_fileinfo_list.clear();
    _roi_fileinfo_list.clear();

    _is_fit_params_loaded = false;
    _is_raw_loaded = false;
    _is_vlm_loaded = false;
    _is_imgdat_loaded = false;
    _is_rois_loaded = false;
    //_dir = new QDir(filepath);

    emit doneUnloading();
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::_load_region_links(QString name, std::shared_ptr<MapsH5Model> model)
{
    for (auto& itr : _region_links_fileinfo_list)
    {
        QStringList flist = itr.second.baseName().split("@");
        if (flist.count() > 1)
        {
            if (flist[0] == name)
            {
                QImage image;
                if (image.load(itr.second.absoluteFilePath()))
                {
                    model->addRegionLink(flist[1], image.mirrored());
                }
                else
                {
                    logW << "Error loading region link " << itr.second.absoluteFilePath().toStdString() << " for " << name.toStdString() << "\n";
                }
            }
        }
    }

}

//---------------------------------------------------------------------------

std::shared_ptr<MapsH5Model> MapsWorkspaceModel::get_MapsH5_Model(QString name)
{
    if(_h5_models.count(name) > 0)
    {
        return _h5_models[name];
    }
    if(_h5_fileinfo_list.count(name) > 0)
    {
        std::shared_ptr<MapsH5Model> model = std::make_shared<MapsH5Model>();
        QFileInfo fileInfo = _h5_fileinfo_list[name];
        if (model->load(fileInfo.absoluteFilePath()))
        {
            QApplication::processEvents();
            // Load region links
            _load_region_links(fileInfo.baseName(), model);
            
            //can not load from here because ifstream.open locks up in thread
            // Load ROI's
            //_load_rois(fileInfo.baseName(), model);
        }
        //_roi_fileinfo_list.count()
        if(model->is_counts_loaded())
        {
            _h5_models.insert( {fileInfo.fileName(), model} );
            return model;
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------

RAW_Model* MapsWorkspaceModel::get_RAW_Model(QString name)
{
	
    if(_raw_models.count(name) > 0)
    {
        return _raw_models[name].get();
    }
    if(_raw_fileinfo_list.count(name) > 0)
    {
        auto model = std::make_unique<RAW_Model>();
        for(auto &itr : _fit_params_override_dict)
        {
            model->setParamOverride(itr.first, itr.second);
        }
        QFileInfo fileInfo = _raw_fileinfo_list[name];
        if(io::file::File_Scan::inst()->get_total_count() == 0)
		{
            QString search_path = fileInfo.absolutePath();

            if(search_path.endsWith("mda"))
            {
                QDir directory(fileInfo.absolutePath());
                directory.cd("..");
                search_path = directory.absolutePath();
            }
			io::file::File_Scan::inst()->populate_netcdf_hdf5_files(search_path.toStdString());
		}
        if(model->load(fileInfo.absolutePath(), fileInfo.fileName()))
        {
            RAW_Model* raw_ptr = model.get();
            _raw_models.insert( {fileInfo.fileName(), std::move(model)} );
            return raw_ptr;
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------

VLM_Model* MapsWorkspaceModel::get_VLM_Model(QString name)
{
    if(_vlm_models.count(name) > 0)
    {
        return _vlm_models[name].get();
    }
    if(_vlm_fileinfo_list.count(name) > 0)
    {
        std::unique_ptr<VLM_Model> model;
        QFileInfo fileInfo = _vlm_fileinfo_list[name];
        QString ext = fileInfo.suffix().toLower();
        if (ext == "tif" || ext == "tiff")
        {
            model = std::make_unique<TIFF_Model>();
        }
        else if (ext == "sws")
        {
            model = std::make_unique<SWSModel>();
        }
        else if (ext == "png")
        {
            model = std::make_unique<PNG_Model>();
        }
        if (model != nullptr)
        {
            if (model->load(fileInfo.absoluteFilePath()))
            {
                VLM_Model* raw_ptr = model.get();
                _vlm_models.insert({ fileInfo.fileName(), std::move(model) });
                return raw_ptr;
            }
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------

std::vector<QString> MapsWorkspaceModel::get_loaded_raw_names()
{
    std::vector<QString>  ret;
    for (auto& itr : _raw_models)
    {
        ret.push_back(itr.first);
    }
    return ret;
}

//---------------------------------------------------------------------------

std::vector<QString> MapsWorkspaceModel::get_loaded_h5_names()
{
    std::vector<QString>  ret;
    for (auto& itr : _h5_models)
    {
        ret.push_back(itr.first);
    }
    return ret;
}

//---------------------------------------------------------------------------

std::vector<QString> MapsWorkspaceModel::get_loaded_vlm_names()
{
    std::vector<QString>  ret;
    for (auto& itr : _vlm_models)
    {
        ret.push_back(itr.first);
    }
    return ret;
}

//---------------------------------------------------------------------------

QStringList MapsWorkspaceModel::get_raw_names_as_qstringlist()
{
    QStringList  ret;
    for (auto& itr : _raw_fileinfo_list)
    {
        ret.push_back(itr.first);
    }
    return ret;
}

//---------------------------------------------------------------------------

QStringList MapsWorkspaceModel::get_h5_names_as_qstringlist()
{
    QStringList  ret;
    for (auto& itr : _h5_fileinfo_list)
    {
        ret.push_back(itr.first);
    }
    return ret;
}

//---------------------------------------------------------------------------

QStringList MapsWorkspaceModel::get_vlm_names_as_qstringlist()
{
    QStringList  ret;
    for (auto& itr : _vlm_fileinfo_list)
    {
        ret.push_back(itr.first);
    }
    return ret;
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::unload_H5_Model(QString name)
{
    if(_h5_models.count(name) > 0)
    {
        _h5_models.erase(name);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::unload_all_H5_Model()
{
    _h5_models.clear();
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::unload_RAW_Model(QString name)
{
    if(_raw_models.count(name) > 0)
    {
        _raw_models.erase(name);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::unload_all_RAW_Model()
{
    _raw_models.clear();
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::unload_VLM_Model(QString name)
{
    if(_vlm_models.count(name) > 0)
    {
        _vlm_models.erase(name);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::unload_all_VLM_Model()
{
    _vlm_models.clear();
}

//---------------------------------------------------------------------------

QString MapsWorkspaceModel::get_directory_name()
{
    if( _dir != nullptr)
    {
        return _dir->path();
    }
    return "";
}

//---------------------------------------------------------------------------

bool MapsWorkspaceModel::load_fit_params()
{

    std::string dataset_dir = _dir->absolutePath().toStdString() + "/";
    for(size_t detector_num = 0; detector_num <= MAX_DETECTORS; detector_num++)
    {
        auto params_override = std::make_shared<data_struct::Params_Override<double>>(dataset_dir, detector_num);
        if( io::file::load_override_params(dataset_dir, detector_num, *params_override, true, false) )
        {
            _fit_params_override_dict[detector_num] = params_override;
            emit newFitParamsFileLoaded(detector_num);
        }
    }
    auto params = std::make_shared<data_struct::Params_Override<double>>(dataset_dir, -1);
    if( io::file::load_override_params(dataset_dir, -1, *params) )
    {
        _fit_params_override_dict[-1] = params;
        emit newFitParamsFileLoaded(-1);
    }


    return true;

}

//---------------------------------------------------------------------------

bool get_filesnames_in_directory(QDir dir, QString sub_dir_name, QList <QString> suffex, std::map<QString, QFileInfo> *fileinfo_list, Check_Func_Def chk_func, bool prepend_sub_dir)
{
    if(sub_dir_name.length() > 0)
    {
        if (!dir.cd(sub_dir_name))
        {
            //QString warn_msg = "Cannot find the "+sub_dir_name+" directory";
            //qWarning(warn_msg.toStdString().c_str());
            //logW<< "Cannot find the " << sub_dir_name.toStdString() << " directory";
            return false;
        }
    }
    dir.setFilter(QDir::Files | QDir::NoSymLinks);

    QFileInfoList list = dir.entryInfoList();

    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        if (suffex.contains(fileInfo.suffix()))
        {
            //if (chk_func(fileInfo)) //TODO: maybe have option for checks 
            {
                if (prepend_sub_dir)
                {
                    fileinfo_list->emplace(sub_dir_name + QDir::separator() + fileInfo.fileName(), fileInfo);
                }
                else
                {
                    fileinfo_list->emplace(fileInfo.fileName(), fileInfo);
                }
            }
        }
    }
    
    if(sub_dir_name.length() > 0 && sub_dir_name != ".")
    {
        dir.cd("..");
    }
    return true;
}

//---------------------------------------------------------------------------

data_struct::Fit_Parameters<double>* MapsWorkspaceModel::getFitParameters(int idx)
{
    if(_is_fit_params_loaded && _fit_params_override_dict.count(idx) > 0)
    {
        return &(_fit_params_override_dict[idx]->fit_params);
    }
    return nullptr;
}

//---------------------------------------------------------------------------

data_struct::Params_Override<double>* MapsWorkspaceModel::getParamOverride(int idx)
{
    if(_is_fit_params_loaded && _fit_params_override_dict.count(idx) > 0)
    {
        return _fit_params_override_dict[idx].get();
    }
    return nullptr;
}

//---------------------------------------------------------------------------

std::shared_ptr<data_struct::Params_Override<double>> MapsWorkspaceModel::getParamOverridePtr(int idx)
{
    if(_is_fit_params_loaded && _fit_params_override_dict.count(idx) > 0)
    {
        return _fit_params_override_dict[idx];
    }
    return nullptr;
}

//---------------------------------------------------------------------------

data_struct::Fit_Element_Map_Dict<double>* MapsWorkspaceModel::getElementToFit(int idx)
{
    if(_is_fit_params_loaded && _fit_params_override_dict.count(idx) > 0)
    {
        return &(_fit_params_override_dict[idx]->elements_to_fit);
    }
    return nullptr;
}

//---------------------------------------------------------------------------

void MapsWorkspaceModel::_roi_files_to_num()
{
    for(auto itr : _roi_fileinfo_list)
    {
        _roi_count_map[itr.second.baseName()] = 0;
        QFile roiFile(itr.second.absoluteFilePath());
        if (!roiFile.open(QIODevice::ReadOnly))
        {
            continue;
        }

        QJsonObject rootJson = QJsonDocument::fromJson(roiFile.readAll()).object();
        roiFile.close();

        if (rootJson.contains(STR_MAPS_ROIS.c_str()) && rootJson[STR_MAPS_ROIS.c_str()].isArray())
        {
            QJsonArray rois = rootJson[STR_MAPS_ROIS.c_str()].toArray();
            _roi_count_map[itr.second.baseName()] = rois.size();
        }
    }
}

//---------------------------------------------------------------------------
