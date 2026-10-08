#pragma once

#include <QImage>
#include <QPixmap>

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Conversions between OpenCV and Qt image types.
// The UI only ever sees QImage/QPixmap; the services only ever see cv::Mat.
// This is shared infrastructure - please do not change the signatures.
// ----------------------------------------------------------------------------
namespace QtCvBridge {

// Converts any cv::Mat (gray / BGR / BGRA, 8U / 16U / float / double) to a QImage.
// Returns a null QImage when the input is empty or has an unsupported layout.
QImage toQImage(const cv::Mat& image);

// Convenience wrapper around toQImage().
QPixmap toPixmap(const cv::Mat& image);

// Converts a QImage (gray / RGB / RGBA) to a BGR or grayscale cv::Mat.
// Returns an empty cv::Mat when the input is null.
cv::Mat toCvMat(const QImage& image);

} // namespace QtCvBridge
