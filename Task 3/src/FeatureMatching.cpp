#include "FeatureLogic.h"
#include <opencv2/features2d.hpp>

// --- TEAM MEMBER TASKS 3 & 4 ---
// Implement SSD and NCC Matching here.

namespace FeatureMatching {

    MatchResult matchSSD(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures) {
        MatchResult mr;
        mr.method = "SSD";
        
        // -- START IMPLEMENTATION (Task 3) --
        
        // Use queryFeatures.descriptors and trainFeatures.descriptors
        // 1. Create matcher: cv::BFMatcher(cv::NORM_L2, false)
        // 2. matcher.match(query, train, matches)
        
        // Temporary placeholder:
        mr.matches.clear();
        mr.matchingTimeMs = 0.0;
        // mr.isEmpty = false; // Uncomment when implemented

        // -- END IMPLEMENTATION --
        
        return mr;
    }

    MatchResult matchNCC(const FeatureSet &queryFeatures, const FeatureSet &trainFeatures) {
        MatchResult mr;
        mr.method = "NCC";
        
        // -- START IMPLEMENTATION (Task 4) --
        
        // For NCC using OpenCV, you typically use cv::BFMatcher(cv::NORM_L2, false)
        // AND ensure your descriptors are normalized, or use cv::NORM_INF.
        
        // Temporary placeholder:
        mr.matches.clear();
        mr.matchingTimeMs = 0.0;
        // mr.isEmpty = false; // Uncomment when implemented

        // -- END IMPLEMENTATION --
        
        return mr;
    }
}