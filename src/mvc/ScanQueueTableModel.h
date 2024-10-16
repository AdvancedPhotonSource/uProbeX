//-----------------------------------------------------------------------------
 // Copyright (c) 2024, UChicago Argonne, LLC
 // See LICENSE file.
 //---------------------------------------------------------------------------

#ifndef ScanQueueTableModel_H
#define ScanQueueTableModel_H

//---------------------------------------------------------------------------

#include <QAbstractTableModel>
#include "mvc/BlueskyPlan.h"
#include <QMimeData>
#include <QIODevice>
#include <QColor>
#include <QDateTime>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class ScanQueueTableModel : public QAbstractTableModel 
{

    Q_OBJECT

public:
    ScanQueueTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) 
    {
        _headers[0] = "Type";
        _last_finished_idx = 0;
    }
    //---------------------------------------------------------------------------
    int rowCount(const QModelIndex& parent = QModelIndex()) const override 
    {
        return _data.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override 
    {
        if(_data.size() > 0)
        {
            return _data.at(0).parameters.size() + 5; // +1 for scan type, + 4 for results
        }
        return 10; 
    }
  
    //---------------------------------------------------------------------------

    void clear()
    {
        _data.clear();
    }

    //---------------------------------------------------------------------------

    const BlueskyPlan& item(int row)
    {
        return _data.at(row);
    }

    //---------------------------------------------------------------------------

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    //---------------------------------------------------------------------------

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override 
    {
        if (action == Qt::IgnoreAction)
        {
            return true;
        }
        if(data != nullptr)
        {
            if( data->hasFormat("application/x-qabstractitemmodeldatalist") )
            {
                QByteArray bytes=data->data("application/x-qabstractitemmodeldatalist");
                QDataStream stream(&bytes,QIODevice::QIODevice::ReadOnly);
                int srow, scol;
                QMap<int,  QVariant> roleDataMap;
                stream >> srow >> scol >> roleDataMap;
                emit moveScanRow(srow - _last_finished_idx, parent.row() - _last_finished_idx);
            }
            
        }
        return QAbstractTableModel::dropMimeData(data, action, row, 0, parent);
    }

    //---------------------------------------------------------------------------

    bool moveRows(const QModelIndex &srcParent, int srcRow, int count, const QModelIndex &dstParent, int dstChild) override
    {
        /*beginMoveRows(QModelIndex(), srcRow, srcRow + count - 1, QModelIndex(), dstChild);
        for(int i = 0; i<count; ++i) 
        {
            m_data.insert(dstChild + i, m_data[srcRow]);
            int removeIndex = dstChild > srcRow ? srcRow : srcRow+1;
            m_data.removeAt(removeIndex);
        }
        endMoveRows();
        */
        emit moveScanRow(srcParent.row() - _last_finished_idx, dstParent.row() - _last_finished_idx);
        return true;
    }

    //---------------------------------------------------------------------------

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override 
    {
        if (!index.isValid() || index.row() >= _data.size())
        {
            return QVariant();
        }

        const BlueskyPlan& rowData = _data[index.row()];

        int section = index.column();
        int parms_size = rowData.parameters.size();
        if(section > parms_size + 5)
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole) 
        {
            if(section == 0)
            {
                return rowData.type;
            }
            else if(section == (parms_size + 1))
            {
                return rowData.result.exit_status;
            }
            else if(section == (parms_size + 2))
            {
                if(rowData.result.time_start == 0.0)
                {
                    return " ";
                }
                QDateTime dateTime = QDateTime::fromSecsSinceEpoch(rowData.result.time_start, Qt::UTC);
                return dateTime.toString("yyyy-MM-dd hh:mm:ss");
            }
            else if(section == (parms_size + 3))
            {
                if(rowData.result.time_stop == 0.0)
                {
                    return " ";
                }
                QDateTime dateTime = QDateTime::fromSecsSinceEpoch(rowData.result.time_stop, Qt::UTC);
                return dateTime.toString("yyyy-MM-dd hh:mm:ss");
            }
            else if(section == (parms_size + 4))
            {
                return rowData.result.msg;
            }

            int idx = 0;
            for( auto itr: rowData.parameters)
            {
                if(idx == section -1 )
                {
                    return itr.second.default_val;
                }
                idx++;
            }
            
        }
        else if (role == Qt::BackgroundRole)
        {
            if(_data.at(index.row()).result.exit_status.length() > 0)
            {
                return QColor(Qt::darkGray);
            }
            else if(_data.at(index.row()).result.time_start > 0.0)
            {
                return QColor(Qt::darkGreen);
            }
        }
        return QVariant();
    }
    //---------------------------------------------------------------------------
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        // Check this is DisplayRole
        if (role != Qt::DisplayRole) return QVariant();

        // Horizontal headers
        if (orientation == Qt::Horizontal)
        {
            if(section == 0)
            {
                return _headers[section];
            }
            else
            {
                if(_data.size() > 0)
                {
                    const BlueskyPlan& rowData = _data[0];
                    int parms_size = rowData.parameters.size();

                    if(section == (parms_size + 1))
                    {
                        return "exit_status";
                    }
                    else if(section == (parms_size + 2))
                    {
                        return "time_start";
                    }
                    else if(section == (parms_size + 3))
                    {
                        return "time_stop";
                    }
                    else if(section == (parms_size + 4))
                    {
                        return "msg";
                    }
                    int idx = 0;
                    for( auto itr: rowData.parameters)
                    {
                        if(idx == section -1 )
                        {
                            return itr.second.name;
                        }
                        idx++;
                    }
                }
                return QVariant(" ");
            }
        }

        // Return empty data
        return QVariant();
    }
    
    //---------------------------------------------------------------------------

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
        {
            return Qt::NoItemFlags;
        }
        
        if(index.row() < _data.size())
        {
            if(_data.at(index.row()).result.exit_status.length() > 0)
            {
                return  Qt::ItemIsSelectable;
            }
        }

        if(index.column() == 0)
        {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    
    //---------------------------------------------------------------------------

    void setAllData(std::vector<BlueskyPlan> &finished_plans, std::vector<BlueskyPlan> &queued_plans, BlueskyPlan &running_plan)
    {
        _last_finished_idx = finished_plans.size();
        beginResetModel();
        _data.clear();
        for(auto itr : finished_plans)
        {
            _data.append(itr);
        }
        if(running_plan.type.length() > 0)
        {
            _data.append(running_plan);
        }
        for(auto itr : queued_plans)
        {
            _data.append(itr);
        }
        endResetModel();
    }

    //---------------------------------------------------------------------------

    bool setData(const QModelIndex& index, const QVariant& value, int role) override
    {
        if (role == Qt::EditRole && index.isValid())
        {
            if( index.row() < _data.size() && index.column() != 0)
            {
                int idx = 1;
                for (auto &itr : _data[index.row()].parameters)
                {
                    if(idx == index.column() && value.toString().length() >0)
                    {
                        // this will be refreshed from qserver
                        itr.second.default_val = value.toString();
                        emit planChanged(_data.at(index.row()));
                        return false; // return false to make sure we get this value from qserver 
                    }
                    idx ++;
                }   
            }
        }
        return false;

    }

    //---------------------------------------------------------------------------

    const int get_finished_idx() {return _last_finished_idx;}

    //---------------------------------------------------------------------------
signals:
    void moveScanRow(int, int);
    void planChanged(const BlueskyPlan&);  

private:
    int _last_finished_idx;
    QList<BlueskyPlan> _data;

    QString _headers[4];
};


//---------------------------------------------------------------------------

#endif /* ScanQueueTableModel.h */

//---------------------------------------------------------------------------