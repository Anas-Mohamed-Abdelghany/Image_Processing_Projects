#pragma once
#include "Types.h"
#include <vector>
#include <string>

class ActiveContour {
public:
    struct SnakeConfig {
        Point2D center;
        double radius = 50.0;
        int numPoints = 40;
        int maxIterations = 80;
        double alpha = 1.0; // Continuity weight
        double beta = 1.0;  // Curvature weight
        double gamma = 1.2; // Edge gradient weight
    };

    // Initialize circular contour around target object
    static std::vector<Point2D> initCircularContour(Point2D center, double radius, int numPoints);
        // Estimates a circle that encloses the object (assumes an object on a fairly uniform background).
    // Returns false if no object is found; center/radius are then left untouched.
    static bool estimateInitialCircle(const GrayImage& img, Point2D& center, double& radius, double margin = 15.0);
    // Evolve the snake using Greedy Algorithm
    static std::vector<Point2D> evolveSnake(
        const GrayImage& originalImage,
        const std::vector<Point2D>& initialContour,
        const SnakeConfig& config
    );

    // Post-convergence analysis
    static std::string computeChainCode(const std::vector<Point2D>& contour);
    static double computePerimeter(const std::vector<Point2D>& contour);
    static double computeArea(const std::vector<Point2D>& contour);
};