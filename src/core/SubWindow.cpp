/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <core/SubWindow.h>
#include <QMessageBox>
#include <QCloseEvent>
#include <QIcon>
#include <QMenu>

//---------------------------------------------------------------------------

SubWindow::SubWindow(QWidget* parent, Qt::WindowFlags flags)
: QMdiSubWindow(parent, flags)
{

   m_id = QUuid::createUuid();
   //this->setAttribute(Qt::WindowSystemMenuHint, false);
   //this->systemMenu()->setVisible(false);
   setWindowIcon(QIcon(":/images/title.png"));

}

//---------------------------------------------------------------------------

SubWindow::~SubWindow()
{

}

//---------------------------------------------------------------------------

// Deliberately does not chain to QMdiSubWindow::closeEvent(). That base
// implementation calls widget()->close() on the content widget, and
// MapsElementsWidget - the content widget of every analyzed-dataset window - sets
// Qt::WA_DeleteOnClose on itself, so chaining would schedule the content widget
// for deletion on top of the deletion it already gets as a child of this window
// when uProbeX::subWindowClosed() calls deleteLater() on us. Nothing is lost by
// skipping it: none of the three content widgets overrides closeEvent, so there
// is no veto or cleanup to run, and the QMdiArea bookkeeping happens when this
// window is actually destroyed.
void SubWindow::closeEvent(QCloseEvent* closeEvent)
{

   // Accepted explicitly rather than relying on QCloseEvent defaulting to
   // accepted: windowClosing() is what gets this window deleted, so the two have
   // to agree that the close is really happening.
   closeEvent->accept();

   emit windowClosing(this);

}

//---------------------------------------------------------------------------
QUuid SubWindow::getUuid()
{

   return m_id;

}

//---------------------------------------------------------------------------
