#pragma once

#include <QImage>
#include <QObject>
#include <QString>

#include <opencv2/core.hpp>

#include "services/TaskTypes.h"

// ----------------------------------------------------------------------------
// ImageProcessorController (backend)
//
// The single bridge between the UI and the Task 0-10 services:
//
//   UI  --calls-->  controller method  --calls-->  TaskX service  --returns-->
//   controller publishes the result  --signal-->  UI (QImage)
//
// Every method already validates its input (validation/InputValidator) and
// invokes the matching service, so once a teammate implements their service
// body the feature works end to end without touching this file.
// ----------------------------------------------------------------------------
class ImageProcessorController : public QObject
{
    Q_OBJECT

public:
    explicit ImageProcessorController(QObject* parent = nullptr);

    bool hasImage() const;
    bool hasSecondImage() const;

    // --- Task 0 -------------------------------------------------------------
    bool loadImage(const QString& filePath);
    bool loadSecondImage(const QString& filePath);
    bool saveImage(const QString& filePath);

    // --- Display helpers ------------------------------------------------------
    void showOriginal();     // shows the image that was loaded (without changing the chain)
    void resetToOriginal();  // discards all applied operations

    // --- Task 1 -------------------------------------------------------------
    void addUniformNoise(int amplitude);
    void addGaussianNoise(double mean, double stddev);
    void addSaltAndPepperNoise(double noiseProbability);

    // --- Task 2 -------------------------------------------------------------
    void applyAverageFilter(int ksize);
    void applyGaussianFilter(int ksize, double sigma);
    void applyMedianFilter(int ksize);

    // --- Task 3 -------------------------------------------------------------
    void applyEdges(EdgeMethod method, Direction direction); // Sobel / Roberts / Prewitt
    void applyCanny(double lowThreshold, double highThreshold);

    // --- Task 4 -------------------------------------------------------------
    void showHistogram(); // histogram + cumulative curve side by side

    // --- Task 5 -------------------------------------------------------------
    void applyEqualization();

    // --- Task 6 -------------------------------------------------------------
    void applyNormalization();

    // --- Task 7 -------------------------------------------------------------
    void applyThreshold(double thresholdValue, double maxValue);

    // --- Task 8 -------------------------------------------------------------
    void applyGrayscaleConversion();
    void showChannelHistograms(); // R/G/B histograms + cumulative curves

    // --- Task 9 -------------------------------------------------------------
    void applyFrequencyLowPass(double cutoffRadius);
    void applyFrequencyHighPass(double cutoffRadius);

    // --- Task 10 ------------------------------------------------------------
    void applyHybridImage(double cutoffRadius);

signals:
    // Emitted whenever a new result image should be displayed.
    void resultReady(const QImage& image);
    // Emitted when validation or a service call fails.
    void errorOccurred(const QString& message);

private:
    // Converts and emits the given image (used for loads / original views).
    void publish(const cv::Mat& image);
    // Stores a service result as the new current image and publishes it.
    void commit(const cv::Mat& result, const QString& taskName);

    cv::Mat m_originalImage; // as loaded, never modified
    cv::Mat m_currentImage;  // result of the last applied operation (chained)
    cv::Mat m_secondImage;   // second image for Task 10 (hybrid)
};
