/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/MapsWorkspaceController.h>
#include <QApplication>

MapsWorkspaceController::MapsWorkspaceController(QObject* parent) : QObject(parent)
{
	_mapsWorkspaceModel = std::make_unique<MapsWorkspaceModel>();
	_imgStackControllWidget = new ImageStackControlWidget();

	_imgStackControllWidget->setModel(_mapsWorkspaceModel.get());

	_imgStackControllWidget->setAttribute(Qt::WA_DeleteOnClose);
	connect(_imgStackControllWidget, &ImageStackControlWidget::widgetClosed, this, &MapsWorkspaceController::imgWidgetClosed);

	QPoint pos = QGuiApplication::screens().at(0)->geometry().center();
	_imgStackControllWidget->show();

	pos.setX(pos.x() - (_imgStackControllWidget->width() * 0.5));
	pos.setY(pos.y() - (_imgStackControllWidget->height() * 0.5));
	_imgStackControllWidget->move(pos);


}

//---------------------------------------------------------------------------

MapsWorkspaceController::~MapsWorkspaceController()
{

	if(_imgStackControllWidget != nullptr)
	{
		_imgStackControllWidget->setModel(nullptr);
		delete _imgStackControllWidget;
	}
}

//---------------------------------------------------------------------------


void MapsWorkspaceController::imgWidgetClosed()
{
	if (_imgStackControllWidget != nullptr)
	{
		_imgStackControllWidget->savePref();

		// Teardown ordering. This runs inside the widget's closeEvent(), and
		// WA_DeleteOnClose only posts a deleteLater(), so the widget outlives this
		// call by the rest of the event loop iteration. controllerClosed() below
		// reaches uProbeX::mapsControllerClosed(), which does "delete controller"
		// and so destroys _mapsWorkspaceModel right now. Detaching the widget here,
		// while both are still alive, is what keeps it from spending that window
		// holding pointers into a freed model - and MapsWorkspaceFilesWidget's
		// setModel() also waits out any thread pool job still running inside it.
		//
		// The widget cannot simply be deleted instead: we are on its own stack.
		_imgStackControllWidget->setModel(nullptr);
	}
	_imgStackControllWidget = nullptr;
	emit controllerClosed(this);

}

//---------------------------------------------------------------------------

void MapsWorkspaceController::setWorkingDir(QString path)
{

    _mapsWorkspaceModel->unload();

    _mapsWorkspaceModel->load(path);

	_imgStackControllWidget->setWindowTitle(path);

}
//---------------------------------------------------------------------------

bool MapsWorkspaceController::openRoiDialog()
{
	return _imgStackControllWidget != nullptr && _imgStackControllWidget->openRoiDialog();
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
