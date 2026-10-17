#include "services/Task8_ColorTransformationService.h"
#include <algorithm>
#include <opencv2/imgproc.hpp>

// ============================================================================
// Task 8 - Color Processing & Channel Histograms               [TODO: Teammate]
// RGB -> grayscale, plus the individual R/G/B histograms and their
// cumulative curves (the mapping curves used for equalization).
// ============================================================================
cv::Mat Task8_ColorTransformationService::toGrayscale(const cv::Mat& image)
{
 
    if (image.empty()) {
        return {};
    }
    // If the image is already grayscale, return a copy
    if (image.type() == CV_8UC1) {
        return image.clone();
    }
    // Support 3-channel and 4-channel color images
    if (image.type() != CV_8UC3 && image.type() != CV_8UC4) {
        return {};
    }
            // Create a new grayscale image with the same dimensions as the input image
        cv::Mat grayImage(image.rows, image.cols, CV_8UC1); 
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            double blue, green, red;
            // Get the BGR pixel values from the input image
            if((image.type() == CV_8UC3)) {
                cv::Vec3b Pixel = image.at<cv::Vec3b>(i, j);
                blue = Pixel[0];
                green = Pixel[1];
                red = Pixel[2];
            } else if(image.type() == CV_8UC4) {
                cv::Vec4b Pixel = image.at<cv::Vec4b>(i, j);
                blue = Pixel[0];
                green = Pixel[1];
                red = Pixel[2];
            } else {
                // If the image is not of type CV_8UC3 or CV_8UC4, return an empty matrix
                return {};
            }
            // Convert to grayscale using the luminosity method
            double gray = 0.299 * red + 0.587 * green + 0.114 * blue;

            // Set the grayscale pixel value in the output image
            grayImage.at<uchar>(i, j) =
            cv::saturate_cast<uchar>(gray);    
        }
    }
    return grayImage;
}

cv::Mat Task8_ColorTransformationService::drawChannelHistograms(const cv::Mat& image)
{
    // channel and draw R, G and B side by side (use the channel colors).
    if (image.empty() || image.type() != CV_8UC3) {
        return {};
    }
    int hist[3][256] = {0}; // Histogram for each channel (B, G, R)
    for(int i = 0; i < image.rows; i++) {
        for(int j = 0; j < image.cols; j++) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);

            hist[0][pixel[0]]++; // Blue channel
            hist[1][pixel[1]]++; // Green channel   
            hist[2][pixel[2]]++; // Red channel
            
        }
    }

    const int panelWidth = 256;
    const int panelHeight = 300;
    const int top = 50;
    const int bottom = top + panelHeight;

    cv::Mat canvas(
        400, panelWidth * 3, CV_8UC3, cv::Scalar(255, 255, 255));

    int channelOrder[3] = {2, 1, 0};
    cv::Scalar colors[3] = {
        cv::Scalar(0, 0, 255),   // Red
        cv::Scalar(0, 180, 0),   // Green
        cv::Scalar(255, 0, 0)    // Blue
    };    

    const char* names[3] = {"Red", "Green", "Blue"};
    for (int p = 0; p < 3; p++) {
        int c = channelOrder[p];
        int offsetX = p * panelWidth;

        int maxCount = 1;

        for (int k = 0; k < 256; k++) {
            maxCount = std::max(maxCount, hist[c][k]);
        }

        cv::putText(
            canvas, names[p], cv::Point(offsetX + 80, 30),
            cv::FONT_HERSHEY_SIMPLEX, 0.7, colors[p], 2);

        cv::line(
            canvas, cv::Point(offsetX, bottom),
            cv::Point(offsetX + 255, bottom),
            cv::Scalar(0, 0, 0), 1);

        for (int k = 0; k < 256; k++) {
            int barHeight = static_cast<int>(
                static_cast<double>(hist[c][k]) / maxCount
                * panelHeight);

            cv::line(
                canvas,
                cv::Point(offsetX + k, bottom),
                cv::Point(offsetX + k, bottom - barHeight),
                colors[p], 1);
        }
    }


    return canvas;
}





cv::Mat Task8_ColorTransformationService::drawChannelCumulativeCurves(
    const cv::Mat& image)
{
    if (image.empty() || image.type() != CV_8UC3) {
        return {};
    }

    int hist[3][256] = {};

    // Step 1: Calculate each channel histogram manually
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);

            for (int c = 0; c < 3; c++) {
                hist[c][pixel[c]]++;
            }
        }
    }

    // Step 2: Calculate cumulative histograms manually
    int cumulative[3][256] = {};

    for (int c = 0; c < 3; c++) {
        cumulative[c][0] = hist[c][0];

        for (int k = 1; k < 256; k++) {
            cumulative[c][k] =
                cumulative[c][k - 1] + hist[c][k];
        }
    }

    // Step 3: Create the drawing canvas
    const int width = 600;
    const int height = 400;
    const int left = 50;
    const int top = 50;
    const int plotWidth = 512;
    const int plotHeight = 280;
    const int bottom = top + plotHeight;

    cv::Mat canvas(
        height, width, CV_8UC3, cv::Scalar(255, 255, 255));

    cv::putText(
        canvas, "Cumulative Distribution Curves",
        cv::Point(80, 30), cv::FONT_HERSHEY_SIMPLEX,
        0.7, cv::Scalar(0, 0, 0), 2);

    // Draw axes
    cv::line(
        canvas, cv::Point(left, top),
        cv::Point(left, bottom), cv::Scalar(0, 0, 0), 1);

    cv::line(
        canvas, cv::Point(left, bottom),
        cv::Point(left + plotWidth, bottom),
        cv::Scalar(0, 0, 0), 1);

    cv::putText(
        canvas, "0", cv::Point(left - 5, bottom + 20),
        cv::FONT_HERSHEY_SIMPLEX, 0.4,
        cv::Scalar(0, 0, 0), 1);

    cv::putText(
        canvas, "255", cv::Point(left + plotWidth - 20, bottom + 20),
        cv::FONT_HERSHEY_SIMPLEX, 0.4,
        cv::Scalar(0, 0, 0), 1);

    const int totalPixels = image.rows * image.cols;

    cv::Scalar colors[3] = {
        cv::Scalar(255, 0, 0),  // Blue
        cv::Scalar(0, 180, 0),  // Green
        cv::Scalar(0, 0, 255)   // Red
    };

    const char* names[3] = {"B", "G", "R"};

    // Step 4: Draw the three cumulative curves
    for (int c = 0; c < 3; c++) {
        for (int k = 1; k < 256; k++) {
            int x1 = left + (k - 1) * (plotWidth - 1) / 255;
            int x2 = left + k * (plotWidth - 1) / 255;

            int y1 = bottom - static_cast<int>(
                static_cast<double>(cumulative[c][k - 1])
                / totalPixels * plotHeight);

            int y2 = bottom - static_cast<int>(
                static_cast<double>(cumulative[c][k])
                / totalPixels * plotHeight);

            cv::line(
                canvas, cv::Point(x1, y1), cv::Point(x2, y2),
                colors[c], 2);
        }

        // Draw channel labels
        cv::putText(
            canvas, names[c],
            cv::Point(left + 30 + c * 45, top + 20),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, colors[c], 2);
    }

    return canvas;
}

