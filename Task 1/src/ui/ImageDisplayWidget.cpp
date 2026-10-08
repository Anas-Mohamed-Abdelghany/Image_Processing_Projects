#include "ui/ImageDisplayWidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>

ImageDisplayWidget::ImageDisplayWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(480, 360);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(true);
}

QImage ImageDisplayWidget::image() const
{
    return m_image;
}

void ImageDisplayWidget::setImage(const QImage& image)
{
    m_image = image;
    update();
}

void ImageDisplayWidget::clearImage()
{
    m_image = QImage();
    update();
}

void ImageDisplayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), QColor(24, 24, 24));

    if (m_image.isNull()) {
        painter.setPen(QColor(160, 160, 160));
        painter.drawText(rect(), Qt::AlignCenter, tr("Load an image to begin"));
        return;
    }

    // Scale down (or up) to fit the widget while keeping the aspect ratio.
    QSize target = m_image.size();
    target.scale(size() - QSize(16, 16), Qt::KeepAspectRatio);

    const QPoint topLeft((width() - target.width()) / 2,
                         (height() - target.height()) / 2);

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(QRect(topLeft, target), m_image);
}
