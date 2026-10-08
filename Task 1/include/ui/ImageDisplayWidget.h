#pragma once

#include <QImage>
#include <QWidget>

// ----------------------------------------------------------------------------
// ImageDisplayWidget (ui)
// Shows the current result image, scaled to fit while keeping the aspect
// ratio. The controller's resultReady(QImage) signal is connected directly to
// the setImage() slot - no extra wiring needed.
// ----------------------------------------------------------------------------
class ImageDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageDisplayWidget(QWidget* parent = nullptr);

    QImage image() const;

public slots:
    void setImage(const QImage& image);
    void clearImage();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_image;
};
