/*-----------------------------------------------------------------------------
 * Copyright (c) 2024, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include "gstar/Annotation/ScanRegionGraphicsItem.h"
#include "gstar/GStarResource.h"
#include <gstar/ImageViewScene.h>
#include "preferences/Preferences.h"
#include <QSize>
#include <iostream>

#include <QApplication>

using namespace gstar;

//---------------------------------------------------------------------------

ScanRegionGraphicsItem::ScanRegionGraphicsItem(AbstractGraphicsItem* parent)
   : UProbeRegionGraphicsItem(parent)
{

   _init();

}

//---------------------------------------------------------------------------

ScanRegionGraphicsItem::ScanRegionGraphicsItem(QMap<QString, QString>& marker,
                                                   AbstractGraphicsItem* parent)
                                                   : UProbeRegionGraphicsItem(marker, parent)
{
   _init();
}

//---------------------------------------------------------------------------

ScanRegionGraphicsItem::~ScanRegionGraphicsItem()
{
   if(_bs_plan.uuid.length() > 0)
   {
      emit planRemoved(_bs_plan);
   }
}

//---------------------------------------------------------------------------

void ScanRegionGraphicsItem::_init()
{

   _bs_plan.uuid = "";
   _queue_status = new AnnotationProperty(SCAN_REGION_QUEUE_STATUS, QVariant(STR_NOT_QUEUED));
   _plan_type = new AnnotationProperty(SCAN_REGION_PLAN_TYPE, QVariant(""));
   prependProperty(_plan_type);   
   prependProperty(_queue_status);

   connect(this, &AbstractGraphicsItem::viewUpdated, this, &ScanRegionGraphicsItem::checkIfPlanDirty);

}

//---------------------------------------------------------------------------

const QString ScanRegionGraphicsItem::displayName() const
{

   return QString("ScanRegion");

}

//---------------------------------------------------------------------------

ScanRegionGraphicsItem* ScanRegionGraphicsItem::cloneRegion()
{
   ScanRegionGraphicsItem* newRegion = new ScanRegionGraphicsItem();

   //newRegion->m_outlineColor = m_outlineColor;
   newRegion->m_rect = m_rect;
   newRegion->_bs_plan = _bs_plan;
   return newRegion;
}

//---------------------------------------------------------------------------

void ScanRegionGraphicsItem::setPlan(const BlueskyPlan& plan)
{

   _bs_plan = plan;

   if(_bs_plan.uuid.length() > 0)
   {
      _queued_pos = QPointF(m_predictXProp->getValue().toDouble(), m_predictYProp->getValue().toDouble());
      _queued_size = QSizeF(m_widthProp->getValue().toDouble(), m_heightProp->getValue().toDouble());
      _queue_status->setValue(STR_QUEUED);
      _plan_type->setValue(plan.type);
      updateView();
   }
   else
   {
      _queue_status->setValue(STR_NOT_QUEUED);
   }

}

//---------------------------------------------------------------------------

bool ScanRegionGraphicsItem::isQueued()
{
   if(_bs_plan.uuid.length() > 0)
   {
      _queue_status->setValue(STR_QUEUED);
      _plan_type->setValue(_bs_plan.type);
      updateView();
      return true;
   }
   
   _queue_status->setValue(STR_NOT_QUEUED);
   
   return false;
}

//---------------------------------------------------------------------------

void ScanRegionGraphicsItem::checkIfPlanDirty()
{
   QPointF cur_pos = QPointF(m_predictXProp->getValue().toDouble(), m_predictYProp->getValue().toDouble());
   QSizeF cur_size = QSizeF(m_widthProp->getValue().toDouble(), m_heightProp->getValue().toDouble());
   if(cur_pos != _queued_pos || cur_size != _queued_size)
   {
      onPlanDirty();
   }
}

//---------------------------------------------------------------------------

void ScanRegionGraphicsItem::onPlanDirty()
{
   // if plan is queue, check if scan links values are the same.
   if(_bs_plan.uuid.length() > 0)
   {
      bool found_link = false;
      QJsonObject scan_link;
      QString scan_name = Preferences::inst()->getValue(STR_PREF_LAST_SCAN_LINK_SELECTED).toString();
	   QJsonArray scan_link_profiles = Preferences::inst()->getValue(STR_PREF_SCAN_LINK_PROFILES).toJsonArray();
      for (const auto &scan_link_ref : scan_link_profiles)
      {
         scan_link  = scan_link_ref.toObject();
         if (scan_link.contains(STR_SCAN_TYPE))
         {
            QString l_scan_name = scan_link[STR_SCAN_TYPE].toString();
            if(_bs_plan.type == l_scan_name)
            {
               found_link = true;
               break;
            }
         }
      }
      if(found_link)
      {
         for( auto itr: scan_link.keys())
         {
            QString scan_link_value = scan_link.value(itr).toString();
            if(scan_link_value.length() > 0)
            {
               for(auto pitr : _bs_plan.parameters)
               {
                  if(pitr.name == scan_link_value)
                  {
                     QString val = this->getValueAsString(itr);
                     
                     if(val != pitr.default_val)
                     {
                        _queue_status->setValue(STR_NEED_UPDATE);
                        m_outlineColor = QColor(Qt::red);
                        return;
                     }
                  }
               }
            }
         }
      }
   }
}

//---------------------------------------------------------------------------