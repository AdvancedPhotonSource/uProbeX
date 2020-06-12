/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef MDA_WIDGET_H
#define MDA_WIDGET_H

/*---------------------------------------------------------------------------*/

#include <QComboBox>
#include <QTableWidget>
#include <gstar/AbstractImageWidget.h>
#include <mvc/MDA_Model.h>
#include <mvc/FitSpectraWidget.h>

/*---------------------------------------------------------------------------*/

class MDA_Widget : public QWidget
{

   Q_OBJECT

public:

   /**
    * Constructor.
    */
   MDA_Widget(QWidget* parent = nullptr);

   /**
    * Destructor.
    */
   ~MDA_Widget();

   //void setModel(MapsH5Model* model);
   void setModel(MDA_Model* model);

   MDA_Model *getModel(){return _model;}

public slots:

   void model_updated();

   void onDetectorSelect(const QString& det);

   void onScalerSelect(const QString& det);

   void on_export_fit_params(data_struct::Fit_Parameters fit_params);

protected:

   void createLayout();

   FitSpectraWidget* _spectra_widget;

   QTabWidget* _tab_widget;

   MDA_Model *_model;

   QComboBox* _cb_detector;

   QComboBox* _cb_scaler;

   QTableWidget* _scaler_table_widget;

   QTableWidget* _extra_pvs_table_widget;

};


/*---------------------------------------------------------------------------*/

#endif /* MapsElementsWidget_H_ */

/*---------------------------------------------------------------------------*/