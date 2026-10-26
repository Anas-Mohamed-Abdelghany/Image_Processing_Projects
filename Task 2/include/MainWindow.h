#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QTabWidget>
#include <QProgressBar>
#include <QImage>

#include "Types.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void openImage();
    void runCanny();
    void runHoughLines();
    void runHoughCircles();
    void runEllipseDetection();
    void runActiveContour();

private:
    void setupUI();
    void updateResultDisplay(const QImage& img);
    void overlayPrimitivesOnImage();
    void displayImage(QLabel* label, const QImage& img);

    // Loading State Helpers
    void startLoading(const QString& taskName);
    void stopLoading();

    QImage currentOriginalImg;
    QImage currentResultImg;

    // Image Data
    QImage loadedImage;
    GrayImage grayMatrix;
    GrayImage edgeMatrix;

    // Detected Primitives for superimposition
    std::vector<LineParam> detectedLines;
    std::vector<CircleParam> detectedCircles;
    std::vector<EllipseParam> detectedEllipses;
    std::vector<Point2D> currentSnakeContour;

    // UI Panels & Indicators
    QWidget *controlPanel;
    QProgressBar *loadingBar;
    QLabel *statusLabel;

    // Display Widgets
    QLabel *originalView;
    QLabel *resultView;

    // Canny UI
    QSpinBox *kernelSizeSpin;
    QDoubleSpinBox *sigmaSpin;
    QDoubleSpinBox *cannyLowSpin;
    QDoubleSpinBox *cannyHighSpin;

    // Hough UI
    QSpinBox *lineThresholdSpin;
    QSpinBox *circleMinRSpin;
    QSpinBox *circleMaxRSpin;
    QSpinBox *circleThreshSpin;

    // Ellipse UI
    QSpinBox *ellipseThreshSpin;

    // Snake UI
    QSpinBox *snakePointsSpin;
    QDoubleSpinBox *snakeRadiusSpin;
    QDoubleSpinBox *snakeCenterXSpin;
    QDoubleSpinBox *snakeCenterYSpin;
    QSpinBox *snakeItersSpin;
    QTextEdit *metricsOutputText;
};