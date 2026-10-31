#include "ActiveContour.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Helpers (file-local)
// ---------------------------------------------------------------------------
namespace {

// Sobel gradient magnitude squared. Image is indexed img[y][x]. Borders are clamped.
std::vector<std::vector<double>> computeGradMagSquared(const GrayImage& img) {
    const int h = static_cast<int>(img.size());
    const int w = h > 0 ? static_cast<int>(img[0].size()) : 0;
    std::vector<std::vector<double>> mag2(h, std::vector<double>(w, 0.0));

    auto px = [&](int x, int y) {
        x = std::max(0, std::min(w - 1, x));
        y = std::max(0, std::min(h - 1, y));
        return img[y][x];
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double gx = -px(x - 1, y - 1) - 2.0 * px(x - 1, y) - px(x - 1, y + 1)
                        + px(x + 1, y - 1) + 2.0 * px(x + 1, y) + px(x + 1, y + 1);
            double gy = -px(x - 1, y - 1) - 2.0 * px(x, y - 1) - px(x + 1, y - 1)
                        + px(x - 1, y + 1) + 2.0 * px(x, y + 1) + px(x + 1, y + 1);
            mag2[y][x] = gx * gx + gy * gy;
        }
    }
    return mag2;
}

double dist(const Point2D& a, const Point2D& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

// Normalize values in-place to [0,1] over the valid entries: (v - min) / (max - min).
// If all valid values are equal, they become 0.
void minMaxNormalize(double* v, const bool* valid, int n) {
    double mn = std::numeric_limits<double>::max();
    double mx = std::numeric_limits<double>::lowest();
    for (int i = 0; i < n; ++i) {
        if (!valid[i]) continue;
        mn = std::min(mn, v[i]);
        mx = std::max(mx, v[i]);
    }
    const double range = mx - mn;
    for (int i = 0; i < n; ++i) {
        if (!valid[i]) continue;
        v[i] = (range > 1e-12) ? (v[i] - mn) / range : 0.0;
    }
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

    // 1. External image energy: E_ext = -|grad I|^2
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
                    if (!valid[k]) { eCont[k] = eCurv[k] = eExt[k] = 0.0; continue; }

                    // Continuity: deviation from the average spacing
                    const double d = avgDist - dist(cand[k], prev);
                    eCont[k] = d * d;

                    // Curvature: |v(i-1) - 2 v(i) + v(i+1)|^2
                    const double cx = prev.x - 2.0 * cand[k].x + next.x;
                    const double cy = prev.y - 2.0 * cand[k].y + next.y;
                    eCurv[k] = cx * cx + cy * cy;

                    // External: -gradient magnitude squared
                    eExt[k] = -mag2[iy][ix];
                }
            }

            // Normalize each term to [0,1] inside the neighborhood so the weights are comparable
            minMaxNormalize(eCont, valid, 9);
            minMaxNormalize(eCurv, valid, 9);
            minMaxNormalize(eExt,  valid, 9);

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
        int x0 = pts[i].first,            y0 = pts[i].second;
        const int x1 = pts[(i + 1) % n].first, y1 = pts[(i + 1) % n].second;

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