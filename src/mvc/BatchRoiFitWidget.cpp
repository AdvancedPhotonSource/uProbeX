/*-----------------------------------------------------------------------------
 * Copyright (c) 2023, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include "mvc/BatchRoiFitWidget.h"
#include <QApplication>
#include <core/process_whole.h>
#include <core/Hdf5Mutex.h>
#include <QScrollArea>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDir>

//---------------------------------------------------------------------------

BatchRoiFitWidget::BatchRoiFitWidget(std::string directory, QWidget *parent) : QWidget(parent)
{

    _directory = directory;
    _total_itr = 0;
    _canceled = false;
    _worker = nullptr;
    if (false == _directory.empty() && _directory.back() != DIR_END_CHAR)
    {
        _directory += DIR_END_CHAR;
    }
    createLayout();

    // Auto connections: status_callback() and the file loop emit these from the
    // fitting thread, so they are delivered as queued events and the slots run on
    // the GUI thread.
    connect(this, &BatchRoiFitWidget::blockProgress, this, &BatchRoiFitWidget::onBlockProgress);
    connect(this, &BatchRoiFitWidget::fileProgress, this, &BatchRoiFitWidget::onFileProgress);

}

//---------------------------------------------------------------------------

BatchRoiFitWidget::~BatchRoiFitWidget()
{

    stopProcessing();

}

//---------------------------------------------------------------------------

void BatchRoiFitWidget::createLayout()
{

    _progressBarBlocks = new QProgressBar();
    _progressBarBlocks->setRange(0,100);
    _progressBarFiles = new QProgressBar();
    _progressBarFiles->setRange(0, 100);

    _btn_run = new QPushButton("Run");
    connect(_btn_run, &QPushButton::released, this, &BatchRoiFitWidget::runProcessing);
    _btn_cancel = new QPushButton("Cancel");
    connect(_btn_cancel, &QPushButton::released, this, &BatchRoiFitWidget::onClose);

    _le_detectors = new QLineEdit("0,1,2,3,4,5,6");

    _file_list_model = std::make_unique<QStandardItemModel>();
    _file_list_view = new QListView();
    _file_list_view->setModel(_file_list_model.get());
    _file_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    QHBoxLayout* buttonlayout = new QHBoxLayout();
    buttonlayout->addWidget(_btn_run);
    buttonlayout->addWidget(_btn_cancel);
   
    QHBoxLayout* hbox_progresss_blocks = new QHBoxLayout();
    hbox_progresss_blocks->addWidget(new QLabel("Current Fitting:"));
    hbox_progresss_blocks->addWidget(_progressBarBlocks);

    QHBoxLayout* hbox_progresss_files = new QHBoxLayout();
    hbox_progresss_files->addWidget(new QLabel("All Dataset Files:"));
    hbox_progresss_files->addWidget(_progressBarFiles);

    QHBoxLayout* detector_hbox = new QHBoxLayout();
    detector_hbox->addWidget(new QLabel("Detectors  "));
    detector_hbox->addWidget(_le_detectors);

    _optimizer_widget = new OptimizerOptionsWidget();
    fitting::optimizers::NLOPT_Optimizer<double> optimizer;
    _optimizer_widget->setOptimizer(optimizer);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addItem(detector_hbox);
    layout->addWidget(_optimizer_widget);
    layout->addWidget(_file_list_view);
    layout->addItem(buttonlayout);
    layout->addItem(hbox_progresss_blocks);
    layout->addItem(hbox_progresss_files);
    
    this->setLayout(layout);
}

//---------------------------------------------------------------------------

void BatchRoiFitWidget::onClose()
{
    _canceled = true;
    
    if (_file_list.size() ==0 || _btn_run->isEnabled())
    {
        close();
    }
}

//---------------------------------------------------------------------------

void BatchRoiFitWidget::updateFileList(std::unordered_map<QString, QFileInfo> roi_map)
{
    _progressBarFiles->setValue(0);
    _progressBarBlocks->setValue(0);

    _file_list_model->clear();

    _roi_map = roi_map;

    if (_roi_map.size() == 0)
    {
        _btn_run->setEnabled(false);
    }
    else
    {
        _btn_run->setEnabled(true);
    }

    _file_list.clear();
    for (auto& itr : _roi_map)
    {
        _file_list.push_back(itr.first);
        _file_list_model->appendRow(new QStandardItem(QIcon(":/images/circle_gray.png"), itr.first));
    }

}

//---------------------------------------------------------------------------

// Runs on the fitting thread.
void BatchRoiFitWidget::status_callback(size_t cur_itr, size_t total_itr)
{
    if (_canceled.load())
    {
        throw 20;
    }

    // This used to drive the progress bar directly and then call processEvents() to
    // repaint it, all on the GUI thread. See the comment on runProcessing() for why
    // that was fatal.
    emit blockProgress(static_cast<qulonglong>(cur_itr), static_cast<qulonglong>(total_itr));
}

//---------------------------------------------------------------------------

// Runs on the GUI thread.
void BatchRoiFitWidget::onBlockProgress(qulonglong cur_itr, qulonglong total_itr)
{
    if (_total_itr != total_itr)
    {
        _total_itr = total_itr;
        _progressBarBlocks->setRange(0, _total_itr);
    }

    _progressBarBlocks->setValue(cur_itr);
}

//---------------------------------------------------------------------------

// Runs on the GUI thread.
void BatchRoiFitWidget::onFileProgress(qulonglong cur_file)
{
    _progressBarFiles->setValue(cur_file);
}


//---------------------------------------------------------------------------

// Reads every widget the fit needs. GUI thread only. Returns false and leaves
// _analysis_job null if the detector field cannot be parsed.
bool BatchRoiFitWidget::buildAnalysisJob()
{
    _analysis_job = std::make_unique<data_struct::Analysis_Job<double>>();
    data_struct::Analysis_Job<double>& analysis_job = *_analysis_job;

    //analysis_job.set_optimizer_algorithm();

    analysis_job.dataset_directory = _directory;
    analysis_job.output_dir = _directory;
    if (_optimizer_widget->isHybrid())
    {
        analysis_job.optimize_fit_routine = OPTIMIZE_FIT_ROUTINE::HYBRID;
    }
    else
    {
        analysis_job.optimize_fit_routine = OPTIMIZE_FIT_ROUTINE::ALL_PARAMS;
    }

    _optimizer_widget->updateOptimizerOptions(*(analysis_job.optimizer()));

    analysis_job.use_weights = _optimizer_widget->useWeights();
    analysis_job.detector_num_arr.clear();

    QString dstring = _le_detectors->text();
    if (dstring.length() > 0)
    {
        if (dstring.count(",") > 0)
        {
            QStringList dlist = dstring.split(",");
            foreach(QString str, dlist)
            {
                analysis_job.detector_num_arr.push_back(str.toInt());
            }
        }
        else if (dstring.count(":") > 0)
        {
            QStringList dlist = dstring.split(":");
            try
            {
                int low = dlist[0].toInt();
                int high = dlist[1].toInt();
                for (int i = low; i < high + 1; i++)
                {
                    analysis_job.detector_num_arr.push_back(i);
                }
            }
            catch (...)
            {
                QMessageBox::warning(this, "Bad detectors", "Could not parse detectors. For range use 'num:max' ");
                _analysis_job.reset();
                return false;
            }
        }
        else
        {
            try
            {
                analysis_job.detector_num_arr.push_back(dstring.toInt());
            }
            catch (...)
            {
                QMessageBox::warning(this, "Bad detectors", "Could not parse detectors. Please enter number for single detector, or use ',' for list of detectors. ");
                _analysis_job.reset();
                return false;
            }
        }
    }
    else
    {
        QMessageBox::warning(this, "No detectors listed", "Please enter detctors ',' separated");
        _analysis_job.reset();
        return false;
    }

    // TODO: will need to update to fitting routes instead of getting fit parameters
    analysis_job.fitting_routines.push_back(Fitting_Routines::NNLS);

    // lock all fit parameters except elastic/inelastic amp and elements
    analysis_job.optimize_fit_params_preset = fitting::models::Fit_Params_Preset::BATCH_FIT_NO_TAILS;
    analysis_job.quantification_standard_filename = "maps_standardinfo.txt";

    return true;
}

//---------------------------------------------------------------------------

// The whole body of this used to run on the GUI thread, kept "responsive" by the
// processEvents() in status_callback() and in the per file loop. That pumped close
// events from inside the fit: closing the workspace during a batch ROI fit destroys
// the ImageStackControlWidget (WA_DeleteOnClose), and with it the
// MapsWorkspaceFilesWidget that owns this widget's unique_ptr - so the fit carried
// on writing through a freed "this" and eventually returned into a freed stack
// frame. No null check can fix that; the work had to come off the GUI thread.
void BatchRoiFitWidget::runProcessing()
{
    if (_directory.empty())
    {
        QMessageBox::warning(this, "No dataset directory", "Select a dataset directory before running the batch ROI fit.");
        return;
    }

    if (_worker != nullptr)
    {
        // Already fitting. The Run button is disabled while a fit is in flight, but
        // the signal can still arrive from elsewhere.
        return;
    }

    // Parse before disabling anything: the old code disabled the controls first and
    // then returned on a bad detector string, leaving the window permanently dead.
    if (false == buildAnalysisJob())
    {
        return;
    }

    _btn_run->setEnabled(false);
    _le_detectors->setEnabled(false);
    _file_list_view->setEnabled(false);
    _canceled = false;

    const int proc_total = static_cast<int>(_roi_map.size() * _analysis_job->detector_num_arr.size());
    _progressBarFiles->setRange(0, proc_total);
    _progressBarBlocks->setRange(0, _total_itr);
    _progressBarBlocks->setValue(0);
    _progressBarFiles->setValue(0);

    data_struct::Analysis_Job<double>* analysis_job = _analysis_job.get();
    // Copied, not captured by reference: updateFileList() can replace _roi_map from
    // the GUI thread while the fit is reading it.
    std::vector<std::string> roi_files;
    roi_files.reserve(_roi_map.size());
    for (const auto& itr : _roi_map)
    {
        roi_files.push_back(itr.second.fileName().toStdString());
    }

    BatchRoiFitWidget* self = this;
    _worker = QThread::create([self, analysis_job, roi_files]()
    {
        Callback_Func_Status_Def cb_func = std::bind(&BatchRoiFitWidget::status_callback, self, std::placeholders::_1, std::placeholders::_2);

        try
        {
            hdf5::Lock hdf5_lock(hdf5::mutex());
            if (io::file::init_analysis_job_detectors(analysis_job))
            {
                io::file::File_Scan::inst()->populate_netcdf_hdf5_files(analysis_job->dataset_directory);

                if (analysis_job->quantification_standard_filename.length() > 0)
                {
                    perform_quantification(analysis_job, false);
                }

                std::map<int, std::map<std::string, data_struct::Fit_Parameters<double>>> roi_fit_params;
                qulonglong i = 0;
                for (const auto& roi_file : roi_files)
                {
                    i++;
                    optimize_single_roi(*analysis_job, roi_file, roi_fit_params, &cb_func);
                    emit self->fileProgress(i);
                }

                // save all to csv
                for (auto detector_num : analysis_job->detector_num_arr)
                {
                    std::string save_path = analysis_job->dataset_directory + "output/specfit_results" + std::to_string(detector_num) + ".csv";
                    std::string quant_save_path = analysis_job->dataset_directory + "output/specfit_results" + std::to_string(detector_num) + "_quantified.csv";
                    io::file::csv::save_v9_specfit(save_path, roi_fit_params.at(detector_num));
                    if (analysis_job->get_detector(detector_num)->quantification_standards.size() > 0)
                    {
                        io::file::csv::save_v9_specfit_quantified(quant_save_path, analysis_job->get_detector(detector_num), analysis_job->fitting_routines, roi_fit_params.at(detector_num));
                    }
                }
            }
        }
        catch (...)
        {
            // status_callback() throws 20 when the user presses Cancel, and
            // optimize_single_roi() calls it from inside the fit loop above.
            // Nothing may escape QThread::run() either. Also covers
            // roi_fit_params.at() throwing for a detector that produced no results.
            logI << "Batch ROI fitting canceled\n";
        }
    });

    connect(_worker, &QThread::finished, this, &BatchRoiFitWidget::onProcessingFinished);
    _worker->start();
}

//---------------------------------------------------------------------------

void BatchRoiFitWidget::onProcessingFinished()
{
    if (false == _canceled.load())
    {
        _progressBarBlocks->setValue(_progressBarBlocks->maximum());
        _progressBarFiles->setValue(_progressBarFiles->maximum());
    }

    _btn_run->setEnabled(true);
    _le_detectors->setEnabled(true);
    _file_list_view->setEnabled(true);
    _btn_cancel->setText("Close");

    if (_worker != nullptr)
    {
        // deleteLater, not delete: this slot is reached from the thread object's own
        // finished() signal.
        _worker->deleteLater();
        _worker = nullptr;
    }
    _analysis_job.reset();

    // open file location
    QDir tmp_dir(QString(_directory.c_str()));
    tmp_dir.cd("output");
    if (false == QDesktopServices::openUrl(QUrl::fromLocalFile(tmp_dir.absolutePath())))
    {
        logE << "Failed to open dir " << tmp_dir.absolutePath().toStdString() << "\n";
    }

}

//---------------------------------------------------------------------------

void BatchRoiFitWidget::stopProcessing()
{
    if (_worker == nullptr)
    {
        return;
    }

    _canceled = true;
    // No processEvents() here. onProcessingFinished() will not run - we are most
    // likely inside the destructor - so the thread object is deleted directly once
    // it has stopped.
    _worker->wait();
    delete _worker;
    _worker = nullptr;
    _analysis_job.reset();
}

//---------------------------------------------------------------------------
