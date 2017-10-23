/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/LiveMapsElementsWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QDebug>

/*---------------------------------------------------------------------------*/

LiveMapsElementsWidget::LiveMapsElementsWidget(QWidget* parent) : QWidget(parent)
{

    _streamWorker = nullptr;
    createLayout();

}

/*---------------------------------------------------------------------------*/

LiveMapsElementsWidget::~LiveMapsElementsWidget()
{

    if(_streamWorker != nullptr)
    {
        _streamWorker->stop();
        _streamWorker->quit();
        _streamWorker->wait();
        delete _streamWorker;
    }
    _streamWorker = nullptr;

}

/*---------------------------------------------------------------------------*/

void LiveMapsElementsWidget::createLayout()
{

    QVBoxLayout* layout = new QVBoxLayout();
    _textEdit = new QTextEdit(this);
    _textEdit->resize(1024, 800);
    _textEdit->scrollBarWidgets(Qt::AlignRight);
    layout->addWidget(_textEdit);

    _progressBar = new QProgressBar(this);
    layout->addWidget(_progressBar);
    setLayout(layout);

    _streamWorker = new NetStreamWorker(this);
    connect(_streamWorker, &QThread::finished, _streamWorker, &QObject::deleteLater);
    connect(_streamWorker, &NetStreamWorker::newData, this, &LiveMapsElementsWidget::newDataArrived);
    _streamWorker->start();

}

/*---------------------------------------------------------------------------*/

void LiveMapsElementsWidget::newDataArrived(data_struct::xrf::Stream_Block *new_packet)
{
    static int cntr = 0;
    static bool first = true;
    //QString str = ">" + QString::number(new_packet->row()) + " " + QString::number(new_packet->col()) ;
    delete new_packet;
    //_textEdit->append(str);

    if(first || new_packet->row() == 0 && new_packet->col() == 0)
    {
        _progressBar->setRange(0, new_packet->height());
        first = !first;
    }

    _progressBar->setValue(new_packet->row());

    cntr ++;

    if(cntr > 100)
    {
        _textEdit->clear();
        _progressBar->update();
        cntr = 0;
    }
    if(new_packet->row() == new_packet->height())
    {
        _progressBar->update();
    }
}

/*---------------------------------------------------------------------------*/
