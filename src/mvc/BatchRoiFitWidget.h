/*-----------------------------------------------------------------------------
 * Copyright (c) 2023, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef BATCH_ROI_FIT_WIDGET_H
#define BATCH_ROI_FIT_WIDGET_H

//---------------------------------------------------------------------------

#include <QWidget>
#include <QThread>
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
#include <QComboBox>
#include <QEventLoop>
#include <QLineEdit>
#include <QGroupBox>
#include <qfileinfo.h>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <memory>
#include <data_struct/analysis_job.h>
#include "mvc/OptimizerOptionsWidget.h"

//---------------------------------------------------------------------------


class BatchRoiFitWidget : public QWidget
{

   Q_OBJECT

public:

   /**
    * Constructor.
    */
	BatchRoiFitWidget(std::string directory, QWidget *parent = nullptr);

   /**
    * Destructor.
    */
   ~BatchRoiFitWidget();

    void updateFileList(std::unordered_map<QString, QFileInfo> roi_map);

    // Called by the fitting code on the worker thread, not on the GUI thread.
    // It must not touch a widget; it only reads the cancel flag and posts a
    // progress update across to the GUI thread.
    void status_callback(size_t cur_itr, size_t total_itr);

signals:

    // Both are emitted from the fitting thread and delivered as queued events, so
    // the progress bars are only ever touched on the GUI thread. qulonglong rather
    // than size_t because a queued connection needs a type Qt has a metatype for.
    void blockProgress(qulonglong cur_itr, qulonglong total_itr);

    void fileProgress(qulonglong cur_file);

public slots:
    void runProcessing();

    void onClose();

protected slots:
    void onBlockProgress(qulonglong cur_itr, qulonglong total_itr);

    void onFileProgress(qulonglong cur_file);

    void onProcessingFinished();

protected:

   /**
    * @brief Create layout
    */
   void createLayout();

   // Cancels a running fit and blocks until the thread has left the analysis job.
   // Called from the destructor: the worker lambda holds this widget and
   // _analysis_job, so neither may be destroyed while it runs.
   void stopProcessing();

   // Everything the GUI thread has to read off the widgets before the fit starts.
   bool buildAnalysisJob();

   QStringList _file_list;
   
   QProgressBar *_progressBarFiles;

   QProgressBar* _progressBarBlocks;

   QPushButton *_btn_run;

   QPushButton *_btn_cancel;

   QListView* _file_list_view;

   std::unique_ptr<QStandardItemModel> _file_list_model;

   std::string _directory;

   QLineEdit* _le_detectors;

   std::unordered_map<QString, QFileInfo> _roi_map;

   size_t _total_itr;

   OptimizerOptionsWidget* _optimizer_widget;

   // Written by the GUI thread, read by the fitting thread.
   std::atomic<bool> _canceled;

   // Non-null exactly while a fit is running.
   QThread* _worker;

   // Owned here rather than on runProcessing()'s stack because the worker thread
   // outlives that call.
   std::unique_ptr<data_struct::Analysis_Job<double>> _analysis_job;
};


//---------------------------------------------------------------------------

#endif 

//---------------------------------------------------------------------------
