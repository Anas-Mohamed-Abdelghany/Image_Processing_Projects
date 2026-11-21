#include "FeatureLogic.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

// =========================================================
// --- TEAM MEMBER TASK 1 ---
// Harris Corner Detection implemented FROM SCRATCH.
// No OpenCV detection helpers are used: no cv::cornerHarris,
// no cv::dilate, no cv::threshold, no cv::compare, no
// cv::GaussianBlur...
// Everything (scale-space blur, gradients, structure tensor,
// Harris response, thresholding and non-max suppression) is
// hand-written.
// =========================================================

namespace {

    inline int clampIndex(int v, int lo, int hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    // Convert any input to 8-bit single-channel, manually.
    cv::Mat convertToGray8(const cv::Mat &input) {
        cv::Mat gray(input.rows, input.cols, CV_8UC1);

        const int channels = input.channels();
        const int depth = input.depth(); // CV_8U, CV_16U, CV_32F ...

        // ---- Single channel ----
        if (channels == 1) {
            if (depth == CV_8U) {
                // Row-wise copy (no cv::Mat::copyTo needed beyond pixel loops)
                for (int y = 0; y < input.rows; ++y) {
                    const uchar *src = input.ptr<uchar>(y);
                    uchar *dst = gray.ptr<uchar>(y);
                    for (int x = 0; x < input.cols; ++x) {
                        dst[x] = src[x];
                    }
                }
                return gray;
            }

            // Manual min-max normalization to [0,255] for other depths.
            double minVal = 0.0, maxVal = 0.0;
            bool first = true;
            for (int y = 0; y < input.rows; ++y) {
                for (int x = 0; x < input.cols; ++x) {
                    double v = 0.0;
                    if (depth == CV_8S)  v = input.ptr<signed char>(y)[x];
                    else if (depth == CV_16U) v = input.ptr<unsigned short>(y)[x];
                    else if (depth == CV_16S) v = input.ptr<short>(y)[x];
                    else if (depth == CV_32F) v = input.ptr<float>(y)[x];
                    else if (depth == CV_64F) v = input.ptr<double>(y)[x];
                    if (first) { minVal = maxVal = v; first = false; }
                    else { minVal = std::min(minVal, v); maxVal = std::max(maxVal, v); }
                }
            }
            const double range = maxVal - minVal;
            const double scale = range > 1e-12 ? 255.0 / range : 0.0;

            for (int y = 0; y < input.rows; ++y) {
                uchar *dst = gray.ptr<uchar>(y);
                for (int x = 0; x < input.cols; ++x) {
                    double v = 0.0;
                    if (depth == CV_8S)  v = input.ptr<signed char>(y)[x];
                    else if (depth == CV_16U) v = input.ptr<unsigned short>(y)[x];
                    else if (depth == CV_16S) v = input.ptr<short>(y)[x];
                    else if (depth == CV_32F) v = input.ptr<float>(y)[x];
                    else if (depth == CV_64F) v = input.ptr<double>(y)[x];
                    dst[x] = static_cast<uchar>(std::min(255.0, std::max(0.0, (v - minVal) * scale)));
                }
            }
            return gray;
        }

        // ---- Multi-channel (BGR / BGRA) -> gray, Rec.601 luma, manually ----
        for (int y = 0; y < input.rows; ++y) {
            uchar *dst = gray.ptr<uchar>(y);
            if (depth == CV_8U) {
                const uchar *src = input.ptr<uchar>(y);
                for (int x = 0; x < input.cols; ++x) {
                    const uchar *px = src + x * channels;
                    // 0.114*B + 0.587*G + 0.299*R
                    const int v = (114 * px[0] + 587 * px[1] + 299 * px[2] + 500) / 1000;
                    dst[x] = static_cast<uchar>(v);
                }
            } else {
                // Fallback: read as double via the same min/max path per pixel.
                for (int x = 0; x < input.cols; ++x) {
                    double b = 0.0, g = 0.0, r = 0.0;
                    if (depth == CV_32F) {
                        const float *px = input.ptr<float>(y) + x * channels;
                        b = px[0]; g = px[1]; r = px[2];
                    } else if (depth == CV_64F) {
                        const double *px = input.ptr<double>(y) + x * channels;
                        b = px[0]; g = px[1]; r = px[2];
                    } else {
                        b = g = r = 0.0;
                    }
                    const int v = static_cast<int>(0.114 * b + 0.587 * g + 0.299 * r + 0.5);
                    dst[x] = static_cast<uchar>(clampIndex(v, 0, 255));
                }
            }
        }
        return gray;
    }

    // Sample an 8-bit image with replicated borders.
    inline uchar sampleReplicate(const std::vector<const uchar *> &rows, int x, int y, int w, int h) {
        const int xs = clampIndex(x, 0, w - 1);
        const int ys = clampIndex(y, 0, h - 1);
        return rows[ys][xs];
    }

    // ------------------------------------------------------------------
    // Separable Gaussian blur, FROM SCRATCH (no cv::GaussianBlur).
    //   kernel radius = ceil(3 * sigma)  -> truncates at ~3 sigma
    //   two passes: horizontal then vertical, replicated borders
    // ------------------------------------------------------------------
    void gaussianBlur8(const std::vector<uchar> &src, std::vector<uchar> &dst,
                       int w, int h, float sigma) {
        if (sigma <= 0.05f) {          // sigma ~ 0: blur is the identity
            dst = src;
            return;
        }

        // 1) Build the normalized 1-D kernel:  G(x) = exp(-x^2 / (2 sigma^2))
        const int radius = std::max(1, static_cast<int>(std::ceil(3.0f * sigma)));
        std::vector<float> kernel(static_cast<size_t>(2 * radius + 1));
        float sum = 0.0f;
        for (int i = -radius; i <= radius; ++i) {
            const float v = std::exp(-(i * i) / (2.0f * sigma * sigma));
            kernel[static_cast<size_t>(i + radius)] = v;
            sum += v;
        }
        for (float &v : kernel) v /= sum;

        // 2) Horizontal pass: src -> tmp (float keeps the precision)
        std::vector<float> tmp(static_cast<size_t>(w * h));
        for (int y = 0; y < h; ++y) {
            const uchar *srow = src.data() + static_cast<size_t>(y) * w;
            float *trow = tmp.data() + static_cast<size_t>(y) * w;
            for (int x = 0; x < w; ++x) {
                float acc = 0.0f;
                for (int i = -radius; i <= radius; ++i) {
                    const int xx = clampIndex(x + i, 0, w - 1);
                    acc += kernel[static_cast<size_t>(i + radius)] * srow[xx];
                }
                trow[x] = acc;
            }
        }

        // 3) Vertical pass: tmp -> dst (rounded back to 8-bit)
        dst.resize(static_cast<size_t>(w * h));
        for (int y = 0; y < h; ++y) {
            uchar *drow = dst.data() + static_cast<size_t>(y) * w;
            for (int x = 0; x < w; ++x) {
                float acc = 0.0f;
                for (int i = -radius; i <= radius; ++i) {
                    const int yy = clampIndex(y + i, 0, h - 1);
                    acc += kernel[static_cast<size_t>(i + radius)] * tmp[static_cast<size_t>(yy) * w + x];
                }
                const int v = static_cast<int>(acc + 0.5f);
                drow[x] = static_cast<uchar>(clampIndex(v, 0, 255));
            }
        }
    }

} // namespace

namespace FeatureDetection {

    FeatureSet computeHarrisCorners(const cv::Mat &grayscaleImage,
                                    float sigma,
                                    int maxKeypoints) {
        FeatureSet fs;
        fs.keypoints.clear();
        fs.computationTimeMs = 0.0;
        fs.isEmpty = true;

        if (grayscaleImage.empty() || sigma <= 0.0f || maxKeypoints < 0) {
            return fs;
        }

        // ---- Start timing (std::chrono, not OpenCV) ----
        const auto t0 = std::chrono::steady_clock::now();

        // 1) Grayscale 8-bit image (manual conversion).
        const cv::Mat gray0 = convertToGray8(grayscaleImage);
        const int w = gray0.cols;
        const int h = gray0.rows;
        if (w < 3 || h < 3) {
            return fs; // too small for a 3x3 neighborhood
        }

        // 1b) Scale-space step: blur at sigma BEFORE gradients, so the Harris
        //     response is computed at the scale the user asked for.
        //     (hand-written separable Gaussian, not cv::GaussianBlur)
        std::vector<uchar> flat(static_cast<size_t>(w) * h);
        for (int y = 0; y < h; ++y) {
            const uchar *src = gray0.ptr<uchar>(y);
            std::copy(src, src + w, flat.data() + static_cast<size_t>(y) * w);
        }
        std::vector<uchar> blurred;
        gaussianBlur8(flat, blurred, w, h, sigma);

        // KeyPoint::size: the scale SIFT will sample this corner at.
        // Heuristic: the main gradient window lives inside ~3 sigma of radius,
        // i.e. a diameter of 6 sigma.
        const float keypointSize = 6.0f * sigma;

        // Gather row pointers once (replicated border sampling).
        std::vector<const uchar *> rows(h);
        for (int y = 0; y < h; ++y) {
            rows[y] = blurred.data() + static_cast<size_t>(y) * w;
        }

        // 2) Sobel gradients Ix, Iy (3x3 kernels, hand-rolled, replicated borders).
        const int n = w * h;
        std::vector<float> ix(n), iy(n);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                //         | -1  0  1 |         | -1 -2 -1 |
                // Gx = 1/8| -2  0  2 |   Gy = 1/8|  0  0  0 |
                //         | -1  0  1 |         |  1  2  1 |
                // (1/8 scaling keeps gradient magnitudes well-conditioned)
                const int tl = sampleReplicate(rows, x - 1, y - 1, w, h);
                const int tc = sampleReplicate(rows, x,     y - 1, w, h);
                const int tr = sampleReplicate(rows, x + 1, y - 1, w, h);
                const int ml = sampleReplicate(rows, x - 1, y,     w, h);
                const int mr = sampleReplicate(rows, x + 1, y,     w, h);
                const int bl = sampleReplicate(rows, x - 1, y + 1, w, h);
                const int bc = sampleReplicate(rows, x,     y + 1, w, h);
                const int br = sampleReplicate(rows, x + 1, y + 1, w, h);

                const int gx = (tr + 2 * mr + br) - (tl + 2 * ml + bl);
                const int gy = (bl + 2 * bc + br) - (tl + 2 * tc + tr);

                ix[y * w + x] = gx / 8.0f;
                iy[y * w + x] = gy / 8.0f;
            }
        }

        // 3) Structure tensor M = [[A, C], [C, B]]:
        //    A = avg(Ix^2), B = avg(Iy^2), C = avg(Ix*Iy)
        //    The integration window must MATCH the detection scale, otherwise
        //    large sigma just smears gradients that a fixed 3x3 window can no
        //    longer integrate (and large corners lose to noise). Window radius
        //    ~ sigma keeps the tensor tuned to the scale we blurred at.
        const int winR = std::max(1, static_cast<int>(std::lround(sigma)));
        const int winN = 2 * winR + 1;
        const float winInv = 1.0f / static_cast<float>(winN * winN);
        std::vector<float> a(n), b(n), c(n);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float sa = 0.0f, sb = 0.0f, sc = 0.0f;
                for (int dy = -winR; dy <= winR; ++dy) {
                    const int yy = clampIndex(y + dy, 0, h - 1);
                    for (int dx = -winR; dx <= winR; ++dx) {
                        const int xx = clampIndex(x + dx, 0, w - 1);
                        const float gx = ix[yy * w + xx];
                        const float gy = iy[yy * w + xx];
                        sa += gx * gx;
                        sb += gy * gy;
                        sc += gx * gy;
                    }
                }
                const int idx = y * w + x;
                a[idx] = sa * winInv;
                b[idx] = sb * winInv;
                c[idx] = sc * winInv;
            }
        }

        // 4) Harris response: R = det(M) - k * trace(M)^2
        //    det = A*B - C^2 , trace = A + B , k = 0.04
        const float k = 0.04f;
        std::vector<float> response(n);
        float maxResponse = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float det = a[i] * b[i] - c[i] * c[i];
            const float trace = a[i] + b[i];
            const float r = det - k * trace * trace;
            response[i] = r;
            if (r > maxResponse) maxResponse = r;
        }

        // No positive response -> no corners.
        if (maxResponse <= 0.0f) {
            const auto t1 = std::chrono::steady_clock::now();
            fs.computationTimeMs =
                std::chrono::duration<double, std::milli>(t1 - t0).count();
            return fs;
        }

        // 5) Global threshold: keep R > 1% of the maximum response (classic heuristic).
        const float thresholdValue = 0.01f * maxResponse;

        // 6) Non-maximum suppression over the 3x3 neighborhood, by hand.
        //    A pixel survives when:
        //      * it is above the threshold, and
        //      * R >= all 8 neighbors, and
        //      * no earlier neighbor (top row / left) ties with it
        //        (this keeps only the first pixel of a flat plateau).
        std::vector<cv::KeyPoint> keypoints;
        keypoints.reserve(1024);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int idx = y * w + x;
                const float r = response[idx];
                if (r <= thresholdValue) continue;

                float neighborhoodMax = r;
                float earlierMax = -1.0f; // ties among earlier (raster) neighbors
                for (int dy = -1; dy <= 1; ++dy) {
                    const int yy = clampIndex(y + dy, 0, h - 1);
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        const int xx = clampIndex(x + dx, 0, w - 1);
                        const float rn = response[yy * w + xx];
                        if (rn > neighborhoodMax) {
                            neighborhoodMax = rn; // a bigger neighbor exists -> not a maximum
                        }
                        if (dy < 0 || (dy == 0 && dx < 0)) {
                            earlierMax = std::max(earlierMax, rn);
                        }
                    }
                }
                if (neighborhoodMax > r) continue;          // not a local maximum
                if (earlierMax >= r) continue;              // plateau -> keep only first

                // size = 6 * sigma (the scale this corner was detected at);
                // Task 2's SIFT reads kp.size as its descriptor patch scale.
                keypoints.emplace_back(static_cast<float>(x), static_cast<float>(y),
                                       keypointSize, -1.0f, r);
            }
        }

        // 7) Strongest corners first.
        std::sort(keypoints.begin(), keypoints.end(),
                  [](const cv::KeyPoint &p1, const cv::KeyPoint &p2) {
                      return p1.response > p2.response;
                  });

        // Optional cap on how many keypoints are handed to Task 2 (0 = all).
        if (maxKeypoints > 0 && static_cast<int>(keypoints.size()) > maxKeypoints) {
            keypoints.resize(static_cast<size_t>(maxKeypoints));
        }

        // ---- Stop timing ----
        const auto t1 = std::chrono::steady_clock::now();
        fs.computationTimeMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();

        fs.keypoints = std::move(keypoints);
        fs.isEmpty = fs.keypoints.empty();

        return fs;
    }
}
