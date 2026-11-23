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
#include <cmath>
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

        // Shared NCC core. Each descriptor is mean-subtracted and divided by its norm,
    // so NCC(a,b) = dot(a_n, b_n) in [-1, 1]. Higher = better.
    // distance = 1 - NCC (lower = better, same convention as SSD for sorting/drawMatches).
    // useRatio=true: keep match only if (1-best) < ratio * (1-second)  (Lowe's test).
    static MatchResult matchNCCCore(const FeatureSet &queryFeatures,
                                    const FeatureSet &trainFeatures,
                                    bool useRatio, float ratioThresh) {
        MatchResult mr;
        mr.method = "NCC";
        mr.matches.clear();
        mr.matchingTimeMs = 0.0;
        mr.isEmpty = true;

        const cv::Mat &queryDesc = queryFeatures.descriptors;
        const cv::Mat &trainDesc = trainFeatures.descriptors;
        if (queryDesc.empty() || trainDesc.empty()) return mr;
        if (queryDesc.cols != trainDesc.cols) return mr;

        const auto t0 = std::chrono::steady_clock::now();

        std::vector<float> q, t;
        if (!flattenDescriptors(queryDesc, q) || !flattenDescriptors(trainDesc, t)) return mr;

        const int numQuery = queryDesc.rows;
        const int numTrain = trainDesc.rows;
        const int dim      = queryDesc.cols;

        // Zero-mean + unit-norm each row in place. Flat descriptors (norm ~ 0) become all zeros -> NCC 0.
        auto normalizeRows = [dim](std::vector<float> &buf, int rows) {
            // For each row, subtract the mean and divide by the norm. If the norm is too small, set the row to all zeros.
            for (int r = 0; r < rows; ++r) {
                float *p = buf.data() + static_cast<std::size_t>(r) * dim;
                double mean = 0.0;
                for (int k = 0; k < dim; ++k) mean += p[k];
                mean /= dim;
                double sq = 0.0;
                for (int k = 0; k < dim; ++k) {
                    p[k] = static_cast<float>(p[k] - mean);
                    sq += static_cast<double>(p[k]) * p[k];
                }
                // Normalize to unit length (NCC = dot product of normalized vectors). If the
                const double norm = std::sqrt(sq);
                if (norm < 1e-12) {
                    for (int k = 0; k < dim; ++k) p[k] = 0.0f;
                } else {
                    for (int k = 0; k < dim; ++k) p[k] = static_cast<float>(p[k] / norm);
                }
            }
        };
        normalizeRows(q, numQuery);
        normalizeRows(t, numTrain);

        mr.matches.reserve(static_cast<std::size_t>(numQuery));
        // For each query descriptor, find the best and second-best NCC match in the train set.
        for (int i = 0; i < numQuery; ++i) {
            const float *qi = q.data() + static_cast<std::size_t>(i) * dim;

            double bestNCC   = -2.0;
            double secondNCC = -2.0;
            int bestIdx = -1;
            // For each train descriptor, compute the NCC (dot product of normalized vectors) and keep track of the best and second-best matches.
            for (int j = 0; j < numTrain; ++j) {
                const float *tj = t.data() + static_cast<std::size_t>(j) * dim;
                double ncc = 0.0;
                for (int k = 0; k < dim; ++k)
                    ncc += static_cast<double>(qi[k]) * static_cast<double>(tj[k]);

                if (ncc > bestNCC) { secondNCC = bestNCC; bestNCC = ncc; bestIdx = j; }
                else if (ncc > secondNCC) { secondNCC = ncc; }
            }

            if (bestIdx < 0) continue;

            const double bestDist   = 1.0 - bestNCC;
            const double secondDist = 1.0 - secondNCC;
            // Lowe's ratio test: keep the match only if best < ratio * second (lower = better).
            const double r2 = static_cast<double>(ratioThresh) * static_cast<double>(ratioThresh);
            if (useRatio && !(bestDist < r2 * secondDist)) continue;

            mr.matches.emplace_back(i, bestIdx, static_cast<float>(bestDist));
        }

        const auto t1 = std::chrono::steady_clock::now();
        mr.matchingTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        mr.isEmpty = mr.matches.empty();
        return mr;
    }

    // Task 4: plain nearest-neighbour NCC.
    MatchResult matchNCC(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures) {
        return matchNCCCore(queryFeatures, trainFeatures, false, 1.0f);
    }

    // Task 4: NCC + ratio test (mirrors the SSD overload).
    MatchResult matchNCC(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures, float ratioThresh) {
        return matchNCCCore(queryFeatures, trainFeatures, true, ratioThresh);
    }
}