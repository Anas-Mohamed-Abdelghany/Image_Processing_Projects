#pragma once
#include "Types.h"
#include <vector>

class HoughTransform {
public:
    struct LineParams {
        double rhoRes = 1.0;
        double thetaRes = 0.01745; // ~1 degree in radians
        int threshold = 100;
    };

    struct CircleParams {
        int minRadius = 15;
        int maxRadius = 60;
        int threshold = 50;
    };

    // Detect straight lines
    static std::vector<LineParam> detectLines(const GrayImage& edgeMap, const LineParams& params);

    // Detect circles
    static std::vector<CircleParam> detectCircles(const GrayImage& edgeMap, const CircleParams& params);
};