/*-----------------------------------------------------------------------------
 * Copyright (c) 2019, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/PerPixelFitWidget.h>
#include <core/process_whole.h>
#include <core/Hdf5Mutex.h>

//---------------------------------------------------------------------------

PerPixelFitWidget::PerPixelFitWidget(QWidget *parent) : QWidget(parent)
{

    _worker = nullptr;
    createLayout();

}

//---------------------------------------------------------------------------

PerPixelFitWidget::~PerPixelFitWidget()
{

    stopProcessing();

}

//---------------------------------------------------------------------------

void PerPixelFitWidget::createLayout()
{

    _optionsWidget = new PerPixelOptionsWidget();

    connect(_optionsWidget, &PerPixelOptionsWidget::onRunning, this, &PerPixelFitWidget::runProcessing);
    connect(_optionsWidget, &PerPixelOptionsWidget::onStop, this, &PerPixelFitWidget::close);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(_optionsWidget);
    
    setLayout(layout);
}

//---------------------------------------------------------------------------

void PerPixelFitWidget::setDir(std::string directory)
{
    _directory = directory;
    if (false == _directory.empty() && _directory.back() != DIR_END_CHAR)
    {
        _directory += DIR_END_CHAR;
    }
}

//---------------------------------------------------------------------------

// The whole body of this used to run on the GUI thread, kept "responsive" by the
// processEvents() inside PerPixelOptionsWidget::status_callback(). That pumped close
// events from inside the fit: closing the workspace during a per pixel fit destroyed
// the MapsWorkspaceFilesWidget that owns this widget, and the fit then kept running
// against freed memory and returned into a freed stack frame. The fit now runs on a
// worker thread, nothing pumps events, and the destructor waits for the thread.
void PerPixelFitWidget::runProcessing()
{
    if (_directory.empty())
    {
        logE << "Cannot run per-pixel fitting without a dataset directory\n";
        return;
    }

    if (_worker != nullptr)
    {
        // Already fitting. The Run button is disabled while a fit is in flight, but
        // the signal can still arrive from elsewhere.
        return;
    }

    // TODO: fix so we can do float
    // Heap allocated and owned by the widget: the worker thread outlives this call.
    _analysis_job = std::make_unique<data_struct::Analysis_Job<double>>();
    _analysis_job->dataset_directory = _directory;
    _analysis_job->output_dir = _analysis_job->dataset_directory;

    // Everything that reads a widget stays here, on the GUI thread.
    _optionsWidget->setRunning();
    _optionsWidget->updateAnalysisJob<double>(*_analysis_job);

    data_struct::Analysis_Job<double>* analysis_job = _analysis_job.get();
    Callback_Func_Status_Def* status_func = &_optionsWidget->status_callback_func;
    const std::string directory = _directory;
    const bool do_quantification = _optionsWidget->performQuantificationChecked();

    _worker = QThread::create([analysis_job, status_func, directory, do_quantification]()
    {
        try
        {
            hdf5::Lock hdf5_lock(hdf5::mutex());
            if (io::file::init_analysis_job_detectors(analysis_job))
            {
                io::file::File_Scan::inst()->populate_netcdf_hdf5_files(directory);

                process_dataset_files(analysis_job, status_func);
                if (do_quantification)
                {
                    perform_quantification(analysis_job, true);
                }
            }
            iterate_datasets_and_update(*analysis_job);
        }
        catch (...)
        {
            // status_callback() throws 20 when the user presses Cancel. Nothing may
            // escape QThread::run() either, so this catch-all stays.
            logI << "Per pixel fitting canceled\n";
        }
    });

    connect(_worker, &QThread::finished, this, &PerPixelFitWidget::onProcessingFinished);
    _worker->start();
}

//---------------------------------------------------------------------------

void PerPixelFitWidget::onProcessingFinished()
{
    emit processed_list_update(_optionsWidget->fileList());
    _optionsWidget->setStoped();

    if (_worker != nullptr)
    {
        // deleteLater, not delete: this slot is reached from the thread object's own
        // finished() signal.
        _worker->deleteLater();
        _worker = nullptr;
    }
    _analysis_job.reset();
}

//---------------------------------------------------------------------------

void PerPixelFitWidget::stopProcessing()
{
    if (_worker == nullptr)
    {
        return;
    }

    _optionsWidget->cancel();
    // No processEvents() here. onProcessingFinished() will not run - we are most
    // likely inside the destructor - so the thread object is deleted directly once
    // it has stopped.
    _worker->wait();
    delete _worker;
    _worker = nullptr;
    _analysis_job.reset();
}

//---------------------------------------------------------------------------
