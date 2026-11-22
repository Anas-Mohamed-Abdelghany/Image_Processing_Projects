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
    // Computes full SIFT pipeline (DoG scale-space detector + 128-D descriptors) from scratch
    FeatureSet computeSIFT(const ImageData &data);
}

// =========================================================
// Tasks 3 & 4: Feature Matching
// Implement in src/FeatureMatching.cpp
// =========================================================
namespace FeatureMatching {
    // Task 3: SSD Matching
    MatchResult matchSSD(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures);

    // Task 3: SSD + Lowe's ratio test. Keeps a match only if bestSSD < ratio^2 * secondBestSSD
    // (smaller ratio = stricter). Used by the two-image Feature Matching window.
    MatchResult matchSSD(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures, float ratioThresh);

    // Task 4: NCC Matching
    MatchResult matchNCC(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures);
        
    MatchResult matchNCC(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures, float ratioThresh);
}

#endif // FEATURELOGIC_H