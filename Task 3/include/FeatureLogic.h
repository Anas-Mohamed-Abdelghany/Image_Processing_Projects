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
    // sigma      : Gaussian scale (px) applied to the image BEFORE the Harris
    //              response is computed (scale-space, hand-blurred). Larger sigma
    //              = features are detected at a coarser scale.
    //              Every keypoint gets kp.size = 6 * sigma, which Task 2's SIFT
    //              uses as its descriptor sampling scale, so the descriptors are
    //              produced at the scale the corner was actually detected at.
    // maxKeypoints : keep only the N strongest corners; 0 = keep all.
    FeatureSet computeHarrisCorners(const cv::Mat &grayscaleImage,
                                    float sigma = 1.0f,
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

    // Bonus/Scale-Invariant Mode: Computes both multi-scale DoG keypoints and SIFT descriptors
    FeatureSet computeFullSIFT(const ImageData &data);
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