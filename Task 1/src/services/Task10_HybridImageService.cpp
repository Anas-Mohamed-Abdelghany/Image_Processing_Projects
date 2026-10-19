#include "services/Task10_HybridImageService.h"

#include "services/Task9_FrequencyFilterService.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

// ============================================================================
// Task 10 - Hybrid Images
// low frequencies of one image + high frequencies of another image.
//   hybrid = Task9::lowPass(lowImage) + Task9::highPass(highImage)
// The two Task 9 filters are used in their raw (float, signed) form so the
// high frequencies keep both their bright and dark side before they are added.
// createHybridFromFilters() generalises this: every image gets its own filter
// (low / high pass) and its own cutoff.
// ============================================================================

namespace {

// 8 bit version of any input depth (16 bit is scaled down, float is stretched).
cv::Mat toEightBit(const cv::Mat& image)
{
    cv::Mat result;
    switch (image.depth()) {
    case CV_8U:
        return image; // shared buffer, only ever read
    case CV_16U:
        image.convertTo(result, CV_8U, 1.0 / 257.0);
        return result;
    default:
        cv::normalize(image, result, 0, 255, cv::NORM_MINMAX, CV_8U);
        return result;
    }
}

// Gray (1 channel) or BGR (3 channels): an alpha channel is dropped because a
// hybrid of two transparency masks makes no sense.
cv::Mat toGrayOrBgr(const cv::Mat& image)
{
    cv::Mat result;
    switch (image.channels()) {
    case 1:
    case 3:
        return image;
    case 4:
        cv::cvtColor(image, result, cv::COLOR_BGRA2BGR);
        return result;
    default:
        cv::extractChannel(image, result, 0);
        return result;
    }
}

// Scales `image` so it covers `target` completely (aspect ratio kept, so
// nothing is stretched), then crops the centre. Both hybrid sources must have
// the same size, and a distorted image would ruin the effect.
cv::Mat coverToSize(const cv::Mat& image, const cv::Size& target)
{
    if (image.size() == target) {
        return image;
    }

    const double scale = std::max(static_cast<double>(target.width) / image.cols,
                                  static_cast<double>(target.height) / image.rows);
    const int width = std::max(target.width, static_cast<int>(std::lround(image.cols * scale)));
    const int height = std::max(target.height, static_cast<int>(std::lround(image.rows * scale)));

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(width, height), 0, 0,
               scale < 1.0 ? cv::INTER_AREA : cv::INTER_CUBIC);

    const cv::Rect centre((width - target.width) / 2, (height - target.height) / 2,
                          target.width, target.height);
    return resized(centre).clone();
}

} // namespace

cv::Mat Task10_HybridImageService::createHybrid(const cv::Mat& lowImage, const cv::Mat& highImage, double cutoffRadius)
{
    // Classic hybrid: low pass of the first image + high pass of the second one,
    // both with the shared cutoff.
    return createHybridFromFilters(lowImage, false, cutoffRadius, highImage, true, cutoffRadius);
}

cv::Mat Task10_HybridImageService::createHybridFromFilters(const cv::Mat& image1, bool highPass1, double cutoff1,
                                                           const cv::Mat& image2, bool highPass2, double cutoff2)
{
    if (image1.empty() || image2.empty()) {
        return cv::Mat();
    }

    // 1) Bring both images to the same type: 8 bit, gray or BGR, same channels.
    //    (These are views / new matrices - the caller's images are not modified.)
    cv::Mat first = toGrayOrBgr(toEightBit(image1));
    cv::Mat second = toGrayOrBgr(toEightBit(image2));

    if (first.channels() != second.channels()) {
        // One image is gray and the other colour: promote the gray one to BGR.
        cv::Mat& grayImage = (first.channels() == 1) ? first : second;
        cv::Mat promoted;
        cv::cvtColor(grayImage, promoted, cv::COLOR_GRAY2BGR);
        grayImage = promoted;
    }

    // 2) Same size: the second image is fitted to the first one.
    second = coverToSize(second, first.size());

    // 3) Frequency filtering with the Task 9 filters (raw float, signed).
    const cv::Mat filtered1 = highPass1 ? Task9_FrequencyFilterService::highPassFloat(first, cutoff1)
                                        : Task9_FrequencyFilterService::lowPassFloat(first, cutoff1);
    const cv::Mat filtered2 = highPass2 ? Task9_FrequencyFilterService::highPassFloat(second, cutoff2)
                                        : Task9_FrequencyFilterService::lowPassFloat(second, cutoff2);

    // 4) Combine and saturate to 0..255. A high pass result is zero-mean
    //    detail, a low pass result carries the brightness / colour of its image.
    double scale = 1.0;
    double offset = 0.0;
    if (!highPass1 && !highPass2) {
        scale = 0.5;      // two low passes: average them to keep the brightness
    } else if (highPass1 && highPass2) {
        offset = 128.0;   // two details: put them on a mid gray background
    }

    const cv::Mat sum = filtered1 + filtered2;
    cv::Mat hybrid;
    sum.convertTo(hybrid, CV_8U, scale, offset);
    return hybrid;
}
