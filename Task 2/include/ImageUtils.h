#pragma once
#include "Types.h"
#include <QImage>

namespace ImageUtils {
    GrayImage qImageToGrayMatrix(const QImage& qimg);
    QImage grayMatrixToQImage(const GrayImage& mat);
}