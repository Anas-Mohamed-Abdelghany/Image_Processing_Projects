#pragma once

#include <QImage>
#include <QString>
#include <QWidget>

// ----------------------------------------------------------------------------
// ImageDisplayWidget (ui)
// Shows the current result image, scaled to fit while keeping the aspect
// ratio. The controller's resultReady(QImage) signal is connected directly to
// the setImage() slot - no extra wiring needed.
//
// A title and an empty-state hint can be passed to the constructor so the
// same widget can be reused for the three side by side previews
// (original / box 2 / box 3).
//
// Layout inside the box:
//
//   +---------------------------+
//   | Title                     |  title strip (22 px)
//   |                           |
//   |        the image          |  image area, aspect preserved
//   |                           |
//   +---------------------------+
//   |        1920 x 1080 px     |  size strip (20 px)
//   +---------------------------+
//
// The size strip only appears once an image has been set.
// ----------------------------------------------------------------------------
class ImageDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageDisplayWidget(QWidget* parent = nullptr);
    explicit ImageDisplayWidget(const QString& title, const QString& placeholder,
                                QWidget* parent = nullptr);

    QImage image() const;
    QString title() const;
    void setTitle(const QString& title);

public slots:
    void setImage(const QImage& image);
    void clearImage();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupSizing();

    QImage m_image;
    QString m_title;
    QString m_placeholder;
    QString m_sizeText;   // "1920 x 1080 px", empty while no image is set
};
