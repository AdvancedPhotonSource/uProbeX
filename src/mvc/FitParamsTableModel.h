/*-----------------------------------------------------------------------------
 * Copyright (c) 2014, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef FitParamsTableModel_H_
#define FitParamsTableModel_H_

#include <algorithm>
#include <QAbstractTableModel>
#include <QList>
#include <QModelIndex>
#include <QVariant>
#include "data_struct/xrf/fit_parameters.h"


/*---------------------------------------------------------------------------*/

/**
 * @brief model to control the solver table
 */
class FitParamsTableModel
: public QAbstractTableModel
{

   Q_OBJECT

public:

   /**
    * Properties corresponding to columns in table.
    */
   enum Properties {
      NAME,
      VALUE,
      MIN_VAL,
      MAX_VAL,
      STEP_SIZE,
      BOUND_TYPE,
       NUM_PROPS
   };

   /**
    * Constructor
    *
    * @param parent - parent Qt widget
    */
   FitParamsTableModel(QObject* parent = 0);

   void setFitParams(data_struct::xrf::Fit_Parameters* fit_params);

   /**
    * @brief Clear all
    */
   void clearAll();

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   int columnCount(const QModelIndex &parent) const {return NUM_PROPS;}

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   QVariant data(const QModelIndex &index,
                 int role = Qt::DisplayRole) const;

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   Qt::ItemFlags flags(const QModelIndex &index) const;


   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   QVariant headerData(int section,
                       Qt::Orientation orientation,
                       int role) const;

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   /*bool insertRows(int row, int count,
                   const QModelIndex& parent = QModelIndex());*/

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   bool removeRows(int row,
                   int count,
                   const QModelIndex& parent = QModelIndex());

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   int rowCount(const QModelIndex &parent) const;

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   bool setData(const QModelIndex &index,
                const QVariant &value,
                int role = Qt::EditRole);

   /**
    * @brief Reimplemented from QAbstractItemModel. See Qt documentation.
    */
   bool setHeaderData(int section,
                      Qt::Orientation orientation,
                      const QVariant &value,
                      int role = Qt::EditRole);


private:

   /**
    * @brief Header names
    */
   QString m_headers[NUM_PROPS];

   data_struct::xrf::Fit_Parameters _fit_parameters;

   std::vector<std::string> _row_indicies;

};

/*---------------------------------------------------------------------------*/

#endif

/*---------------------------------------------------------------------------*/