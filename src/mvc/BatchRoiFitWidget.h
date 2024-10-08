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
#include <thread>
#include <unordered_map>
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

    void status_callback(size_t cur_itr, size_t total_itr);

public slots:
    void runProcessing();

    void onClose();

protected:

   /**
    * @brief Create layout
    */
   void createLayout();

   QStringList _file_list;
   
   QProgressBar *_progressBarFiles;

   QProgressBar* _progressBarBlocks;

   QPushButton *_btn_run;

   QPushButton *_btn_cancel;

   QListView* _file_list_view;

   QStandardItemModel* _file_list_model;

   std::string _directory;

   QLineEdit* _le_detectors;

   std::unordered_map<QString, QFileInfo> _roi_map;

   size_t _total_itr;

   OptimizerOptionsWidget* _optimizer_widget;

   bool _canceled;
};


//---------------------------------------------------------------------------

#endif 

//---------------------------------------------------------------------------
