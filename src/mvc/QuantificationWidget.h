﻿/*-----------------------------------------------------------------------------
 * Copyright (c) 2023, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef QUANTIFICATION_WIDGET_H
#define QUANTIFICATION_WIDGET_H

//---------------------------------------------------------------------------

#include <QComboBox>
#include <QVBoxLayout>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QLogValueAxis>
#include "mvc/ChartView.h"
#include <mvc/MapsH5Model.h>
#include "preferences/Preferences.h"


//---------------------------------------------------------------------------

class QuantificationWidget : public QWidget
{

   Q_OBJECT

public:

   /**
    * Constructor.
    */
   QuantificationWidget(QWidget* parent = nullptr);

   /**
    * Destructor.
    */
   virtual ~QuantificationWidget();

   void setModel(MapsH5Model* h5_model);

   MapsH5Model *getModel(){return _model;}

public slots:
    void update(const QString&);

protected:

   /**
    * @brief Create layout
    */
   void _createLayout();

   bool _display_log10;

   QLineSeries* _calib_curve_series_k;
   QLineSeries* _calib_curve_series_l;
   QLineSeries* _calib_curve_series_m;

   QLogValueAxis* _axisYLog10;

   QCategoryAxis* _axisX;

   QValueAxis* _axisY;

   QAbstractAxis* _currentYAxis;

   QChart* _chart;

   QChartView* _chartView;

   QLineSeries* _line_series;

   QComboBox* _cb_analysis_types;

   QComboBox* _cb_scalers;

   MapsH5Model *_model;

   Calibration_curve<double>* _calib_curve;

   QList< QScatterSeries*> _scatter_list;

};


//---------------------------------------------------------------------------

#endif /* QuantificationWidget_H_ */

//---------------------------------------------------------------------------

