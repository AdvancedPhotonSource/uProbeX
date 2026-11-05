/*-----------------------------------------------------------------------------
 * Copyright (c) 2013, UChicago Argonne, LLC
 * See LICENSE file.
 *---------------------------------------------------------------------------*/

#include "gstar/Annotation/RoiMaskGraphicsItem.h"

#include "gstar/AnnotationProperty.h"
#include "gstar/ImageViewScene.h"
#include "core/str_defines.h"
#include "preferences/Preferences.h"
#include <math.h>
#include <QLineF>

using namespace gstar;

//-----------------------------------------------------------------------------

#ifdef _BUILD_WITH_OPENCV
RoiMaskGraphicsItem::RoiMaskGraphicsItem(const cv::Mat& mat, int idx, QColor col, AbstractGraphicsItem* parent)
    : AbstractGraphicsItem(parent)
{
    _mask = std::make_unique<QImage>(mat.cols, mat.rows, QImage::Format_ARGB32);

    for (int w = 0; w < mat.cols; w++)
    {
        for (int h = 0; h < mat.rows; h++)
        {        
            int color_idx = mat.at<int>(h, w);
            if (color_idx == idx)
            {
                col.setAlpha(178);
                _mask->setPixelColor(w, h, col);
            }
            else
            {
                _mask->setPixelColor(w, h, QColor(0, 0, 0, 0));
            }
        }
    }

    _init(col, 70);

}
#endif

//-----------------------------------------------------------------------------

RoiMaskGraphicsItem::RoiMaskGraphicsItem(const Eigen::Array<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& mask, int idx, QColor col, AbstractGraphicsItem* parent)
 : AbstractGraphicsItem(parent)
{
    _mask = std::make_unique<QImage>(mask.cols(), mask.rows(), QImage::Format_ARGB32);

    for (int w = 0; w < mask.cols(); w++)
    {
        for (int h = 0; h < mask.rows(); h++)
        {        
            int color_idx = mask(h, w);
            if (color_idx == idx)
            {
                col.setAlpha(178);
                _mask->setPixelColor(w, h, col);
            }
            else
            {
                _mask->setPixelColor(w, h, QColor(0, 0, 0, 0));
            }
        }
    }

    _init(col, 70);

}

//-----------------------------------------------------------------------------

RoiMaskGraphicsItem::RoiMaskGraphicsItem(int rows, int cols, QColor col, AbstractGraphicsItem* parent)
    : AbstractGraphicsItem(parent)
{
    _mask = std::make_unique<QImage>(cols, rows, QImage::Format_ARGB32);

    for (int w = 0; w < cols; w++)
    {
        for (int h = 0; h < rows; h++)
        {
            _mask->setPixelColor(w, h, QColor(0, 0, 0, 0));
        }
    }

    _init(col, 70);
}

//-----------------------------------------------------------------------------

RoiMaskGraphicsItem::RoiMaskGraphicsItem(QImage mask, QColor color, int alpha, AbstractGraphicsItem* parent) : AbstractGraphicsItem(parent)
{
    _mask = std::make_unique<QImage>(mask.width(), mask.height(), QImage::Format_ARGB32);
    QColor black_col = QColor(0, 0, 0, 0);
    for (int i = 0; i < _mask->width(); i++)
    {
        for (int j = 0; j < _mask->height(); j++)
        {
            if (mask.pixelColor(i, j) == black_col)
            {
                _mask->setPixelColor(i, j, black_col);
            }
            else
            {
                _mask->setPixelColor(i, j, color);
            }
        }
    }
    _init(color, alpha);
}

//-----------------------------------------------------------------------------

RoiMaskGraphicsItem::RoiMaskGraphicsItem(QString name, QColor color, int alpha, int width, int height, std::vector<std::pair<int, int>> pixel_list, AbstractGraphicsItem* parent) : AbstractGraphicsItem(parent)
{
    _mask = std::make_unique<QImage>(width, height, QImage::Format_ARGB32);
    // clear the image
    for (int w = 0; w < width; w++)
    {
        for (int h = 0; h < height; h++)
        {
            _mask->setPixelColor(w, h, QColor(0, 0, 0, 0));
        }
    }
    color.setAlpha(alpha);
    // set roi pixels
    for (auto& itr : pixel_list)
    {
        _mask->setPixelColor(itr.first, itr.second, color);
    }
    bool found_name = false;
    for (const auto& itr : m_data)
    {
        if (itr->getName() == DEF_STR_DISPLAY_NAME)
        {
            itr->setValue(name);
            found_name = true;
        }
    }
    if(false == found_name)
    {
        appendProperty(new AnnotationProperty(DEF_STR_DISPLAY_NAME, name));
    }
    _init(color, alpha);
}

//-----------------------------------------------------------------------------

RoiMaskGraphicsItem::~RoiMaskGraphicsItem()
{
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::_init(QColor color, int alpha)
{
    _cursor.setWidth(10.);
    _cursor.setHeight(10.);
    _cursor.setColor(Qt::gray);

    setFlags(ItemIsSelectable);
    setAcceptHoverEvents(true);

    _polygon.clear();
    _polygon.push_back(QPoint(0, 0));
    _polygon.push_back(QPoint(0, _mask->height()));
    _polygon.push_back(QPoint(_mask->width(), _mask->height()));
    _polygon.push_back(QPoint(_mask->width(), 0));

    _mouse_down = false;
    _draw_action = DRAW_ACTION_MODES::OFF;
    _brush_size = QSize(10, 10);
    _render_scale = 1;

    _color_ano = new AnnotationProperty();
    _color_ano->setName("Color");
    _color_ano->setValue(color);

    _alpha_value = new AnnotationProperty();
    _alpha_value->setName("Alpha %");
    _alpha_value->setValue(alpha);

    m_data.push_back(_color_ano);
    m_data.push_back(_alpha_value);
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::setMaskSize(QRectF size)
{
    _polygon.clear();
    _polygon.push_back(QPoint(0, 0));
    _polygon.push_back(QPoint(0, size.height()));
    _polygon.push_back(QPoint(size.width(), size.height()));
    _polygon.push_back(QPoint(size.width(), 0));

    _mask = std::make_unique<QImage>(size.width(), size.height(), QImage::Format_ARGB32);
    for (int w = 0; w < size.width(); w++)
    {
        for (int h = 0; h < size.height(); h++)
        {
            _mask->setPixelColor(w, h, QColor(0, 0, 0, 0));
        }
    }
    update();
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::add_to_roi(QPointF pos)
{
    // Scene position in image pixels -> the measurement under it.
    int x = (int)pos.x() / _render_scale;
    int y = (int)pos.y() / _render_scale;

    if (x > -1 && x < _mask->width())
    {
        if (y > -1 && y < _mask->height())
        {
            int xsize = x + _brush_size.width();
            if (xsize > _mask->width())
                xsize = _mask->width();
            int ysize = y + _brush_size.height();
            if (ysize > _mask->height())
                ysize = _mask->height();
            QVariant variant = _color_ano->getValue();
            QColor col = variant.value<QColor>();
            col.setAlpha(_alpha_value->getValue().toInt());
            for (int i = x; i < xsize; i++)
            {
                for (int j = y; j < ysize; j++)
                {
                    _mask->setPixelColor(i, j, col);
                }
            }
            // update() takes item coordinates, which are image pixels.
            QRect rect(x * _render_scale, y * _render_scale, _brush_size.width() * _render_scale, _brush_size.height() * _render_scale);
            update(rect);
        }
    }
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::add_to_roi(QPolygon polygon)
{
    QVariant variant = _color_ano->getValue();
    QColor col = variant.value<QColor>();
    col.setAlpha(_alpha_value->getValue().toInt());
    for (int i = 0; i < _mask->width(); i++)
    {
        for (int j = 0; j < _mask->height(); j++)
        {
            // The polygon was drawn in scene coordinates, so test the center of the block of
            // pixels this measurement occupies rather than the measurement index itself.
            QPoint point((i * _render_scale) + (_render_scale / 2), (j * _render_scale) + (_render_scale / 2));
            if( polygon.containsPoint(point, Qt::OddEvenFill) )
            {
                _mask->setPixelColor(i, j, col);
            }
        }
    }
    QRect rect(0, 0, _mask->width() * _render_scale, _mask->height() * _render_scale);
    update(rect);
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::invertMask()
{
    QVariant variant = _color_ano->getValue();
    QColor col = variant.value<QColor>();
    QColor black_col = QColor(0, 0, 0, 0);
    col.setAlpha(_alpha_value->getValue().toInt());
    for (int i = 0; i < _mask->width(); i++)
    {
        for (int j = 0; j < _mask->height(); j++)
        {
            if (_mask->pixelColor(i, j) == black_col)
            {
                _mask->setPixelColor(i, j, col);
            }
            else
            {
                _mask->setPixelColor(i, j, black_col);
            }
        }
    }
    QRect rect(0, 0, _mask->width(), _mask->height());
    update(rect);
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::erase_from_roi(QPointF pos)
{

    // Scene position in image pixels -> the measurement under it.
    int x = (int)pos.x() / _render_scale;
    int y = (int)pos.y() / _render_scale;
    if (x > -1 && x < _mask->width())
    {
        if (y > -1 && y < _mask->height())
        {
            int xsize = x + _brush_size.width();
            if (xsize > _mask->width())
                xsize = _mask->width();
            int ysize = y + _brush_size.height();
            if (ysize > _mask->height())
                ysize = _mask->height();
            for (int i = x; i < xsize; i++)
            {
                for (int j = y; j < ysize; j++)
                {
                    _mask->setPixelColor(i, j, QColor(0, 0, 0, 0));
                }
            }
            // update() takes item coordinates, which are image pixels.
            QRect rect(x * _render_scale, y * _render_scale, _brush_size.width() * _render_scale, _brush_size.height() * _render_scale);
            update(rect);
        }
    }
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::erase_from_roi(QPolygon polygon)
{
    for (int i = 0; i < _mask->width(); i++)
    {
        for (int j = 0; j < _mask->height(); j++)
        {
            // Scene coordinate polygon, so test the center of this measurement's pixel block.
            QPoint point((i * _render_scale) + (_render_scale / 2), (j * _render_scale) + (_render_scale / 2));
            if (polygon.containsPoint(point, Qt::OddEvenFill))
            {
                _mask->setPixelColor(i, j, QColor(0, 0, 0, 0));
            }
        }
    }
    QRect rect(0, 0, _mask->width() * _render_scale, _mask->height() * _render_scale);
    update(rect);
}

//-----------------------------------------------------------------------------

QString RoiMaskGraphicsItem::getName()
{

    for (const auto& itr : m_data)
    {
        if (itr->getName() == DEF_STR_DISPLAY_NAME)
            return itr->getValue().toString();
    }
    return "";

}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::calculate()
{
    QVariant variant = _color_ano->getValue();
    QColor color = variant.value<QColor>();
    int alpha = _alpha_value->getValue().toInt();
    alpha = (int)(((float)alpha / 100.0) * 255.0);
    color.setAlpha(alpha);
    

    for (int w = 0; w < _mask->width(); w++)
    {
        for (int h = 0; h < _mask->height(); h++)
        {
            if(_mask->pixel(w,h) != 0)
            {
                _mask->setPixelColor(w, h, color);
            }
        }
    }
}

//-----------------------------------------------------------------------------

QPainterPath RoiMaskGraphicsItem::shape() const
{

    QPainterPath path;
    path.addPolygon(_polygon);
    return path;

}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::updateModel()
{
    int val = _alpha_value->getValue().toInt();

    val = std::min(val, 100);
    val = std::max(val, 0);

    _alpha_value->setValue(val);
    calculate();
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::updateView()
{
    calculate();
    update();
}

//-----------------------------------------------------------------------------

QColor RoiMaskGraphicsItem::getColor() const
{
    QVariant variant = _color_ano->getValue();
    QColor color = variant.value<QColor>();
    return color;
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::to_roi_vec(std::vector<std::pair<int, int>>& rois)
{
    for (int w = 0; w < _mask->width(); w++)
    {
        for (int h = 0; h < _mask->height(); h++)
        {
            if( _mask->pixel(w, h) != 0)
            {
                rois.push_back({ w, h });
            }
        }
    }
}

//-----------------------------------------------------------------------------

QRectF RoiMaskGraphicsItem::boundingRect() const
{

   return QRectF(0, 0, _mask->width() * _render_scale, _mask->height() * _render_scale);

}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::setRenderScale(int scale)
{

   scale = std::max(1, scale);
   if(scale == _render_scale)
   {
      return;
   }
   prepareGeometryChange();
   _render_scale = scale;
   update();

}

//-----------------------------------------------------------------------------

QRectF RoiMaskGraphicsItem::boundingRectMarker() const
{

   return QRectF(0, 0, _mask->width() * _render_scale, _mask->height() * _render_scale);

}

//-----------------------------------------------------------------------------

const QString RoiMaskGraphicsItem::displayName() const
{
   return CONST_STATIC_ROI_NAME;
}

//-----------------------------------------------------------------------------

AbstractGraphicsItem* RoiMaskGraphicsItem::duplicate()
{
    QColor col = QColor(_color_ano->getValue().toString());
    RoiMaskGraphicsItem* item = new RoiMaskGraphicsItem(*_mask, col, _alpha_value->getValue().toInt());
    item->prependProperty(new AnnotationProperty(DEF_STR_DISPLAY_NAME, getName()));
    item->setRenderScale(_render_scale);
    return item;

}
//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::paint(QPainter* painter,
                                 const QStyleOptionGraphicsItem* option,
                                 QWidget* widget )
{

    Q_UNUSED(option);
    Q_UNUSED(widget);
    // hack to fix roi display bug
    bool compact_view = Preferences::inst()->getValue(STR_PRF_COMPACT_COUNTS_VIEW).toBool();
    int rows = Preferences::inst()->getValue(STR_GRID_ROWS).toInt();
    int cols = Preferences::inst()->getValue(STR_GRID_COLS).toInt();
    if(false == compact_view && rows == 1 && cols == 1)
    {
        setPos(0, 0); //uncomment for fix single window offset bug
    }
    // The mask is one pixel per measurement; stretch it over the map's render scale. Smoothing
    // off so a roi pixel lines up exactly with the block of pixels its measurement occupies.
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter->drawPixmap(QRect(0, 0, _mask->width() * _render_scale, _mask->height() * _render_scale), QPixmap::fromImage(*_mask));
    // _roi_polygon is collected in scene coordinates, so it is already in this item's units.
    if (_roi_polygon.size() > 0)
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addPolygon(_roi_polygon);
        QPen pen;
        QVariant variant = _color_ano->getValue();
        pen.setColor(variant.value<QColor>());
        painter->setPen(pen);
        painter->drawPath(path);
        //painter->drawConvexPolygon(_roi_polygon);
        //painter->drawPolygon(_roi_polygon);
    }
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::setDrawAction(DRAW_ACTION_MODES action_mode)
{
    _draw_action = action_mode; 
    if (_draw_action == DRAW_ACTION_MODES::ADD_POLY || _draw_action == DRAW_ACTION_MODES::ERASE_POLY)
    {
        _roi_polygon.clear();
        update();
    }
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::onMousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _mouse_down = true;
        if (_draw_action == DRAW_ACTION_MODES::ADD_DRAW)
        {
            add_to_roi(event->scenePos());
        }
        else if (_draw_action == DRAW_ACTION_MODES::ERASE_DRAW)
        {
            erase_from_roi(event->scenePos());
        }
        else if (_draw_action == DRAW_ACTION_MODES::ADD_POLY)
        {
            _roi_polygon.push_back(event->scenePos().toPoint());
            update();
        }
        else if (_draw_action == DRAW_ACTION_MODES::ERASE_POLY)
        {
            _roi_polygon.push_back(event->scenePos().toPoint());
            update();
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (_roi_polygon.size() > 2)
        {
            _roi_polygon.push_back(_roi_polygon[0]);
            if (_draw_action == DRAW_ACTION_MODES::ADD_POLY)
            {
                add_to_roi(_roi_polygon);
            }
            else if (_draw_action == DRAW_ACTION_MODES::ERASE_POLY)
            {
                erase_from_roi(_roi_polygon);
            }
            _roi_polygon.clear();
        }
    }

    QGraphicsItem::mousePressEvent(event);
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::onMouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QPointF spos = event->scenePos();
    spos.setX( spos.x() - .5) ;
    spos.setY( spos.y() - .5) ;
    if(_mouse_down == true)
    {
        if (_draw_action == DRAW_ACTION_MODES::ADD_DRAW)
        {
            add_to_roi(event->scenePos());
        }
        else if (_draw_action == DRAW_ACTION_MODES::ERASE_DRAW)
        {
            erase_from_roi(event->scenePos());
        }
        /*
        else if (_draw_action == DRAW_ACTION_MODES::ADD_POLY)
        {

        }
        else if (_draw_action == DRAW_ACTION_MODES::ERASE_POLY)
        {

        }
        */
    }
   _cursor.setPos(spos);
   //_cursor.setPos(event->scenePos());
   QGraphicsItem::mouseMoveEvent(event);
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::onMouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _mouse_down = false;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    //_cursor.setPos(event->scenePos());
    this->scene()->addItem(&_cursor);
    update(boundingRect());
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    this->scene()->removeItem(&_cursor);
    update(boundingRect());
}

//-----------------------------------------------------------------------------

void RoiMaskGraphicsItem::setBrushSize(QSize brushSize)
{
    if(brushSize.width() < 1)
    {
        brushSize.setWidth(1);
    }
    if(brushSize.height() < 1)
    {
        brushSize.setHeight(1);
    }
    _brush_size = brushSize;
    double w = (double)(brushSize.width());
    double h = (double)(brushSize.height());
    _cursor.setWidth(w); 
    _cursor.setHeight(h);
}

//-----------------------------------------------------------------------------