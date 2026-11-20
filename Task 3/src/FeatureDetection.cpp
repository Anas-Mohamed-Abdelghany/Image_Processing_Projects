#include "FeatureLogic.h"
// Include OpenCV modules needed for Harris
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

#include <algorithm>
#include <cmath>

// --- TEAM MEMBER TASK 1 ---
// Harris Corner Detection + Non-Maximum Suppression + timing.

namespace FeatureDetection {

    FeatureSet computeHarrisCorners(const cv::Mat &grayscaleImage) {
        FeatureSet fs;
        fs.keypoints.clear();
        fs.computationTimeMs = 0.0;
        fs.isEmpty = true;

        // Nothing to do with an empty/invalid input.
        if (grayscaleImage.empty()) {
            return fs;
        }

        // Ensure we work on a single-channel 8-bit image.
        cv::Mat gray;
        if (grayscaleImage.channels() == 1) {
            if (grayscaleImage.type() == CV_8UC1) {
                gray = grayscaleImage;
            } else {
                grayscaleImage.convertTo(gray, CV_8UC1);
            }
        } else {
            cv::cvtColor(grayscaleImage, gray, cv::COLOR_BGR2GRAY);
        }

        // Start the timer (covers the whole Harris pipeline).
        const int64 startTicks = cv::getTickCount();

        // 1) Harris response: R = det(M) - k * trace(M)^2
        const int blockSize = 2;   // size of the averaging window for the gradient sums
        const int apertureSize = 3; // Sobel aperture
        const double k = 0.04;      // Harris free parameter (typical value)
        cv::Mat harrisResponse;
        cv::cornerHarris(gray, harrisResponse, blockSize, apertureSize, k);

        // 2) Candidate peaks: keep raw responses above a relative threshold.
        //    (0.01 of the maximum response, a standard Harris heuristic.)
        //    NOTE: we threshold the RAW response, not a min-max normalized one:
        //    flat regions give responses near 0 (sometimes slightly negative),
        //    and min-max scaling would map those to mid-range values.
        double maxResponse = 0.0;
        cv::minMaxLoc(harrisResponse, nullptr, &maxResponse);

        cv::Mat candidates;
        cv::compare(harrisResponse, 0.01 * maxResponse, candidates, cv::CMP_GT); // CV_8U

        // 3) Non-maximum suppression: dilate the response with a 3x3 kernel so
        //    only the local maxima of the response survive.
        const cv::Mat nmsKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::Mat localMaxima;
        cv::dilate(harrisResponse, localMaxima, nmsKernel);

        cv::Mat isLocalMax;
        cv::compare(harrisResponse, localMaxima, isLocalMax, cv::CMP_GE); // CV_8U

        cv::Mat nonMaxSuppressed;
        cv::bitwise_and(candidates, isLocalMax, nonMaxSuppressed);

        // 4) Collect the surviving peaks as keypoints (sorted by response, desc).
        std::vector<cv::KeyPoint> keypoints;
        keypoints.reserve(1024);
        for (int y = 0; y < nonMaxSuppressed.rows; ++y) {
            const uchar *rowPtr = nonMaxSuppressed.ptr<uchar>(y);
            const float *respPtr = harrisResponse.ptr<float>(y);
            for (int x = 0; x < nonMaxSuppressed.cols; ++x) {
                if (rowPtr[x] != 0) {
                    keypoints.emplace_back(static_cast<float>(x), static_cast<float>(y),
                                           6.0f, -1.0f, respPtr[x]);
                }
            }
        }

        std::sort(keypoints.begin(), keypoints.end(),
                  [](const cv::KeyPoint &a, const cv::KeyPoint &b) {
                      return a.response > b.response;
                  });

        // Stop the timer and store elapsed milliseconds.
        const int64 endTicks = cv::getTickCount();
        const double ticksPerMs = cv::getTickFrequency() / 1000.0;
        fs.computationTimeMs = static_cast<double>(endTicks - startTicks) / ticksPerMs;

        fs.keypoints = std::move(keypoints);
        fs.isEmpty = fs.keypoints.empty();

        return fs;
    }
}
