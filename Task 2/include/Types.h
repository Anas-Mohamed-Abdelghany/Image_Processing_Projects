#pragma once
#include <vector>
#include <string>

// 2D Matrix of pixel intensities (0 to 255)
using GrayImage = std::vector<std::vector<double>>;

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

struct LineParam {
    double rho = 0.0;
    double theta = 0.0; // In radians
};

struct CircleParam {
    double cx = 0.0;
    double cy = 0.0;
    double radius = 0.0;
};

struct EllipseParam {
    double cx = 0.0;
    double cy = 0.0;
    double a = 0.0;     // Semi-major axis
    double b = 0.0;     // Semi-minor axis
    double angle = 0.0; // Orientation in radians
};

struct SnakeMetrics {
    std::string chainCode;
    double perimeter = 0.0;
    double area = 0.0;
};