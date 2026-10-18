#pragma once

#include <opencv2/core.hpp>

// ----------------------------------------------------------------------------
// Task 9 - Frequency Domain Filtering
// High pass and low pass filters in the frequency domain
// (FFT -> multiply by a filter mask -> inverse FFT), using OpenCV's cv::dft /
// cv::idft (the library FFT, not written from scratch).
//
// Filter used: GAUSSIAN (no ringing, smooth roll-off, one parameter):
//     H_low (u,v) = exp( -D(u,v)^2 / (2 * D0^2) )
//     H_high(u,v) = 1 - H_low(u,v)           (so low + high = original image)
// where D(u,v) is the distance of the frequency (u,v) from the DC component and
// D0 = cutoffRadius is chosen in the UI. D is measured in frequency samples of
// the image's longer side, so the same D0 gives the same blur on any image size
// and the blur is round (isotropic) even for non-square images.
//
// Every service here works on COPIES: the input image is never modified, so the
// controller's original / current / second images stay intact.
//
// Supported input: 1 / 3 / 4 channel images, 8 bit, 16 bit or float. Each colour
// channel is filtered on its own; an alpha channel is passed through untouched.
// ----------------------------------------------------------------------------
class Task9_FrequencyFilterService
{
public:
    Task9_FrequencyFilterService() = delete;

    // Keeps only the low frequencies (smoothed image).
    // Result has the same size / type / channels as the input.
    static cv::Mat lowPass(const cv::Mat& image, double cutoffRadius);

    // Keeps only the high frequencies (edges / fine detail).
    // The raw result is signed (zero mean), so it is converted for display:
    // absolute value, scaled so the strongest details reach full brightness
    // (flat areas -> black). Same size / type / channels as the input.
    static cv::Mat highPass(const cv::Mat& image, double cutoffRadius);

    // ---- Raw versions (used by Task 10) ------------------------------------
    // Same filters, but the result is returned as CV_32F with the input's
    // channel count and NO clipping / scaling, so the signed high frequencies
    // can be added to another image without losing information.
    static cv::Mat lowPassFloat(const cv::Mat& image, double cutoffRadius);
    static cv::Mat highPassFloat(const cv::Mat& image, double cutoffRadius);
};
