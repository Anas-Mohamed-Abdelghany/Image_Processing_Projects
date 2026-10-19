#include "ui/ImageDisplayWidget.h"

#include <QDialog>
#include <QFont>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>

// Small box so all three previews fit side by side. The size is capped, not
// fixed, so the previews still shrink on a smaller window instead of forcing
// it wider.
//
// Note: the app runs under Windows DPI scaling (150% on this machine), so
// these are LOGICAL pixels - a 260x185 box paints as 390x278 physical pixels.
// Keep them small or the right-hand panel is pushed off a 1366x768 screen.
static const int kMinBoxWidth = 140;
static const int kMinBoxHeight = 100;
static const int kMaxBoxWidth = 320;
static const int kMaxBoxHeight = 240;

ImageDisplayWidget::ImageDisplayWidget(QWidget* parent)
    : QWidget(parent)
{
    setupSizing();
}

ImageDisplayWidget::ImageDisplayWidget(const QString& title, const QString& placeholder,
                                       QWidget* parent)
    : QWidget(parent)
    , m_title(title)
    , m_placeholder(placeholder)
{
    setupSizing();
}

void ImageDisplayWidget::setupSizing()
{
    // NOTE ON DPI: these are LOGICAL pixels. Qt reports the desktop as 150%
    // here, so a 260x185 box paints as 390x278 physical pixels. Three of them
    // plus the control panel have to fit a 1280x720 screen (853x480 logical),
    // which is why the caps are deliberately small.
    setMinimumSize(kMinBoxWidth, kMinBoxHeight);
    setMaximumSize(kMaxBoxWidth, kMaxBoxHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(true);
}

QImage ImageDisplayWidget::image() const
{
    return m_image;
}

QString ImageDisplayWidget::title() const
{
    return m_title;
}

void ImageDisplayWidget::setTitle(const QString& title)
{
    m_title = title;
    update();
}

void ImageDisplayWidget::setImage(const QImage& image)
{
    m_image = image;
    m_sizeText = image.isNull()
        ? QString()
        : tr("%1 x %2 px").arg(image.width()).arg(image.height());
    update();
}

void ImageDisplayWidget::clearImage()
{
    m_image = QImage();
    m_sizeText.clear();
    update();
}

void ImageDisplayWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (m_image.isNull()) {
        return;
    }

    auto* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Draw the pixmap 1:1 in PHYSICAL pixels (the app runs under Windows DPI
    // scaling), so every pixel of the image is shown without resampling.
    const qreal ratio = dialog->devicePixelRatioF();
    QPixmap pixmap = QPixmap::fromImage(m_image);
    pixmap.setDevicePixelRatio(ratio);

    auto* label = new QLabel(dialog);
    label->setPixmap(pixmap);

    auto* scroll = new QScrollArea(dialog);
    scroll->setWidget(label);
    scroll->setAlignment(Qt::AlignCenter);

    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    dialog->setWindowTitle(tr("%1 - %2 x %3 px (100%)")
                               .arg(m_title.simplified())
                               .arg(m_image.width())
                               .arg(m_image.height()));
    dialog->resize(qMin(static_cast<int>(m_image.width() / ratio) + 24, 800),
                   qMin(static_cast<int>(m_image.height() / ratio) + 24, 520));
    dialog->show();
}

void ImageDisplayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), QColor(24, 24, 24));

    // Strips: title on top, image size on the bottom, image in between.
    const int titleHeight = m_title.isEmpty() ? 0 : 22;
    const int sizeHeight = m_sizeText.isEmpty() ? 0 : 18;
    const QRect imageArea = rect().adjusted(0, titleHeight, 0, -sizeHeight);

    if (!m_title.isEmpty()) {
        painter.setPen(QColor(235, 235, 235));
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF() * 0.95);
        painter.setFont(font);
        painter.drawText(rect().adjusted(8, 0, -8, -(height() - titleHeight)),
                         Qt::AlignLeft | Qt::AlignVCenter, m_title);
    }

    if (!m_sizeText.isEmpty()) {
        // Drawn below the image: "1920 x 1080 px".
        const QRect sizeArea(0, height() - sizeHeight, width(), sizeHeight - 1);
        painter.fillRect(sizeArea, QColor(36, 36, 36));
        painter.setPen(QColor(170, 170, 170));
        QFont font = painter.font();
        font.setBold(false);
        font.setPointSizeF(font.pointSizeF() * 0.9);
        painter.setFont(font);
        painter.drawText(sizeArea, Qt::AlignCenter, m_sizeText);
    }

    if (m_image.isNull()) {
        painter.setPen(QColor(160, 160, 160));
        QFont font = painter.font();
        font.setBold(false);
        painter.setFont(font);
        painter.drawText(imageArea, Qt::AlignCenter,
                         m_placeholder.isEmpty() ? tr("Load an image to begin") : m_placeholder);
        return;
    }

    // Scale down (or up) to fit the widget while keeping the aspect ratio.
    QSize target = m_image.size();
    target.scale(imageArea.size() - QSize(16, 16), Qt::KeepAspectRatio);

    const QPoint topLeft(imageArea.x() + (imageArea.width() - target.width()) / 2,
                         imageArea.y() + (imageArea.height() - target.height()) / 2);

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(QRect(topLeft, target), m_image);
}
