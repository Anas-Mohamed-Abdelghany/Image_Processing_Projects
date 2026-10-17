#include "services/Task6_NormalizationService.h"

// ============================================================================
// Task 6 - Image Normalization                                 [TODO: Teammate]
// Normalize the pixel values of the image.
// ============================================================================
cv::Mat Task6_NormalizationService::normalize(const cv::Mat& image)
{
    // TODO(Task6): normalize the pixel values, e.g. linear scaling of
    // [min, max] to [0, 255] with cv::normalize (NORM_MINMAX) or manually.
    if (image.empty()) {
        return {};
    }
    if(image.type() != CV_8UC1 &&
       image.type() != CV_8UC3) {
        // If the image is not of type CV_8U (grayscale or color), return an empty matrix
        return {};
    }
    // Find the minimum and maximum pixel values in the image

    double minVal = 255.0; // Initialize minVal to the maximum possible value for an 8-bit image
    double maxVal = 0.0; // Initialize maxVal to the minimum possible value for an 8-bit image

    // Total number of values in a row (width * channels)
    const int totalvalues = image.cols * image.channels();
   
    for (int i = 0; i < image.rows; i++) {
        const uchar* row = image.ptr<uchar>(i); // Get a pointer to the current row of pixel values

        for (int j = 0; j < totalvalues; j++) {
            if (row[j] < minVal) {
                minVal = row[j];
            }

            if (row[j] > maxVal) {
                maxVal = row[j];
            }
        }
    }
    // Create a new image to store the normalized values
    cv::Mat normalizedImage(image.size(), image.type());


    if(maxVal == minVal) {
        normalizedImage.setTo(0);// If all pixel values are the same, set the normalized image to 0
        return normalizedImage;
    }
        // Normalize the pixel values
    for (int i = 0; i < image.rows; i++) {
         // Get a pointer to the current row of pixel values
        const uchar* row = image.ptr<uchar>(i); 
       
        // Get a pointer to the current row of the normalized image
        uchar* normalizedRow = normalizedImage.ptr<uchar>(i);
        
        for (int j = 0; j < totalvalues; j++) {
            double value =
                (row[j] - minVal) * 255.0
                / (maxVal - minVal);

            normalizedRow[j] = static_cast<uchar>(value);
        }
    }
        return normalizedImage;
}
