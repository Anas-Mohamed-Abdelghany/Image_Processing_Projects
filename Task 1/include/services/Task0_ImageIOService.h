#pragma once

#include <string>

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 0 - Image Input
// Read RGB (color) and grayscale images and write results back to disk.
// (Using OpenCV for I/O is explicitly allowed.)
// ----------------------------------------------------------------------------
class Task0_ImageIOService
{
public:
    Task0_ImageIOService() = delete;

    // Reads an image from disk (color or grayscale). Returns an empty Mat on failure.
    static cv::Mat read(const std::string& filePath);

    // Writes the given image to disk. Returns false on failure.
    static bool write(const std::string& filePath, const cv::Mat& image);
};
