/*-----------------------------------------------------------------------------
 * Copyright (c) 2012, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include <gstar/HistogramPlot.h>

#include <QWidget>
#include <QApplication>

#include <algorithm>
#include <cmath>

using namespace gstar;

//---------------------------------------------------------------------------

HistogramPlot::HistogramPlot(QWidget* parent)
{

   m_widget = parent;
   m_widget->installEventFilter(this);
   _n_bins = 128;
   _bins.resize(_n_bins);
   for (int i = 0; i < _n_bins; i++)
   {
       _bins[i] = 0;
   }

   // A paint event can arrive before the first updatePoints(), and
   // drawHistogram()/ymap() read all of these.
   _minCoef = 0.0f;
   _maxCoef = 0.0f;
   _bin_width = 0.0f;
   _min_line = 0.0;
   _max_line = 0.0;
   _max_bin_val = 1;      // ymap() divides by this


   _default_pen = QPen(QColor(188, 187, 192, 200),1);
   _min_pen = QPen(QColor(255, 107, 102, 200));
   _max_pen = QPen(QColor(255, 107, 102, 200));


}

//---------------------------------------------------------------------------

HistogramPlot::~HistogramPlot()
{


}

//---------------------------------------------------------------------------

bool HistogramPlot::eventFilter(QObject* source, QEvent* event)
{

   if (source == m_widget)
   {
      if (!(m_widget->isEnabled()))
      {
         if ((event->type()) != QEvent::Paint)
         {
            return true;
         }
      }

      switch (event->type())
      {
         case QEvent::Paint:
         {
            QWidget* tmp = m_widget;
            m_widget = 0;
            QApplication::sendEvent(source, event);
            m_widget = tmp;
            drawHistogram();
            return true;
            break;
         }
         default:
            break;
      }
   }

   return false;

}

//---------------------------------------------------------------------------

void HistogramPlot::drawHistogram()
{

    QPainter painter(m_widget);
    painter.setPen( _default_pen);
    painter.setBrush(QBrush(QColor(188, 187, 192, 90)));
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    float y = 0.0;

    QPolygonF polylinePoints;

    qreal bins = (qreal)_n_bins;
    for (int i = 0; i< m_widget->width(); i++)
    {
        int bin = std::floor(( (qreal)(i) / (qreal)(m_widget->width()) ) * bins);
        polylinePoints.append(QPointF(i, ymap(0)));
        polylinePoints.append(QPointF(i, ymap(_bins[bin])));
    }

    painter.drawPolygon(polylinePoints, Qt::OddEvenFill);

    // draw min and max lines. With no range (a uniform map) the position below
    // would be 0.0/0.0, and converting that NaN to int is undefined; there is
    // no meaningful place to put the markers, so skip them.
    const qreal coef_range = (qreal)_maxCoef - (qreal)_minCoef;
    if (coef_range > 0.0)
    {
        polylinePoints.clear();
        painter.setPen(_min_pen);
        int pos = std::floor(((_min_line - _minCoef) / coef_range) * (qreal)(m_widget->width()));
        polylinePoints.insert(0, QPointF(pos, ymap(0)));
        polylinePoints.insert(1, QPointF(pos, ymap(_max_bin_val)));
        painter.drawPolygon(polylinePoints, Qt::OddEvenFill);

        polylinePoints.clear();
        painter.setPen(_max_pen);
        pos = std::floor(((_max_line - _minCoef) / coef_range) * (qreal)(m_widget->width()));
        polylinePoints.insert(0, QPointF(pos, ymap(0)));
        polylinePoints.insert(1, QPointF(pos, ymap(_max_bin_val)));
        painter.drawPolygon(polylinePoints, Qt::OddEvenFill);
    }

}

//---------------------------------------------------------------------------

qreal HistogramPlot::ymap(const qreal& value)
{

   // Leaving a small gap from the top
   qreal ymax = 0.0 + 0.03 * m_widget->height();
   qreal ymin = m_widget->height(); // the max-value of Y is at the bottom of
                                    // the widget, for display purposes that is
                                    //  the minimum Y.
   qreal newRange = ymin - ymax;
   qreal res = m_widget->height() - ( ( value  / _max_bin_val) * newRange );

   // If data point is too big, just make it point to the top.
   if (res < 0) res = ymax;

   return res;

}

//---------------------------------------------------------------------------

void HistogramPlot::updatePoints(const data_struct::ArrayTr<float>& pts)
{

    // Start from an empty histogram; these used to accumulate across datasets.
    std::fill(_bins.begin(), _bins.end(), 0);
    _max_bin_val = 1;

    if (pts.size() <= 0)
    {
        _minCoef = 0.0f;
        _maxCoef = 0.0f;
        _min_line = 0.0;
        _max_line = 0.0;
        _bin_width = 0.0f;
        m_widget->update();
        return;
    }

    _minCoef = pts.minCoeff();
    _maxCoef = pts.maxCoeff();
    // A NaN anywhere in the map propagates through minCoeff()/maxCoeff().
    if (false == std::isfinite(_minCoef) || false == std::isfinite(_maxCoef))
    {
        _minCoef = 0.0f;
        _maxCoef = 0.0f;
    }

    _min_line = _minCoef;
    _max_line = _maxCoef;

    // A uniform map - an element not present in the scan, or a failed fit -
    // has a zero range, which made the bin index below 0.0f/0.0f. (int)NaN is
    // undefined and yields INT_MIN on x86-64, so _bins[bin] wrote far outside
    // the vector. Everything lands in bin 0 instead.
    const float range = _maxCoef - _minCoef;
    const bool single_bin = (range <= 0.0f);
    _bin_width = single_bin ? 0.0f : (range / (float)(_n_bins - 1));

    for (int i = 0; i < pts.size(); i++)
    {
        const float val = pts(i);
        if (false == std::isfinite(val))
        {
            continue;
        }
        int bin = 0;
        if (false == single_bin)
        {
            bin = (int)std::floor((val - _minCoef) / _bin_width);
            // Rounding at the ends can still step outside the range.
            bin = std::max(0, std::min(bin, _n_bins - 1));
        }
        _bins[bin] = _bins[bin] + 1;
    }

    for (int i = 0; i < _n_bins; i++)
    {
        _max_bin_val  = std::max(_bins[i], _max_bin_val);
    }

    m_widget->update();
   
}

//---------------------------------------------------------------------------

void HistogramPlot::set_min_max_lines(qreal minCoef, qreal maxCoef)
{

    _min_line = minCoef;
    _max_line = maxCoef;
    drawHistogram();

}

//---------------------------------------------------------------------------