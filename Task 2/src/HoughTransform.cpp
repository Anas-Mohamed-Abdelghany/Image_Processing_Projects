#include "HoughTransform.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr double PI = 3.14159265358979323846;

struct EdgePoint {
    double x;
    double y;
    double normalX;
    double normalY;
};

struct LineCandidate {
    LineParam line;
    int votes;
};

struct CircleCandidate {
    CircleParam circle;
    int votes;
    double support;
};

bool isRectangularImage(const GrayImage& image)
{
    if (image.empty() || image.front().empty())
        return false;

    const std::size_t width = image.front().size();
    return std::all_of(image.begin(), image.end(), [width](const auto& row) {
        return row.size() == width;
    });
}

bool isEdge(double value)
{
    return value != 0.0;
}

GrayImage thinEdgeMap(const GrayImage& edgeMap)
{
    const int height = static_cast<int>(edgeMap.size());
    const int width = static_cast<int>(edgeMap.front().size());
    GrayImage skeleton(height, std::vector<double>(width, 0.0));

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            skeleton[y][x] = isEdge(edgeMap[y][x]) ? 1.0 : 0.0;

    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int pass = 0; pass < 2; ++pass)
        {
            std::vector<std::pair<int, int>> remove;
            for (int y = 1; y < height - 1; ++y)
            {
                for (int x = 1; x < width - 1; ++x)
                {
                    if (skeleton[y][x] == 0.0)
                        continue;

                    const bool neighbors[8] = {
                        skeleton[y - 1][x] != 0.0,
                        skeleton[y - 1][x + 1] != 0.0,
                        skeleton[y][x + 1] != 0.0,
                        skeleton[y + 1][x + 1] != 0.0,
                        skeleton[y + 1][x] != 0.0,
                        skeleton[y + 1][x - 1] != 0.0,
                        skeleton[y][x - 1] != 0.0,
                        skeleton[y - 1][x - 1] != 0.0
                    };

                    int neighborCount = 0;
                    int transitions = 0;
                    for (int i = 0; i < 8; ++i)
                    {
                        neighborCount += neighbors[i] ? 1 : 0;
                        if (!neighbors[i] && neighbors[(i + 1) % 8])
                            ++transitions;
                    }

                    if (neighborCount < 2 || neighborCount > 6 || transitions != 1)
                        continue;

                    const bool north = neighbors[0];
                    const bool east = neighbors[2];
                    const bool south = neighbors[4];
                    const bool west = neighbors[6];
                    const bool preserveTopology = pass == 0
                        ? !(north && east && south) && !(east && south && west)
                        : !(north && east && west) && !(north && south && west);
                    if (preserveTopology)
                        remove.emplace_back(y, x);
                }
            }

            for (const auto& pixel : remove)
            {
                skeleton[pixel.first][pixel.second] = 0.0;
                changed = true;
            }
        }
    }

    return skeleton;
}

std::vector<EdgePoint> getCircleEdgePoints(
    const GrayImage& grayImage,
    const GrayImage& edgeMap)
{
    const int height = static_cast<int>(grayImage.size());
    const int width = static_cast<int>(grayImage.front().size());
    std::vector<EdgePoint> points;
    const GrayImage skeleton = thinEdgeMap(edgeMap);

    if (height < 3 || width < 3)
        return points;

    GrayImage smoothed(height, std::vector<double>(width, 0.0));
    GrayImage gradientX(height, std::vector<double>(width, 0.0));
    GrayImage gradientY(height, std::vector<double>(width, 0.0));
    GrayImage magnitude(height, std::vector<double>(width, 0.0));

    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            smoothed[y][x] = (
                grayImage[y - 1][x - 1] + 2.0 * grayImage[y - 1][x] + grayImage[y - 1][x + 1]
                + 2.0 * grayImage[y][x - 1] + 4.0 * grayImage[y][x] + 2.0 * grayImage[y][x + 1]
                + grayImage[y + 1][x - 1] + 2.0 * grayImage[y + 1][x] + grayImage[y + 1][x + 1]
            ) / 16.0;
        }
    }

    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            gradientX[y][x] =
                -smoothed[y - 1][x - 1] + smoothed[y - 1][x + 1]
                -2.0 * smoothed[y][x - 1] + 2.0 * smoothed[y][x + 1]
                -smoothed[y + 1][x - 1] + smoothed[y + 1][x + 1];
            gradientY[y][x] =
                -smoothed[y - 1][x - 1] - 2.0 * smoothed[y - 1][x]
                -smoothed[y - 1][x + 1] + smoothed[y + 1][x - 1]
                +2.0 * smoothed[y + 1][x] + smoothed[y + 1][x + 1];
            magnitude[y][x] = std::hypot(gradientX[y][x], gradientY[y][x]);
        }
    }

    std::vector<EdgePoint> edgePoints;
    for (int y = 1; y < height - 1; ++y)
    {
        for (int x = 1; x < width - 1; ++x)
        {
            if (skeleton[y][x] == 0.0)
                continue;

            double normalX = 0.0;
            double normalY = 0.0;
            if (magnitude[y][x] > 8.0)
            {
                normalX = gradientX[y][x] / magnitude[y][x];
                normalY = gradientY[y][x] / magnitude[y][x];
            }
            else
            {
                double meanX = 0.0;
                double meanY = 0.0;
                int count = 0;
                for (int yy = std::max(1, y - 2); yy <= std::min(height - 2, y + 2); ++yy)
                {
                    for (int xx = std::max(1, x - 2); xx <= std::min(width - 2, x + 2); ++xx)
                    {
                        if (skeleton[yy][xx] == 0.0)
                            continue;
                        meanX += xx;
                        meanY += yy;
                        ++count;
                    }
                }
                if (count < 3)
                    continue;

                meanX /= count;
                meanY /= count;
                double covarianceXX = 0.0;
                double covarianceXY = 0.0;
                double covarianceYY = 0.0;
                for (int yy = std::max(1, y - 2); yy <= std::min(height - 2, y + 2); ++yy)
                {
                    for (int xx = std::max(1, x - 2); xx <= std::min(width - 2, x + 2); ++xx)
                    {
                        if (skeleton[yy][xx] == 0.0)
                            continue;
                        const double dx = xx - meanX;
                        const double dy = yy - meanY;
                        covarianceXX += dx * dx;
                        covarianceXY += dx * dy;
                        covarianceYY += dy * dy;
                    }
                }

                if (std::hypot(covarianceXX - covarianceYY, 2.0 * covarianceXY) <
                    0.25 * (covarianceXX + covarianceYY))
                    continue;

                const double tangentAngle = 0.5 * std::atan2(
                    2.0 * covarianceXY,
                    covarianceXX - covarianceYY);
                normalX = -std::sin(tangentAngle);
                normalY = std::cos(tangentAngle);
            }

            points.push_back({static_cast<double>(x), static_cast<double>(y),
                              normalX, normalY});
        }
    }

    return points;
}

bool isLineDuplicate(const LineParam& line, const LineParam& accepted,
                     double rhoTolerance, double thetaTolerance)
{
    const double rawAngleDifference = std::abs(line.theta - accepted.theta);
    const double angleDifference = std::min(rawAngleDifference, PI - rawAngleDifference);
    const double rhoDifference = rawAngleDifference > PI / 2.0
        ? std::abs(line.rho + accepted.rho)
        : std::abs(line.rho - accepted.rho);

    return angleDifference <= thetaTolerance && rhoDifference <= rhoTolerance;
}

double measureCircleSupport(
    const std::vector<EdgePoint>& edgePoints,
    int centerX,
    int centerY,
    int radius)
{
    constexpr int angleBins = 72;
    constexpr double minNormalAlignment = 0.70;

    std::vector<bool> occupiedBins(angleBins, false);
    const double radialTolerance = std::max(1.5, radius * 0.0125);

    for (const EdgePoint& point : edgePoints)
    {
        const double dx = point.x - centerX;
        const double dy = point.y - centerY;
        const double distance = std::hypot(dx, dy);

        if (distance == 0.0 || std::abs(distance - radius) > radialTolerance)
            continue;

        const double radialAlignment =
            std::abs((dx * point.normalX + dy * point.normalY) / distance);
        if (radialAlignment < minNormalAlignment)
            continue;

        double angle = std::atan2(dy, dx);
        if (angle < 0.0)
            angle += 2.0 * PI;

        const int bin = std::min(
            angleBins - 1,
            static_cast<int>(angle * angleBins / (2.0 * PI)));
        occupiedBins[bin] = true;
    }

    return static_cast<double>(
        std::count(occupiedBins.begin(), occupiedBins.end(), true)) / angleBins;
}
}

std::vector<LineParam> HoughTransform::detectLines(
    const GrayImage& edgeMap,
    const LineParams& params)
{
    std::vector<LineParam> lines;
    if (!isRectangularImage(edgeMap) ||
        !std::isfinite(params.rhoRes) || params.rhoRes <= 0.0 ||
        !std::isfinite(params.thetaRes) || params.thetaRes <= 0.0 ||
        params.threshold <= 0)
        return lines;

    const int height = static_cast<int>(edgeMap.size());
    const int width = static_cast<int>(edgeMap.front().size());
    const double diagonal = std::hypot(width - 1.0, height - 1.0);
    const int rhoBins = static_cast<int>(std::floor(2.0 * diagonal / params.rhoRes)) + 1;
    const int thetaBins = std::max(
        1,
        static_cast<int>(std::ceil(PI / params.thetaRes)));

    std::vector<double> cosines(thetaBins);
    std::vector<double> sines(thetaBins);
    for (int thetaIndex = 0; thetaIndex < thetaBins; ++thetaIndex)
    {
        const double theta = thetaIndex * params.thetaRes;
        cosines[thetaIndex] = std::cos(theta);
        sines[thetaIndex] = std::sin(theta);
    }

    std::vector<int> accumulator(
        static_cast<std::size_t>(rhoBins) * thetaBins, 0);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (!isEdge(edgeMap[y][x]))
                continue;

            for (int thetaIndex = 0; thetaIndex < thetaBins; ++thetaIndex)
            {
                const double rho = x * cosines[thetaIndex] + y * sines[thetaIndex];
                const int rhoIndex = static_cast<int>(
                    std::lround((rho + diagonal) / params.rhoRes));
                if (rhoIndex >= 0 && rhoIndex < rhoBins)
                    ++accumulator[static_cast<std::size_t>(rhoIndex) * thetaBins + thetaIndex];
            }
        }
    }

    std::vector<LineCandidate> candidates;
    for (int rhoIndex = 0; rhoIndex < rhoBins; ++rhoIndex)
    {
        for (int thetaIndex = 0; thetaIndex < thetaBins; ++thetaIndex)
        {
            const int votes =
                accumulator[static_cast<std::size_t>(rhoIndex) * thetaBins + thetaIndex];
            if (votes < params.threshold)
                continue;

            bool isMaximum = true;
            for (int rhoOffset = -2; rhoOffset <= 2 && isMaximum; ++rhoOffset)
            {
                for (int thetaOffset = -2; thetaOffset <= 2; ++thetaOffset)
                {
                    if (rhoOffset == 0 && thetaOffset == 0)
                        continue;

                    int neighborRho = rhoIndex + rhoOffset;
                    int neighborTheta = thetaIndex + thetaOffset;
                    bool invertRho = false;

                    while (neighborTheta < 0)
                    {
                        neighborTheta += thetaBins;
                        invertRho = !invertRho;
                    }
                    while (neighborTheta >= thetaBins)
                    {
                        neighborTheta -= thetaBins;
                        invertRho = !invertRho;
                    }

                    if (invertRho)
                    {
                        const double rho = neighborRho * params.rhoRes - diagonal;
                        neighborRho = static_cast<int>(
                            std::lround((-rho + diagonal) / params.rhoRes));
                    }

                    if (neighborRho < 0 || neighborRho >= rhoBins)
                        continue;

                    const int neighborVotes =
                        accumulator[static_cast<std::size_t>(neighborRho) * thetaBins + neighborTheta];
                    if (neighborVotes > votes ||
                        (neighborVotes == votes &&
                         (neighborRho * thetaBins + neighborTheta <
                          rhoIndex * thetaBins + thetaIndex)))
                    {
                        isMaximum = false;
                        break;
                    }
                }
            }

            if (isMaximum)
            {
                candidates.push_back({
                    {rhoIndex * params.rhoRes - diagonal,
                     thetaIndex * params.thetaRes},
                    votes
                });
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(),
        [](const LineCandidate& first, const LineCandidate& second) {
            return first.votes > second.votes;
        });

    const double rhoTolerance = std::max(2.0, 4.0 * params.rhoRes);
    const double thetaTolerance = std::max(2.0 * params.thetaRes, PI / 180.0);
    for (const LineCandidate& candidate : candidates)
    {
        const bool duplicate = std::any_of(lines.begin(), lines.end(),
            [&](const LineParam& accepted) {
                return isLineDuplicate(candidate.line, accepted, rhoTolerance, thetaTolerance);
            });
        if (!duplicate)
            lines.push_back(candidate.line);
    }

    return lines;
}

std::vector<CircleParam> HoughTransform::detectCircles(
    const GrayImage& grayImage,
    const GrayImage& edgeMap,
    const CircleParams& params)
{
    std::vector<CircleParam> circles;
    if (!isRectangularImage(grayImage) || !isRectangularImage(edgeMap) ||
        grayImage.size() != edgeMap.size() ||
        grayImage.front().size() != edgeMap.front().size() ||
        params.minRadius < 1 || params.maxRadius < params.minRadius)
        return circles;

    const int height = static_cast<int>(grayImage.size());
    const int width = static_cast<int>(grayImage.front().size());
    const int maxRadius = std::min(
        params.maxRadius,
        static_cast<int>(std::hypot(width, height) / 2.0));
    if (params.minRadius > maxRadius)
        return circles;

    const double minimumSupport = std::clamp(params.threshold, 1, 100) / 100.0;
    const std::vector<EdgePoint> edgePoints = getCircleEdgePoints(grayImage, edgeMap);
    if (edgePoints.empty())
        return circles;

    std::vector<CircleCandidate> candidates;
    for (int radius = params.minRadius; radius <= maxRadius; ++radius)
    {
        std::vector<int> accumulator(static_cast<std::size_t>(width) * height, 0);
        for (const EdgePoint& point : edgePoints)
        {
            for (const int direction : {-1, 1})
            {
                const int centerX = static_cast<int>(std::lround(
                    point.x + direction * radius * point.normalX));
                const int centerY = static_cast<int>(std::lround(
                    point.y + direction * radius * point.normalY));

                if (centerX >= 0 && centerX < width &&
                    centerY >= 0 && centerY < height)
                {
                    ++accumulator[static_cast<std::size_t>(centerY) * width + centerX];
                }
            }
        }

        const int minimumVotes = std::max(1, params.minVotes);
        const int centerNeighborhood = std::max(2, std::min(5, radius / 12));

        for (int centerY = 0; centerY < height; ++centerY)
        {
            for (int centerX = 0; centerX < width; ++centerX)
            {
                const int votes =
                    accumulator[static_cast<std::size_t>(centerY) * width + centerX];
                if (votes < minimumVotes)
                    continue;

                bool isMaximum = true;
                for (int dy = -centerNeighborhood;
                     dy <= centerNeighborhood && isMaximum;
                     ++dy)
                {
                    for (int dx = -centerNeighborhood; dx <= centerNeighborhood; ++dx)
                    {
                        if (dx == 0 && dy == 0)
                            continue;

                        const int neighborX = centerX + dx;
                        const int neighborY = centerY + dy;
                        if (neighborX < 0 || neighborX >= width ||
                            neighborY < 0 || neighborY >= height)
                            continue;

                        const int neighborVotes =
                            accumulator[static_cast<std::size_t>(neighborY) * width + neighborX];
                        if (neighborVotes > votes ||
                            (neighborVotes == votes &&
                             (neighborY < centerY ||
                              (neighborY == centerY && neighborX < centerX))))
                        {
                            isMaximum = false;
                            break;
                        }
                    }
                }

                if (!isMaximum)
                    continue;

                const double support =
                    measureCircleSupport(edgePoints, centerX, centerY, radius);
                if (support >= minimumSupport)
                {
                    candidates.push_back({
                        {static_cast<double>(centerX),
                         static_cast<double>(centerY),
                         static_cast<double>(radius)},
                        votes,
                        support
                    });
                }
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(),
        [](const CircleCandidate& first, const CircleCandidate& second) {
            if (first.votes != second.votes)
                return first.votes > second.votes;
            return first.support > second.support;
        });

    for (const CircleCandidate& candidate : candidates)
    {
        const bool duplicate = std::any_of(circles.begin(), circles.end(),
            [&](const CircleParam& accepted) {
                const double centerDistance = std::hypot(
                    candidate.circle.cx - accepted.cx,
                    candidate.circle.cy - accepted.cy);
                const double referenceRadius =
                    std::min(candidate.circle.radius, accepted.radius);
                const double centerTolerance = std::max(1.5, referenceRadius * 0.02);
                const double radiusTolerance = std::max(1.0, referenceRadius * 0.01);
                return centerDistance <= centerTolerance &&
                    std::abs(candidate.circle.radius - accepted.radius) <= radiusTolerance;
            });

        if (!duplicate)
            circles.push_back(candidate.circle);
    }

    return circles;
}
