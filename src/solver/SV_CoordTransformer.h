/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#ifndef SV_COORDINATE_TRANSFORMER_H
#define SV_COORDINATE_TRANSFORMER_H

//---------------------------------------------------------------------------

#include <gstar/ITransformer.h>
#include <QString>
#include <QMap>
#include "preferences/Attribute.h"
#include <unordered_map>

//---------------------------------------------------------------------------

/**
 * @brief
 *
 */
class SV_CoordTransformer : public gstar::ITransformer
{

public:

   /**
    * Constructor.
    */
   SV_CoordTransformer();

   /**
    * Destructor.
    */
   virtual ~SV_CoordTransformer();

   /**
    * @brief Init
    * @param globalVars
    * @return
    */
   bool Init(QMap<QString, double> globalVars);

   /**
    * @brief getAllCoef
    * @return
    */
   virtual QMap<QString, double> getAllCoef();

   /**
    * @brief getVariable
    * @param name
    * @param val
    * @return
    */
   virtual bool getVariable(QString name, double* val);

   /**
    * @brief setVariable
    * @param name
    * @param val
    * @return
    */
   virtual bool setVariable(QString name, double val);

   /**
    * @brief transformCommand
    * @param inX
    * @param inY
    * @param inZ
    * @param outX
    * @param outY
    */
   virtual void transformCommand(double inX,
                                 double inY,
                                 double inZ,
                                 double *outX,
                                 double *outY,
                                 double *outZ);

protected:

   // Every coefficient below is initialized here because the constructor body is empty
   // and Init() assigns each one only "if (globalVars.count(key) > 0)". A profile that
   // does not supply a key therefore left that coefficient indeterminate and then used
   // it in transformCommand()'s arithmetic. SolverParameterParse treats only m2xfm_x
   // and m2xfm_y as required, so the other twelve were routinely unset.
   // Scale factors default to 1.0 and everything else to 0.0, the neutral affine
   // transform; garbage coordinates become identity coordinates rather than UB.

   /**
    * @brief m_2xfm_x
    */
   double m_2xfm_x = 0.0;

   /**
    * @brief m_2xfm_y
    */
   double m_2xfm_y = 0.0;

   /**
    * @brief m_angleAlpha
    */
   double m_angle_alpha = 0.0;

   /**
    * @brief m_offset_a
    */
   double m_offset_a = 0.0;

   /**
    * @brief m_offset_b
    */
   double m_offset_b = 0.0;

   /**
    * @brief m_offsetC
    */
   double m_offset_c = 0.0;

   /**
    * @brief m_offset_d
    */
   double m_offset_d = 0.0;

   /**
    * @brief m_omega
    */
   double m_omega = 0.0;

   /**
    * @brief m_omega_prime
    */
   double m_omega_prime = 0.0;

   /**
    * @brief m_scaling_XFM_X
    */
   double m_scaling_XFM_X = 1.0;

   /**
    * @brief m_scaling_XFM_Y
    */
   double m_scaling_XFM_Y = 1.0;

   /**
    * @brief m_z_offset
    */
   double m_z_offset = 0.0;

   /**
    * @brief m_z_lin_x
    */
   double m_z_lin_x = 0.0;

   /**
    * @brief m_z_lin_y
    */
   double m_z_lin_y = 0.0;

};

//---------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------
