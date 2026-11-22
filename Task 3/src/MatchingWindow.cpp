#include "MatchingWindow.h"
#include "FeatureLogic.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QVBoxLayout>

#include <opencv2/imgcodecs.hpp>

#include <algorithm>

namespace {
    void drawPlaceholder(QPainter &p, const QRect &r, const QString &text)
    {
        p.fillRect(r, QColor("#150d26"));
        p.setPen(QColor("#6d5a9a"));
        QFont f = p.font();
        f.setPixelSize(std::max(14, r.height() / 18));
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, text);
    }
}

MatchingWindow::MatchingWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlag(Qt::Window, true);   // separate top-level window even though it has a parent
    buildUI();
    applyStyles();
    rebuildCanvas();
    updateDisplay();
}

// =========================================================
// UI
// =========================================================

void MatchingWindow::buildUI()
{
    setWindowTitle("Image's Feature - Feature Matching");
    resize(1280, 760);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);

    // ---- Top bar: centered title + "Load Image" on the right ----
    QHBoxLayout *top = new QHBoxLayout();
    QWidget *spacer = new QWidget(this);
    spacer->setFixedWidth(160);
    QLabel *title = new QLabel("Image's Feature", this);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    btnLoadFirst = new QPushButton("Load Image", this);
    btnLoadFirst->setObjectName("primary");
    btnLoadFirst->setFixedWidth(160);
    top->addWidget(spacer);
    top->addWidget(title, 1);
    top->addWidget(btnLoadFirst);
    root->addLayout(top);

    // ---- Body: controls (left) | images (right) ----
    QHBoxLayout *body = new QHBoxLayout();
    body->setSpacing(16);

    // Left column
    QWidget *left = new QWidget(this);
    left->setFixedWidth(340);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    QLabel *lblMode = new QLabel("Choose The Mode:", left);
    lblMode->setObjectName("heading");
    comboMode = new QComboBox(left);
    comboMode->addItem("Feature Matching");
    leftLayout->addWidget(lblMode);
    leftLayout->addWidget(comboMode);

    QGroupBox *group = new QGroupBox("Matching Methods", left);
    QVBoxLayout *g = new QVBoxLayout(group);
    g->setContentsMargins(12, 22, 12, 12);
    g->setSpacing(8);

    btnLoadSecond = new QPushButton("Load Second Image", group);
    btnLoadSecond->setObjectName("primary");
    g->addWidget(btnLoadSecond);

    radioSSD = new QRadioButton("SSD Method", group);
    radioSSD->setChecked(true);
    QLabel *lblSSD = new QLabel("SSD Ratio Test:", group);
    lblSSD->setObjectName("heading");
    sliderSSD = new QSlider(Qt::Horizontal, group);
    sliderSSD->setRange(1, 100);
    sliderSSD->setValue(65);
    lblSSDValue = new QLabel("0.65", group);
    lblSSDValue->setObjectName("heading");
    QHBoxLayout *rowSSD = new QHBoxLayout();
    rowSSD->addWidget(sliderSSD, 1);
    rowSSD->addWidget(lblSSDValue);

    radioNCC = new QRadioButton("NCC Method", group);
    QLabel *lblNCC = new QLabel("NCC Ratio Test:", group);
    lblNCC->setObjectName("heading");
    sliderNCC = new QSlider(Qt::Horizontal, group);
    sliderNCC->setRange(1, 100);
    sliderNCC->setValue(65);
    sliderNCC->setToolTip("Applied once the NCC matcher (Task 4) supports a ratio test.");
    lblNCCValue = new QLabel("0.65", group);
    lblNCCValue->setObjectName("heading");
    QHBoxLayout *rowNCC = new QHBoxLayout();
    rowNCC->addWidget(sliderNCC, 1);
    rowNCC->addWidget(lblNCCValue);

    g->addSpacing(4);
    g->addWidget(radioSSD);
    g->addWidget(lblSSD);
    g->addLayout(rowSSD);
    g->addSpacing(4);
    g->addWidget(radioNCC);
    g->addWidget(lblNCC);
    g->addLayout(rowNCC);
    leftLayout->addWidget(group);

    btnApply = new QPushButton("Apply Matching", left);
    btnApply->setObjectName("primaryBig");
    leftLayout->addWidget(btnApply);

    lblResults = new QLabel("Load both images, then press Apply Matching.", left);
    lblResults->setObjectName("results");
    lblResults->setWordWrap(true);
    leftLayout->addWidget(lblResults);
    leftLayout->addStretch(1);

    // Right column
    QVBoxLayout *right = new QVBoxLayout();
    right->setSpacing(6);

    titlesLayout = new QHBoxLayout();
    titlesLayout->setContentsMargins(0, 0, 0, 0);
    lblTitle1 = new QLabel("Image 1", this);
    lblTitle2 = new QLabel("Image 2", this);
    lblTitle1->setObjectName("heading");
    lblTitle2->setObjectName("heading");
    lblTitle1->setAlignment(Qt::AlignCenter);
    lblTitle2->setAlignment(Qt::AlignCenter);
    titlesLayout->addWidget(lblTitle1, 1);
    titlesLayout->addWidget(lblTitle2, 1);
    right->addLayout(titlesLayout);

    lblCanvas = new QLabel(this);
    lblCanvas->setObjectName("canvas");
    lblCanvas->setAlignment(Qt::AlignCenter);
    lblCanvas->setMinimumSize(420, 300);
    lblCanvas->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    right->addWidget(lblCanvas, 1);

    body->addWidget(left);
    body->addLayout(right, 1);
    root->addLayout(body, 1);

    // ---- Connections ----
    connect(btnLoadFirst, &QPushButton::clicked, this, &MatchingWindow::onLoadFirstClicked);
    connect(btnLoadSecond, &QPushButton::clicked, this, &MatchingWindow::onLoadSecondClicked);
    connect(btnApply, &QPushButton::clicked, this, &MatchingWindow::onApplyClicked);
    connect(sliderSSD, &QSlider::valueChanged, this, [this](int v) {
        lblSSDValue->setText(QString::number(v / 100.0, 'f', 2));
    });
    connect(sliderNCC, &QSlider::valueChanged, this, [this](int v) {
        lblNCCValue->setText(QString::number(v / 100.0, 'f', 2));
    });
}

void MatchingWindow::applyStyles()
{
    setStyleSheet(R"(
        MatchingWindow { background-color: #0f0919; }
        QWidget { color: #efe6ff; font-family: "Segoe UI", "Inter", sans-serif; font-size: 13px; }

        QLabel#title { color: #d8b4fe; font-size: 28px; font-weight: 700; }
        QLabel#heading { color: #ead9ff; font-size: 14px; font-weight: 700; }
        QLabel#results {
            background-color: #1a1030; border: 1px solid #4c1d95; border-radius: 8px;
            padding: 10px 12px; color: #d9c8f5;
        }
        QLabel#canvas {
            background-color: #0b0714; border: 2px solid #4c1d95; border-radius: 4px;
            color: #6d5a9a; font-size: 15px;
        }

        QPushButton#primary, QPushButton#primaryBig {
            background-color: #6b21a8; color: #ffffff; border: none;
            border-radius: 8px; padding: 11px 14px; font-weight: 700;
        }
        QPushButton#primaryBig { font-size: 15px; padding: 16px 14px; }
        QPushButton#primary:hover, QPushButton#primaryBig:hover { background-color: #7e2bc4; }
        QPushButton#primary:pressed, QPushButton#primaryBig:pressed { background-color: #581c87; }

        QComboBox {
            background-color: #2a1b4a; color: #ffffff; border: 1px solid #4c1d95;
            border-radius: 8px; padding: 10px 12px; font-weight: 700;
        }
        QComboBox QAbstractItemView {
            background-color: #2a1b4a; color: #ffffff; selection-background-color: #6b21a8;
        }

        QGroupBox {
            border: 1px solid #5b21b6; border-radius: 10px; margin-top: 12px;
            font-weight: 700; color: #d8b4fe;
        }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 8px; }

        QRadioButton { color: #f3eaff; font-weight: 700; spacing: 8px; }
        QRadioButton::indicator {
            width: 8px; height: 8px; border-radius: 9px;
            border: 5px solid #5b21b6; background: #1a1030;
        }
        QRadioButton::indicator:checked { border: 5px solid #2f80ed; background: #ffffff; }

        QSlider::groove:horizontal { height: 6px; background: #3b2a63; border-radius: 3px; }
        QSlider::sub-page:horizontal { background: #9b5cf6; border-radius: 3px; }
        QSlider::handle:horizontal {
            background: #d2b4ff; width: 16px; height: 16px; margin: -5px 0; border-radius: 8px;
        }
    )");
}

// =========================================================
// Images
// =========================================================

QImage MatchingWindow::toQImage(const cv::Mat &bgr) const
{
    // rgbSwapped() returns a deep copy, so the QImage does not depend on the cv::Mat memory.
    return QImage(bgr.data, bgr.cols, bgr.rows, static_cast<int>(bgr.step), QImage::Format_RGB888)
        .rgbSwapped();
}

bool MatchingWindow::loadImage(const QString &path, ImageData &dst, QImage &qimg)
{
    const std::string p = path.toStdString();
    cv::Mat color = cv::imread(p, cv::IMREAD_COLOR);
    cv::Mat gray  = cv::imread(p, cv::IMREAD_GRAYSCALE);
    if (color.empty() || gray.empty()) return false;

    dst.filePath = p;
    dst.colorImage = color;
    dst.grayscaleImage = gray;
    dst.isLoaded = true;
    qimg = toQImage(color);
    return true;
}

void MatchingWindow::setFirstImage(const ImageData &data)
{
    if (!data.isLoaded || data.colorImage.empty() || data.grayscaleImage.empty()) return;
    if (img1.isLoaded && img1.filePath == data.filePath) return;   // already Image 1

    img1 = data;
    qimg1 = toQImage(data.colorImage);
    feat1Ready = false;
    clearMatches();
    rebuildCanvas();
    updateDisplay();
}

void MatchingWindow::useSSD(bool ssd)
{
    radioSSD->setChecked(ssd);
    radioNCC->setChecked(!ssd);
}

void MatchingWindow::clearMatches()
{
    haveResult = false;
    lastResult = MatchResult();
}

void MatchingWindow::onLoadFirstClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open Image 1", "", "Images (*.png *.jpg *.jpeg)");
    if (path.isEmpty()) return;

    ImageData tmp;
    QImage tmpQ;
    if (!loadImage(path, tmp, tmpQ)) {
        QMessageBox::warning(this, "Feature Matching", "Could not load the image.");
        return;
    }
    img1 = tmp;
    qimg1 = tmpQ;
    feat1Ready = false;
    clearMatches();
    lblResults->setText("Image 1: " + QFileInfo(path).fileName());
    rebuildCanvas();
    updateDisplay();
}

void MatchingWindow::onLoadSecondClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open Image 2", "", "Images (*.png *.jpg *.jpeg)");
    if (path.isEmpty()) return;

    ImageData tmp;
    QImage tmpQ;
    if (!loadImage(path, tmp, tmpQ)) {
        QMessageBox::warning(this, "Feature Matching", "Could not load the image.");
        return;
    }
    img2 = tmp;
    qimg2 = tmpQ;
    feat2Ready = false;
    clearMatches();
    lblResults->setText("Image 2: " + QFileInfo(path).fileName());
    rebuildCanvas();
    updateDisplay();
}

// =========================================================
// Matching
// =========================================================

void MatchingWindow::onApplyClicked()
{
    if (!img1.isLoaded || !img2.isLoaded) {
        QMessageBox::warning(this, "Feature Matching", "Please load both Image 1 and Image 2 first.");
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // SIFT (Task 2) for each image, computed once and cached until the image changes
    if (!feat1Ready) {
        feat1 = FeatureDescription::computeSIFT(img1);
        feat1Ready = true;
    }
    if (!feat2Ready) {
        feat2 = FeatureDescription::computeSIFT(img2);
        feat2Ready = true;
    }

    if (feat1.descriptors.empty() || feat2.descriptors.empty()) {
        QApplication::restoreOverrideCursor();
        clearMatches();
        rebuildCanvas();
        updateDisplay();
        lblResults->setText("SIFT produced no descriptors for one of the images.");
        return;
    }

    const bool ssd = radioSSD->isChecked();
    const float ratio = sliderSSD->value() / 100.0f;
    lastResult = ssd ? FeatureMatching::matchSSD(feat1, feat2, ratio)   // SSD + Lowe's ratio test
                     : FeatureMatching::matchNCC(feat1, feat2);          // Task 4
    haveResult = !lastResult.matches.empty();

    QApplication::restoreOverrideCursor();

    rebuildCanvas();
    updateDisplay();

    const QString siftText = QString("SIFT keypoints: %1 / %2   (%3 / %4 ms)")
                                 .arg(feat1.keypoints.size())
                                 .arg(feat2.keypoints.size())
                                 .arg(feat1.computationTimeMs, 0, 'f', 1)
                                 .arg(feat2.computationTimeMs, 0, 'f', 1);
    if (haveResult) {
        lblResults->setText(QString("%1\n%2%3: %4 matches   ⏱ %5 ms")
                                .arg(siftText)
                                .arg(QString(ssd ? "SSD" : "NCC"))
                                .arg(ssd ? QString(" (ratio %1)").arg(ratio, 0, 'f', 2) : QString())
                                .arg(lastResult.matches.size())
                                .arg(lastResult.matchingTimeMs, 0, 'f', 3));
    } else if (ssd) {
        lblResults->setText(siftText + "\nNo matches pass the ratio test. Try a larger ratio.");
    } else {
        lblResults->setText(siftText + "\nNCC returned no matches (NCC matching is Task 4 and may not be implemented yet).");
    }
}

// =========================================================
// Drawing
// =========================================================

void MatchingWindow::rebuildCanvas()
{
    const bool has1 = !qimg1.isNull();
    const bool has2 = !qimg2.isNull();

    if (!has1 && !has2) {
        canvasFull = QPixmap();
        slotW1 = slotW2 = 1;
        return;
    }

    // A missing image gets an empty slot the size of the other one
    const int w1 = has1 ? qimg1.width()  : qimg2.width();
    const int h1 = has1 ? qimg1.height() : qimg2.height();
    const int w2 = has2 ? qimg2.width()  : qimg1.width();
    const int h2 = has2 ? qimg2.height() : qimg1.height();
    slotW1 = w1;
    slotW2 = w2;

    const int W = w1 + w2;
    const int H = std::max(h1, h2);

    QImage canvas(W, H, QImage::Format_RGB32);
    canvas.fill(QColor("#0b0714"));

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (has1) p.drawImage(0, 0, qimg1);
    else      drawPlaceholder(p, QRect(0, 0, w1, h1), "Image 1");
    if (has2) p.drawImage(w1, 0, qimg2);
    else      drawPlaceholder(p, QRect(w1, 0, w2, h2), "Load Second Image");

    // Matches: one coloured line per match + a ring on each end (same look as cv::drawMatches)
    if (haveResult) {
        const double lineWidth = std::max(1.5, W / 900.0);
        const double radius    = std::max(3.5, W / 380.0);

        for (size_t i = 0; i < lastResult.matches.size(); ++i) {
            const cv::DMatch &m = lastResult.matches[i];
            if (m.queryIdx < 0 || m.queryIdx >= static_cast<int>(feat1.keypoints.size())) continue;
            if (m.trainIdx < 0 || m.trainIdx >= static_cast<int>(feat2.keypoints.size())) continue;

            const cv::Point2f a = feat1.keypoints[m.queryIdx].pt;
            const cv::Point2f b = feat2.keypoints[m.trainIdx].pt;
            const QPointF pa(a.x, a.y);
            const QPointF pb(b.x + w1, b.y);

            QPen pen(QColor::fromHsv(static_cast<int>((i * 47) % 360), 200, 255));
            pen.setWidthF(lineWidth);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawLine(pa, pb);
            p.drawEllipse(pa, radius, radius);
            p.drawEllipse(pb, radius, radius);
        }
    }
    p.end();

    canvasFull = QPixmap::fromImage(canvas);
}

void MatchingWindow::updateDisplay()
{
    if (canvasFull.isNull()) {
        lblCanvas->setPixmap(QPixmap());
        lblCanvas->setText("Load Image 1 and Image 2 to begin");
        titlesLayout->setContentsMargins(0, 0, 0, 0);
        titlesLayout->setStretch(0, 1);
        titlesLayout->setStretch(1, 1);
        return;
    }

    const QSize avail = lblCanvas->size() - QSize(8, 8);
    const QPixmap shown = canvasFull.scaled(avail, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    lblCanvas->setText(QString());
    lblCanvas->setPixmap(shown);

    // Keep "Image 1" / "Image 2" centred over their own half of the picture
    const int pad = std::max(0, (lblCanvas->width() - shown.width()) / 2);
    titlesLayout->setContentsMargins(pad, 0, pad, 0);
    titlesLayout->setStretch(0, slotW1);
    titlesLayout->setStretch(1, slotW2);
}

void MatchingWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDisplay();
}
