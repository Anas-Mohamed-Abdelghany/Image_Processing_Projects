#include "TaskManager.h"
#include "FeatureLogic.h" // Include the combined header
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QApplication>
#include <QPainter>
#include <opencv2/imgcodecs.hpp>

TaskManager::TaskManager(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

TaskManager::~TaskManager()
{
}

// =========================================================
// UI Construction
// =========================================================

void TaskManager::setupUI()
{
    this->setWindowTitle("CV Feature Analysis Suite");
    this->resize(1100, 660);

    centralWidget = new QWidget(this);
    centralWidget->setObjectName("root");
    this->setCentralWidget(centralWidget);

    mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupSidebar();
    setupContent();

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(contentWidget, 1);

    // Connections
    connect(btnLoadImage, &QPushButton::clicked, this, &TaskManager::onLoadImageClicked);
    connect(btnRun, &QPushButton::clicked, this, &TaskManager::runSelectedTask);
    connect(taskList, &QListWidget::currentRowChanged, this, [this](int) {
        updateTaskTitle();
    });

    // Sidebar starts locked until an image is loaded
    taskList->setCurrentRow(0);
    setTaskControlsEnabled(false);

    applyStyles();
}

void TaskManager::setupSidebar()
{
    sidebar = new QWidget(this);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(264);

    sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(18, 22, 18, 18);
    sidebarLayout->setSpacing(8);

    // Branding
    QLabel *lblBrand = new QLabel("CV Feature Suite", sidebar);
    lblBrand->setObjectName("brand");
    QLabel *lblSubtitle = new QLabel("Image Analysis Tools", sidebar);
    lblSubtitle->setObjectName("subtitle");
    sidebarLayout->addWidget(lblBrand);
    sidebarLayout->addWidget(lblSubtitle);

    sidebarLayout->addSpacing(14);

    btnLoadImage = new QPushButton("  Load Image...", sidebar);
    btnLoadImage->setObjectName("btnLoad");
    sidebarLayout->addWidget(btnLoadImage);

    sidebarLayout->addSpacing(18);

    QLabel *lblSection = new QLabel("TASKS", sidebar);
    lblSection->setObjectName("section");
    sidebarLayout->addWidget(lblSection);

    taskList = new QListWidget(sidebar);
    taskList->setObjectName("taskList");
    taskList->setFocusPolicy(Qt::NoFocus);
    taskList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    const QStringList tasks = {
        "1.  Harris Corner Detection",
        "2.  SIFT Descriptors",
        "3.  SSD Matching",
        "4.  NCC Matching"
    };
    for (const QString &t : tasks) {
        QListWidgetItem *item = new QListWidgetItem(t);
        item->setSizeHint(QSize(0, 44));
        taskList->addItem(item);
    }
    sidebarLayout->addWidget(taskList, 1);

    btnRun = new QPushButton("▶   Run Task", sidebar);
    btnRun->setObjectName("btnRun");
    sidebarLayout->addWidget(btnRun);
}

void TaskManager::setupContent()
{
    contentWidget = new QWidget(this);
    contentWidget->setObjectName("content");

    contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(28, 22, 28, 24);
    contentLayout->setSpacing(16);

    // Header row: task title (left) + live status (right)
    QHBoxLayout *headerLayout = new QHBoxLayout();
    lblTaskTitle = new QLabel("Harris Corner Detection", contentWidget);
    lblTaskTitle->setObjectName("pageTitle");
    lblStatus = new QLabel("Load an image to begin.", contentWidget);
    lblStatus->setObjectName("status");
    lblStatus->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    headerLayout->addWidget(lblTaskTitle, 1);
    headerLayout->addWidget(lblStatus, 0, Qt::AlignVCenter);
    contentLayout->addLayout(headerLayout);

    // Detection parameters: Task 1 writes them into the keypoints, Task 2 reads them
    QHBoxLayout *paramsLayout = new QHBoxLayout();
    paramsLayout->setSpacing(8);

    QLabel *lblParams = new QLabel("Scale params", contentWidget);
    lblParams->setObjectName("paramLabel");
    paramsLayout->addWidget(lblParams);

    spinSigma = new QDoubleSpinBox(contentWidget);
    spinSigma->setObjectName("paramSpin");
    spinSigma->setDecimals(1);
    spinSigma->setRange(0.5, 10.0);
    spinSigma->setSingleStep(0.5);
    spinSigma->setValue(1.0);
    spinSigma->setSuffix(" sigma");
    spinSigma->setToolTip("Detection scale sigma (px). The image is Gaussian-blurred\n"
                          "at this sigma before the Harris response is computed,\n"
                          "and every keypoint gets kp.size = 6 * sigma.\n"
                          "SIFT (Task 2) samples its descriptor at kp.size, so\n"
                          "keypoints are described at the scale they were found at.");
    paramsLayout->addWidget(spinSigma);

    spinMaxKeypoints = new QSpinBox(contentWidget);
    spinMaxKeypoints->setObjectName("paramSpin");
    spinMaxKeypoints->setRange(0, 5000);
    spinMaxKeypoints->setSingleStep(50);
    spinMaxKeypoints->setValue(0);
    spinMaxKeypoints->setSpecialValueText("all");
    spinMaxKeypoints->setSuffix(" keypoints");
    spinMaxKeypoints->setToolTip("Keep only the N strongest corners (0 = all).\n"
                                 "This is the length of the keypoint vector Task 2 consumes.");
    paramsLayout->addWidget(spinMaxKeypoints);

    paramsLayout->addStretch(1);
    contentLayout->addLayout(paramsLayout);

    // Image display box
    lblImageBox = new QLabel(contentWidget);
    lblImageBox->setObjectName("imageBox");
    lblImageBox->setAlignment(Qt::AlignCenter);
    lblImageBox->setMinimumSize(420, 320);
    lblImageBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    lblImageBox->setText("Load an image to begin");
    contentLayout->addWidget(lblImageBox, 1);

    // Results / timing card
    lblResults = new QLabel("No results yet.", contentWidget);
    lblResults->setObjectName("results");
    contentLayout->addWidget(lblResults);
}

void TaskManager::applyStyles()
{
    setStyleSheet(R"(
        /* ---------- Base ---------- */
        QWidget#root {
            background-color: #1a1d24;
            color: #e6e9f0;
            font-family: "Segoe UI", "Inter", sans-serif;
            font-size: 13px;
        }
        QWidget#content { background-color: #1a1d24; }

        /* ---------- Sidebar ---------- */
        QWidget#sidebar {
            background-color: #14171f;
            border-right: 1px solid #242938;
        }
        QLabel#brand {
            color: #ffffff;
            font-size: 17px;
            font-weight: 700;
        }
        QLabel#subtitle {
            color: #6b7285;
            font-size: 12px;
        }
        QLabel#section {
            color: #6b7285;
            font-size: 11px;
            font-weight: 700;
        }
        QPushButton#btnLoad {
            background-color: #232838;
            color: #dfe5f2;
            border: 1px solid #303748;
            border-radius: 8px;
            padding: 11px 14px;
            font-size: 13px;
            font-weight: 600;
            text-align: left;
        }
        QPushButton#btnLoad:hover {
            background-color: #2b3246;
            border-color: #3d4660;
        }
        QPushButton#btnLoad:pressed { background-color: #1d2230; }

        /* ---------- Task list ---------- */
        QListWidget#taskList {
            background-color: transparent;
            border: none;
            outline: none;
            color: #aab2c5;
            font-size: 13px;
        }
        QListWidget#taskList::item {
            padding: 12px 14px;
            margin: 2px 0px;
            border-radius: 8px;
            background-color: transparent;
        }
        QListWidget#taskList::item:hover {
            background-color: #1e222d;
            color: #e6e9f0;
        }
        QListWidget#taskList::item:selected {
            background-color: #4f8cff;
            color: #ffffff;
            font-weight: 600;
        }

        /* ---------- Run button ---------- */
        QPushButton#btnRun {
            background-color: #4f8cff;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 13px 14px;
            font-size: 14px;
            font-weight: 700;
        }
        QPushButton#btnRun:hover { background-color: #3f7ef5; }
        QPushButton#btnRun:pressed { background-color: #3570e0; }
        QPushButton#btnRun:disabled {
            background-color: #2a3040;
            color: #6b7285;
        }

        /* ---------- Content header ---------- */
        QLabel#pageTitle {
            color: #ffffff;
            font-size: 21px;
            font-weight: 700;
        }
        QLabel#status {
            color: #8b93a7;
            font-size: 12.5px;
        }

        /* ---------- Image box ---------- */
        QLabel#imageBox {
            background-color: #0e1015;
            border: 1px solid #2a2f3a;
            border-radius: 12px;
            color: #5b6478;
            font-size: 14px;
        }

        /* ---------- Keypoint parameter controls ---------- */
        QLabel#paramLabel {
            color: #8b93a7;
            font-size: 12px;
            font-weight: 600;
            margin-right: 4px;
        }
        QDoubleSpinBox#paramSpin, QSpinBox#paramSpin {
            background-color: #20242e;
            color: #dfe5f2;
            border: 1px solid #2f3546;
            border-radius: 6px;
            padding: 4px 6px;
            min-width: 108px;
        }
        QDoubleSpinBox#paramSpin:focus, QSpinBox#paramSpin:focus {
            border-color: #4f8cff;
        }
        QDoubleSpinBox#paramSpin:disabled, QSpinBox#paramSpin:disabled {
            color: #5b6478;
            background-color: #1a1d24;
        }

        /* ---------- Results card ---------- */
        QLabel#results {
            background-color: #20242e;
            border: 1px solid #2a2f3a;
            border-radius: 10px;
            padding: 12px 16px;
            color: #c9d1e0;
            font-size: 13px;
        }

        QScrollBar:vertical {
            background: #14171f;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #2f3546;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");
}

// =========================================================
// UI Helpers
// =========================================================

void TaskManager::updateTaskTitle()
{
    const int row = taskList->currentRow();
    if (row < 0) return;
    const QString text = taskList->item(row)->text();
    // Strip the leading "N.  " index for the page title
    const int dot = text.indexOf('.');
    lblTaskTitle->setText(dot >= 0 ? text.mid(dot + 2).trimmed() : text);
}

void TaskManager::setTaskControlsEnabled(bool enabled)
{
    taskList->setEnabled(enabled);
    btnRun->setEnabled(enabled);
    spinSigma->setEnabled(enabled);
    spinMaxKeypoints->setEnabled(enabled);
}

void TaskManager::updateStatus(const QString &message)
{
    lblStatus->setText(message);
}

void TaskManager::updateResults(const QString &message)
{
    lblResults->setText(message);
}

void TaskManager::refreshImageDisplay()
{
    if (originalPixmap.isNull()) {
        lblImageBox->setPixmap(QPixmap());
        lblImageBox->setText("Load an image to begin");
        return;
    }

    // Work on a copy so the overlay never mutates the source image
    QPixmap canvas = originalPixmap;

    if (overlayKeypoints && !cachedFeatures.isEmpty) {
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Scale keypoint coordinates if the pixmap differs from the source image
        const double sx = static_cast<double>(canvas.width()) /
                          std::max(1, currentImageData.grayscaleImage.cols);
        const double sy = static_cast<double>(canvas.height()) /
                          std::max(1, currentImageData.grayscaleImage.rows);

        QPen pen(QColor(0, 255, 136));
        pen.setWidthF(qMax(1.5, canvas.width() / 500.0));
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        const double fallback = qMax(5.0, canvas.height() / 90.0);
        for (const cv::KeyPoint &kp : cachedFeatures.keypoints) {
            // Draw at the keypoint's own scale: kp.size is a diameter in
            // source-image pixels, sx converts it to pixmap pixels.
            const double r = (kp.size > 0.0f) ? kp.size * 0.5 * sx : fallback;
            painter.drawEllipse(QPointF(kp.pt.x * sx, kp.pt.y * sy), r, r);
        }
    }

    // Fit the image inside the box, preserving aspect ratio
    const QSize target = lblImageBox->size() - QSize(24, 24);
    if (target.width() > 0 && target.height() > 0 &&
        (canvas.width() > target.width() || canvas.height() > target.height())) {
        canvas = canvas.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    lblImageBox->setText(QString());
    lblImageBox->setPixmap(canvas);
}

void TaskManager::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    refreshImageDisplay(); // keep the image fitted when the window resizes
}

// =========================================================
// Image Loading
// =========================================================

void TaskManager::onLoadImageClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.jpeg)");
    if (filePath.isEmpty()) return;

    if (loadSelectedImage(filePath)) {
        updateStatus("Loaded: " + QFileInfo(filePath).fileName());
        setTaskControlsEnabled(true);
        updateResults("Image loaded — choose a task and press Run.");

        // Reset cached data on new image load
        cachedFeatures = FeatureSet();
        cachedMatchesSSD = MatchResult();
        cachedMatchesNCC = MatchResult();
        overlayKeypoints = false;

        refreshImageDisplay();
    }
}

bool TaskManager::loadSelectedImage(const QString &filePath)
{
    std::string pathStr = filePath.toStdString();
    // Load both formats immediately
    currentImageData.colorImage = cv::imread(pathStr, cv::IMREAD_COLOR);
    currentImageData.grayscaleImage = cv::imread(pathStr, cv::IMREAD_GRAYSCALE);

    if (currentImageData.colorImage.empty() || currentImageData.grayscaleImage.empty()) {
        updateStatus("Error loading image.");
        return false;
    }

    currentImageData.filePath = pathStr;
    currentImageData.isLoaded = true;

    // Copy() so the pixmap owns its pixels (the QImage wraps cv::Mat memory)
    originalPixmap = QPixmap::fromImage(QImage(currentImageData.colorImage.data,
                                               currentImageData.colorImage.cols,
                                               currentImageData.colorImage.rows,
                                               currentImageData.colorImage.step,
                                               QImage::Format_RGB888)
                                            .rgbSwapped()
                                            .copy());
    return true;
}

// =========================================================
// Task dispatch
// =========================================================

void TaskManager::runSelectedTask()
{
    switch (taskList->currentRow()) {
        case 0: runHarrisTask(); break;
        case 1: runSIFTTask();   break;
        case 2: runSSDTask();    break;
        case 3: runNCCTask();    break;
        default: break;
    }
}

// =========================================================
// Logic Connections
// =========================================================

void TaskManager::runHarrisTask()
{
    if (!currentImageData.isLoaded) return;
    updateStatus("Running Harris Detector...");

    // Values chosen in the UI -> detection scale / vector length handed to Task 2
    const float sigma = static_cast<float>(spinSigma->value());
    const int maxKeypoints = spinMaxKeypoints->value();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    // Call logic from FeatureDetection.cpp
    cachedFeatures = FeatureDetection::computeHarrisCorners(currentImageData.grayscaleImage,
                                                            sigma,
                                                            maxKeypoints);
    QApplication::restoreOverrideCursor();

    if (!cachedFeatures.isEmpty) {
        overlayKeypoints = true;
        updateStatus("Detection complete.");
        updateResults(QString("  Corners found: %1   ·   sigma %2 px -> kp.size %3 px   ⏱ %4 ms")
                      .arg(cachedFeatures.keypoints.size())
                      .arg(sigma, 0, 'f', 1)
                      .arg(6.0f * sigma, 0, 'f', 1)
                      .arg(cachedFeatures.computationTimeMs, 0, 'f', 2));
    } else {
        overlayKeypoints = false;
        updateStatus("Harris Detection failed.");
        updateResults("No corners detected.");
    }

    refreshImageDisplay();
}

void TaskManager::runSIFTTask()
{
    if (!currentImageData.isLoaded) return;
    updateStatus("Running SIFT Pipeline (Scale-Invariant DoG)...");

    QApplication::setOverrideCursor(Qt::WaitCursor);
    cachedFeatures = FeatureDescription::computeSIFT(currentImageData);
    QApplication::restoreOverrideCursor();

    if (!cachedFeatures.descriptors.empty()) {
        overlayKeypoints = true;
        updateStatus("SIFT complete.");
        // Report the scale range SIFT actually sampled at
        float minSize = cachedFeatures.keypoints.front().size;
        float maxSize = minSize;
        for (const cv::KeyPoint &kp : cachedFeatures.keypoints) {
            minSize = std::min(minSize, kp.size);
            maxSize = std::max(maxSize, kp.size);
        }
        const QString sizeText = (minSize == maxSize)
            ? QString("%1 px").arg(minSize, 0, 'f', 1)
            : QString("%1–%2 px").arg(minSize, 0, 'f', 1).arg(maxSize, 0, 'f', 1);
        updateResults(QString("  Keypoints & Descriptors: %1   ·   kp.size range: %2   ⏱ %3 ms")
                      .arg(cachedFeatures.descriptors.rows)
                      .arg(sizeText)
                      .arg(cachedFeatures.computationTimeMs, 0, 'f', 2));
    } else {
        updateStatus("SIFT Description failed.");
        updateResults("No descriptors generated.");
    }

    refreshImageDisplay();
}

void TaskManager::runSSDTask()
{
    // Check dependency
    if (cachedFeatures.descriptors.empty()) {
        QMessageBox::warning(this, "Dependency", "Please run Task 2 (Description) first.");
        return;
    }
    updateStatus("Running SSD Matching (self-comparison)...");

    QApplication::setOverrideCursor(Qt::WaitCursor);
    // Call logic from FeatureMatching.cpp. For testing simplicity, match image to itself.
    cachedMatchesSSD = FeatureMatching::matchSSD(cachedFeatures, cachedFeatures);
    QApplication::restoreOverrideCursor();

    if (!cachedMatchesSSD.isEmpty) {
        updateStatus("SSD Matching complete.");
        updateResults(QString("  Matches: %1      ⏱ %2 ms")
                      .arg(cachedMatchesSSD.matches.size())
                      .arg(cachedMatchesSSD.matchingTimeMs, 0, 'f', 2));
    } else {
        updateStatus("SSD Matching failed.");
        updateResults("No matches found.");
    }
}

void TaskManager::runNCCTask()
{
    // Check dependency
    if (cachedFeatures.descriptors.empty()) {
        QMessageBox::warning(this, "Dependency", "Please run Task 2 (Description) first.");
        return;
    }
    updateStatus("Running NCC Matching (self-comparison)...");

    QApplication::setOverrideCursor(Qt::WaitCursor);
    // Call logic from FeatureMatching.cpp
    cachedMatchesNCC = FeatureMatching::matchNCC(cachedFeatures, cachedFeatures);
    QApplication::restoreOverrideCursor();

    if (!cachedMatchesNCC.isEmpty) {
        updateStatus("NCC Matching complete.");
        updateResults(QString("  Matches: %1      ⏱ %2 ms")
                      .arg(cachedMatchesNCC.matches.size())
                      .arg(cachedMatchesNCC.matchingTimeMs, 0, 'f', 2));
    } else {
        updateStatus("NCC Matching failed.");
        updateResults("No matches found.");
    }
}