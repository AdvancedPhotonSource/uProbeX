/*-----------------------------------------------------------------------------
 * Copyright (c) 2023, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef PER_PIXEL_OPTIONS_WIDGET_H
#define PER_PIXEL_OPTIONS_WIDGET_H

//---------------------------------------------------------------------------

#include <QWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QEventLoop>
#include <QLineEdit>
#include <QGroupBox>
#include <QMessageBox>
#include <atomic>
#include <memory>
#include <data_struct/analysis_job.h>

//---------------------------------------------------------------------------


class PerPixelOptionsWidget : public QWidget
{

   Q_OBJECT

public:

   /**
    * Constructor.
    */
	PerPixelOptionsWidget(QWidget *parent = nullptr);

   /**
    * Destructor.
    */
   ~PerPixelOptionsWidget();

    void updateFileList(QStringList file_list);

    // Called by the fitting code on the worker thread, not on the GUI thread.
    // It must not touch a widget; it only reads the cancel flag and posts a
    // progress update across to the GUI thread.
    void status_callback(size_t cur_block, size_t total_blocks);

    void setRunning();

    void setStoped();

    // Asks a running fit to stop at its next status callback. Safe to call from
    // any thread.
    void cancel() { _canceled = true; }

    bool isCanceled() const { return _canceled.load(); }

    bool performQuantificationChecked() { return _perform_quantification->isChecked();  }

    QStringList fileList() { return _file_list;}

    void setExportOptionsVisible(bool val) {_saving_grp->setVisible(val);}

    template <typename T_real>
    void updateAnalysisJob(data_struct::Analysis_Job<T_real>& analysis_job)
    {
        if (_proc_roi->isChecked())
        {
            analysis_job.fitting_routines.push_back(data_struct::Fitting_Routines::ROI);
        }
        if (_proc_nnls->isChecked())
        {
            analysis_job.fitting_routines.push_back(data_struct::Fitting_Routines::NNLS);
        }
        if (_proc_matrix->isChecked())
        {
            analysis_job.fitting_routines.push_back(data_struct::Fitting_Routines::GAUSS_MATRIX);
        }

        if (_save_avg->isChecked())
        {
            analysis_job.generate_average_h5 = true;
        }

        //add v9 layout soft links
        if (_save_v9->isChecked())
        {
            analysis_job.add_v9_layout = true;
        }


        //add exchange
        if (_save_exchange->isChecked())
        {
            analysis_job.add_exchange_layout = true;
        }

        //export csv
        if (_save_csv->isChecked())
        {
            analysis_job.export_int_fitted_to_csv = true;
        }

        if (_perform_quantification->isChecked())
        {
            analysis_job.quantification_standard_filename = "maps_standardinfo.txt";
        }

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
                    return;
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
                    return;
                }
            }
        }
        else
        {
            QMessageBox::warning(this, "No detectors listed", "Please enter detctors ',' separated");
            return;
        }

        QModelIndex parent = QModelIndex();
        for (int r = 0; r < _file_list_model->rowCount(parent); ++r)
        {
            QModelIndex index = _file_list_model->index(r, 0, parent);
            QVariant name = _file_list_model->data(index);
            analysis_job.dataset_files.push_back(name.toString().toStdString());
        }

        _total_file_range = 0;
        if (analysis_job.quick_and_dirty == true)
        {
            _total_file_range = (analysis_job.fitting_routines.size() * analysis_job.dataset_files.size());
        }
        else
        {
            _total_file_range = (analysis_job.fitting_routines.size() * analysis_job.dataset_files.size() * analysis_job.detector_num_arr.size());
        }
        _progressBarFiles->setRange(0, _total_file_range);

    }

    Callback_Func_Status_Def status_callback_func;

    void incrCurBlock() { status_callback(_cur_block++, _cur_file); }

    void incrCurFile() { status_callback(_cur_block, _cur_file++); }

    void setStatusDone() { status_callback(_total_blocks, _total_file_range); }

signals:

    void onRunning();

    void onStop();

    // Emitted from the fitting thread; delivered to onStatusUpdate() as a queued
    // event so the progress bars are only ever touched on the GUI thread.
    // qulonglong rather than size_t because a queued connection needs a type Qt
    // has a metatype for, and size_t is not one of them.
    void statusUpdate(qulonglong cur_block, qulonglong total_blocks);

protected slots:
    void onCancel();

    void onStatusUpdate(qulonglong cur_block, qulonglong total_blocks);

protected:

   /**
    * @brief Create layout
    */
   void createLayout();

   // Written by the GUI thread, read by the fitting thread.
   std::atomic<bool> _canceled;

   size_t _total_file_range;

   QStringList _file_list;
   
   QTextEdit *_textEdit;

   QProgressBar *_progressBarFiles;

   QProgressBar* _progressBarBlocks;

   QPushButton *_btn_run;

   QPushButton *_btn_cancel;

   QListView* _file_list_view;

   std::unique_ptr<QStandardItemModel> _file_list_model;

   QCheckBox* _proc_roi;

   QCheckBox* _proc_nnls;

   QCheckBox* _proc_matrix;

   QCheckBox* _save_avg;

   QCheckBox* _save_v9;

   QCheckBox* _save_csv;
   
   QCheckBox* _save_exchange;

   QCheckBox* _perform_quantification;

   std::string _directory;

   size_t _cur_file;

   size_t _cur_block;
   
   QLineEdit* _le_detectors;
   
   QHBoxLayout* _proc_save_layout;

   QGroupBox* _saving_grp;

   QGroupBox* _processing_grp;

   size_t _total_blocks;
};


//---------------------------------------------------------------------------

#endif 

//---------------------------------------------------------------------------
