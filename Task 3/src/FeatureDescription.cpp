#include "FeatureLogic.h"
// Include OpenCV modules needed for SIFT
#include <opencv2/xfeatures2d.hpp> // SIFT is often here in older OpenCV versions
// #include <opencv2/features2d.hpp> // Or here in OpenCV 4.4+

// --- TEAM MEMBER TASK 2 ---
// Implement SIFT Descriptor Generation here.

namespace FeatureDescription {

    void computeSIFTDescriptors(const ImageData &data, FeatureSet &features) {
        
        // -- START IMPLEMENTATION --
        
        // Input: data.grayscaleImage (Mat), features.keypoints (std::vector<KeyPoint>)
        // Output: features.descriptors (Mat), features.computationTimeMs (double)

        // 1. Create SIFT extractor: cv::xfeatures2d::SIFT::create() or cv::SIFT::create()
        // 2. sift->compute(data.grayscaleImage, features.keypoints, features.descriptors)
        
        // Temporary placeholder:
        features.descriptors = cv::Mat(); // Initialize empty
        
        // -- END IMPLEMENTATION --
        
    }
}