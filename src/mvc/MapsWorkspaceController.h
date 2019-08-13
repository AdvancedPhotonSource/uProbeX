/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef MAPS_WORKSPACE_CONTROLLER_H
#define MAPS_WORKSPACE_CONTROLLER_H

/*---------------------------------------------------------------------------*/

#include <QObject>
#include <QMainWindow>
#include <QDockWidget>
#include <mvc/MapsWorkspaceModel.h>
#include <mvc/MapsWorkspaceFilesWidget.h>
#include <mvc/ImageStackControlWidget.h>


class MapsWorkspaceController : public QObject
{

   Q_OBJECT

public:

   /**
    * Constructor.
    */
	MapsWorkspaceController(QMainWindow * parentWindow, QObject* parent = nullptr);

   /**
    * Destructor.
    */
   ~MapsWorkspaceController();

   void setWorkingDir(QString path);

protected:

	MapsWorkspaceFilesWidget* _mapsFilsWidget;

	ImageStackControlWidget* _imgStackControllWidget;

	MapsWorkspaceModel* _mapsWorkspaceModel;

	ImageGridWidget* _imageGrid;

	QMainWindow * _parentWindowPtr;

	QDockWidget *_maps_workspace_dock;

	QDockWidget *_image_stack_control_dock;

};


/*---------------------------------------------------------------------------*/

#endif /* MapsWorkspaceController_H_ */

/*---------------------------------------------------------------------------*/

