#include "validation/InputValidator.h"

#include <QObject>

namespace InputValidator {

bool isValidImage(const cv::Mat& image, const QString& name, QString* errorOut)
{
    if (image.empty() || image.data == nullptr) {
        if (errorOut != nullptr) {
            *errorOut = QObject::tr("%1 is empty - please load an image first.").arg(name);
        }
        return false;
    }
    return true;
}

bool isValidKernelSize(int ksize, QString* errorOut)
{
    if (ksize < 3 || ksize > 21 || (ksize % 2) == 0) {
        if (errorOut != nullptr) {
            *errorOut = QObject::tr("Kernel size must be an odd number between 3 and 21 (got %1).").arg(ksize);
        }
        return false;
    }
    return true;
}

bool isInRange(double value, double minValue, double maxValue,
               const QString& name, QString* errorOut)
{
    if (value < minValue || value > maxValue) {
        if (errorOut != nullptr) {
            *errorOut = QObject::tr("%1 must be between %2 and %3 (got %4).")
                            .arg(name)
                            .arg(minValue)
                            .arg(maxValue)
                            .arg(value);
        }
        return false;
    }
    return true;
}

bool isLowerOrEqual(double first, double second,
                    const QString& firstName, const QString& secondName,
                    QString* errorOut)
{
    if (first > second) {
        if (errorOut != nullptr) {
            *errorOut = QObject::tr("%1 (%2) must not be larger than %3 (%4).")
                            .arg(firstName).arg(first)
                            .arg(secondName).arg(second);
        }
        return false;
    }
    return true;
}

} // namespace InputValidator
