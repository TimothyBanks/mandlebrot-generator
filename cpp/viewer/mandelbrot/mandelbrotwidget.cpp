#include <QPainter>
#include <QKeyEvent>

#include <math.h>

#include "mandelbrotwidget.h"

constexpr auto s_default_center_x = double{-0.637011};
constexpr auto s_default_center_y = double{-0.0395159};
constexpr auto s_default_scale = double{0.00403897};
constexpr auto s_zoom_in_factor = double{0.8};
constexpr auto s_zoom_out_factor = double{1 / s_zoom_in_factor};
constexpr auto s_scroll_step = int32_t{20};

MandelbrotWidget::MandelbrotWidget(QWidget *parent)
: QWidget(parent),
  m_center_x{s_default_center_x},
  m_center_y{s_default_center_y},
  m_pixmap_scale{s_default_scale},
  m_current_scale{s_default_scale}
{
    connect(&m_thread, SIGNAL(renderedImage(QImage,double)), this, SLOT(updatePixmap(QImage,double)));

    setWindowTitle(tr("Mandelbrot"));
#ifndef QT_NO_CURSOR
    setCursor(Qt::CrossCursor);
#endif
    resize(1463, 1315);
}

void MandelbrotWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (m_pixmap.isNull())
    {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("Rendering initial image, please wait..."));
        return;
    }

    if (m_current_scale == m_pixmap_scale)
    {
        painter.drawPixmap(m_pixmap_offset, m_pixmap);
    }
    else
    {
        auto scale_factor = m_pixmap_scale / m_current_scale;
        auto new_width = int(m_pixmap.width() * scale_factor);
        auto new_height = int(m_pixmap.height() * scale_factor);
        auto new_x = m_pixmap_offset.x() + (m_pixmap.width() - new_width) / 2;
        auto new_y = m_pixmap_offset.y() + (m_pixmap.height() - new_height) / 2;

        painter.save();
        painter.translate(new_x, new_y);
        painter.scale(scale_factor, scale_factor);
        auto exposed = painter.matrix().inverted().mapRect(rect()).adjusted(-1, -1, 1, 1);
        painter.drawPixmap(exposed, m_pixmap, exposed);
        painter.restore();
    }

    QString text = tr("Use mouse wheel or the '+' and '-' keys to zoom. "
                      "Press and hold left mouse button to scroll.");
    auto metrics = painter.fontMetrics();
    auto textWidth = metrics.horizontalAdvance(text);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 127));
    painter.drawRect((width() - textWidth) / 2 - 5, 0, textWidth + 10, metrics.lineSpacing() + 5);
    painter.setPen(Qt::white);
    painter.drawText((width() - textWidth) / 2, metrics.leading() + metrics.ascent(), text);
}

void MandelbrotWidget::resizeEvent(QResizeEvent * /* event */)
{
    m_thread.render(m_center_x, m_center_y, m_current_scale, size());
}

void MandelbrotWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoom(s_zoom_in_factor);
        break;
    case Qt::Key_Minus:
        zoom(s_zoom_out_factor);
        break;
    case Qt::Key_Left:
        scroll(-s_scroll_step, 0);
        break;
    case Qt::Key_Right:
        scroll(+s_scroll_step, 0);
        break;
    case Qt::Key_Down:
        scroll(0, -s_scroll_step);
        break;
    case Qt::Key_Up:
        scroll(0, +s_scroll_step);
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

#if QT_CONFIG(wheelevent)
void MandelbrotWidget::wheelEvent(QWheelEvent *event)
{    
    auto num_degrees = event->delta() / 8;
    auto num_steps = num_degrees / 15.0f;
    zoom(pow(s_zoom_in_factor, num_steps));
}
#endif

void MandelbrotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_last_drag_pos = event->pos();
    }
}

void MandelbrotWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        m_pixmap_offset += event->pos() - m_last_drag_pos;
        m_last_drag_pos = event->pos();
        update();
    }
}

void MandelbrotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pixmap_offset += event->pos() - m_last_drag_pos;
        m_last_drag_pos = QPoint();

        auto delta_x = (width() - m_pixmap.width()) / 2 - m_pixmap_offset.x();
        auto delta_y = (height() - m_pixmap.height()) / 2 - m_pixmap_offset.y();
        scroll(delta_x, delta_y);
    }
}

void MandelbrotWidget::updatePixmap(const QImage &image, double scaleFactor)
{
    if (!m_last_drag_pos.isNull())
    {
        return;
    }

    m_pixmap = QPixmap::fromImage(image);
    m_pixmap_offset = QPoint();
    m_last_drag_pos = QPoint();
    m_pixmap_scale = scaleFactor;
    update();
}

void MandelbrotWidget::zoom(double zoomFactor)
{
    m_current_scale *= zoomFactor;
    update();
    m_thread.render(m_center_x, m_center_y, m_current_scale, size());
}

void MandelbrotWidget::scroll(int deltaX, int deltaY)
{
    m_center_x += deltaX * m_current_scale;
    m_center_y += deltaY * m_current_scale;
    update();
    m_thread.render(m_center_x, m_center_y, m_current_scale, size());
}
