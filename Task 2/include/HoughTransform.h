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
        int minVotes = 16;
        int threshold = 30; // Minimum percentage of the circumference supported by edges.
    };

    static std::vector<LineParam> detectLines(
        const GrayImage& edgeMap,
        const LineParams& params
    );

    static std::vector<CircleParam> detectCircles(
        const GrayImage& grayImage,
        const GrayImage& edgeMap,
        const CircleParams& params
    );
};