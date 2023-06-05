/*-----------------------------------------------------------------------------
 * Copyright (c) 2023, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include "mvc/BatchRoiFitWidget.h"
#include <QApplication>
#include <core/process_whole.h>
#include <QScrollArea>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDir>

/*---------------------------------------------------------------------------*/

BatchRoiFitWidget::BatchRoiFitWidget(std::string directory, QWidget *parent) : QWidget(parent)
{

    _directory = directory;
    _total_itr = 0;
    _canceled = false;
    if (_directory[directory.length() - 1] != DIR_END_CHAR)
    {
        _directory += DIR_END_CHAR;
    }
    createLayout();

}

/*---------------------------------------------------------------------------*/

BatchRoiFitWidget::~BatchRoiFitWidget()
{

 
}

/*---------------------------------------------------------------------------*/

void BatchRoiFitWidget::createLayout()
{

    _progressBarBlocks = new QProgressBar();
    _progressBarBlocks->setRange(0,100);
    _progressBarFiles = new QProgressBar();
    _progressBarFiles->setRange(0, 100);

    _btn_run = new QPushButton("Run");
    connect(_btn_run, &QPushButton::released, this, &BatchRoiFitWidget::runProcessing);
    _btn_cancel = new QPushButton("Cancel");
    connect(_btn_cancel, &QPushButton::released, this, &BatchRoiFitWidget::close);

    _le_detectors = new QLineEdit("0,1,2,3,4,5,6");
    
    _cb_opt_method = new QComboBox();
    _cb_opt_method->addItem(STR_LM_FIT);
    _cb_opt_method->addItem(STR_MP_FIT);
    _cb_opt_method->addItem(STR_HYBRID_MP_FIT);
    connect(_cb_opt_method, SIGNAL(currentIndexChanged(QString)), this, SLOT(optimizer_changed(QString)));

    _file_list_model = new QStandardItemModel();
    _file_list_view = new QListView();
    _file_list_view->setModel(_file_list_model);
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
    _optimizer_widget->setOptimizer(STR_LM_FIT, *(_analysis_job.optimizer()));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addItem(detector_hbox);
    layout->addWidget(_cb_opt_method);
    layout->addWidget(_optimizer_widget);
    layout->addWidget(_file_list_view);
    layout->addItem(buttonlayout);
    layout->addItem(hbox_progresss_blocks);
    layout->addItem(hbox_progresss_files);
    
    this->setLayout(layout);
}

/*---------------------------------------------------------------------------*/

void BatchRoiFitWidget::optimizer_changed(QString val)
{
    _analysis_job.set_optimizer(val.toStdString());
    _optimizer_widget->setOptimizer(val, *(_analysis_job.optimizer()));
}

/*---------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------*/

void BatchRoiFitWidget::status_callback(size_t cur_itr, size_t total_itr)
{
    if (_canceled)
    {
        throw 20;
    }

    if (_total_itr != total_itr)
    {
        _total_itr = total_itr;
        _progressBarBlocks->setRange(0, _total_itr);
    }

    if (cur_itr == _total_itr)
    {
        int val = _progressBarFiles->value() + 1;
        _progressBarFiles->setValue(val);
        
    }

    _progressBarBlocks->setValue(cur_itr);
    QCoreApplication::processEvents();
}


/*---------------------------------------------------------------------------*/

void BatchRoiFitWidget::runProcessing()
{
    _btn_run->setEnabled(false);
    _le_detectors->setEnabled(false);
    _file_list_view->setEnabled(false);

    //run in thread
    _analysis_job.dataset_directory = _directory;
    if (_cb_opt_method->currentText() == "Hybrid NNLS")
    {
        _analysis_job.set_optimizer("MPFIT");
        _analysis_job.optimize_fit_routine = OPTIMIZE_FIT_ROUTINE::HYBRID;
    }
    else
    {
        _analysis_job.set_optimizer(_cb_opt_method->currentText().toStdString());
    }

    _optimizer_widget->updateOptimizerOptions(*(_analysis_job.optimizer()));

    QString dstring = _le_detectors->text();
    if (dstring.length() > 0)
    {
        if (dstring.count(",") > 0)
        {
            QStringList dlist = dstring.split(",");
            foreach(QString str, dlist)
            {
                _analysis_job.detector_num_arr.push_back(str.toInt());
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
                    _analysis_job.detector_num_arr.push_back(i);
                }
            }
            catch (...)
            {
                QMessageBox::warning(this, "Bad detectors", "Could not parse detectors. For range use 'num:max' ");
                return;
            }
        }
        else
        {
            try
            {
                _analysis_job.detector_num_arr.push_back(dstring.toInt());
            }
            catch (...)
            {
                QMessageBox::warning(this, "Bad detectors", "Could not parse detectors. Please enter number for single detector, or use ',' for list of detectors. ");
                return;
            }
        }
    }
    else
    {
        QMessageBox::warning(this, "No detectors listed", "Please enter detctors ',' separated");
        return;
    }
    
    Callback_Func_Status_Def cb_func = std::bind(&BatchRoiFitWidget::status_callback, this, std::placeholders::_1, std::placeholders::_2);
    
    if (io::file::init_analysis_job_detectors(&_analysis_job))
    {
        _progressBarFiles->setRange(0, _roi_map.size() * _analysis_job.detector_num_arr.size());

        std::map<int, std::map<std::string, data_struct::Fit_Parameters<double>>> roi_fit_params;
        int i = 0;
        for (auto& itr : _roi_map)
        {
            i++;
            optimize_single_roi(_analysis_job, itr.second.fileName().toStdString(), roi_fit_params, &cb_func);
            _progressBarFiles->setValue(i);
            QCoreApplication::processEvents();
        }
    }
    
    _btn_run->setEnabled(true);
    _le_detectors->setEnabled(true);
    _file_list_view->setEnabled(true);
    _btn_cancel->setText("Close");

    // open file location
    QDir tmp_dir(QString(_directory.c_str()));
    tmp_dir.cd("output");
    if (false == QDesktopServices::openUrl(QUrl::fromLocalFile(tmp_dir.absolutePath())))
    {
        logE << "Failed to open dir " << tmp_dir.absolutePath().toStdString() << "\n";
    }

}

/*---------------------------------------------------------------------------*/