#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <vector>
#include <string>
#include <opencv2/core.hpp>

struct ImageData {
    std::string filePath;
    cv::Mat grayscaleImage;
    cv::Mat colorImage;
    bool isLoaded = false;
};

struct FeatureSet {
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    double computationTimeMs = 0.0;
    double descriptionTimeMs = 0.0;
    bool isEmpty = true;
};

struct MatchResult {
    std::vector<cv::DMatch> matches;
    double matchingTimeMs = 0.0;
    std::string method = "Unknown";
    bool isEmpty = true;
};

#endif // DATASTRUCTURES_H