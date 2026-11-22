#include "FeatureLogic.h"
// features2d.hpp is not used by the SSD code; kept (guarded) so the file still
// builds with OpenCV versions that renamed/moved that header (e.g. OpenCV 5).
#if __has_include(<opencv2/features2d.hpp>)
#include <opencv2/features2d.hpp>
#endif

#include <chrono>
#include <cstddef>
#include <limits>
#include <vector>

// --- TEAM MEMBER TASKS 3 & 4 ---
// Implement SSD and NCC Matching here.
// Task 3 (Member 3): matchSSD is implemented FROM SCRATCH below (plain loops,
// no cv::BFMatcher / cv::norm). matchNCC (Task 4) is untouched.

namespace {

    // Copy the rows of a descriptor Mat (one descriptor per row) into one flat
    // float buffer. Works for non-continuous Mats and for the usual depths, so
    // the matcher itself never has to care about OpenCV's memory layout.
    bool flattenDescriptors(const cv::Mat &m, std::vector<float> &out) {
        const int rows = m.rows;
        const int cols = m.cols;
        const int depth = m.depth();

        if (m.channels() != 1) return false;
        if (depth != CV_32F && depth != CV_64F && depth != CV_8U) return false;

        out.assign(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols), 0.0f);

        for (int r = 0; r < rows; ++r) {
            float *dst = out.data() + static_cast<std::size_t>(r) * cols;
            if (depth == CV_32F) {
                const float *src = m.ptr<float>(r);
                for (int c = 0; c < cols; ++c) dst[c] = src[c];
            } else if (depth == CV_64F) {
                const double *src = m.ptr<double>(r);
                for (int c = 0; c < cols; ++c) dst[c] = static_cast<float>(src[c]);
            } else { // CV_8U
                const uchar *src = m.ptr<uchar>(r);
                for (int c = 0; c < cols; ++c) dst[c] = static_cast<float>(src[c]);
            }
        }
        return true;
    }

} // namespace

namespace FeatureMatching {

    // Shared SSD core. useRatio=false -> plain nearest neighbour (every query gets
    // its best match). useRatio=true  -> Lowe's ratio test on top of it:
    //     keep the match only if  bestSSD < ratio^2 * secondBestSSD
    // (SSD is a squared distance, so ratio^2 on SSD == ratio on the L2 distance).
    static MatchResult matchSSDCore(const FeatureSet &queryFeatures,
                                    const FeatureSet &trainFeatures,
                                    bool useRatio, float ratioThresh) {
        MatchResult mr;
        mr.method = "SSD";
        mr.matches.clear();
        mr.matchingTimeMs = 0.0;
        mr.isEmpty = true;

        const cv::Mat &queryDesc = queryFeatures.descriptors;
        const cv::Mat &trainDesc = trainFeatures.descriptors;

        // Nothing to match, or the two descriptor sets are not comparable.
        if (queryDesc.empty() || trainDesc.empty()) return mr;
        if (queryDesc.cols != trainDesc.cols) return mr;

        // ---- Start timing (std::chrono, not OpenCV) ----
        const auto t0 = std::chrono::steady_clock::now();

        std::vector<float> q, t;
        if (!flattenDescriptors(queryDesc, q) || !flattenDescriptors(trainDesc, t)) return mr;

        const int numQuery = queryDesc.rows;
        const int numTrain = trainDesc.rows;
        const int dim      = queryDesc.cols;
        const double ratio2 = static_cast<double>(ratioThresh) * static_cast<double>(ratioThresh);

        mr.matches.reserve(static_cast<std::size_t>(numQuery));

        for (int i = 0; i < numQuery; ++i) {
            const float *qi = q.data() + static_cast<std::size_t>(i) * dim;

            double bestSSD   = std::numeric_limits<double>::max();
            double secondSSD = std::numeric_limits<double>::max();
            int bestIdx = -1;

            for (int j = 0; j < numTrain; ++j) {
                const float *tj = t.data() + static_cast<std::size_t>(j) * dim;

                double ssd = 0.0;
                for (int k = 0; k < dim; ++k) {
                    const double d = static_cast<double>(qi[k]) - static_cast<double>(tj[k]);
                    ssd += d * d;
                    // Early exit: this candidate can no longer become the best OR the
                    // second best, so it cannot change the result (only saves time).
                    if (ssd >= secondSSD) break;
                }

                if (ssd < bestSSD) {            // new best: old best becomes second best
                    secondSSD = bestSSD;
                    bestSSD   = ssd;
                    bestIdx   = j;
                } else if (ssd < secondSSD) {   // new second best
                    secondSSD = ssd;
                }
            }

            if (bestIdx < 0) continue;
            if (useRatio && !(bestSSD < ratio2 * secondSSD)) continue;   // Lowe's ratio test

            // queryIdx / trainIdx index into the keypoint vectors, distance = SSD value.
            mr.matches.emplace_back(i, bestIdx, static_cast<float>(bestSSD));
        }

        // ---- Stop timing ----
        const auto t1 = std::chrono::steady_clock::now();
        mr.matchingTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        mr.isEmpty = mr.matches.empty();
        return mr;
    }

    // Task 3: plain nearest-neighbour SSD (original interface, behaviour unchanged).
    MatchResult matchSSD(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures) {
        return matchSSDCore(queryFeatures, trainFeatures, false, 1.0f);
    }

    // Task 3: SSD + Lowe's ratio test (used by the Feature Matching window).
    MatchResult matchSSD(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures, float ratioThresh) {
        return matchSSDCore(queryFeatures, trainFeatures, true, ratioThresh);
    }

    MatchResult matchNCC(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures) {
        MatchResult mr;
        mr.method = "NCC";
        
        // -- START IMPLEMENTATION (Task 4) --
        
        // For NCC using OpenCV, you typically use cv::BFMatcher(cv::NORM_L2, false)
        // AND ensure your descriptors are normalized, or use cv::NORM_INF.
        
        // Temporary placeholder:
        mr.matches.clear();
        mr.matchingTimeMs = 0.0;
        // mr.isEmpty = false; // Uncomment when implemented

        // -- END IMPLEMENTATION --
        
        return mr;
    }
}