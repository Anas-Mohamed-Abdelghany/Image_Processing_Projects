#include "HoughTransform.h"

std::vector<LineParam> HoughTransform::detectLines(const GrayImage& edgeMap, const LineParams& params) {
    std::vector<LineParam> lines;
    // TODO [Member 2]: Build (rho, theta) accumulator, vote on edge pixels, detect peaks
    return lines;
}

std::vector<CircleParam> HoughTransform::detectCircles(const GrayImage& edgeMap, const CircleParams& params) {
    std::vector<CircleParam> circles;
    // TODO [Member 2]: Build (cx, cy, r) accumulator, vote and extract circle parameters
    return circles;
}