#include "ImageUtils.h"
#include <algorithm>

namespace ImageUtils {

GrayImage qImageToGrayMatrix(const QImage& qimg) {
    int w = qimg.width();
    int h = qimg.height();
    GrayImage gray(h, std::vector<double>(w, 0.0));
    
    QImage rgbImg = qimg.convertToFormat(QImage::Format_RGB888);
    for (int y = 0; y < h; ++y) {
        const uchar* scanLine = rgbImg.scanLine(y);
        for (int x = 0; x < w; ++x) {
            double r = scanLine[x * 3 + 0];
            double g = scanLine[x * 3 + 1];
            double b = scanLine[x * 3 + 2];
            gray[y][x] = 0.299 * r + 0.587 * g + 0.114 * b;
        }
    }
    return gray;
}

QImage grayMatrixToQImage(const GrayImage& mat) {
    if (mat.empty() || mat[0].empty()) return QImage();
    int h = static_cast<int>(mat.size());
    int w = static_cast<int>(mat[0].size());
    
    QImage result(w, h, QImage::Format_Grayscale8);
    for (int y = 0; y < h; ++y) {
        uchar* line = result.scanLine(y);
        for (int x = 0; x < w; ++x) {
            line[x] = static_cast<uchar>(std::clamp(mat[y][x], 0.0, 255.0));
        }
    }
    return result;
}

}