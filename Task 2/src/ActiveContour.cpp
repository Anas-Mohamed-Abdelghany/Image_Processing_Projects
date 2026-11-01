#include "ActiveContour.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <utility>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Helpers (file-local)
// ---------------------------------------------------------------------------
namespace {

inline int clampi(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}

double dist(const Point2D& a, const Point2D& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

// Gaussian blur (separable) followed by Sobel. Returns gradient magnitude squared,
// globally normalized to [0,1]. Image is indexed img[y][x]. Borders are clamped.
std::vector<std::vector<double>> computeGradMagSquared(const GrayImage& img) {
    const int h = static_cast<int>(img.size());
    const int w = h > 0 ? static_cast<int>(img[0].size()) : 0;
    std::vector<std::vector<double>> mag2(h, std::vector<double>(w, 0.0));
    if (h == 0 || w == 0) return mag2;

    // --- Gaussian blur to widen the edge's capture range ---
    const double sigma = 2.0;
    const int r = static_cast<int>(std::ceil(3 * sigma));
    std::vector<double> kern(2 * r + 1);
    double ksum = 0.0;
    for (int i = -r; i <= r; ++i) {
        kern[i + r] = std::exp(-(i * i) / (2.0 * sigma * sigma));
        ksum += kern[i + r];
    }
    for (double& k : kern) k /= ksum;

    GrayImage tmp(h, std::vector<double>(w, 0.0));
    GrayImage blur(h, std::vector<double>(w, 0.0));

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            double s = 0.0;
            for (int i = -r; i <= r; ++i)
                s += kern[i + r] * img[y][clampi(x + i, 0, w - 1)];
            tmp[y][x] = s;
        }
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            double s = 0.0;
            for (int i = -r; i <= r; ++i)
                s += kern[i + r] * tmp[clampi(y + i, 0, h - 1)][x];
            blur[y][x] = s;
        }

    // --- Sobel on the blurred image ---
    auto px = [&](int x, int y) {
        return blur[clampi(y, 0, h - 1)][clampi(x, 0, w - 1)];
    };

    double maxV = 0.0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const double gx = -px(x - 1, y - 1) - 2.0 * px(x - 1, y) - px(x - 1, y + 1)
                              + px(x + 1, y - 1) + 2.0 * px(x + 1, y) + px(x + 1, y + 1);
            const double gy = -px(x - 1, y - 1) - 2.0 * px(x, y - 1) - px(x + 1, y - 1)
                              + px(x - 1, y + 1) + 2.0 * px(x, y + 1) + px(x + 1, y + 1);
            mag2[y][x] = gx * gx + gy * gy;
            maxV = std::max(maxV, mag2[y][x]);
        }

    // --- Global normalization to [0,1] ---
    if (maxV > 1e-12)
        for (auto& row : mag2)
            for (double& v : row) v /= maxV;

    return mag2;
}

// Divide valid entries by their maximum (-> [0,1]).
void maxNormalize(double* v, const bool* valid, int n) {
    double mx = 0.0;
    for (int j = 0; j < n; ++j)
        if (valid[j]) mx = std::max(mx, v[j]);
    if (mx > 1e-12)
        for (int j = 0; j < n; ++j)
            if (valid[j]) v[j] /= mx;
}

} // namespace

// ---------------------------------------------------------------------------
std::vector<Point2D> ActiveContour::initCircularContour(Point2D center, double radius, int numPoints) {
    std::vector<Point2D> points;
    if (numPoints <= 0) return points;
    // Initialize equidistant points around circle
    double step = (2.0 * M_PI) / numPoints;
    for (int i = 0; i < numPoints; ++i) {
        double angle = i * step;
        points.push_back({
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        });
    }
    return points;
}

// ---------------------------------------------------------------------------
std::vector<Point2D> ActiveContour::evolveSnake(
    const GrayImage& originalImage,
    const std::vector<Point2D>& initialContour,
    const SnakeConfig& config)
{
    std::vector<Point2D> evolved = initialContour;
    const int n = static_cast<int>(evolved.size());
    const int h = static_cast<int>(originalImage.size());
    const int w = h > 0 ? static_cast<int>(originalImage[0].size()) : 0;
    if (n < 3 || w == 0 || h == 0) return evolved;

    // 1. External image energy: E_ext = -|grad I|^2 (blurred, globally normalized)
    const auto mag2 = computeGradMagSquared(originalImage);

    // 2. Iterate
    for (int iter = 0; iter < config.maxIterations; ++iter) {
        // Average distance between consecutive points (closed contour)
        double avgDist = 0.0;
        for (int i = 0; i < n; ++i)
            avgDist += dist(evolved[i], evolved[(i + 1) % n]);
        avgDist /= n;

        int moved = 0;

        for (int i = 0; i < n; ++i) {
            const Point2D prev = evolved[(i - 1 + n) % n];
            const Point2D next = evolved[(i + 1) % n];
            const Point2D cur  = evolved[i];

            double eCont[9], eCurv[9], eExt[9];
            bool valid[9];
            Point2D cand[9];

            // 3. Search 3x3 neighborhood
            int k = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx, ++k) {
                    cand[k] = { cur.x + dx, cur.y + dy };
                    const int ix = static_cast<int>(std::lround(cand[k].x));
                    const int iy = static_cast<int>(std::lround(cand[k].y));
                    valid[k] = (ix >= 0 && ix < w && iy >= 0 && iy < h);
                    if (!valid[k]) {
                        eCont[k] = eCurv[k] = eExt[k] = 0.0;
                        continue;
                    }

                    // Continuity: deviation from the average spacing
                    const double d = avgDist - dist(cand[k], prev);
                    eCont[k] = d * d;

                    // Curvature: |v(i-1) - 2 v(i) + v(i+1)|^2
                    const double cx = prev.x - 2.0 * cand[k].x + next.x;
                    const double cy = prev.y - 2.0 * cand[k].y + next.y;
                    eCurv[k] = cx * cx + cy * cy;

                    // External: -gradient magnitude squared (already in [-1, 0])
                    eExt[k] = -mag2[iy][ix];
                }
            }

            // Internal terms scaled by the neighborhood maximum
            maxNormalize(eCont, valid, 9);
            maxNormalize(eCurv, valid, 9);

            int best = 4; // index 4 = no movement (dx=0, dy=0)
            double bestE = std::numeric_limits<double>::max();
            for (int j = 0; j < 9; ++j) {
                if (!valid[j]) continue;
                const double E = config.alpha * eCont[j]
                               + config.beta  * eCurv[j]
                               + config.gamma * eExt[j];
                if (E < bestE) { bestE = E; best = j; }
            }

            if (best != 4) {
                evolved[i] = cand[best];
                ++moved;
            }
        }

        // Stop early when the snake has converged (no point moved)
        if (moved == 0) break;
    }

    return evolved;
}

// ---------------------------------------------------------------------------
std::string ActiveContour::computeChainCode(const std::vector<Point2D>& contour) {
    // Freeman 8-direction code, image coordinates (y points down):
    // 0=E, 1=NE, 2=N, 3=NW, 4=W, 5=SW, 6=S, 7=SE
    static const int DX[8] = { 1,  1,  0, -1, -1, -1,  0,  1 };
    static const int DY[8] = { 0, -1, -1, -1,  0,  1,  1,  1 };

    const int n = static_cast<int>(contour.size());
    if (n < 2) return "";

    auto dirOf = [&](int dx, int dy) {
        for (int d = 0; d < 8; ++d)
            if (DX[d] == dx && DY[d] == dy) return d;
        return -1;
    };

    // Round points to pixels
    std::vector<std::pair<int, int>> pts(n);
    for (int i = 0; i < n; ++i)
        pts[i] = { static_cast<int>(std::lround(contour[i].x)),
                   static_cast<int>(std::lround(contour[i].y)) };

    std::string code;

    // Walk each segment with Bresenham (closed contour: last point -> first point)
    for (int i = 0; i < n; ++i) {
        int x0 = pts[i].first,  y0 = pts[i].second;
        const int x1 = pts[(i + 1) % n].first;
        const int y1 = pts[(i + 1) % n].second;

        const int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
        const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (x0 != x1 || y0 != y1) {
            const int px = x0, py = y0;
            const int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
            const int d = dirOf(x0 - px, y0 - py);
            if (d >= 0) code += static_cast<char>('0' + d); // no separators
        }
    }
    return code;
}

// ---------------------------------------------------------------------------
double ActiveContour::computePerimeter(const std::vector<Point2D>& contour) {
    const int n = static_cast<int>(contour.size());
    if (n < 2) return 0.0;
    double p = 0.0;
    for (int i = 0; i < n; ++i)
        p += std::hypot(contour[(i + 1) % n].x - contour[i].x,
                        contour[(i + 1) % n].y - contour[i].y);
    return p; // closed contour: includes last -> first
}

// ---------------------------------------------------------------------------
double ActiveContour::computeArea(const std::vector<Point2D>& contour) {
    const int n = static_cast<int>(contour.size());
    if (n < 3) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        const Point2D& a = contour[i];
        const Point2D& b = contour[(i + 1) % n];
        sum += a.x * b.y - b.x * a.y; // Shoelace / Green's theorem
    }
    return std::abs(sum) * 0.5;
}

// ---------------------------------------------------------------------------
bool ActiveContour::estimateInitialCircle(const GrayImage& img, Point2D& center, double& radius, double margin) {
    const int h = static_cast<int>(img.size());
    const int w = h > 0 ? static_cast<int>(img[0].size()) : 0;
    if (w < 3 || h < 3) return false;

    // Background level = median of the border pixels
    std::vector<double> border;
    border.reserve(2 * (w + h));
    for (int x = 0; x < w; ++x) { border.push_back(img[0][x]); border.push_back(img[h - 1][x]); }
    for (int y = 1; y < h - 1; ++y) { border.push_back(img[y][0]); border.push_back(img[y][w - 1]); }
    std::nth_element(border.begin(), border.begin() + border.size() / 2, border.end());
    const double bg = border[border.size() / 2];

    // Adaptive threshold: 25% of the largest difference from the background
    double maxDiff = 0.0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            maxDiff = std::max(maxDiff, std::abs(img[y][x] - bg));
    if (maxDiff < 10.0) return false; // nearly uniform image, no object found
    const double thr = 0.25 * maxDiff;

    // Bounding box of the object pixels
    int minX = w, maxX = -1, minY = h, maxY = -1;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (std::abs(img[y][x] - bg) > thr) {
                minX = std::min(minX, x); maxX = std::max(maxX, x);
                minY = std::min(minY, y); maxY = std::max(maxY, y);
            }
    if (maxX < 0) return false;

    const double cx = (minX + maxX) / 2.0;
    const double cy = (minY + maxY) / 2.0;

    // Radius = farthest object pixel from the center + margin
    double maxR2 = 0.0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (std::abs(img[y][x] - bg) > thr)
                maxR2 = std::max(maxR2, (x - cx) * (x - cx) + (y - cy) * (y - cy));
    double r = std::sqrt(maxR2) + margin;

    // Keep the whole circle inside the image (points outside are invalid for the snake)
    const double limit = std::min(std::min(cx, cy), std::min((w - 1) - cx, (h - 1) - cy)) - 2.0;
    r = std::min(r, std::max(limit, 5.0));

    center = { cx, cy };
    radius = r;
    return true;
}
