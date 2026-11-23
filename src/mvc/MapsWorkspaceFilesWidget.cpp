/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/MapsWorkspaceFilesWidget.h>

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPointer>
#include <preferences/Preferences.h>
#include "core/GlobalThreadPool.h"
#include "core/str_defines.h"

//---------------------------------------------------------------------------

MapsWorkspaceFilesWidget::MapsWorkspaceFilesWidget(QWidget* parent) : QWidget(parent)
{

	_model = nullptr;
    _open_type = MODEL_TYPE::MAPS_H5;
    _open_total = 0;
    _open_done = 0;
    _open_cursor_set = false;
    _open_poll_timer.setInterval(100);
    connect(&_open_poll_timer, &QTimer::timeout, this, &MapsWorkspaceFilesWidget::_pollOpenJob);
    createLayout();

}

//---------------------------------------------------------------------------

MapsWorkspaceFilesWidget::~MapsWorkspaceFilesWidget()
{
    _waitForOpenJob();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::createLayout()
{

    //std::vector<std::string> bound_types {"Not Initialized", "Fixed", "Limited Low High", "Limited Low", "Limited High", "Fit"};
    _lbl_workspace = new QLabel();
    _tab_widget = new QTabWidget();

    _h5_tab_widget = new FileTabWidget();
    QAction *h5avg_file = new QAction("All Avg files  | *.h5", this);
    connect(h5avg_file, &QAction::triggered, [this](bool val){ _h5_tab_widget->filterTextChanged("*.h5"); });
    QAction *h5det_file = new QAction("All Detector 0 | *.h50", this);
    connect(h5det_file, &QAction::triggered, [this](bool val){ _h5_tab_widget->filterTextChanged("*.h50"); });
    _h5_tab_widget->appendFilterHelpAction(h5avg_file);
    _h5_tab_widget->appendFilterHelpAction(h5det_file);

    _h5_tab_widget->addCustomContext(STR_PROCESS_ANALYZED);
    _h5_tab_widget->addCustomContext(STR_BATCH_ROI);
    _h5_tab_widget->addCustomButtonRow(STR_PROCESS_ANALYZED, STR_BATCH_ROI);    
    // TODO: need to implement
    //_h5_tab_widget->addCustomContext(STR_H5_EXPORT, "Export Images"); 

    connect(_h5_tab_widget, &FileTabWidget::loadList, [this](const QStringList& sl) { this->onOpenModel(sl, MODEL_TYPE::MAPS_H5); });
    connect(_h5_tab_widget, &FileTabWidget::unloadList, [this](const QStringList& sl) { this->onCloseModel(sl, MODEL_TYPE::MAPS_H5); });
    connect(_h5_tab_widget, &FileTabWidget::customContext, this, &MapsWorkspaceFilesWidget::onCustomContext);
    connect(_h5_tab_widget, &FileTabWidget::customButton, this, &MapsWorkspaceFilesWidget::onCustomButton);
    connect(_h5_tab_widget, &FileTabWidget::selectNewRow, this, &MapsWorkspaceFilesWidget::onDatasetSelected);

    _mda_tab_widget = new FileTabWidget();
    connect(_mda_tab_widget, &FileTabWidget::loadList, [this](const QStringList& sl) { this->onOpenModel(sl, MODEL_TYPE::RAW); });
    connect(_mda_tab_widget, &FileTabWidget::unloadList, [this](const QStringList& sl) { this->onCloseModel(sl, MODEL_TYPE::RAW); });
    connect(_mda_tab_widget, &FileTabWidget::customContext, this, &MapsWorkspaceFilesWidget::onCustomContext);
    connect(_mda_tab_widget, &FileTabWidget::customButton, this, &MapsWorkspaceFilesWidget::onCustomButton);
    connect(_mda_tab_widget, &FileTabWidget::selectNewRow, this, &MapsWorkspaceFilesWidget::onDatasetSelected);
    _mda_tab_widget->addCustomContext(STR_PROCESS);
    _mda_tab_widget->addCustomContext(STR_BATCH_ROI);
    _mda_tab_widget->addCustomButtonRow(STR_PROCESS, STR_BATCH_ROI);

    _vlm_tab_widget = new FileTabWidget();
    QAction* sws_file = new QAction("SWS | *.sws", this);
    connect(sws_file, &QAction::triggered, [this](bool val) { _vlm_tab_widget->filterTextChanged("*.sws"); });
    QAction* tiff_file = new QAction("TIFF | *.tiff", this);
    connect(tiff_file, &QAction::triggered, [this](bool val) { _vlm_tab_widget->filterTextChanged("*.tiff"); });
    QAction* tif_file = new QAction("TIF | *.tif", this);
    connect(tif_file, &QAction::triggered, [this](bool val) { _vlm_tab_widget->filterTextChanged("*.tif"); });
    QAction* png_file = new QAction("PNG | *.png", this);
    connect(png_file, &QAction::triggered, [this](bool val) { _vlm_tab_widget->filterTextChanged("*.png"); });
    connect(_vlm_tab_widget, &FileTabWidget::loadList, [this](const QStringList& sl) { this->onOpenModel(sl, MODEL_TYPE::VLM); });
    connect(_vlm_tab_widget, &FileTabWidget::unloadList, [this](const QStringList& sl) { this->onCloseModel(sl, MODEL_TYPE::VLM); });
    connect(_vlm_tab_widget, &FileTabWidget::customContext, this, &MapsWorkspaceFilesWidget::onCustomContext);
    connect(_vlm_tab_widget, &FileTabWidget::customButton, this, &MapsWorkspaceFilesWidget::onCustomButton);
    connect(_vlm_tab_widget, &FileTabWidget::selectNewRow, this, &MapsWorkspaceFilesWidget::onDatasetSelected);
    _vlm_tab_widget->appendFilterHelpAction(sws_file);
    _vlm_tab_widget->appendFilterHelpAction(tiff_file);
    _vlm_tab_widget->appendFilterHelpAction(tif_file);
    _vlm_tab_widget->appendFilterHelpAction(png_file);
    _vlm_tab_widget->addCustomContext(STR_GEN_SCAN_AREA);
    _vlm_tab_widget->addCustomButtonRow(STR_GEN_SCAN_AREA);


    _per_pixel_fit_widget = std::make_unique<PerPixelFitWidget>();
    connect(_per_pixel_fit_widget.get(), &PerPixelFitWidget::processed_list_update, this, &MapsWorkspaceFilesWidget::onProcessed_list_update);

    connect(&_gen_scan_vlm_widget, &GenScanVlmWidget::new_scan_area, this, &MapsWorkspaceFilesWidget::newScanArea);

    QLayout* vlayout = new QVBoxLayout();

    _tab_widget->insertTab(0, _h5_tab_widget, "Analyzed Data");
    _tab_widget->insertTab(1, _mda_tab_widget, "Raw Data");
    _tab_widget->insertTab(2, _vlm_tab_widget, "Light Microscope");
    
    vlayout->addWidget(_tab_widget);
    vlayout->setSpacing(0);
	vlayout->setContentsMargins(0, 0, 0, 0);
    setLayout(vlayout);
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::setModel(MapsWorkspaceModel *model)
{
    // An open job captured the old model pointer and is calling into it right now.
    // Swapping the model out from under it, and then the caller destroying it, is
    // what the wait is here to prevent.
    _waitForOpenJob();
    _finishOpenBatch();
    _open_queue.clear();

	if (_model != nullptr)
	{
        disconnect(_model, &MapsWorkspaceModel::doneLoadingMDA, this, &MapsWorkspaceFilesWidget::updatedMDA);
        disconnect(_model, &MapsWorkspaceModel::doneLoadingVLM, this, &MapsWorkspaceFilesWidget::updatedVLM);
        disconnect(_model, &MapsWorkspaceModel::doneLoadingImgDat, this, &MapsWorkspaceFilesWidget::updatedHDF);
        disconnect(_model, &MapsWorkspaceModel::doneUnloading, this, &MapsWorkspaceFilesWidget::clearLists);
        //disconnect(_model, &MapsWorkspaceModel::newFitParamsFileLoaded, this, &MapsWorkspaceFilesWidget::loadedFitParams);
	}
	
	_model = model;
	if (_model != nullptr)
	{
		QString path = _model->get_directory_name();
		if (path.length() > 0)
		{
			_lbl_workspace->setText(path);
		}

        connect(_model, &MapsWorkspaceModel::doneLoadingMDA, this, &MapsWorkspaceFilesWidget::updatedMDA);
        connect(_model, &MapsWorkspaceModel::doneLoadingVLM, this, &MapsWorkspaceFilesWidget::updatedVLM);
        connect(_model, &MapsWorkspaceModel::doneLoadingImgDat, this, &MapsWorkspaceFilesWidget::updatedHDF);
        connect(_model, &MapsWorkspaceModel::doneLoadingROIS, this, &MapsWorkspaceFilesWidget::updateROIS);
        connect(_model, &MapsWorkspaceModel::doneUnloading, this, &MapsWorkspaceFilesWidget::clearLists);
        //connect(_model, &MapsWorkspaceModel::newFitParamsFileLoaded, this, &MapsWorkspaceFilesWidget::loadedFitParams);
        connect(_h5_tab_widget, &FileTabWidget::onRefresh, _model, &MapsWorkspaceModel::reload_analyzed);
        connect(_vlm_tab_widget, &FileTabWidget::onRefresh, _model, &MapsWorkspaceModel::reload_vlm);
        connect(_mda_tab_widget, &FileTabWidget::onRefresh, _model, &MapsWorkspaceModel::reload_raw);
	}

}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::updatedMDA()
{
    _mda_tab_widget->set_file_list(_model->get_raw_file_list());
    std::vector<QString> loaded_names = _model->get_loaded_raw_names();
    for (const auto& itr : loaded_names)
    {
        _mda_tab_widget->loaded_file_status_changed(File_Loaded_Status::LOADED, itr);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::updatedVLM()
{
    _vlm_tab_widget->set_file_list(_model->get_vlm_file_list());
    std::vector<QString> loaded_names = _model->get_loaded_vlm_names();
    for (const auto& itr : loaded_names)
    {
        _vlm_tab_widget->loaded_file_status_changed(File_Loaded_Status::LOADED, itr);
    }
    //_vlm_tab_widget->set_roi_num_list(_model->get_region_num_list());
}
//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::updatedHDF()
{
    _h5_tab_widget->set_file_list(_model->get_hdf5_file_list());
    std::vector<QString> loaded_names = _model->get_loaded_h5_names();
    for (const auto& itr : loaded_names)
    {
        _h5_tab_widget->loaded_file_status_changed(File_Loaded_Status::LOADED, itr);
    }
    _h5_tab_widget->set_roi_num_list(_model->get_roi_num_list());
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::updateROIS()
{
    _h5_tab_widget->set_roi_num_list(_model->get_roi_num_list());
    _mda_tab_widget->set_roi_num_list(_model->get_roi_num_list());
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::update_roi_num(QString name, int val)
{
    _h5_tab_widget->set_roi_num(name, val);
    _mda_tab_widget->set_roi_num(name, val);
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::setFileTabActionsEnabled(bool val)
{
    
    _h5_tab_widget->setActionsAndButtonsEnabled(val);
    _mda_tab_widget->setActionsAndButtonsEnabled(val);
    _vlm_tab_widget->setActionsAndButtonsEnabled(val);
    
}

//---------------------------------------------------------------------------

// This used to be a foreach over names_list that handed each analyzed file to the
// thread pool and then span on QCoreApplication::processEvents() until the future
// was ready. The pool job captured "this" and read _model through it, so a close
// delivered inside that pump destroyed the widget and the controller's
// MapsWorkspaceModel while the job was still running through both, and the rest of
// the loop then ran on freed memory.
//
// It is now a state machine driven by the real event loop: one file at a time,
// nothing pumps events, the pool job holds a model pointer captured by value
// instead of reaching back through the widget, and setModel()/the destructor wait
// for whatever is in flight.
void MapsWorkspaceFilesWidget::onOpenModel(const QStringList& names_list, MODEL_TYPE mt)
{
    if (_open_total > 0)
    {
        // A batch is already draining. The file tab actions are disabled while one
        // runs so this is not expected, but appending beats dropping the request -
        // as long as the type matches, since each type reports to a different tab.
        if (mt == _open_type)
        {
            _open_queue.append(names_list);
            _open_total += names_list.count();
        }
        else
        {
            logW << "Ignoring open request: a batch of a different model type is still loading\n";
        }
        return;
    }

    if (_model == nullptr || names_list.count() == 0)
    {
        return;
    }

    setFileTabActionsEnabled(false);

    _open_queue = names_list;
    _open_type = mt;
    _open_total = names_list.count();
    _open_done = 0;

    _startNextOpen();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::_startNextOpen()
{
    if (_open_queue.isEmpty() || _model == nullptr)
    {
        _finishOpenBatch();
        return;
    }

    _open_current = _open_queue.takeFirst();
    const QString name = _open_current;
    // Captured by value: the pool thread must not read _model, which setModel() can
    // change while the job runs.
    MapsWorkspaceModel* model = _model;

    if (_open_type == MODEL_TYPE::MAPS_H5)
    {
        _open_h5_future = Global_Thread_Pool::inst()->enqueue([model, name] { return model->get_MapsH5_Model(name); });
        _open_poll_timer.start();
        return;
    }

    if (_open_type == MODEL_TYPE::RAW)
    {
        if (false == _open_cursor_set)
        {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            _open_cursor_set = true;
        }

        if (name.endsWith(".h5") || name.endsWith(".hdf5") || name.endsWith(".emd"))
        {
            _open_raw_future = Global_Thread_Pool::inst()->enqueue([model, name] { return model->get_RAW_Model(name); });
            _open_poll_timer.start();
            return;
        }

        if (name.endsWith(".mda") || name.contains(".mca"))
        {
            // loading through background thread on windows seems to lock up on fopen
            _completeOpenRaw(model->get_RAW_Model(name));
        }
        else
        {
            _completeOpenRaw(nullptr);
        }
        return;
    }

    // MODEL_TYPE::VLM. Also loaded on the GUI thread; the thread pool variant this
    // replaced was already commented out before any of this.
    File_Loaded_Status load_status = FAILED_LOADING;
    VLM_Model* vlm_model = model->get_VLM_Model(name);
    if (vlm_model != nullptr)
    {
        load_status = LOADED;
        vlm_model->getImage();
    }

    // loaded_model() can reach a slot that closes the workspace and deletes this.
    QPointer<MapsWorkspaceFilesWidget> alive(this);
    emit loaded_model(name, MODEL_TYPE::VLM);
    if (alive.isNull())
    {
        return;
    }
    _vlm_tab_widget->loaded_file_status_changed(load_status, name);
    _advanceOpenBatch();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::_pollOpenJob()
{
    if (_open_h5_future.valid())
    {
        if (_open_h5_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        {
            return;
        }
        _open_poll_timer.stop();
        std::shared_ptr<MapsH5Model> h5_model;
        try
        {
            // get() rethrows whatever the job threw. Letting that out of a timer slot
            // would unwind into the event loop and terminate.
            h5_model = _open_h5_future.get();
        }
        catch (...)
        {
            logE << "Failed loading " << _open_current.toStdString() << "\n";
        }
        _completeOpenH5(h5_model);
        return;
    }

    if (_open_raw_future.valid())
    {
        if (_open_raw_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        {
            return;
        }
        _open_poll_timer.stop();
        RAW_Model* raw_model = nullptr;
        try
        {
            raw_model = _open_raw_future.get();
        }
        catch (...)
        {
            logE << "Failed loading " << _open_current.toStdString() << "\n";
        }
        _completeOpenRaw(raw_model);
        return;
    }

    // Nothing in flight; the timer should not be running.
    _open_poll_timer.stop();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::_completeOpenH5(std::shared_ptr<MapsH5Model> h5_model)
{
    const QString name = _open_current;
    File_Loaded_Status load_status = FAILED_LOADING;
    QPointer<MapsWorkspaceFilesWidget> alive(this);

    if (h5_model != nullptr && _model != nullptr)
    {
        // have to call from main thread because ifstream.open locks up in threadpool
        //_model->load_v9_rois(name, h5_model);

        int idx = -1;
        for (int b = 0; b < MAX_DETECTORS; b++)
        {
            QString bs = QString::number(b);
            if (name.endsWith(bs))
            {
                idx = b;
                break;
            }
        }
        // Share ownership with MapsWorkspaceModel rather than aliasing its storage
        // with a no-op deleter. The alias did not take a reference, so re-running
        // load_fit_params() (Save Fit Parameters does) reassigned the dict entry,
        // freed the override, and left every loaded model reading and writing the
        // freed maps.
        std::shared_ptr<data_struct::Params_Override<double>> param_override_ptr = _model->getParamOverridePtr(idx);
        //if we can't find override for specific detector, try to use the avg one.
        if (param_override_ptr == nullptr)
        {
            param_override_ptr = _model->getParamOverridePtr(-1);
        }
        if (param_override_ptr == nullptr)
        {
            // generate param file; MapsH5Model becomes sole owner
            param_override_ptr = std::make_shared<data_struct::Params_Override<double>>();
        }
        h5_model->set_fit_parameters_override(param_override_ptr);
        load_status = LOADED;
        emit loaded_model(name, MODEL_TYPE::MAPS_H5);
        if (alive.isNull())
        {
            return;
        }
    }

    _h5_tab_widget->loaded_file_status_changed(load_status, name);
    _advanceOpenBatch();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::_completeOpenRaw(RAW_Model* raw_model)
{
    const QString name = _open_current;
    File_Loaded_Status load_status = FAILED_LOADING;
    QPointer<MapsWorkspaceFilesWidget> alive(this);

    if (raw_model != nullptr)
    {
        load_status = LOADED;
        emit loaded_model(name, MODEL_TYPE::RAW);
        if (alive.isNull())
        {
            return;
        }
    }

    _mda_tab_widget->loaded_file_status_changed(load_status, name);
    _advanceOpenBatch();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::_advanceOpenBatch()
{
    _open_done++;

    QPointer<MapsWorkspaceFilesWidget> alive(this);
    emit loaded_perc(_open_done, _open_total);
    if (alive.isNull())
    {
        return;
    }

    if (_open_queue.isEmpty())
    {
        _finishOpenBatch();
        return;
    }

    // Single shot rather than calling straight through: it keeps the stack flat over
    // a long file list and gives the event loop a turn between files without
    // anything in here pumping it.
    QTimer::singleShot(0, this, &MapsWorkspaceFilesWidget::_startNextOpen);
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::_finishOpenBatch()
{
    _open_poll_timer.stop();
    _open_queue.clear();
    _open_current.clear();

    const bool was_running = (_open_total > 0);
    _open_total = 0;
    _open_done = 0;

    if (_open_cursor_set)
    {
        QApplication::restoreOverrideCursor();
        _open_cursor_set = false;
    }

    if (was_running)
    {
        setFileTabActionsEnabled(true);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::_waitForOpenJob()
{
    _open_poll_timer.stop();

    // Deliberately a blocking wait with no processEvents(): the job is running
    // through the MapsWorkspaceModel, and pumping here is exactly what let that
    // model be destroyed underneath it.
    if (_open_h5_future.valid())
    {
        try
        {
            static_cast<void>(_open_h5_future.get());
        }
        catch (...)
        {
            logE << "Failed loading " << _open_current.toStdString() << "\n";
        }
    }
    if (_open_raw_future.valid())
    {
        try
        {
            static_cast<void>(_open_raw_future.get());
        }
        catch (...)
        {
            logE << "Failed loading " << _open_current.toStdString() << "\n";
        }
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::closeAllModels(MODEL_TYPE mt)
{
    setFileTabActionsEnabled(false);
    File_Loaded_Status load_status = UNLOADED;
    QStringList names_list;
    if (_model != nullptr)
    {
        switch (mt)
        {
        case MODEL_TYPE::MAPS_H5:
            _model->unload_all_H5_Model();
            _h5_tab_widget->loaded_file_status_changed_all(load_status);
            names_list = _model->get_h5_names_as_qstringlist();
            break;
        case MODEL_TYPE::RAW:
            _model->unload_all_RAW_Model();
            _mda_tab_widget->loaded_file_status_changed_all(load_status);
            break;
        case MODEL_TYPE::VLM:
            _model->unload_all_VLM_Model();
            _vlm_tab_widget->loaded_file_status_changed_all(load_status);
            break;
        }
    
    }

    setFileTabActionsEnabled(true);
    emit unloadList_model(names_list, mt);
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onCloseModel(const QStringList& names_list, MODEL_TYPE mt)
{
    setFileTabActionsEnabled(false);
    File_Loaded_Status load_status = UNLOADED;
    if (_model != nullptr)
    {
        foreach(QString name, names_list)
        {
            switch (mt)
            {
            case MODEL_TYPE::MAPS_H5:
                _model->unload_H5_Model(name);
                _h5_tab_widget->loaded_file_status_changed(load_status, name);
                break;
            case MODEL_TYPE::RAW:
                _model->unload_RAW_Model(name);
                _mda_tab_widget->loaded_file_status_changed(load_status, name);
                break;
            case MODEL_TYPE::VLM:
                _model->unload_VLM_Model(name);
                _vlm_tab_widget->loaded_file_status_changed(load_status, name);
                break;
            }
        }
    }

    setFileTabActionsEnabled(true);
    emit unloadList_model(names_list, mt);
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::clearLists()
{
	_h5_tab_widget->unload_all();
	_mda_tab_widget->unload_all();
	_vlm_tab_widget->unload_all();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onCustomContext(const QString& context_label, const QStringList& file_list)
{
    if (context_label == STR_PROCESS)
    {
        onPerPixelProcessList(file_list);
    }
    else if (context_label == STR_PROCESS_ANALYZED)
    {
        onPerPixelProcessListAnalyzed(file_list);
    }
    else if (context_label == STR_BATCH_ROI)
    {
        onBatchRoiList(file_list);
    }
    else if (context_label == STR_H5_EXPORT)
    {
        //onExportImages(file_list);
    }
    else if (context_label == STR_GEN_SCAN_AREA)
    {
        onGenScanArea(file_list);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onCustomButton(const QString& context_label, const QStringList& file_list)
{
    if (context_label == STR_PROCESS)
    {
        onPerPixelProcessList(file_list);
    }
    if (context_label == STR_PROCESS_ANALYZED)
    {
        onPerPixelProcessListAnalyzed(file_list);
    }
    else if (context_label == STR_BATCH_ROI)
    {
        onBatchRoiList(file_list);
    }
    else if (context_label == STR_GEN_SCAN_AREA)
    {
        onGenScanArea(file_list);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onPerPixelProcessList(const QStringList& file_list)
{
    if (_model != nullptr)
    {
        _per_pixel_fit_widget->setDir(_model->get_directory_name().toStdString());
    }
    onCloseModel(file_list, MODEL_TYPE::MAPS_H5);
    _per_pixel_fit_widget->updateFileList(file_list);
    _per_pixel_fit_widget->show();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onPerPixelProcessListAnalyzed(const QStringList& file_list)
{
    if (_model != nullptr)
    {
        _per_pixel_fit_widget->setDir(_model->get_directory_name().toStdString());
    }
    onCloseModel(file_list, MODEL_TYPE::MAPS_H5);
    QStringList nfile_list = file_list;
    // for each files replace .mda.h5[0-n] to .mda and remove duplicates
    for(QString &str : nfile_list)
    {
        if(str.endsWith(".mda.h5"))
        {
            str = str.replace(".mda.h5", ".mda");
        }
        else if(str.endsWith(".h5"))
        {
            str = str.replace(".h5", ".mda");
        }
        else
        {
            for(int i =0; i<30; i++)
            {
                QString cmp = ".h5"+QString::number(i);
                if(str.endsWith(cmp))
                {
                    str = str.replace(cmp, "");
                }
            }
        }
    }
    nfile_list.removeDuplicates();
    _per_pixel_fit_widget->updateFileList(nfile_list);
    _per_pixel_fit_widget->show();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onBatchRoiList(const QStringList& file_list)
{
    std::map<QString, std::string> map_set;
    std::unordered_map<QString, QFileInfo> roi_map;
    for (auto& itr : file_list)
    {
        QFileInfo finfo(itr);
        map_set[finfo.baseName()] = finfo.baseName().toStdString();
    }

    logI << "file cnt = " << file_list.size() << "\n";
    logI << "map_set cnt = " << map_set.size() << "\n";

    QDir directory = _model->get_directory();
    if (false == directory.cd(STR_MAPS_ROIS_DIR_NAME.c_str()))
    {
        QMessageBox::warning(nullptr, "ROI Warning", "Did not find folder 'rois' in this workspace. Please create ROI's and try again");
        return;
    }
    QStringList roi_list = directory.entryList(QStringList() << "*.roi" << "*.r0i", QDir::Files);
    logI << "roi_list = " << roi_list.size();

    foreach(QString filename, roi_list)
    {
        QFileInfo finfo(filename);
        if (map_set.count(finfo.baseName()) > 0)
        {
            roi_map[finfo.baseName()] = finfo;
        }
    }

    
    //create batch roi process widget and pass workspace
    if (_batch_roi_fit_widget == nullptr)
    {
        _batch_roi_fit_widget = std::make_unique<BatchRoiFitWidget>(_model->get_directory_name().toStdString());
    }
    _batch_roi_fit_widget->updateFileList(roi_map);
    _batch_roi_fit_widget->show();
    
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onGenScanArea(const QStringList& file_list)
{
    if(_model!= nullptr)
    {
        _gen_scan_vlm_widget.setDir(_model->get_directory_name());
        _gen_scan_vlm_widget.updateFileList(_model->get_hdf5_file_list());
    }
    _gen_scan_vlm_widget.show();
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::newScanArea(const QString &name)
{
    QStringList vlm_list;
    vlm_list.append(name);
    if(_model != nullptr)
    {
        _model->reload_vlm();
    }
    onOpenModel(vlm_list, MODEL_TYPE::VLM);
    
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onProcessed_list_update(QStringList file_list)
{
    if (_model != nullptr)
    {
        _model->reload_analyzed();

        onOpenModel(file_list, MODEL_TYPE::MAPS_H5);
    }
}

//---------------------------------------------------------------------------

void MapsWorkspaceFilesWidget::onDatasetSelected(const QString name)
{
    if (Preferences::inst()->getValue(STR_PRF_SHOW_DATASET_ON_FILE_SELECT).toBool())
    {
        emit datasetSelected(name);
    }
}

//---------------------------------------------------------------------------