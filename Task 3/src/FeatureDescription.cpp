#include "FeatureLogic.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <utility>
#include <vector>
#include <opencv2/core.hpp>

// =========================================================
// SIFT implemented FROM SCRATCH (Lowe 2004).
// No cv::SIFT / cv::xfeatures2d, no cv::GaussianBlur, no cv::Sobel,
// no cv::normalize ... cv::Mat is only used as input image / output container.
//
// computeSIFT() does everything itself, no external keypoints needed:
//   1. gray float image, 2x upsample, blur to sigma 1.6
//   2. Gaussian pyramid (3 scales per octave) + Difference-of-Gaussians
//   3. scale-space extrema (26 neighbours)
//   4. sub-pixel / sub-scale refinement, low-contrast + edge rejection
//   5. orientation assignment (36-bin histogram, multiple peaks)
//   6. 4x4x8 = 128-D descriptor, L2 -> clamp 0.2 -> L2
//
// Output: fs.descriptors is CV_32F, row i belongs to fs.keypoints[i].
// keypoint.size = 2*sigma (original image pixels), keypoint.angle set.
//
// Define FD_NO_THREADS if your MinGW uses the "win32" thread model (no std::thread).
// =========================================================

namespace {

    constexpr float kPi            = 3.14159265358979323846f;
    constexpr float kTwoPi         = 2.0f * kPi;
    constexpr float kBaseSigma     = 1.6f;   // smallest scale
    constexpr int   kOriBins       = 36;
    constexpr float kOriSigmaFact  = 1.5f;   // orientation window sigma = 1.5 * keypoint sigma
    constexpr float kOriRadiusFact = 3.0f * kOriSigmaFact;
    constexpr float kOriPeakRatio  = 0.8f;   // secondary peaks, only used if kMultiOrientation
    constexpr int   kOriSmoothPasses = 1;    // 1 = same as OpenCV (Lowe's original used 6)
    constexpr int   kDescWidth     = 4;      // 4x4 spatial cells
    constexpr int   kDescBins      = 8;      // 8 orientation bins per cell
    constexpr int   kDescLength    = kDescWidth * kDescWidth * kDescBins; // 128
    constexpr float kCellScale     = 3.0f;   // cell width = 3 * sigma
    constexpr float kMagClamp      = 0.2f;   // illumination-saturation clamp

    // true: scale values like OpenCV (x512, saturate to 255) so absolute distance
    // thresholds match cv::SIFT. false: values stay in [0, 0.2] (unit L2 norm).
    constexpr bool  kOpenCVScale = false;

    inline int clampIndex(int v, int lo, int hi) {
        return v < lo ? lo : (v > hi ? hi : v);
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
    // Separable, replicated borders, multi-threaded.
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

    // ---------------------------------------------------------------- orientation
    // (cx, cy) is the exact sub-pixel keypoint position.
    std::vector<float> assignOrientations(const FloatImage &mag, const FloatImage &ang,
                                          float cx, float cy, float sigmaKp, bool multi) {
        const int px = static_cast<int>(std::lround(cx));
        const int py = static_cast<int>(std::lround(cy));
        const int radius = static_cast<int>(std::lround(kOriRadiusFact * sigmaKp));
        const float sigma = kOriSigmaFact * sigmaKp;
        const float expDenom = 2.0f * sigma * sigma;

        std::vector<float> hist(kOriBins, 0.0f);
        for (int i = -radius; i <= radius; ++i) {
            const int y = py + i;
            if (y < 0 || y >= mag.h) continue;
            const float dy = static_cast<float>(y) - cy;
            for (int j = -radius; j <= radius; ++j) {
                const int x = px + j;
                if (x < 0 || x >= mag.w) continue;
                const float dx = static_cast<float>(x) - cx;
                const float w = std::exp(-(dx * dx + dy * dy) / expDenom);
                int bin = static_cast<int>(std::lround(ang.at(x, y) * kOriBins / kTwoPi));
                if (bin >= kOriBins) bin -= kOriBins;
                hist[bin] += w * mag.at(x, y);
            }
        }

        // Circular smoothing with [1 4 6 4 1]/16.
        for (int pass = 0; pass < kOriSmoothPasses; ++pass) {
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
            if (!multi) break;
        }
        return angles;
    }

    // ---------------------------------------------------------------- descriptor
    inline int descriptorRadius(float sigmaKp) {
        const float cell = kCellScale * sigmaKp;
        return static_cast<int>(std::lround(cell * std::sqrt(2.0f) * (kDescWidth + 1) * 0.5f));
    }

    // Build the 128-D descriptor around the exact position (cx, cy).
    // Returns false if the patch has no gradient energy.
    bool computeDescriptor(const FloatImage &mag, const FloatImage &ang, float cx, float cy,
                           float sigmaKp, float angleDeg, float *out) {
        const int px = static_cast<int>(std::lround(cx));
        const int py = static_cast<int>(std::lround(cy));
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
            const float dy = static_cast<float>(y) - cy;
            for (int j = -radius; j <= radius; ++j) {
                const int x = px + j;
                if (x < 0 || x >= mag.w) continue;
                const float dx = static_cast<float>(x) - cx;

                const float u = (dx * cosT + dy * sinT) / cell;
                const float v = (-dx * sinT + dy * cosT) / cell;
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

        if (kOpenCVScale) {
            for (int k = 0; k < kDescLength; ++k)
                out[k] = std::min(255.0f, std::round(out[k] * 512.0f));
        }
        return true;
    }

    // =====================================================================
    // FULL SIFT (DoG detector) FROM SCRATCH
    // =====================================================================
    constexpr int   kSiftLayers      = 3;       // scales per octave (S)
    constexpr float kSiftSigma0      = 1.6f;
    constexpr float kSiftInitBlur    = 0.5f;    // assumed blur of the input image
    constexpr bool  kSiftUpsample    = true;    // double the image first (Lowe / OpenCV default)
    constexpr float kSiftContrastThr = 0.04f;   // on a 0..1 intensity scale
    constexpr float kSiftEdgeThr     = 10.0f;   // r in Lowe's edge test
    constexpr int   kSiftBorder      = 5;
    constexpr int   kSiftMaxIters    = 5;

    FloatImage upsample2x(const FloatImage &in) {
        FloatImage out;
        out.w = in.w * 2; out.h = in.h * 2;
        out.data.assign(static_cast<size_t>(out.w) * out.h, 0.0f);
        parallelFor(out.h, [&](int y) {
            const float sy = (y + 0.5f) * 0.5f - 0.5f;
            const int y0 = static_cast<int>(std::floor(sy));
            const float fy = sy - y0;
            const int ya = clampIndex(y0, 0, in.h - 1), yb = clampIndex(y0 + 1, 0, in.h - 1);
            for (int x = 0; x < out.w; ++x) {
                const float sx = (x + 0.5f) * 0.5f - 0.5f;
                const int x0 = static_cast<int>(std::floor(sx));
                const float fx = sx - x0;
                const int xa = clampIndex(x0, 0, in.w - 1), xb = clampIndex(x0 + 1, 0, in.w - 1);
                const float top = in.at(xa, ya) * (1.0f - fx) + in.at(xb, ya) * fx;
                const float bot = in.at(xa, yb) * (1.0f - fx) + in.at(xb, yb) * fx;
                out.data[static_cast<size_t>(y) * out.w + x] = top * (1.0f - fy) + bot * fy;
            }
        });
        return out;
    }

    FloatImage downsample2x(const FloatImage &in) {
        FloatImage out;
        out.w = in.w / 2; out.h = in.h / 2;
        out.data.assign(static_cast<size_t>(out.w) * out.h, 0.0f);
        for (int y = 0; y < out.h; ++y)
            for (int x = 0; x < out.w; ++x)
                out.data[static_cast<size_t>(y) * out.w + x] = in.at(2 * x, 2 * y);
        return out;
    }

    FloatImage subtract(const FloatImage &a, const FloatImage &b) { // a - b
        FloatImage out;
        out.w = a.w; out.h = a.h;
        out.data.resize(a.data.size());
        for (size_t i = 0; i < a.data.size(); ++i) out.data[i] = a.data[i] - b.data[i];
        return out;
    }

    struct Candidate { int layer = 0, x = 0, y = 0; };
    struct Detection { int layer = 0; float x = 0, y = 0, xi = 0, response = 0; };

    // 26-neighbour scale-space extrema of the DoG layers 1..S.
    std::vector<Candidate> findExtrema(const std::vector<FloatImage> &dog) {
        std::vector<Candidate> all;
        const int w = dog[0].w, h = dog[0].h;
        const int rowsN = h - 2 * kSiftBorder;
        if (rowsN <= 0 || w <= 2 * kSiftBorder) return all;
        const float thr = 0.5f * kSiftContrastThr / kSiftLayers * 255.0f;

        for (int l = 1; l <= kSiftLayers; ++l) {
            const FloatImage *img[3] = {&dog[l - 1], &dog[l], &dog[l + 1]};
            std::vector<std::vector<Candidate>> rows(h);
            parallelFor(rowsN, [&](int r) {
                const int y = r + kSiftBorder;
                for (int x = kSiftBorder; x < w - kSiftBorder; ++x) {
                    const float v = img[1]->at(x, y);
                    if (std::fabs(v) <= thr) continue;
                    bool ok = true;
                    for (int a = 0; a < 3 && ok; ++a)
                        for (int dy = -1; dy <= 1 && ok; ++dy)
                            for (int dx = -1; dx <= 1; ++dx) {
                                if (a == 1 && dy == 0 && dx == 0) continue;
                                const float n = img[a]->at(x + dx, y + dy);
                                if (v > 0.0f ? n > v : n < v) { ok = false; break; }
                            }
                    if (ok) rows[y].push_back({l, x, y});
                }
            });
            for (auto &r : rows) all.insert(all.end(), r.begin(), r.end());
        }
        return all;
    }

    // Sub-pixel / sub-scale refinement (3x3 quadratic fit), low-contrast and edge rejection.
    bool refineExtremum(const std::vector<FloatImage> &dog, int layer, int x, int y, Detection &out) {
        const int w = dog[0].w, h = dog[0].h;
        float xc = 0, xr = 0, xi = 0, gx = 0, gy = 0, gs = 0, dxx = 0, dyy = 0, dxy = 0;
        bool converged = false;

        for (int it = 0; it < kSiftMaxIters; ++it) {
            const FloatImage &prev = dog[layer - 1], &cur = dog[layer], &next = dog[layer + 1];
            gx = (cur.at(x + 1, y) - cur.at(x - 1, y)) * 0.5f;
            gy = (cur.at(x, y + 1) - cur.at(x, y - 1)) * 0.5f;
            gs = (next.at(x, y) - prev.at(x, y)) * 0.5f;
            const float v2 = 2.0f * cur.at(x, y);
            dxx = cur.at(x + 1, y) + cur.at(x - 1, y) - v2;
            dyy = cur.at(x, y + 1) + cur.at(x, y - 1) - v2;
            const float dss = next.at(x, y) + prev.at(x, y) - v2;
            dxy = (cur.at(x + 1, y + 1) - cur.at(x - 1, y + 1) - cur.at(x + 1, y - 1) + cur.at(x - 1, y - 1)) * 0.25f;
            const float dxs = (next.at(x + 1, y) - next.at(x - 1, y) - prev.at(x + 1, y) + prev.at(x - 1, y)) * 0.25f;
            const float dys = (next.at(x, y + 1) - next.at(x, y - 1) - prev.at(x, y + 1) + prev.at(x, y - 1)) * 0.25f;

            // Solve H * t = -g for the symmetric 3x3 Hessian via cofactors.
            const float c00 = dyy * dss - dys * dys;
            const float c01 = dxs * dys - dxy * dss;
            const float c02 = dxy * dys - dyy * dxs;
            const float c11 = dxx * dss - dxs * dxs;
            const float c12 = dxy * dxs - dxx * dys;
            const float c22 = dxx * dyy - dxy * dxy;
            const float det = dxx * c00 + dxy * c01 + dxs * c02;
            if (std::fabs(det) < 1e-12f) return false;
            const float b0 = -gx, b1 = -gy, b2 = -gs;
            xc = (c00 * b0 + c01 * b1 + c02 * b2) / det;
            xr = (c01 * b0 + c11 * b1 + c12 * b2) / det;
            xi = (c02 * b0 + c12 * b1 + c22 * b2) / det;
            if (!std::isfinite(xc) || !std::isfinite(xr) || !std::isfinite(xi)) return false;

            if (std::fabs(xc) < 0.5f && std::fabs(xr) < 0.5f && std::fabs(xi) < 0.5f) {
                converged = true;
                break;
            }
            if (std::fabs(xc) > 1e4f || std::fabs(xr) > 1e4f || std::fabs(xi) > 1e4f) return false;

            x += static_cast<int>(std::lround(xc));
            y += static_cast<int>(std::lround(xr));
            layer += static_cast<int>(std::lround(xi));
            if (layer < 1 || layer > kSiftLayers || x < kSiftBorder || x >= w - kSiftBorder ||
                y < kSiftBorder || y >= h - kSiftBorder)
                return false;
        }
        if (!converged) return false;

        // Low-contrast rejection (values are on a 0..255 scale).
        const float contr = dog[layer].at(x, y) + 0.5f * (gx * xc + gy * xr + gs * xi);
        if (std::fabs(contr) * kSiftLayers < kSiftContrastThr * 255.0f) return false;

        // Edge rejection: principal-curvature ratio via the 2x2 spatial Hessian.
        const float tr = dxx + dyy;
        const float det2 = dxx * dyy - dxy * dxy;
        if (det2 <= 0.0f || tr * tr * kSiftEdgeThr >= (kSiftEdgeThr + 1.0f) * (kSiftEdgeThr + 1.0f) * det2)
            return false;

        out.layer = layer;
        out.x = x + xc;
        out.y = y + xr;
        out.xi = xi;
        out.response = std::fabs(contr);
        return true;
    }

    struct SiftOut {
        float sigma = 0.0f;
        std::vector<float> angles;
        std::vector<float> descs;
    };

} // namespace

namespace FeatureDescription {

    // FULL SIFT FROM SCRATCH: DoG detector + descriptor. No OpenCV algorithms used.
    FeatureSet computeSIFT(const ImageData &data) {
        FeatureSet fs;
        fs.keypoints.clear();
        fs.computationTimeMs = 0.0;
        fs.descriptionTimeMs = 0.0;
        fs.isEmpty = true;

        const cv::Mat &src = !data.grayscaleImage.empty() ? data.grayscaleImage : data.colorImage;
        if (src.empty()) return fs;

        const auto t0 = std::chrono::steady_clock::now();
        const int S = kSiftLayers;

        // ---- base image: optional 2x upsample, then blur up to sigma0 ----
        FloatImage base = toFloatGray(src);
        float assumedBlur = kSiftInitBlur;
        if (kSiftUpsample) { base = upsample2x(base); assumedBlur *= 2.0f; }
        const float sigDiff = std::sqrt(std::max(kSiftSigma0 * kSiftSigma0 - assumedBlur * assumedBlur, 0.01f));
        FloatImage cur = gaussianBlur(base, sigDiff);

        // Incremental blur between consecutive Gaussian layers of an octave.
        std::vector<float> incSigma(S + 3, 0.0f);
        {
            const float k = std::pow(2.0f, 1.0f / S);
            float prev = kSiftSigma0;
            for (int i = 1; i < S + 3; ++i) {
                const float total = prev * k;
                incSigma[i] = std::sqrt(total * total - prev * prev);
                prev = total;
            }
        }

        std::vector<float> allDescs;
        double descMs = 0.0;

        for (int o = 0; std::min(cur.w, cur.h) >= 16; ++o) {
            // ---- Gaussian pyramid + DoG for this octave ----
            std::vector<FloatImage> gauss(S + 3), dog(S + 2);
            gauss[0] = cur;
            for (int i = 1; i < S + 3; ++i) gauss[i] = gaussianBlur(gauss[i - 1], incSigma[i]);
            for (int i = 0; i < S + 2; ++i) dog[i] = subtract(gauss[i + 1], gauss[i]);

            // ---- detection: extrema -> refinement -> contrast/edge rejection ----
            const std::vector<Candidate> cands = findExtrema(dog);
            const int nC = static_cast<int>(cands.size());
            std::vector<Detection> det(nC);
            std::vector<char> valid(nC, 0);
            parallelFor(nC, [&](int i) {
                valid[i] = refineExtremum(dog, cands[i].layer, cands[i].x, cands[i].y, det[i]) ? 1 : 0;
            });

            // ---- description: orientation + 128-D descriptor on the keypoint's own layer ----
            const auto d0 = std::chrono::steady_clock::now();
            std::vector<std::vector<int>> byLayer(S + 2);
            for (int i = 0; i < nC; ++i)
                if (valid[i]) byLayer[det[i].layer].push_back(i);

            std::vector<SiftOut> outs(nC);
            FloatImage mag, ang;
            for (int l = 1; l <= S; ++l) {
                const std::vector<int> &group = byLayer[l];
                if (group.empty()) continue;
                computeMagAngle(gauss[l], mag, ang);
                parallelFor(static_cast<int>(group.size()), [&](int g) {
                    const int idx = group[g];
                    const Detection &d = det[idx];
                    const float sigmaOct = kSiftSigma0 *
                        std::pow(2.0f, (static_cast<float>(d.layer) + d.xi) / static_cast<float>(S));
                    SiftOut &so = outs[idx];
                    so.sigma = sigmaOct;
                    float desc[kDescLength];
                    for (float angle : assignOrientations(mag, ang, d.x, d.y, sigmaOct, true)) {
                        if (!computeDescriptor(mag, ang, d.x, d.y, sigmaOct, angle, desc)) continue;
                        so.angles.push_back(angle);
                        so.descs.insert(so.descs.end(), desc, desc + kDescLength);
                    }
                });
            }
            const auto d1 = std::chrono::steady_clock::now();
            descMs += std::chrono::duration<double, std::milli>(d1 - d0).count();

            // ---- pack keypoints in original-image coordinates ----
            const float octScale = static_cast<float>(1 << o);
            for (int i = 0; i < nC; ++i) {
                if (!valid[i]) continue;
                const SiftOut &so = outs[i];
                for (size_t a = 0; a < so.angles.size(); ++a) {
                    const float xb = det[i].x * octScale;
                    const float yb = det[i].y * octScale;
                    cv::KeyPoint k;
                    k.pt.x = kSiftUpsample ? (xb + 0.5f) * 0.5f - 0.5f : xb;
                    k.pt.y = kSiftUpsample ? (yb + 0.5f) * 0.5f - 0.5f : yb;
                    k.size = 2.0f * so.sigma * octScale * (kSiftUpsample ? 0.5f : 1.0f);
                    k.angle = so.angles[a];
                    k.response = det[i].response;
                    k.octave = o;
                    fs.keypoints.push_back(k);
                    allDescs.insert(allDescs.end(), so.descs.begin() + a * kDescLength,
                                    so.descs.begin() + (a + 1) * kDescLength);
                }
            }

            cur = downsample2x(gauss[S]); // gauss[S] has sigma = 2*sigma0 -> base of the next octave
        }

        const int total = static_cast<int>(fs.keypoints.size());
        if (total > 0) {
            fs.descriptors = cv::Mat(total, kDescLength, CV_32F);
            for (int r = 0; r < total; ++r) {
                float *dst = fs.descriptors.ptr<float>(r);
                const float *s = &allDescs[static_cast<size_t>(r) * kDescLength];
                for (int c = 0; c < kDescLength; ++c) dst[c] = s[c];
            }
        }

        const auto t1 = std::chrono::steady_clock::now();
        fs.computationTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        fs.descriptionTimeMs = descMs;
        fs.isEmpty = fs.keypoints.empty() || fs.descriptors.empty();
        return fs;
    }
}