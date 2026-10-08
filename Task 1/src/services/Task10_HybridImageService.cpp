#include "services/Task10_HybridImageService.h"

#include "services/Task9_FrequencyFilterService.h"

// ============================================================================
// Task 10 - Hybrid Images                                      [TODO: Teammate]
// low frequencies of one image + high frequencies of another image.
// Reuse the Task 9 filters for both components.
// ============================================================================
cv::Mat Task10_HybridImageService::createHybrid(const cv::Mat& lowImage, const cv::Mat& highImage, double cutoffRadius)
{
    // TODO(Task10): hybrid = Task9::lowPass(lowImage, cutoffRadius)
    //                    + Task9::highPass(highImage, cutoffRadius)
    // (make sure both images have the same size/type first, then saturate_add).
    (void)cutoffRadius;
    return lowImage;
}
