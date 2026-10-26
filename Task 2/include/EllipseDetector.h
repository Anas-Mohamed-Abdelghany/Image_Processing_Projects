#pragma once
#include "Types.h"
#include <vector>

class EllipseDetector {
public:
    struct Params {
        double minMajorAxis = 10.0;
        double maxMajorAxis = 150.0;
        int threshold = 30;
    };

    // Detect ellipses using the assigned paper/algorithm
    static std::vector<EllipseParam> detectEllipses(const GrayImage& edgeMap, const Params& params);
};