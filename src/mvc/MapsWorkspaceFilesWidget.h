/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef MAPS_WORKSPACE_FILES_WIDGET_H
#define MAPS_WORKSPACE_FILES_WIDGET_H

//---------------------------------------------------------------------------

#include <QAction>
#include <QMenu>
#include <QWidget>
#include <QLabel>
#include <memory>
#include <mvc/MapsWorkspaceModel.h>
#include <QListView>
#include <QTableView>
#include <QStringListModel>
#include <QListWidgetItem>
#include <QPushButton>
#include <QTimer>
#include <future>
#include "FitParamsTableModel.h"
#include "mvc/ComboBoxDelegate.h"
#include "mvc/FileTabWidget.h"
#include "mvc/PerPixelFitWidget.h"
#include "mvc/BatchRoiFitWidget.h"
#include <mvc/GenScanVlmWidget.h>

enum class MODEL_TYPE { MAPS_H5, RAW, VLM };

//---------------------------------------------------------------------------
/**
 * @brief When open the acquisition window, the widget is showing for capturing
 * the image from the area detector writer, the window will also be updated to
 * show the image.
 */
class MapsWorkspaceFilesWidget : public QWidget
{

   Q_OBJECT

public:

   /**
    * Constructor.
    */
   MapsWorkspaceFilesWidget(QWidget* parent = nullptr);

   /**
    * Destructor.
    */
   ~MapsWorkspaceFilesWidget();

   void setModel(MapsWorkspaceModel* model);

   void setLabelWorkspacePath(QString path) {_lbl_workspace->setText(path);}

   void setFileTabActionsEnabled(bool val);

signals:

   void loaded_model(const QString, MODEL_TYPE);

   void unloadList_model(const QStringList, MODEL_TYPE);

   void loaded_perc(int, int);

   void datasetSelected(const QString);

public slots:

   void updatedMDA();

   void updatedVLM();

   void updatedHDF();

   void updateROIS();

   void update_roi_num(QString, int);

   void clearLists();

   void onOpenModel(const QStringList& names_list, MODEL_TYPE mt);

   void onCloseModel(const QStringList& names_list, MODEL_TYPE mt);

   void closeAllModels(MODEL_TYPE mt);

   void onCustomContext(const QString& context_label, const QStringList& file_list);

   void onCustomButton(const QString& context_label, const QStringList& file_list);

   void onPerPixelProcessList(const QStringList& file_list);

   void onPerPixelProcessListAnalyzed(const QStringList& file_list);

   void onBatchRoiList(const QStringList& file_list);

   void onProcessed_list_update(QStringList file_list);

   void onDatasetSelected(const QString name);

   void onGenScanArea(const QStringList& file_list);

   void newScanArea(const QString& name);
   
protected:

   /**
    * @brief Create layout
    */
   void createLayout();

   // The onOpenModel() state machine. One file at a time: _startNextOpen() takes the
   // next name off _open_queue, _pollOpenJob() collects the thread pool future from
   // the real event loop, _completeOpen*() does the GUI side work, and
   // _advanceOpenBatch() re-enters _startNextOpen() through a zero delay single shot.
   void _startNextOpen();

   void _pollOpenJob();

   void _completeOpenH5(std::shared_ptr<MapsH5Model> h5_model);

   void _completeOpenRaw(RAW_Model* raw_model);

   void _advanceOpenBatch();

   void _finishOpenBatch();

   // Blocks until any in flight open job has finished. The pool jobs hold the
   // MapsWorkspaceModel pointer, so neither this widget nor that model may be torn
   // down while one is still running.
   void _waitForOpenJob();

private:

   QTabWidget *_tab_widget;

   FileTabWidget* _h5_tab_widget;

   FileTabWidget* _mda_tab_widget;

   FileTabWidget* _vlm_tab_widget;

   QLabel* _lbl_workspace;

   MapsWorkspaceModel* _model;

   std::unique_ptr<PerPixelFitWidget> _per_pixel_fit_widget;

   std::unique_ptr<BatchRoiFitWidget> _batch_roi_fit_widget;

   GenScanVlmWidget _gen_scan_vlm_widget;

   // Names still waiting to be opened, and the name currently being opened.
   QStringList _open_queue;

   QString _open_current;

   MODEL_TYPE _open_type;

   // Non zero exactly while a batch is draining.
   int _open_total;

   int _open_done;

   // setOverrideCursor() and restoreOverrideCursor() have to pair across the
   // callbacks the batch is now spread over.
   bool _open_cursor_set;

   QTimer _open_poll_timer;

   // At most one of these is valid at a time; which one says what is in flight.
   std::future<std::shared_ptr<MapsH5Model>> _open_h5_future;

   std::future<RAW_Model*> _open_raw_future;
};


//---------------------------------------------------------------------------

#endif /* MapsWorkspaceFilesWidget_H_ */

//---------------------------------------------------------------------------

