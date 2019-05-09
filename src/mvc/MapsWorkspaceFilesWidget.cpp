/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <mvc/MapsWorkspaceFilesWidget.h>


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QRegExp>
#include <QDebug>

/*---------------------------------------------------------------------------*/

FileTabWidget::FileTabWidget(QWidget* parent) : QWidget(parent)
{

    _contextMenu = new QMenu(("Context menu"), this);
    QAction* action = _contextMenu->addAction("Load");
    connect(action, SIGNAL(triggered()), this, SLOT(onLoadFile()));
    action = _contextMenu->addAction("UnLoad");
    connect(action, SIGNAL(triggered()), this, SLOT(onUnloadFile()));

    _file_list_model = new QStandardItemModel();
    _file_list_view = new QListView();
   // _file_list_view->setViewMode(QListView::IconMode);
    _file_list_view->setModel(_file_list_model);
    _file_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _file_list_view->setContextMenuPolicy(Qt::CustomContextMenu);
    _file_list_view->setSelectionMode(QAbstractItemView::ExtendedSelection); //MultiSelection

    connect(_file_list_view, SIGNAL(doubleClicked(const QModelIndex)),
                this, SLOT(onDoubleClickElement(const QModelIndex)));
    connect(_file_list_view, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(ShowContextMenu(const QPoint &)));

    _filter_line = new QLineEdit();
    connect(_filter_line, SIGNAL(textChanged(const QString &)),
            this, SLOT(filterTextChanged(const QString &)));

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addWidget(new QLabel("Filter"));
    hlayout->addWidget(_filter_line);

    QLayout* vlayout = new QVBoxLayout();
    vlayout->addItem(hlayout);
    vlayout->addWidget(_file_list_view);
    setLayout(vlayout);

}

/*---------------------------------------------------------------------------*/

void FileTabWidget::set_file_list(const map<QString, QFileInfo>& fileinfo_list)
{
    for(auto & itr : fileinfo_list)
    {
        _file_list_model->appendRow(new QStandardItem(QIcon(":/images/circle_gray.png"), itr.first));
    }
}

/*---------------------------------------------------------------------------*/

void FileTabWidget::ShowContextMenu(const QPoint &pos)
{
    _contextMenu->exec(mapToGlobal(pos));
}

/*---------------------------------------------------------------------------*/

void FileTabWidget::onDoubleClickElement(const QModelIndex idx)
{
    QVariant data = idx.data(0);
    emit onOpenItem(data.toString());
}

/*---------------------------------------------------------------------------*/

void FileTabWidget::onLoadFile()
{
    QModelIndexList list = _file_list_view->selectionModel()->selectedIndexes();
    for(int i =0; i<list.length(); i++)
    {
        QModelIndex idx = list.at(i);
        QVariant data = idx.data(0);
        emit onOpenItem(data.toString());
    }
}

/*---------------------------------------------------------------------------*/

void FileTabWidget::onUnloadFile()
{
    QModelIndexList list = _file_list_view->selectionModel()->selectedIndexes();
    for(int i =0; i<list.length(); i++)
    {
        QModelIndex idx = list.at(i);
        QVariant data = idx.data(0);
        emit onCloseItem(data.toString());
    }
}

/*---------------------------------------------------------------------------*/

void FileTabWidget::filterTextChanged(const QString &text)
{
    QString filter_text = _filter_line->text();

    if(filter_text.length() > 0)
    {
        QRegExp re (_filter_line->text());
        re.setPatternSyntax(QRegExp::Wildcard);

        for(int i=0; i < _file_list_model->rowCount(); i++)
        {
            _file_list_view->setRowHidden(i, true);
            QStandardItem *val = _file_list_model->item(i, 0);
            if(re.exactMatch(val->text()))
            {
                _file_list_view->setRowHidden(i, false);
            }
        }
    }
    else
    {
        for(int i=0; i < _file_list_model->rowCount(); i++)
        {
            _file_list_view->setRowHidden(i, false);
        }
    }
}

/*---------------------------------------------------------------------------*/

void FileTabWidget::loaded_file_status_changed(File_Loaded_Status status, const QString& filename)
{
    for(int i=0; i < _file_list_model->rowCount(); i++)
    {
        QStandardItem *val = _file_list_model->item(i, 0);
        if(filename == val->text())
        {
            switch(status)
            {
            case UNLOADED:
                val->setIcon(QIcon(":/images/circle_gray.png"));
                break;
            case LOADED:
                val->setIcon(QIcon(":/images/circle_green.png"));
                break;
            case FAILED_LOADING:
                val->setIcon(QIcon(":/images/circle_red.png"));
                break;
            }
            break;
        }
    }

}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MapsWorkspaceFilesWidget::MapsWorkspaceFilesWidget(QWidget* parent) : QWidget(parent)
{

    createLayout();

}

/*---------------------------------------------------------------------------*/

MapsWorkspaceFilesWidget::~MapsWorkspaceFilesWidget()
{

}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::createLayout()
{
    std::vector<std::string> bound_types {"Not Initialized", "Fixed", "Limited Low High", "Limited Low", "Limited High", "Fit"};
    _lbl_workspace = new QLabel();
    _tab_widget = new QTabWidget();

    _h5_tab_widget = new FileTabWidget();
    connect(_h5_tab_widget, SIGNAL(onOpenItem(QString)),
            this, SLOT(onOpenHDF5(QString)));

    connect(_h5_tab_widget, SIGNAL(onCloseItem(QString)),
            this, SLOT(onCloseHDF5(QString)));

    connect(this, SIGNAL(loaded_hdf5(File_Loaded_Status, const QString&)),
            _h5_tab_widget, SLOT(loaded_file_status_changed(File_Loaded_Status, const QString&)));

    _mda_tab_widget = new FileTabWidget();
    connect(_mda_tab_widget, SIGNAL(onOpenItem(QString)),
            this, SLOT(onOpenMDA(QString)));

    connect(_mda_tab_widget, SIGNAL(onCloseItem(QString)),
            this, SLOT(onCloseMDA(QString)));

    connect(this, SIGNAL(loaded_mda(File_Loaded_Status, const QString&)),
            _mda_tab_widget, SLOT(loaded_file_status_changed(File_Loaded_Status, const QString&)));

    _sws_tab_widget = new FileTabWidget();
    connect(_sws_tab_widget, SIGNAL(onOpenItem(QString)),
            this, SLOT(onOpenSWS(QString)));

    connect(_sws_tab_widget, SIGNAL(onCloseItem(QString)),
            this, SLOT(onCloseSWS(QString)));

    connect(this, SIGNAL(loaded_sws(File_Loaded_Status, const QString&)),
            _sws_tab_widget, SLOT(loaded_file_status_changed(File_Loaded_Status, const QString&)));

    _fit_params_table_model = new FitParamsTableModel();
    ComboBoxDelegate *cbDelegate = new ComboBoxDelegate(bound_types);

    _fit_params_table = new QTableView();
    _fit_params_table->setModel(_fit_params_table_model);
    //_fit_params_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //_fit_params_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _fit_params_table->sortByColumn(0, Qt::AscendingOrder);
    _fit_params_table->setItemDelegateForColumn(2, cbDelegate);

    QLayout* vlayout = new QVBoxLayout();

    _tab_widget->insertTab(0, _h5_tab_widget, "Analyized H5");
    _tab_widget->insertTab(1, _mda_tab_widget, "Raw MDA");
    _tab_widget->insertTab(2, _sws_tab_widget, "Light Microscope");

    vlayout->addWidget(_tab_widget);
    setLayout(vlayout);
}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::setModel(MapsWorkspaceModel *model)
{
    _model = model;
    QString path = _model->get_directory_name();
    if(path.length() > 0)
    {
        _lbl_workspace->setText(path);
    }
    connect(_model,
            SIGNAL(doneLoading()),
            this,
            SLOT(model_done_loading()));

    connect(_model,
            SIGNAL(newFitParamsFileLoaded(int)),
            this,
            SLOT(loadedFitParams(int)));


}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::model_done_loading()
{

    _h5_tab_widget->set_file_list(_model->get_hdf5_file_list());
    _mda_tab_widget->set_file_list(_model->get_mda_file_list());
    _sws_tab_widget->set_file_list(_model->get_sws_file_list());

}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::loadedFitParams(int idx)
{
    if(idx == -1) //avg fit params
    {
        data_struct::Fit_Parameters* fit_params = _model->getFitParameters(idx);
        if(fit_params != nullptr)
        {
            _fit_params_table_model->setFitParams(*fit_params);
        }
    }
}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::onOpenHDF5(QString name)
{
    if(_model != nullptr)
    {
        File_Loaded_Status load_status = UNLOADED;
        MapsH5Model* h5Model = _model->getMapsH5Model(name);
        if(h5Model != nullptr)
        {
            load_status = LOADED;
            emit showFitSpecWindow(h5Model, _model->getFitParameters(-1), _model->getElementToFit(-1));
        }
        else
        {
            load_status = FAILED_LOADING;
        }
        emit loaded_hdf5(load_status, name);
    }
}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::onCloseHDF5(QString name)
{
    if(_model != nullptr)
    {
        File_Loaded_Status load_status = UNLOADED;
        _model->unload_H5_Model(name);
        emit loaded_hdf5(load_status, name);
    }
}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::onOpenMDA(QString name)
{
    if(_model != nullptr)
    {
        File_Loaded_Status load_status = UNLOADED;
        MDA_Model* mda_model = _model->get_MDA_Model(name);
        if(mda_model != nullptr)
        {
            load_status = LOADED;
            emit show_MDA_Window(mda_model);
        }
        else
        {
            load_status = FAILED_LOADING;
        }
        emit loaded_mda(load_status, name);
    }
}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::onCloseMDA(QString name)
{
    if(_model != nullptr)
    {
        File_Loaded_Status load_status = UNLOADED;
        _model->unload_MDA_Model(name);
        emit loaded_mda(load_status, name);
    }
}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::onOpenSWS(QString name)
{
    if(_model != nullptr)
    {
        File_Loaded_Status load_status = UNLOADED;
        SWSModel* sws_model = _model->get_SWS_Model(name);
        if(sws_model != nullptr)
        {
            load_status = LOADED;
            emit show_SWS_Window(sws_model);
        }
        else
        {
            load_status = FAILED_LOADING;
        }
        emit loaded_sws(load_status, name);
    }
}

/*---------------------------------------------------------------------------*/

void MapsWorkspaceFilesWidget::onCloseSWS(QString name)
{
    if(_model != nullptr)
    {
        File_Loaded_Status load_status = UNLOADED;
        _model->unload_SWS_Model(name);
        emit loaded_sws(load_status, name);
    }
}

/*---------------------------------------------------------------------------*/