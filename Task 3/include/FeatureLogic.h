#ifndef FEATURELOGIC_H
#define FEATURELOGIC_H

#include "DataStructures.h"

// =========================================================
// Task 1: Feature Detection
// Implement in src/FeatureDetection.cpp
// =========================================================
namespace FeatureDetection {
    // Input: Grayscale image
    // Output: FeatureSet containing keypoints (computationTimeMs should be set)
    // keypointSize : size (px) written into every cv::KeyPoint. SIFT (Task 2) uses
    //                this as the descriptor patch scale.
    // maxKeypoints : keep only the N strongest corners; 0 = keep all.
    FeatureSet computeHarrisCorners(const cv::Mat &grayscaleImage,
                                    float keypointSize = 6.0f,
                                    int maxKeypoints = 0);
}

// =========================================================
// Task 2: Feature Description
// Implement in src/FeatureDescription.cpp
// =========================================================
namespace FeatureDescription {
    // Input: Full image data (for context), FeatureSet containing keypoints from Task 1
    // Output: Updates the FeatureSet with computed descriptors (computationTimeMs should be updated)
    void computeSIFTDescriptors(const ImageData &data, FeatureSet &features);
}

// =========================================================
// Tasks 3 & 4: Feature Matching
// Implement in src/FeatureMatching.cpp
// =========================================================
namespace FeatureMatching {
    // Task 3: SSD Matching
    MatchResult matchSSD(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures);

    // Task 4: NCC Matching
    MatchResult matchNCC(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures);
}

#endif // FEATURELOGIC_H