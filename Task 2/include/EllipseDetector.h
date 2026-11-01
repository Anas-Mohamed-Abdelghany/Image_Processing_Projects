#pragma once
#include "Types.h"
#include <vector>

// Member 3: Hough-based ellipse detector (axis-aligned ellipses).
// Same logic as the standalone OpenCV version, ported to the project's
// GrayImage type so it needs no extra dependency.
class EllipseDetector {
public:
    struct Params {
        // Semi-axis search ranges in ORIGINAL image pixels.
        // a = semi-axis along X, b = semi-axis along Y (both orientations are covered).
        // A value <= 0 means "automatic" (max = half of the image width / height).
        double minMajorAxis = 20.0;   // min "a"
        double maxMajorAxis = 0.0;    // max "a"  (0 = auto)
        double minMinorAxis = 20.0;   // min "b"
        double maxMinorAxis = 0.0;    // max "b"  (0 = auto)

        // Minimum support, as a PERCENTAGE (1-100) of the ellipse perimeter
        // that must lie on edge pixels.
        int threshold = 60;

        int workingSize = 300;   // edge map is shrunk so its longest side is <= this (speed)
        int axisStep = 5;        // step used when scanning a and b (working-size pixels)
        int angleStepDeg = 2;    // angular sampling step (smaller = more accurate, slower)
        int minAxisDiff = 10;    // skip near-circles while searching (|a-b| < this, working-size pixels)
        double minAxisDiffRatio = 0.2; // reject results whose axes differ by less than this fraction (circles belong to Hough circles)
        bool refineFullRes = true;      // re-fit each result on the full-resolution edge map (accuracy)
        int nmsRadius = 2;       // non-maximum suppression window radius (5x5)
        int maxVotingPoints = 6000; // speed guard: max edge pixels used for voting (0 = all)
        int maxResults = 10;     // keep only the strongest N ellipses
    };

    // Detect ellipses in a binary edge map (edge pixel == 255, e.g. Canny output).
    // Returned centre/axes are in the original image pixels.
    static std::vector<EllipseParam> detectEllipses(const GrayImage& edgeMap, const Params& params);

private:
    // Core Hough search on an (already shrunk) edge map, axes in the same pixels as the map.
    static std::vector<EllipseParam> detectCore(const GrayImage& edgeMap, const Params& params,
                                                int minA, int maxA, int minB, int maxB);
};
