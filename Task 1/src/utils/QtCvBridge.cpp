#include "utils/QtCvBridge.h"

#include <opencv2/imgproc.hpp>

namespace QtCvBridge {

QImage toQImage(const cv::Mat& image)
{
    if (image.empty()) {
        return QImage();
    }

    // ---- 1) Normalise the bit depth to 8 bit -------------------------------
    cv::Mat image8;
    switch (image.depth()) {
    case CV_8U:
        image8 = image;
        break;
    case CV_16U:
        // 16 bit images (e.g. microscopy / HDR) are scaled down to 8 bit.
        image.convertTo(image8, CV_8U, 1.0 / 256.0);
        break;
    default: {
        // Float / double results (histograms, frequency filters, ...):
        // linearly stretch [min, max] to [0, 255].
        const cv::Mat flat = image.isContinuous() ? image : image.clone();
        double minValue = 0.0;
        double maxValue = 0.0;
        cv::minMaxLoc(flat.reshape(1), &minValue, &maxValue);
        const double scale = (maxValue > minValue) ? 255.0 / (maxValue - minValue) : 1.0;
        image.convertTo(image8, CV_8U, scale, -minValue * scale);
        break;
    }
    }

    // ---- 2) Map the channels to a QImage format ----------------------------
    switch (image8.channels()) {
    case 1: {
        QImage view(image8.data, image8.cols, image8.rows,
                    static_cast<int>(image8.step), QImage::Format_Grayscale8);
        return view.copy();
    }
    case 3: {
        // OpenCV stores colour images in BGR order, QImage expects RGB.
        cv::Mat rgb;
        cv::cvtColor(image8, rgb, cv::COLOR_BGR2RGB);
        QImage view(rgb.data, rgb.cols, rgb.rows,
                    static_cast<int>(rgb.step), QImage::Format_RGB888);
        return view.copy();
    }
    case 4: {
        cv::Mat rgba;
        cv::cvtColor(image8, rgba, cv::COLOR_BGRA2RGBA);
        QImage view(rgba.data, rgba.cols, rgba.rows,
                    static_cast<int>(rgba.step), QImage::Format_RGBA8888);
        return view.copy();
    }
    default:
        return QImage();
    }
}

QPixmap toPixmap(const cv::Mat& image)
{
    return QPixmap::fromImage(toQImage(image));
}

cv::Mat toCvMat(const QImage& image)
{
    if (image.isNull()) {
        return cv::Mat();
    }

    if (image.hasAlphaChannel()) {
        QImage converted = image.convertToFormat(QImage::Format_RGBA8888);
        cv::Mat rgba(converted.height(), converted.width(), CV_8UC4,
                     converted.bits(), static_cast<size_t>(converted.bytesPerLine()));
        cv::Mat bgra;
        cv::cvtColor(rgba, bgra, cv::COLOR_RGBA2BGRA);
        return bgra.clone();
    }

    if (image.isGrayscale()) {
        QImage converted = image.convertToFormat(QImage::Format_Grayscale8);
        cv::Mat gray(converted.height(), converted.width(), CV_8UC1,
                     converted.bits(), static_cast<size_t>(converted.bytesPerLine()));
        return gray.clone();
    }

    QImage converted = image.convertToFormat(QImage::Format_RGB888);
    cv::Mat rgb(converted.height(), converted.width(), CV_8UC3,
                converted.bits(), static_cast<size_t>(converted.bytesPerLine()));
    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
    return bgr.clone();
}

} // namespace QtCvBridge
