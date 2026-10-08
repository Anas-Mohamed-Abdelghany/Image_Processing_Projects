#include "backend/ImageProcessorController.h"

#include "services/Task0_ImageIOService.h"
#include "services/Task1_NoiseService.h"
#include "services/Task2_LowPassFilterService.h"
#include "services/Task3_EdgeDetectionService.h"
#include "services/Task4_HistogramService.h"
#include "services/Task5_EqualizationService.h"
#include "services/Task6_NormalizationService.h"
#include "services/Task7_ThresholdingService.h"
#include "services/Task8_ColorTransformationService.h"
#include "services/Task9_FrequencyFilterService.h"
#include "services/Task10_HybridImageService.h"
#include "utils/QtCvBridge.h"
#include "validation/InputValidator.h"

#include <opencv2/opencv.hpp>

namespace {

// Puts two result images side by side, falling back to whichever is available.
cv::Mat concatHorizontal(const cv::Mat& left, const cv::Mat& right)
{
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    // A service that is not implemented yet returns its input unchanged, so
    // both sides can share the same buffer - avoid drawing the image twice.
    if (left.data == right.data) {
        return left;
    }
    if (left.rows != right.rows || left.type() != right.type()) {
        return left;
    }

    cv::Mat combined;
    cv::hconcat(left, right, combined);
    return combined;
}

} // namespace

ImageProcessorController::ImageProcessorController(QObject* parent)
    : QObject(parent)
{
}

bool ImageProcessorController::hasImage() const
{
    return !m_originalImage.empty();
}

bool ImageProcessorController::hasSecondImage() const
{
    return !m_secondImage.empty();
}

// ---------------------------------------------------------------------------
// Task 0 - Image input / output
// ---------------------------------------------------------------------------

bool ImageProcessorController::loadImage(const QString& filePath)
{
    const cv::Mat image = Task0_ImageIOService::read(filePath.toStdString());
    if (image.empty()) {
        emit errorOccurred(tr("Could not read the image: %1").arg(filePath));
        return false;
    }

    m_originalImage = image.clone();
    m_currentImage = image;
    publish(m_currentImage);
    return true;
}

bool ImageProcessorController::loadSecondImage(const QString& filePath)
{
    const cv::Mat image = Task0_ImageIOService::read(filePath.toStdString());
    if (image.empty()) {
        emit errorOccurred(tr("Could not read the second image: %1").arg(filePath));
        return false;
    }

    m_secondImage = image;
    return true;
}

bool ImageProcessorController::saveImage(const QString& filePath)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return false;
    }

    if (!Task0_ImageIOService::write(filePath.toStdString(), m_currentImage)) {
        emit errorOccurred(tr("Could not save the image: %1").arg(filePath));
        return false;
    }
    return true;
}

void ImageProcessorController::showOriginal()
{
    QString error;
    if (!InputValidator::isValidImage(m_originalImage, tr("Original image"), &error)) {
        emit errorOccurred(error);
        return;
    }
    publish(m_originalImage);
}

void ImageProcessorController::resetToOriginal()
{
    QString error;
    if (!InputValidator::isValidImage(m_originalImage, tr("Original image"), &error)) {
        emit errorOccurred(error);
        return;
    }
    m_currentImage = m_originalImage.clone();
    publish(m_currentImage);
}

// ---------------------------------------------------------------------------
// Task 1 - Noise
// ---------------------------------------------------------------------------

void ImageProcessorController::addUniformNoise(int amplitude)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isInRange(amplitude, 1, 255, tr("Noise amplitude"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task1_NoiseService::addUniformNoise(m_currentImage, amplitude), tr("Task 1 (uniform noise)"));
}

void ImageProcessorController::addGaussianNoise(double mean, double stddev)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isInRange(mean, -255, 255, tr("Noise mean"), &error) ||
        !InputValidator::isInRange(stddev, 0, 255, tr("Noise standard deviation"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task1_NoiseService::addGaussianNoise(m_currentImage, mean, stddev), tr("Task 1 (Gaussian noise)"));
}

void ImageProcessorController::addSaltAndPepperNoise(double noiseProbability)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isInRange(noiseProbability, 0.0, 1.0, tr("Salt and pepper probability"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task1_NoiseService::addSaltAndPepperNoise(m_currentImage, noiseProbability),
           tr("Task 1 (salt and pepper noise)"));
}

// ---------------------------------------------------------------------------
// Task 2 - Low pass filtering
// ---------------------------------------------------------------------------

void ImageProcessorController::applyAverageFilter(int ksize)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isValidKernelSize(ksize, &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task2_LowPassFilterService::applyAverageFilter(m_currentImage, ksize), tr("Task 2 (average filter)"));
}

void ImageProcessorController::applyGaussianFilter(int ksize, double sigma)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isValidKernelSize(ksize, &error) ||
        !InputValidator::isInRange(sigma, 0.1, 100.0, tr("Gaussian sigma"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task2_LowPassFilterService::applyGaussianFilter(m_currentImage, ksize, sigma),
           tr("Task 2 (Gaussian filter)"));
}

void ImageProcessorController::applyMedianFilter(int ksize)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isValidKernelSize(ksize, &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task2_LowPassFilterService::applyMedianFilter(m_currentImage, ksize), tr("Task 2 (median filter)"));
}

// ---------------------------------------------------------------------------
// Task 3 - Edge detection
// ---------------------------------------------------------------------------

void ImageProcessorController::applyEdges(EdgeMethod method, Direction direction)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return;
    }

    cv::Mat edges;
    switch (method) {
    case EdgeMethod::Sobel:
        edges = Task3_EdgeDetectionService::sobel(m_currentImage, direction);
        break;
    case EdgeMethod::Roberts:
        edges = Task3_EdgeDetectionService::roberts(m_currentImage, direction);
        break;
    case EdgeMethod::Prewitt:
        edges = Task3_EdgeDetectionService::prewitt(m_currentImage, direction);
        break;
    }

    commit(edges, tr("Task 3 (edge detection)"));
}

void ImageProcessorController::applyCanny(double lowThreshold, double highThreshold)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isInRange(lowThreshold, 0, 255, tr("Canny low threshold"), &error) ||
        !InputValidator::isInRange(highThreshold, 0, 255, tr("Canny high threshold"), &error) ||
        !InputValidator::isLowerOrEqual(lowThreshold, highThreshold,
                                        tr("Canny low threshold"), tr("Canny high threshold"), &error)) {
        emit errorOccurred(error);
        return;
    }

    commit(Task3_EdgeDetectionService::canny(m_currentImage, lowThreshold, highThreshold),
           tr("Task 3 (Canny)"));
}

// ---------------------------------------------------------------------------
// Task 4 - Histograms
// ---------------------------------------------------------------------------

void ImageProcessorController::showHistogram()
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return;
    }

    const cv::Mat histogram = Task4_HistogramService::drawHistogram(m_currentImage);
    const cv::Mat curve = Task4_HistogramService::drawCumulativeCurve(m_currentImage);
    commit(concatHorizontal(histogram, curve), tr("Task 4 (histogram)"));
}

// ---------------------------------------------------------------------------
// Task 5 - Equalization
// ---------------------------------------------------------------------------

void ImageProcessorController::applyEqualization()
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task5_EqualizationService::equalize(m_currentImage), tr("Task 5 (equalization)"));
}

// ---------------------------------------------------------------------------
// Task 6 - Normalization
// ---------------------------------------------------------------------------

void ImageProcessorController::applyNormalization()
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task6_NormalizationService::normalize(m_currentImage), tr("Task 6 (normalization)"));
}

// ---------------------------------------------------------------------------
// Task 7 - Thresholding
// ---------------------------------------------------------------------------

void ImageProcessorController::applyThreshold(double thresholdValue, double maxValue)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isInRange(thresholdValue, 0, 255, tr("Threshold value"), &error) ||
        !InputValidator::isInRange(maxValue, 0, 255, tr("Maximum value"), &error)) {
        emit errorOccurred(error);
        return;
    }

    commit(Task7_ThresholdingService::applyThreshold(m_currentImage, thresholdValue, maxValue),
           tr("Task 7 (thresholding)"));
}

// ---------------------------------------------------------------------------
// Task 8 - Color processing & channel histograms
// ---------------------------------------------------------------------------

void ImageProcessorController::applyGrayscaleConversion()
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task8_ColorTransformationService::toGrayscale(m_currentImage), tr("Task 8 (RGB to gray)"));
}

void ImageProcessorController::showChannelHistograms()
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return;
    }

    const cv::Mat histograms = Task8_ColorTransformationService::drawChannelHistograms(m_currentImage);
    const cv::Mat curves = Task8_ColorTransformationService::drawChannelCumulativeCurves(m_currentImage);
    commit(concatHorizontal(histograms, curves), tr("Task 8 (channel histograms)"));
}

// ---------------------------------------------------------------------------
// Task 9 - Frequency domain filtering
// ---------------------------------------------------------------------------

void ImageProcessorController::applyFrequencyLowPass(double cutoffRadius)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isInRange(cutoffRadius, 1, 10000, tr("Cutoff radius"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task9_FrequencyFilterService::lowPass(m_currentImage, cutoffRadius),
           tr("Task 9 (frequency low pass)"));
}

void ImageProcessorController::applyFrequencyHighPass(double cutoffRadius)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error) ||
        !InputValidator::isInRange(cutoffRadius, 1, 10000, tr("Cutoff radius"), &error)) {
        emit errorOccurred(error);
        return;
    }
    commit(Task9_FrequencyFilterService::highPass(m_currentImage, cutoffRadius),
           tr("Task 9 (frequency high pass)"));
}

// ---------------------------------------------------------------------------
// Task 10 - Hybrid image
// ---------------------------------------------------------------------------

void ImageProcessorController::applyHybridImage(double cutoffRadius)
{
    QString error;
    if (!InputValidator::isValidImage(m_currentImage, tr("Current image"), &error)) {
        emit errorOccurred(error);
        return;
    }
    if (!InputValidator::isValidImage(m_secondImage, tr("Second image"), &error)) {
        emit errorOccurred(error + tr(" Use \"Open Second Image\" first."));
        return;
    }
    if (!InputValidator::isInRange(cutoffRadius, 1, 10000, tr("Cutoff radius"), &error)) {
        emit errorOccurred(error);
        return;
    }

    commit(Task10_HybridImageService::createHybrid(m_currentImage, m_secondImage, cutoffRadius),
           tr("Task 10 (hybrid image)"));
}

// ---------------------------------------------------------------------------
// Internals
// ---------------------------------------------------------------------------

void ImageProcessorController::publish(const cv::Mat& image)
{
    emit resultReady(QtCvBridge::toQImage(image));
}

void ImageProcessorController::commit(const cv::Mat& result, const QString& taskName)
{
    if (result.empty()) {
        emit errorOccurred(tr("%1 returned no image - the service may not be implemented yet.").arg(taskName));
        return;
    }
    m_currentImage = result;
    publish(m_currentImage);
}
