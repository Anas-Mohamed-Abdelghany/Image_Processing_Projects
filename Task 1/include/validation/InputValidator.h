#pragma once

#include <QString>

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Central input validation for every task.
// ImageProcessorController calls these BEFORE invoking a service, so invalid
// parameters (even kernel sizes, out-of-range probabilities, ...) never reach
// the algorithms. When a check fails a human readable message is written into
// *errorOut and shown in the main window status bar.
// ----------------------------------------------------------------------------
namespace InputValidator {

// True when the matrix contains a usable image (not empty).
bool isValidImage(const cv::Mat& image, const QString& name, QString* errorOut);

// Filter kernels must be odd and within [3, 21].
bool isValidKernelSize(int ksize, QString* errorOut);

// Generic inclusive range check: minValue <= value <= maxValue.
bool isInRange(double value, double minValue, double maxValue,
               const QString& name, QString* errorOut);

// Ensures first <= second (e.g. Canny low threshold <= high threshold).
bool isLowerOrEqual(double first, double second,
                    const QString& firstName, const QString& secondName,
                    QString* errorOut);

} // namespace InputValidator
