#include "MainWindow.h"
#include "ImageUtils.h"
#include "CannyEdgeDetector.h"
#include "HoughTransform.h"
#include "EllipseDetector.h"
#include "ActiveContour.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QPainter>
#include <QPen>
#include <QStatusBar>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    resize(1200, 780);
    setWindowTitle("Computer Vision Studio - Shape & Contour Detection");
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // ================= LEFT PANEL: CONTROLS =================
    controlPanel = new QWidget(this);
    controlPanel->setFixedWidth(380);
    QVBoxLayout *panelLayout = new QVBoxLayout(controlPanel);

    QPushButton *btnOpen = new QPushButton("📁 Load Image...", this);
    btnOpen->setFixedHeight(35);
    connect(btnOpen, &QPushButton::clicked, this, &MainWindow::openImage);
    panelLayout->addWidget(btnOpen);

    QTabWidget *tabWidget = new QTabWidget(this);

    // --- TAB 1: SHAPES ---
    QWidget *tabShapes = new QWidget();
    QVBoxLayout *shapesLayout = new QVBoxLayout(tabShapes);

    // Canny
    QGroupBox *cannyBox = new QGroupBox("Canny Edge Detector");
    QGridLayout *cannyGrid = new QGridLayout(cannyBox);
    kernelSizeSpin = new QSpinBox(); kernelSizeSpin->setValue(5); kernelSizeSpin->setSingleStep(2);
    sigmaSpin = new QDoubleSpinBox(); sigmaSpin->setValue(1.4);
    cannyLowSpin = new QDoubleSpinBox(); cannyLowSpin->setMaximum(255); cannyLowSpin->setValue(20);
    cannyHighSpin = new QDoubleSpinBox(); cannyHighSpin->setMaximum(255); cannyHighSpin->setValue(50);
    QPushButton *btnCanny = new QPushButton("Run Canny");
    connect(btnCanny, &QPushButton::clicked, this, &MainWindow::runCanny);

    cannyGrid->addWidget(new QLabel("Kernel:"), 0, 0); cannyGrid->addWidget(kernelSizeSpin, 0, 1);
    cannyGrid->addWidget(new QLabel("Sigma:"), 1, 0); cannyGrid->addWidget(sigmaSpin, 1, 1);
    cannyGrid->addWidget(new QLabel("Low Thresh:"), 2, 0); cannyGrid->addWidget(cannyLowSpin, 2, 1);
    cannyGrid->addWidget(new QLabel("High Thresh:"), 3, 0); cannyGrid->addWidget(cannyHighSpin, 3, 1);
    cannyGrid->addWidget(btnCanny, 4, 0, 1, 2);
    shapesLayout->addWidget(cannyBox);

    // Hough
    QGroupBox *houghBox = new QGroupBox("Hough Transform");
    QGridLayout *houghGrid = new QGridLayout(houghBox);
    lineThresholdSpin = new QSpinBox(); lineThresholdSpin->setMaximum(1000); lineThresholdSpin->setValue(100);
    QPushButton *btnLines = new QPushButton("Detect Lines");
    connect(btnLines, &QPushButton::clicked, this, &MainWindow::runHoughLines);

    circleMinRSpin = new QSpinBox(); circleMinRSpin->setValue(15);
    circleMaxRSpin = new QSpinBox(); circleMaxRSpin->setValue(80);
    circleThreshSpin = new QSpinBox(); circleThreshSpin->setValue(50);
    QPushButton *btnCircles = new QPushButton("Detect Circles");
    connect(btnCircles, &QPushButton::clicked, this, &MainWindow::runHoughCircles);

    houghGrid->addWidget(new QLabel("Line Votes:"), 0, 0); houghGrid->addWidget(lineThresholdSpin, 0, 1);
    houghGrid->addWidget(btnLines, 1, 0, 1, 2);
    houghGrid->addWidget(new QLabel("Min/Max R:"), 2, 0); 
    QHBoxLayout *rBox = new QHBoxLayout(); rBox->addWidget(circleMinRSpin); rBox->addWidget(circleMaxRSpin);
    houghGrid->addLayout(rBox, 2, 1);
    houghGrid->addWidget(new QLabel("Circle Votes:"), 3, 0); houghGrid->addWidget(circleThreshSpin, 3, 1);
    houghGrid->addWidget(btnCircles, 4, 0, 1, 2);
    shapesLayout->addWidget(houghBox);

    // Ellipse
    QGroupBox *ellipseBox = new QGroupBox("Ellipse Detection");
    QVBoxLayout *ellipseLayout = new QVBoxLayout(ellipseBox);
    ellipseThreshSpin = new QSpinBox(); ellipseThreshSpin->setValue(40);
    QPushButton *btnEllipses = new QPushButton("Detect Ellipses");
    connect(btnEllipses, &QPushButton::clicked, this, &MainWindow::runEllipseDetection);
    ellipseLayout->addWidget(new QLabel("Votes Threshold:"));
    ellipseLayout->addWidget(ellipseThreshSpin);
    ellipseLayout->addWidget(btnEllipses);
    shapesLayout->addWidget(ellipseBox);
    shapesLayout->addStretch();

    tabWidget->addTab(tabShapes, "Shapes");

    // --- TAB 2: SNAKE ---
    QWidget *tabSnake = new QWidget();
    QVBoxLayout *snakeLayout = new QVBoxLayout(tabSnake);

    QGroupBox *snakeParamBox = new QGroupBox("Snake Initialization");
    QGridLayout *snakeGrid = new QGridLayout(snakeParamBox);

    snakePointsSpin = new QSpinBox(); snakePointsSpin->setRange(10, 500); snakePointsSpin->setValue(40);
    snakeRadiusSpin = new QDoubleSpinBox(); snakeRadiusSpin->setRange(5, 500); snakeRadiusSpin->setValue(60.0);
    snakeCenterXSpin = new QDoubleSpinBox(); snakeCenterXSpin->setRange(0, 2000); snakeCenterXSpin->setValue(150.0);
    snakeCenterYSpin = new QDoubleSpinBox(); snakeCenterYSpin->setRange(0, 2000); snakeCenterYSpin->setValue(150.0);
    snakeItersSpin = new QSpinBox(); snakeItersSpin->setRange(1, 1000); snakeItersSpin->setValue(50);

    snakeGrid->addWidget(new QLabel("No. Points:"), 0, 0); snakeGrid->addWidget(snakePointsSpin, 0, 1);
    snakeGrid->addWidget(new QLabel("Radius:"), 1, 0); snakeGrid->addWidget(snakeRadiusSpin, 1, 1);
    snakeGrid->addWidget(new QLabel("Center X:"), 2, 0); snakeGrid->addWidget(snakeCenterXSpin, 2, 1);
    snakeGrid->addWidget(new QLabel("Center Y:"), 3, 0); snakeGrid->addWidget(snakeCenterYSpin, 3, 1);
    snakeGrid->addWidget(new QLabel("Iterations:"), 4, 0); snakeGrid->addWidget(snakeItersSpin, 4, 1);

    QPushButton *btnSnake = new QPushButton("Evolve Snake");
    connect(btnSnake, &QPushButton::clicked, this, &MainWindow::runActiveContour);
    snakeGrid->addWidget(btnSnake, 5, 0, 1, 2);
    snakeLayout->addWidget(snakeParamBox);

    QGroupBox *metricsBox = new QGroupBox("Contour Metrics");
    QVBoxLayout *metricsLayout = new QVBoxLayout(metricsBox);
    metricsOutputText = new QTextEdit();
    metricsOutputText->setReadOnly(true);
    metricsLayout->addWidget(metricsOutputText);
    snakeLayout->addWidget(metricsBox);

    tabWidget->addTab(tabSnake, "Snake");
    panelLayout->addWidget(tabWidget);
    mainLayout->addWidget(controlPanel);

    // ================= RIGHT PANEL: VIEWPORTS =================
    QWidget *displayPanel = new QWidget(this);
    QVBoxLayout *displayContainer = new QVBoxLayout(displayPanel);

    QHBoxLayout *viewportsLayout = new QHBoxLayout();
    originalView = new QLabel("Original Image");
    originalView->setAlignment(Qt::AlignCenter);
    originalView->setStyleSheet("border: 1px dashed #555; background-color: #1e1e1e; color: #aaa;");

    resultView = new QLabel("Processed Image");
    resultView->setAlignment(Qt::AlignCenter);
    resultView->setStyleSheet("border: 1px dashed #555; background-color: #1e1e1e; color: #aaa;");

    viewportsLayout->addWidget(originalView);
    viewportsLayout->addWidget(resultView);
    displayContainer->addLayout(viewportsLayout);

    mainLayout->addWidget(displayPanel, 1);

    // ================= STATUS BAR & LOADING BAR =================
    statusLabel = new QLabel("Ready", this);
    loadingBar = new QProgressBar(this);
    loadingBar->setRange(0, 0); // Sets bar to continuous indeterminate pulsing
    loadingBar->setFixedWidth(160);
    loadingBar->setVisible(false);

    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(loadingBar);
}

// ============================================================================
// LOADING INDICATOR CONTROLLER
// ============================================================================

void MainWindow::startLoading(const QString& taskName) {
    statusLabel->setText("⏳ " + taskName + "... Please wait.");
    loadingBar->setVisible(true);
    controlPanel->setEnabled(false); // Disable buttons to prevent conflicting calls
    QApplication::setOverrideCursor(Qt::WaitCursor); // Show loading cursor
    QApplication::processEvents(); // Force GUI to immediately paint loading bar
}

void MainWindow::stopLoading() {
    QApplication::restoreOverrideCursor();
    loadingBar->setVisible(false);
    controlPanel->setEnabled(true);
    statusLabel->setText("✅ Completed.");
}

// ============================================================================
// PROCESSING SLOTS WITH LOADING INTEGRATION
// ============================================================================

void MainWindow::openImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.bmp *.jpeg)");
    if (fileName.isEmpty()) return;

    loadedImage.load(fileName);
    grayMatrix = ImageUtils::qImageToGrayMatrix(loadedImage);

    // Save and display smoothly
    currentOriginalImg = loadedImage;
    displayImage(originalView, currentOriginalImg);

    // Reset detected shape memory
    detectedLines.clear();
    detectedCircles.clear();
    detectedEllipses.clear();
    currentSnakeContour.clear();

    snakeCenterXSpin->setValue(loadedImage.width() / 2.0);
    snakeCenterYSpin->setValue(loadedImage.height() / 2.0);
    statusLabel->setText("Image loaded: " + fileName);
}

void MainWindow::updateResultDisplay(const QImage& img) {
    if (img.isNull()) return;
    currentResultImg = img;
    displayImage(resultView, currentResultImg);
}

void MainWindow::displayImage(QLabel* label, const QImage& img) {
    if (img.isNull() || label == nullptr) return;

    // Get the actual available space inside the label
    QSize targetSize = label->size();
    if (targetSize.width() <= 10 || targetSize.height() <= 10) {
        targetSize = QSize(600, 600); // Sensible fallback
    }

    // Qt::SmoothTransformation ensures high-quality anti-aliased resampling
    QPixmap pixmap = QPixmap::fromImage(img).scaled(
        targetSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    label->setPixmap(pixmap);
}

// Automatically re-render smoothly if the user resizes the window
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (!currentOriginalImg.isNull()) {
        displayImage(originalView, currentOriginalImg);
    }
    if (!currentResultImg.isNull()) {
        displayImage(resultView, currentResultImg);
    }
}

void MainWindow::runCanny() {
    if (grayMatrix.empty()) return;

    startLoading("Running Canny Edge Detector");

    CannyEdgeDetector::Params p;
    p.kernelSize = kernelSizeSpin->value();
    p.sigma = sigmaSpin->value();
    p.lowThreshold = cannyLowSpin->value();
    p.highThreshold = cannyHighSpin->value();

    edgeMatrix = CannyEdgeDetector::detectEdges(grayMatrix, p);
    updateResultDisplay(ImageUtils::grayMatrixToQImage(edgeMatrix));

    stopLoading();
}

void MainWindow::runHoughLines() {
    if (grayMatrix.empty()) return;
    if (edgeMatrix.empty()) runCanny();

    startLoading("Detecting Lines using Hough Transform");

    HoughTransform::LineParams p;
    p.threshold = lineThresholdSpin->value();
    detectedLines = HoughTransform::detectLines(edgeMatrix, p);
    overlayPrimitivesOnImage();

    stopLoading();
}

void MainWindow::runHoughCircles() {
    if (grayMatrix.empty()) return;
    if (edgeMatrix.empty()) runCanny();

    startLoading("Detecting Circles using Hough Transform");

    HoughTransform::CircleParams p;
    p.minRadius = circleMinRSpin->value();
    p.maxRadius = circleMaxRSpin->value();
    p.threshold = circleThreshSpin->value();
    detectedCircles = HoughTransform::detectCircles(edgeMatrix, p);
    overlayPrimitivesOnImage();

    stopLoading();
}

void MainWindow::runEllipseDetection() {
    if (grayMatrix.empty()) return;
    if (edgeMatrix.empty()) runCanny();

    startLoading("Detecting Ellipses (Paper Implementation)");

    EllipseDetector::Params p;
    p.threshold = ellipseThreshSpin->value();
    detectedEllipses = EllipseDetector::detectEllipses(edgeMatrix, p);
    overlayPrimitivesOnImage();

    stopLoading();
}

void MainWindow::runActiveContour() {
    if (grayMatrix.empty()) return;

    startLoading("Evolving Active Contour (Greedy Snake)");

    ActiveContour::SnakeConfig cfg;
    cfg.center = {snakeCenterXSpin->value(), snakeCenterYSpin->value()};
    cfg.radius = snakeRadiusSpin->value();
    cfg.numPoints = snakePointsSpin->value();
    cfg.maxIterations = snakeItersSpin->value();

    std::vector<Point2D> initContour = ActiveContour::initCircularContour(cfg.center, cfg.radius, cfg.numPoints);
    currentSnakeContour = ActiveContour::evolveSnake(grayMatrix, initContour, cfg);

    std::string chainCode = ActiveContour::computeChainCode(currentSnakeContour);
    double perimeter = ActiveContour::computePerimeter(currentSnakeContour);
    double area = ActiveContour::computeArea(currentSnakeContour);

    metricsOutputText->setText(QString(
        "--- Snake Results ---\n"
        "Perimeter: %1 px\n"
        "Area: %2 sq px\n\n"
        "Chain Code:\n%3"
    ).arg(perimeter).arg(area).arg(QString::fromStdString(chainCode)));

    overlayPrimitivesOnImage();

    stopLoading();
}

void MainWindow::overlayPrimitivesOnImage() {
    if (loadedImage.isNull()) return;

    // Use ARGB32_Premultiplied for crisp alpha blending & vector drawing
    QImage canvas = loadedImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 1. Draw Lines (Red)
    painter.setPen(QPen(Qt::red, 2, Qt::SolidLine, Qt::RoundCap));
    for (const auto& line : detectedLines) {
        double cosT = std::cos(line.theta);
        double sinT = std::sin(line.theta);
        double x0 = cosT * line.rho;
        double y0 = sinT * line.rho;
        painter.drawLine(
            QPointF(x0 + 3000 * (-sinT), y0 + 3000 * (cosT)),
            QPointF(x0 - 3000 * (-sinT), y0 - 3000 * (cosT))
        );
    }

    // 2. Draw Circles (Green)
    painter.setPen(QPen(Qt::green, 2, Qt::SolidLine, Qt::RoundCap));
    for (const auto& c : detectedCircles) {
        painter.drawEllipse(QPointF(c.cx, c.cy), c.radius, c.radius);
    }

    // 3. Draw Ellipses (Yellow)
    painter.setPen(QPen(Qt::yellow, 2, Qt::SolidLine, Qt::RoundCap));
    for (const auto& el : detectedEllipses) {
        painter.save();
        painter.translate(el.cx, el.cy);
        painter.rotate(el.angle * 180.0 / M_PI);
        painter.drawEllipse(QPointF(0, 0), el.a, el.b);
        painter.restore();
    }

    // 4. Draw Snake Contour (Cyan)
    if (!currentSnakeContour.empty()) {
        painter.setPen(QPen(Qt::cyan, 2, Qt::SolidLine, Qt::RoundCap));
        for (size_t i = 0; i < currentSnakeContour.size(); ++i) {
            size_t next = (i + 1) % currentSnakeContour.size();
            painter.drawLine(
                QPointF(currentSnakeContour[i].x, currentSnakeContour[i].y),
                QPointF(currentSnakeContour[next].x, currentSnakeContour[next].y)
            );
        }
    }

    painter.end();
    updateResultDisplay(canvas);
}