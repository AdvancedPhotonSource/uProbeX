/*-----------------------------------------------------------------------------
 * Copyright (c) 2019, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef PER_PIXEL_FIT_WIDGET_H
#define PER_PIXEL_FIT_WIDGET_H

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
#include <QThread>
#include <memory>
#include <data_struct/analysis_job.h>
#include "mvc/PerPixelOptionsWidget.h"

//---------------------------------------------------------------------------


class PerPixelFitWidget : public QWidget
{

   Q_OBJECT

public:

   /**
    * Constructor.
    */
	PerPixelFitWidget(QWidget *parent = nullptr);

   /**
    * Destructor.
    */
   ~PerPixelFitWidget();

   void updateFileList(QStringList file_list) { _optionsWidget->updateFileList(file_list); }

   void setDir(std::string directory);

signals:

    void processed_list_update(QStringList);

public slots:

    void runProcessing();

protected slots:

    void onProcessingFinished();

protected:

   /**
    * @brief Create layout
    */
   void createLayout();

   // Cancels a running fit and blocks until the thread has left the analysis job.
   // Called from the destructor: the worker lambda holds this widget, its options
   // widget and _analysis_job, so none of them may be destroyed while it runs.
   void stopProcessing();

   PerPixelOptionsWidget* _optionsWidget;

   std::string _directory;

   // Non-null exactly while a fit is running.
   QThread* _worker;

   // Owned here rather than on runProcessing()'s stack because the worker thread
   // outlives that call.
   std::unique_ptr<data_struct::Analysis_Job<double>> _analysis_job;

};


//---------------------------------------------------------------------------

#endif 

//---------------------------------------------------------------------------
