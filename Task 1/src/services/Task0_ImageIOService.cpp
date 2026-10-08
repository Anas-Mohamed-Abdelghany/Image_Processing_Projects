#include "services/Task0_ImageIOService.h"

// ============================================================================
// Task 0 - Image Input                                          [TODO: Teammate]
// ============================================================================
cv::Mat Task0_ImageIOService::read(const std::string& filePath)
{
    // TODO(Task0): read the image with cv::imread (keep IMREAD_UNCHANGED or
    // IMREAD_COLOR so both RGB and grayscale inputs work).
    // NOTE: cv::imread cannot open paths with non-ASCII characters on Windows -
    // if that matters, read the bytes via QImage/QFile and use cv::imdecode.
    (void)filePath;
    return {};
}

bool Task0_ImageIOService::write(const std::string& filePath, const cv::Mat& image)
{
    // TODO(Task0): write the image with cv::imwrite.
    (void)filePath;
    (void)image;
    return false;
}
