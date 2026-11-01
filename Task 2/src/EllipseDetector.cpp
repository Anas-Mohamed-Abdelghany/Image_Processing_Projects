#include "EllipseDetector.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

struct Candidate {
    int cx, cy, a, b;
    int votes;
    double score; // votes / perimeter, used to rank ellipses of different sizes
};

// Ramanujan approximation of an ellipse perimeter.
double ellipsePerimeter(double a, double b) {
    return M_PI * (3.0 * (a + b) - std::sqrt((3.0 * a + b) * (a + 3.0 * b)));
}

} // namespace

// ============================================================================
// PUBLIC ENTRY: shrink the edge map (speed), search, scale the result back
// ============================================================================
std::vector<EllipseParam> EllipseDetector::detectEllipses(const GrayImage& edgeMap, const Params& params) {
    std::vector<EllipseParam> result;
    if (edgeMap.empty() || edgeMap[0].empty()) return result;

    const int H = static_cast<int>(edgeMap.size());
    const int W = static_cast<int>(edgeMap[0].size());

    // Shrink factor so the longest side is <= workingSize.
    const int s = std::max(1, static_cast<int>(std::ceil(std::max(W, H) / static_cast<double>(std::max(50, params.workingSize)))));

    GrayImage work;
    if (s == 1) {
        work = edgeMap;
    } else {
        // Max-pooling keeps thin edges alive and merges the two sides of thick lines.
        const int w = (W + s - 1) / s, h = (H + s - 1) / s;
        work.assign(h, std::vector<double>(w, 0.0));
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (edgeMap[y][x] >= 255.0) work[y / s][x / s] = 255.0;
    }

    const double maxAorig = params.maxMajorAxis > 0 ? params.maxMajorAxis : W / 2.0;
    const double maxBorig = params.maxMinorAxis > 0 ? params.maxMinorAxis : H / 2.0;
    const int minA = std::max(3, static_cast<int>(std::lround(params.minMajorAxis / s)));
    const int minB = std::max(3, static_cast<int>(std::lround(params.minMinorAxis / s)));
    const int maxA = static_cast<int>(std::lround(maxAorig / s));
    const int maxB = static_cast<int>(std::lround(maxBorig / s));

    std::vector<EllipseParam> found = detectCore(work, params, minA, maxA, minB, maxB);

    // Scale back to the original image.
    std::vector<EllipseParam> scaled;
    for (auto e : found) {
        e.cx = e.cx * s + s / 2.0;
        e.cy = e.cy * s + s / 2.0;
        e.a *= s;
        e.b *= s;
        scaled.push_back(e);
    }
    if (!params.refineFullRes || scaled.empty()) return scaled;

    // ---- Full-resolution re-fit -------------------------------------------
    // Distance from every pixel to the nearest edge pixel (3-4 chamfer, 2 passes).
    const float INF = 1e9f;
    std::vector<float> dist(static_cast<size_t>(W) * H, INF);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (edgeMap[y][x] >= 255.0) dist[static_cast<size_t>(y) * W + x] = 0.f;
    auto D = [&](int x, int y) -> float& { return dist[static_cast<size_t>(y) * W + x]; };
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float d = D(x, y);
            if (x > 0)              d = std::min(d, D(x - 1, y) + 1.f);
            if (y > 0)              d = std::min(d, D(x, y - 1) + 1.f);
            if (x > 0 && y > 0)     d = std::min(d, D(x - 1, y - 1) + 1.4142f);
            if (x < W - 1 && y > 0) d = std::min(d, D(x + 1, y - 1) + 1.4142f);
            D(x, y) = d;
        }
    for (int y = H - 1; y >= 0; --y)
        for (int x = W - 1; x >= 0; --x) {
            float d = D(x, y);
            if (x < W - 1)              d = std::min(d, D(x + 1, y) + 1.f);
            if (y < H - 1)              d = std::min(d, D(x, y + 1) + 1.f);
            if (x < W - 1 && y < H - 1) d = std::min(d, D(x + 1, y + 1) + 1.4142f);
            if (x > 0 && y < H - 1)     d = std::min(d, D(x - 1, y + 1) + 1.4142f);
            D(x, y) = d;
        }

    const int N = 360;
    std::vector<double> sn(N), cs(N);
    for (int i = 0; i < N; ++i) { sn[i] = std::sin(i * 2.0 * M_PI / N); cs[i] = std::cos(i * 2.0 * M_PI / N); }
    const double thrFracFull = std::clamp(params.threshold, 1, 100) / 100.0;
    const float cap = 6.f; // robust: far samples (missing edge parts) cost at most "cap"
    auto cost = [&](double cx, double cy, double a, double b) {
        double sum = 0.0;
        for (int i = 0; i < N; ++i) {
            int x = static_cast<int>(std::lround(cx + a * cs[i]));
            int y = static_cast<int>(std::lround(cy + b * sn[i]));
            if (x < 0 || x >= W || y < 0 || y >= H) { sum += cap; continue; }
            sum += std::min(D(x, y), cap);
        }
        return sum / N;
    };

    std::vector<unsigned char> claimed(static_cast<size_t>(W) * H, 0);
    std::vector<EllipseParam> refinedList;
    for (auto e : scaled) {
        double cx = std::round(e.cx), cy = std::round(e.cy), a = std::round(e.a), b = std::round(e.b);
        double best = cost(cx, cy, a, b);
        for (int step = std::max(1, s); step >= 1; step = (step == 1 ? 0 : std::max(1, step / 2))) {
            bool improved = true;
            int guard = 0;
            while (improved && guard++ < 100) {
                improved = false;
                const double cand[12][4] = {
                    {double(step), 0, 0, 0}, {double(-step), 0, 0, 0}, {0, double(step), 0, 0}, {0, double(-step), 0, 0},
                    {0, 0, double(step), 0}, {0, 0, double(-step), 0}, {0, 0, 0, double(step)}, {0, 0, 0, double(-step)},
                    {0, 0, double(step), double(step)}, {0, 0, double(-step), double(-step)},
                    {0, 0, double(step), double(-step)}, {0, 0, double(-step), double(step)}};
                for (const auto& d : cand) {
                    double na = a + d[2], nb = b + d[3];
                    if (na < 3 || nb < 3) continue;
                    double c = cost(cx + d[0], cy + d[1], na, nb);
                    if (c < best - 1e-9) { best = c; cx += d[0]; cy += d[1]; a = na; b = nb; improved = true; }
                }
            }
        }
        // Reject if the re-fit left the allowed axis range or became a circle.
        if (a < params.minMajorAxis || b < params.minMinorAxis || a > maxAorig || b > maxBorig) continue;
        if (std::abs(a - b) < params.minAxisDiffRatio * std::max(a, b)) continue;

        // The ellipse must follow enough edge pixels that no better ellipse has already explained.
        {
            int onEdge = 0;
            for (int i = 0; i < N; ++i) {
                int x = static_cast<int>(std::lround(cx + a * cs[i]));
                int y = static_cast<int>(std::lround(cy + b * sn[i]));
                bool hit = false;
                for (int dy = -2; dy <= 2 && !hit; ++dy)
                    for (int dx = -2; dx <= 2; ++dx) {
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < W && ny >= 0 && ny < H &&
                            edgeMap[ny][nx] >= 255.0 && !claimed[static_cast<size_t>(ny) * W + nx]) { hit = true; break; }
                    }
                if (hit) ++onEdge;
            }
            if (onEdge < thrFracFull * N) continue;
        }
        // Claim every edge pixel near this ellipse (both sides of a thick stroke).
        for (int i = 0; i < N; ++i) {
            int x = static_cast<int>(std::lround(cx + a * cs[i]));
            int y = static_cast<int>(std::lround(cy + b * sn[i]));
            for (int dy = -7; dy <= 7; ++dy)
                for (int dx = -7; dx <= 7; ++dx) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < W && ny >= 0 && ny < H) claimed[static_cast<size_t>(ny) * W + nx] = 1;
                }
        }
        EllipseParam r;
        r.cx = cx; r.cy = cy; r.a = a; r.b = b; r.angle = 0.0;
        refinedList.push_back(r);
    }
    return refinedList;
}

// ============================================================================
// CORE HOUGH SEARCH
// ============================================================================
std::vector<EllipseParam> EllipseDetector::detectCore(const GrayImage& edgeMap, const Params& params,
                                                      int minA, int maxA, int minB, int maxB) {
    std::vector<EllipseParam> ellipses;
    const int height = static_cast<int>(edgeMap.size());
    const int width  = static_cast<int>(edgeMap[0].size());

    // 1) Gather edge pixels once to speed up the voting loops.
    std::vector<std::pair<int, int>> edgePts; // (x, y)
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (edgeMap[y][x] >= 255.0)
                edgePts.emplace_back(x, y);
    if (edgePts.empty()) return ellipses;

    // Speed guard: only a regular subset of the edge pixels takes part in voting.
    std::vector<std::pair<int, int>> votePts;
    double sampleRatio = 1.0;
    if (params.maxVotingPoints > 0 && static_cast<int>(edgePts.size()) > params.maxVotingPoints) {
        const size_t stride = (edgePts.size() + params.maxVotingPoints - 1) / params.maxVotingPoints;
        for (size_t i = 0; i < edgePts.size(); i += stride) votePts.push_back(edgePts[i]);
        sampleRatio = static_cast<double>(votePts.size()) / edgePts.size();
    } else {
        votePts = edgePts;
    }

    // 2) Pre-compute sin/cos for the sampled angles.
    const int angleStep = std::max(1, params.angleStepDeg);
    const int numAngles = 360 / angleStep;
    std::vector<double> sinT(numAngles), cosT(numAngles);
    for (int t = 0; t < numAngles; ++t) {
        double rad = t * angleStep * M_PI / 180.0;
        sinT[t] = std::sin(rad);
        cosT[t] = std::cos(rad);
    }

    const int axisStep = std::max(1, params.axisStep);
    const double thrFrac = std::clamp(params.threshold, 1, 100) / 100.0;

    std::vector<int> acc(static_cast<size_t>(width) * height, 0);
    std::vector<Candidate> candidates;

    // 3) Loop over the two semi-axes (a: along X, b: along Y).
    for (int a = minA; a <= maxA; a += axisStep) {
        for (int b = minB; b <= maxB; b += axisStep) {

            // Skip (near-)circles.
            if (std::abs(a - b) < params.minAxisDiff) continue;

            std::fill(acc.begin(), acc.end(), 0);

            // 4) Voting: every edge pixel votes for the centre it would imply
            //    if it sat at angle t on an ellipse with axes (a, b).
            for (const auto& pt : votePts) {
                for (int t = 0; t < numAngles; ++t) {
                    int xc = static_cast<int>(std::lround(pt.first  - a * cosT[t]));
                    int yc = static_cast<int>(std::lround(pt.second - b * sinT[t]));
                    if (xc >= 0 && xc < width && yc >= 0 && yc < height)
                        ++acc[static_cast<size_t>(yc) * width + xc];
                }
            }

            // Peaks only PROPOSE candidates (angular sampling and the axis grid
            // smear the votes), so a lower bar is used here; the real threshold
            // is applied in the verification step below.
            const double perim = ellipsePerimeter(a, b);
            const int minVotes = std::max(3, static_cast<int>(std::lround(0.3 * thrFrac * perim * sampleRatio)));

            // 5) Peak finding with non-maximum suppression.
            const int r = params.nmsRadius;
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int v = acc[static_cast<size_t>(y) * width + x];
                    if (v < minVotes) continue;

                    bool isMax = true;
                    for (int dy = -r; dy <= r && isMax; ++dy) {
                        for (int dx = -r; dx <= r; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            int ny = y + dy, nx = x + dx;
                            if (ny < 0 || ny >= height || nx < 0 || nx >= width) continue;
                            if (acc[static_cast<size_t>(ny) * width + nx] > v) { isMax = false; break; }
                        }
                    }
                    if (isMax) candidates.push_back({x, y, a, b, v, v / perim});
                }
            }
        }
    }
    if (candidates.empty()) return ellipses;

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& l, const Candidate& r) { return l.votes > r.votes; });
    if (candidates.size() > 300) candidates.resize(300);

    // 6) Verification helpers. "unclaimed" holds the edge pixels not yet explained
    //    by an already accepted ellipse.
    std::vector<unsigned char> unclaimed(static_cast<size_t>(width) * height, 0);
    for (const auto& pt : edgePts) unclaimed[static_cast<size_t>(pt.second) * width + pt.first] = 1;

    auto hasEdge = [&](int x, int y) {
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                    unclaimed[static_cast<size_t>(ny) * width + nx]) return true;
            }
        return false;
    };

    const int verifySamples = 180;
    std::vector<double> vs(verifySamples), vc(verifySamples);
    for (int i = 0; i < verifySamples; ++i) {
        double rad = i * 2.0 * M_PI / verifySamples;
        vs[i] = std::sin(rad);
        vc[i] = std::cos(rad);
    }
    auto coverage = [&](int cx, int cy, int a, int b) {
        int hits = 0;
        for (int i = 0; i < verifySamples; ++i)
            if (hasEdge(static_cast<int>(std::lround(cx + a * vc[i])),
                        static_cast<int>(std::lround(cy + b * vs[i])))) ++hits;
        return hits;
    };

    // 7) Refinement: the (a, b) grid and the angular sampling are coarse, so
    //    nudge every candidate (+-1 px on centre and axes) towards the edges.
    struct Refined { int cx, cy, a, b, hits; };
    std::vector<Refined> refined;
    for (const auto& c : candidates) {
        Refined cur{c.cx, c.cy, c.a, c.b, coverage(c.cx, c.cy, c.a, c.b)};
        for (int iter = 0; iter < 40; ++iter) {
            Refined best = cur;
            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                    for (int da = -1; da <= 1; ++da)
                        for (int db = -1; db <= 1; ++db) {
                            if (!dx && !dy && !da && !db) continue;
                            int na = cur.a + da, nb = cur.b + db;
                            if (na < minA || na > maxA || nb < minB || nb > maxB) continue;
                            int h = coverage(cur.cx + dx, cur.cy + dy, na, nb);
                            if (h > best.hits) best = {cur.cx + dx, cur.cy + dy, na, nb, h};
                        }
            if (best.hits == cur.hits) break;
            cur = best;
        }
        refined.push_back(cur);
    }
    // Prefer big, well supported ellipses: rank by number of edge pixels covered.
    std::sort(refined.begin(), refined.end(), [](const Refined& l, const Refined& r) {
        double pl = ellipsePerimeter(l.a, l.b) * l.hits, pr = ellipsePerimeter(r.a, r.b) * r.hits;
        return pl > pr;
    });

    // 8) Greedy acceptance: enough of the perimeter must lie on UNCLAIMED edge pixels.
    //    This removes duplicates and the small false ellipses that fit only an arc of a bigger one.
    std::vector<Refined> kept;
    for (const auto& c : refined) {
        // circles are left to the Hough circle detector
        if (std::abs(c.a - c.b) < params.minAxisDiffRatio * std::max(c.a, c.b)) continue;
        if (c.hits < thrFrac * verifySamples) continue;
        // re-check against current unclaimed pixels (earlier ellipses may have taken them)
        if (coverage(c.cx, c.cy, c.a, c.b) < thrFrac * verifySamples) continue;

        for (int i = 0; i < verifySamples; ++i) {
            int x = static_cast<int>(std::lround(c.cx + c.a * vc[i]));
            int y = static_cast<int>(std::lround(c.cy + c.b * vs[i]));
            // radius 2: also swallows the second edge of a thick line (outer + inner side)
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -2; dx <= 2; ++dx) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                        unclaimed[static_cast<size_t>(ny) * width + nx] = 0;
                }
        }
        kept.push_back(c);
        if (params.maxResults > 0 && static_cast<int>(kept.size()) >= params.maxResults) break;
    }

    for (const auto& k : kept) {
        EllipseParam e;
        e.cx = k.cx; e.cy = k.cy;
        e.a = k.a;   e.b = k.b;
        e.angle = 0.0; // axis-aligned ellipses
        ellipses.push_back(e);
    }
    return ellipses;
}
