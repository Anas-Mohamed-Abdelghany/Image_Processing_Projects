#include "ActiveContour.h"
#include <cmath>

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

std::vector<Point2D> ActiveContour::evolveSnake(
    const GrayImage& originalImage,
    const std::vector<Point2D>& initialContour,
    const SnakeConfig& config) 
{
    std::vector<Point2D> evolved = initialContour;
    // TODO [Member 4]: Greedy snake optimization loop
    // 1. Compute external image energy (e.g. -magnitude^2)
    // 2. Iterate up to maxIterations
    // 3. For each point, search 3x3 neighborhood minimizing: E = alpha*E_cont + beta*E_curv + gamma*E_ext
    return evolved;
}

std::string ActiveContour::computeChainCode(const std::vector<Point2D>& contour) {
    // TODO [Member 4]: Discretize contour and build Freeman 8-direction chain code string
    return "Not implemented yet";
}

double ActiveContour::computePerimeter(const std::vector<Point2D>& contour) {
    // TODO [Member 4]: Sum Euclidean distances between consecutive points
    return 0.0;
}

double ActiveContour::computeArea(const std::vector<Point2D>& contour) {
    // TODO [Member 4]: Compute polygon area via Shoelace formula / Green's theorem
    return 0.0;
}