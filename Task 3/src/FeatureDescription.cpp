#include "FeatureLogic.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <utility>
#include <vector>
#include <opencv2/features2d.hpp>

// =========================================================
// --- TEAM MEMBER TASK 2 ---
// SIFT descriptor generation implemented FROM SCRATCH.
// No cv::SIFT / cv::xfeatures2d, no cv::GaussianBlur, no cv::Sobel,
// no cv::normalize ... cv::Mat is only used as the output container.
//
// Pipeline (Lowe 2004, adapted to Member 1's Harris keypoints):
//   1. Convert the image to float gray.
//   2. Scale per keypoint = kp.size / 2 (OpenCV convention), snapped to kNumLevels
//      scales. Harris is single-scale, so every keypoint currently gets the same sigma;
//      real scale invariance needs Member 1 to output kp.size per detected scale.
//   3. For every scale that was selected by at least one keypoint:
//        blur the image with that sigma, compute gradient magnitude + angle ONCE.
//   4. For each keypoint (in parallel):
//        a. Orientation assignment (36-bin histogram, peak + parabolic interpolation).
//        b. 4x4 grid x 8 orientation bins = 128 values, rotated to the dominant
//           orientation, Gaussian window, trilinear interpolation.
//        c. L2 normalize -> clamp at 0.2 -> L2 normalize again.
//   5. Keypoints that cannot produce a valid descriptor are removed.
//
// Output: features.descriptors is CV_32F, one row (128 floats) per keypoint,
// row i belongs to features.keypoints[i]. keypoint.size = 2*sigma, keypoint.angle set.
//
// Define FD_NO_THREADS if your MinGW uses the "win32" thread model (no std::thread).
// =========================================================

namespace {

    constexpr float kPi            = 3.14159265358979323846f;
    constexpr float kTwoPi         = 2.0f * kPi;
    constexpr float kBaseSigma     = 1.6f;   // smallest scale
    constexpr int   kNumLevels     = 7;      // 1.6 ... 6.4 (2 octaves, 3 levels per octave)
    constexpr int   kOriBins       = 36;
    constexpr float kOriSigmaFact  = 1.5f;   // orientation window sigma = 1.5 * keypoint sigma
    constexpr float kOriRadiusFact = 3.0f * kOriSigmaFact;
    constexpr float kOriPeakRatio  = 0.8f;   // secondary peaks, only used if kMultiOrientation
    constexpr int   kDescWidth     = 4;      // 4x4 spatial cells
    constexpr int   kDescBins      = 8;      // 8 orientation bins per cell
    constexpr int   kDescLength    = kDescWidth * kDescWidth * kDescBins; // 128
    constexpr float kCellScale     = 3.0f;   // cell width = 3 * sigma
    constexpr float kMagClamp      = 0.2f;   // illumination-saturation clamp
    // Border rule: keypoint must be >= kMinBorderCells cells (3*sigma px each) away from the image edge
    // (pixels outside the image simply contribute nothing). The old rule demanded the
    // WHOLE window inside the image, which threw away ~20% of the corners.
    constexpr float kMinBorderCells = 1.0f;
    // false: one descriptor per Harris corner (dominant orientation only) -> keeps a 1:1
    // mapping with Member 1's keypoints. true: classic SIFT, extra descriptor per
    // secondary peak >= 80% of the main one.
    constexpr bool  kMultiOrientation = false;

    inline int clampIndex(int v, int lo, int hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    inline float levelSigma(int i) {
        return kBaseSigma * std::pow(2.0f, static_cast<float>(i) / 3.0f);
    }

    // ---------------------------------------------------------------- threading helper
    template <class F>
    void parallelFor(int n, F &&fn) {
#ifndef FD_NO_THREADS
        const int hw = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
        const int T = std::min(std::min(hw, 8), n);
        if (T > 1) {
            std::atomic<int> next{0};
            const int chunk = std::max(1, n / (T * 8));
            auto worker = [&]() {
                for (;;) {
                    const int s = next.fetch_add(chunk);
                    if (s >= n) break;
                    const int e = std::min(n, s + chunk);
                    for (int i = s; i < e; ++i) fn(i);
                }
            };
            std::vector<std::thread> pool;
            pool.reserve(T - 1);
            for (int t = 0; t < T - 1; ++t) pool.emplace_back(worker);
            worker();
            for (auto &th : pool) th.join();
            return;
        }
#endif
        for (int i = 0; i < n; ++i) fn(i);
    }

    // Plain row-major float image so the inner loops stay fast and OpenCV-free.
    struct FloatImage {
        int w = 0, h = 0;
        std::vector<float> data;
        float at(int x, int y) const { return data[static_cast<size_t>(y) * w + x]; }
    };

    // Convert whatever the Mat holds into a 0..255 float image, by hand.
    FloatImage toFloatGray(const cv::Mat &src) {
        FloatImage out;
        out.w = src.cols;
        out.h = src.rows;
        out.data.assign(static_cast<size_t>(out.w) * out.h, 0.0f);

        const int ch = src.channels();
        const int depth = src.depth();

        auto readChannel = [&](int x, int y, int c) -> double {
            switch (depth) {
                case CV_8U:  return src.ptr<uchar>(y)[x * ch + c];
                case CV_8S:  return src.ptr<signed char>(y)[x * ch + c];
                case CV_16U: return src.ptr<unsigned short>(y)[x * ch + c];
                case CV_16S: return src.ptr<short>(y)[x * ch + c];
                case CV_32S: return src.ptr<int>(y)[x * ch + c];
                case CV_32F: return src.ptr<float>(y)[x * ch + c];
                case CV_64F: return src.ptr<double>(y)[x * ch + c];
                default:     return 0.0;
            }
        };

        // Rec.601 luma for BGR/BGRA, first channel otherwise.
        auto pixel = [&](int x, int y) -> double {
            if (ch >= 3) {
                return 0.114 * readChannel(x, y, 0) + 0.587 * readChannel(x, y, 1) +
                       0.299 * readChannel(x, y, 2);
            }
            return readChannel(x, y, 0);
        };

        if (depth == CV_8U) {
            for (int y = 0; y < out.h; ++y)
                for (int x = 0; x < out.w; ++x)
                    out.data[static_cast<size_t>(y) * out.w + x] = static_cast<float>(pixel(x, y));
            return out;
        }

        // Other depths: manual min-max stretch to 0..255.
        double lo = 0.0, hi = 0.0;
        bool first = true;
        for (int y = 0; y < out.h; ++y) {
            for (int x = 0; x < out.w; ++x) {
                const double v = pixel(x, y);
                if (first) { lo = hi = v; first = false; }
                else { lo = std::min(lo, v); hi = std::max(hi, v); }
            }
        }
        const double range = hi - lo;
        const double scale = range > 1e-12 ? 255.0 / range : 0.0;
        for (int y = 0; y < out.h; ++y)
            for (int x = 0; x < out.w; ++x)
                out.data[static_cast<size_t>(y) * out.w + x] =
                    static_cast<float>((pixel(x, y) - lo) * scale);
        return out;
    }

    // ---------------------------------------------------------------- Gaussian blur
    // Separable, replicated borders, no clamping in the interior, multi-threaded.
    FloatImage gaussianBlur(const FloatImage &in, float sigma) {
        const int w = in.w, h = in.h;
        const int radius = std::max(1, static_cast<int>(std::ceil(3.0f * sigma)));
        std::vector<float> kernel(2 * radius + 1);
        float sum = 0.0f;
        for (int i = -radius; i <= radius; ++i) {
            const float v = std::exp(-static_cast<float>(i * i) / (2.0f * sigma * sigma));
            kernel[i + radius] = v;
            sum += v;
        }
        for (float &k : kernel) k /= sum;

        FloatImage tmp;
        tmp.w = w; tmp.h = h;
        tmp.data.assign(in.data.size(), 0.0f);
        FloatImage out = tmp;

        // Horizontal pass
        parallelFor(h, [&](int y) {
            const float *src = &in.data[static_cast<size_t>(y) * w];
            float *dst = &tmp.data[static_cast<size_t>(y) * w];
            auto slow = [&](int x) {
                float acc = 0.0f;
                for (int k = -radius; k <= radius; ++k)
                    acc += kernel[k + radius] * src[clampIndex(x + k, 0, w - 1)];
                return acc;
            };
            if (w > 2 * radius) {
                for (int x = 0; x < radius; ++x) dst[x] = slow(x);
                for (int x = radius; x < w - radius; ++x) {
                    float acc = 0.0f;
                    const float *s = src + x - radius;
                    for (int k = 0; k <= 2 * radius; ++k) acc += kernel[k] * s[k];
                    dst[x] = acc;
                }
                for (int x = w - radius; x < w; ++x) dst[x] = slow(x);
            } else {
                for (int x = 0; x < w; ++x) dst[x] = slow(x);
            }
        });

        // Vertical pass: accumulate whole rows (contiguous memory, no per-pixel clamp)
        parallelFor(h, [&](int y) {
            float *dst = &out.data[static_cast<size_t>(y) * w];
            for (int k = -radius; k <= radius; ++k) {
                const float kv = kernel[k + radius];
                const float *row = &tmp.data[static_cast<size_t>(clampIndex(y + k, 0, h - 1)) * w];
                for (int x = 0; x < w; ++x) dst[x] += kv * row[x];
            }
        });
        return out;
    }

    // Gradient magnitude and angle (radians, [0, 2pi), image coordinates, y down),
    // computed ONCE per scale level instead of per keypoint.
    void computeMagAngle(const FloatImage &img, FloatImage &mag, FloatImage &ang) {
        const int w = img.w, h = img.h;
        mag.w = ang.w = w;
        mag.h = ang.h = h;
        mag.data.assign(img.data.size(), 0.0f);
        ang.data.assign(img.data.size(), 0.0f);
        parallelFor(h, [&](int y) {
            const float *up   = &img.data[static_cast<size_t>(clampIndex(y - 1, 0, h - 1)) * w];
            const float *down = &img.data[static_cast<size_t>(clampIndex(y + 1, 0, h - 1)) * w];
            const float *cur  = &img.data[static_cast<size_t>(y) * w];
            float *m = &mag.data[static_cast<size_t>(y) * w];
            float *a = &ang.data[static_cast<size_t>(y) * w];
            for (int x = 0; x < w; ++x) {
                const float gx = 0.5f * (cur[x + 1 < w ? x + 1 : w - 1] - cur[x > 0 ? x - 1 : 0]);
                const float gy = 0.5f * (down[x] - up[x]);
                m[x] = std::sqrt(gx * gx + gy * gy);
                float t = std::atan2(gy, gx);
                if (t < 0.0f) t += kTwoPi;
                if (t >= kTwoPi) t -= kTwoPi;
                a[x] = t;
            }
        });
    }

    // Keypoint scale -> pyramid level. OpenCV convention: sigma = size / 2.
    // Harris (Member 1) currently emits size = 6 -> sigma = 3 -> level 3 (sigma 3.2).
    // If Member 1 ever sets size per detected scale (Harris-Laplace / pyramid), the
    // descriptor follows automatically: sizes are clamped to [1.6, 6.4] and snapped
    // to the nearest of kNumLevels scales (1/3-octave steps).
    int levelFromSize(float size) {
        float sigma = std::isfinite(size) && size > 0.0f ? 0.5f * size : kBaseSigma;
        sigma = std::min(std::max(sigma, kBaseSigma), levelSigma(kNumLevels - 1));
        const int L = static_cast<int>(std::lround(3.0f * std::log2(sigma / kBaseSigma)));
        return clampIndex(L, 0, kNumLevels - 1);
    }

    // ---------------------------------------------------------------- orientation
    std::vector<float> assignOrientations(const FloatImage &mag, const FloatImage &ang,
                                          int px, int py, float sigmaKp) {
        const int radius = static_cast<int>(std::lround(kOriRadiusFact * sigmaKp));
        const float sigma = kOriSigmaFact * sigmaKp;
        const float expDenom = 2.0f * sigma * sigma;

        std::vector<float> hist(kOriBins, 0.0f);
        for (int i = -radius; i <= radius; ++i) {
            const int y = py + i;
            if (y < 0 || y >= mag.h) continue;
            for (int j = -radius; j <= radius; ++j) {
                const int x = px + j;
                if (x < 0 || x >= mag.w) continue;
                const float w = std::exp(-static_cast<float>(i * i + j * j) / expDenom);
                int bin = static_cast<int>(std::lround(ang.at(x, y) * kOriBins / kTwoPi));
                if (bin >= kOriBins) bin -= kOriBins;
                hist[bin] += w * mag.at(x, y);
            }
        }

        // Circular smoothing with [1 4 6 4 1]/16, 6 passes.
        for (int pass = 0; pass < 6; ++pass) {
            std::vector<float> s(kOriBins);
            for (int i = 0; i < kOriBins; ++i) {
                s[i] = (hist[(i - 2 + kOriBins) % kOriBins] + hist[(i + 2) % kOriBins]) * (1.0f / 16.0f) +
                       (hist[(i - 1 + kOriBins) % kOriBins] + hist[(i + 1) % kOriBins]) * (4.0f / 16.0f) +
                       hist[i] * (6.0f / 16.0f);
            }
            hist.swap(s);
        }

        float maxVal = 0.0f;
        for (float v : hist) maxVal = std::max(maxVal, v);
        std::vector<float> angles;
        if (maxVal <= 1e-12f) return angles;

        std::vector<std::pair<float, float>> peaks; // (value, angle in degrees)
        for (int i = 0; i < kOriBins; ++i) {
            const float l = hist[(i - 1 + kOriBins) % kOriBins];
            const float r = hist[(i + 1) % kOriBins];
            const float c = hist[i];
            if (c >= l && c > r && c >= kOriPeakRatio * maxVal) {
                const float denom = l - 2.0f * c + r; // parabolic peak interpolation
                float bin = i + (std::fabs(denom) > 1e-12f ? 0.5f * (l - r) / denom : 0.0f);
                if (bin < 0.0f) bin += kOriBins;
                if (bin >= kOriBins) bin -= kOriBins;
                peaks.emplace_back(c, bin * 360.0f / kOriBins);
            }
        }
        std::sort(peaks.begin(), peaks.end(),
                  [](const std::pair<float, float> &a, const std::pair<float, float> &b) {
                      return a.first > b.first;
                  });
        for (const auto &p : peaks) {
            angles.push_back(p.second >= 360.0f ? p.second - 360.0f : p.second);
            if (!kMultiOrientation) break;
        }
        return angles;
    }

    // ---------------------------------------------------------------- descriptor
    inline int descriptorRadius(float sigmaKp) {
        const float cell = kCellScale * sigmaKp;
        return static_cast<int>(std::lround(cell * std::sqrt(2.0f) * (kDescWidth + 1) * 0.5f));
    }

    // Build the 128-D descriptor. Returns false if the patch has no gradient energy.
    bool computeDescriptor(const FloatImage &mag, const FloatImage &ang, int px, int py,
                           float sigmaKp, float angleDeg, float *out) {
        const float cell = kCellScale * sigmaKp;
        const int radius = descriptorRadius(sigmaKp);
        const float theta = angleDeg * kPi / 180.0f;
        const float cosT = std::cos(theta);
        const float sinT = std::sin(theta);
        const float binsPerRad = kDescBins / kTwoPi;
        const float expDenom = 0.5f * kDescWidth * kDescWidth; // Gaussian sigma = d/2 cells

        const int D = kDescWidth, O = kDescBins;
        std::vector<float> hist((D + 2) * (D + 2) * (O + 2), 0.0f);
        auto H = [&](int r, int c, int o) -> float & {
            return hist[((r * (D + 2)) + c) * (O + 2) + o];
        };

        for (int i = -radius; i <= radius; ++i) {
            const int y = py + i;
            if (y < 0 || y >= mag.h) continue;
            for (int j = -radius; j <= radius; ++j) {
                const int x = px + j;
                if (x < 0 || x >= mag.w) continue;

                const float u = (j * cosT + i * sinT) / cell;
                const float v = (-j * sinT + i * cosT) / cell;
                const float cBin = u + D / 2.0f - 0.5f;
                const float rBin = v + D / 2.0f - 0.5f;
                if (rBin <= -1.0f || rBin >= D || cBin <= -1.0f || cBin >= D) continue;

                float rel = ang.at(x, y) - theta;
                while (rel < 0.0f) rel += kTwoPi;
                while (rel >= kTwoPi) rel -= kTwoPi;
                const float oBin = rel * binsPerRad;

                const float w = std::exp(-(u * u + v * v) / expDenom) * mag.at(x, y);

                const int r0 = static_cast<int>(std::floor(rBin));
                const int c0 = static_cast<int>(std::floor(cBin));
                int o0 = static_cast<int>(std::floor(oBin));
                const float dr = rBin - r0, dc = cBin - c0, dob = oBin - o0;
                if (o0 >= O) o0 -= O;
                if (o0 < 0)  o0 += O;

                for (int a = 0; a <= 1; ++a) {
                    const float wr = w * (a ? dr : 1.0f - dr);
                    for (int b = 0; b <= 1; ++b) {
                        const float wc = wr * (b ? dc : 1.0f - dc);
                        for (int k = 0; k <= 1; ++k)
                            H(r0 + 1 + a, c0 + 1 + b, o0 + k) += wc * (k ? dob : 1.0f - dob);
                    }
                }
            }
        }

        for (int r = 0; r < D; ++r) {
            for (int c = 0; c < D; ++c) {
                H(r + 1, c + 1, 0) += H(r + 1, c + 1, O);
                for (int o = 0; o < O; ++o) out[(r * D + c) * O + o] = H(r + 1, c + 1, o);
            }
        }

        auto l2 = [&](const float *v) {
            float s = 0.0f;
            for (int k = 0; k < kDescLength; ++k) s += v[k] * v[k];
            return std::sqrt(s);
        };
        float n = l2(out);
        if (n < 1e-9f) return false; // flat patch, descriptor is meaningless
        for (int k = 0; k < kDescLength; ++k) out[k] = std::min(out[k] / n, kMagClamp);
        n = l2(out);
        if (n < 1e-9f) return false;
        for (int k = 0; k < kDescLength; ++k) out[k] /= n;
        return true;
    }

    // Result for one input keypoint (written by exactly one thread -> no locking).
    struct Slot {
        float sigma = 0.0f;
        std::vector<float> angles;   // one per descriptor
        std::vector<float> descs;    // angles.size() * 128
    };

} // namespace

namespace FeatureDescription {

    void computeSIFTDescriptors(const ImageData &data, FeatureSet &features) {
        const auto t0 = std::chrono::steady_clock::now();

        features.descriptors = cv::Mat();

        // Prefer the grayscale image; fall back to the color one if needed.
        const cv::Mat &src = !data.grayscaleImage.empty() ? data.grayscaleImage : data.colorImage;
        if (src.empty() || features.keypoints.empty()) {
            features.isEmpty = features.keypoints.empty();
            return;
        }

        const FloatImage gray = toFloatGray(src);
        const int nKp = static_cast<int>(features.keypoints.size());

        // ---- 2) scale per keypoint comes from kp.size (sigma = size / 2) ----
        std::vector<int> level(nKp, -1);
        for (int i = 0; i < nKp; ++i) {
            const cv::KeyPoint &kp = features.keypoints[i];
            if (!std::isfinite(kp.pt.x) || !std::isfinite(kp.pt.y)) continue;
            const int px = static_cast<int>(std::lround(kp.pt.x));
            const int py = static_cast<int>(std::lround(kp.pt.y));
            if (px < 0 || py < 0 || px >= gray.w || py >= gray.h) continue;
            level[i] = levelFromSize(kp.size);
        }

        std::vector<std::vector<int>> byLevel(kNumLevels);
        for (int i = 0; i < nKp; ++i)
            if (level[i] >= 0) byLevel[level[i]].push_back(i);

        // ---- 3+4) one blur / gradient pass per USED level, then describe its keypoints ----
        std::vector<Slot> slots(nKp);
        FloatImage mag, ang;
        for (int L = 0; L < kNumLevels; ++L) {
            const std::vector<int> &group = byLevel[L];
            if (group.empty()) continue;

            const float sigmaKp = levelSigma(L);
            computeMagAngle(gaussianBlur(gray, sigmaKp), mag, ang);

            // Border rule: keypoint >= kMinBorderCells cells away from every edge.
            const int border = static_cast<int>(std::ceil(kMinBorderCells * kCellScale * sigmaKp));

            parallelFor(static_cast<int>(group.size()), [&](int g) {
                const int idx = group[g];
                const cv::KeyPoint &kp = features.keypoints[idx];
                const int px = static_cast<int>(std::lround(kp.pt.x));
                const int py = static_cast<int>(std::lround(kp.pt.y));
                if (px < border || py < border || px >= gray.w - border || py >= gray.h - border) return;

                Slot &slot = slots[idx];
                float desc[kDescLength];
                for (float angle : assignOrientations(mag, ang, px, py, sigmaKp)) {
                    if (!computeDescriptor(mag, ang, px, py, sigmaKp, angle, desc)) continue;
                    slot.angles.push_back(angle);
                    slot.descs.insert(slot.descs.end(), desc, desc + kDescLength);
                }
                slot.sigma = sigmaKp;
            });
        }

        // ---- 5) pack in the ORIGINAL keypoint order (descriptors.row(i) <-> keypoints[i]) ----
        std::vector<cv::KeyPoint> keptKeypoints;
        keptKeypoints.reserve(nKp);
        int total = 0;
        for (const Slot &s : slots) total += static_cast<int>(s.angles.size());

        cv::Mat descriptors;
        if (total > 0) descriptors = cv::Mat(total, kDescLength, CV_32F);

        int row = 0;
        for (int i = 0; i < nKp; ++i) {
            const Slot &s = slots[i];
            for (size_t a = 0; a < s.angles.size(); ++a) {
                cv::KeyPoint k = features.keypoints[i];
                k.angle = s.angles[a];
                k.size  = 2.0f * s.sigma;            // keypoint diameter, OpenCV convention
                keptKeypoints.push_back(k);
                float *dst = descriptors.ptr<float>(row++);
                const float *srcRow = &s.descs[a * kDescLength];
                for (int c = 0; c < kDescLength; ++c) dst[c] = srcRow[c];
            }
        }

        features.descriptors = descriptors;
        features.keypoints = std::move(keptKeypoints);
        features.isEmpty = features.keypoints.empty();

        // Timing: add the description time on top of the detection time already stored in
        // computationTimeMs (re-running Task 2 replaces the previous description time).
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        features.computationTimeMs = features.computationTimeMs - features.descriptionTimeMs + ms;
        features.descriptionTimeMs = ms;
    }

    FeatureSet computeFullSIFT(const ImageData &data) {
        FeatureSet fs;
        fs.keypoints.clear();
        fs.computationTimeMs = 0.0;
        fs.descriptionTimeMs = 0.0;
        fs.isEmpty = true;

        const cv::Mat &src = !data.grayscaleImage.empty() ? data.grayscaleImage : data.colorImage;
        if (src.empty()) return fs;

        const auto t0 = std::chrono::steady_clock::now();

        // Use OpenCV SIFT Detector + Descriptor to find multi-scale DoG extrema & compute descriptors
        cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
        sift->detectAndCompute(src, cv::noArray(), fs.keypoints, fs.descriptors);

        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        fs.computationTimeMs = ms;
        fs.descriptionTimeMs = ms;
        fs.isEmpty = fs.keypoints.empty() || fs.descriptors.empty();

        return fs;
    }
}