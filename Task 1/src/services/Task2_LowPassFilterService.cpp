#include "services/Task2_LowPassFilterService.h"
#include <algorithm>
#include <cmath>
#include <vector>

static int makeOdd(int ksize, int minSize)
{
    ksize = std::max(ksize, minSize);
    return (ksize % 2 == 0) ? ksize + 1 : ksize;
}

// Per-channel 2D correlation with a ksize x ksize float kernel, replicated borders.
// (Average and Gaussian kernels are symmetric, so correlation == convolution.)
static cv::Mat applyKernel(const cv::Mat& image, const std::vector<double>& k, int ksize)
{
    const int cn = image.channels(), r = ksize / 2;
    cv::Mat result(image.size(), image.type());

    for (int y = 0; y < image.rows; ++y)
    {
        uchar* dst = result.ptr<uchar>(y);
        for (int x = 0; x < image.cols; ++x)
            for (int c = 0; c < cn; ++c)
            {
                double sum = 0.0;
                for (int i = 0; i < ksize; ++i)
                {
                    const uchar* row = image.ptr<uchar>(std::clamp(y + i - r, 0, image.rows - 1));
                    for (int j = 0; j < ksize; ++j)
                    {
                        const int xx = std::clamp(x + j - r, 0, image.cols - 1);
                        sum += k[i * ksize + j] * row[xx * cn + c];
                    }
                }
                dst[x * cn + c] = cv::saturate_cast<uchar>(sum);
            }
    }
    return result;
}

cv::Mat Task2_LowPassFilterService::applyAverageFilter(const cv::Mat& image, int ksize)
{
    if (image.empty()) return image.clone();
    ksize = makeOdd(ksize, 1);
    // h(i,j) = 1 / (k*k)
    std::vector<double> k(ksize * ksize, 1.0 / (ksize * ksize));
    return applyKernel(image, k, ksize);
}

cv::Mat Task2_LowPassFilterService::applyGaussianFilter(const cv::Mat& image, int ksize, double sigma)
{
    if (image.empty()) return image.clone();
    ksize = makeOdd(ksize, 1);
    if (sigma <= 0.0) sigma = 0.3 * ((ksize - 1) * 0.5 - 1) + 0.8;   // same default as OpenCV

    // G(x,y) = exp(-(x^2 + y^2) / (2 sigma^2)), then normalise so the sum is 1
    const int r = ksize / 2;
    std::vector<double> k(ksize * ksize);
    double total = 0.0;
    for (int i = -r; i <= r; ++i)
        for (int j = -r; j <= r; ++j)
        {
            const double g = std::exp(-(i * i + j * j) / (2.0 * sigma * sigma));
            k[(i + r) * ksize + (j + r)] = g;
            total += g;
        }
    for (double& v : k) v /= total;
    return applyKernel(image, k, ksize);
}

cv::Mat Task2_LowPassFilterService::applyMedianFilter(const cv::Mat& image, int ksize)
{
    if (image.empty()) return image.clone();
    ksize = makeOdd(ksize, 3);

    const int cn = image.channels(), r = ksize / 2;
    cv::Mat result(image.size(), image.type());
    std::vector<uchar> win(ksize * ksize);

    for (int y = 0; y < image.rows; ++y)
    {
        uchar* dst = result.ptr<uchar>(y);
        for (int x = 0; x < image.cols; ++x)
            for (int c = 0; c < cn; ++c)
            {
                int n = 0;
                for (int i = -r; i <= r; ++i)
                {
                    const uchar* row = image.ptr<uchar>(std::clamp(y + i, 0, image.rows - 1));
                    for (int j = -r; j <= r; ++j)
                        win[n++] = row[std::clamp(x + j, 0, image.cols - 1) * cn + c];
                }
                std::nth_element(win.begin(), win.begin() + n / 2, win.end());
                dst[x * cn + c] = win[n / 2];
            }
    }
    return result;
}