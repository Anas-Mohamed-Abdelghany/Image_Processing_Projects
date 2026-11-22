#include "MatchingPanel.h"
#include "FeatureLogic.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include <opencv2/imgcodecs.hpp>

#include <algorithm>

namespace {
    void drawPlaceholder(QPainter &p, const QRect &r, const QString &text)
    {
        p.fillRect(r, QColor("#14171f"));
        p.setPen(QColor("#5b6478"));
        QFont f = p.font();
        f.setPixelSize(std::max(14, r.height() / 18));
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, text);
    }
}

MatchingPanel::MatchingPanel(QWidget *parent)
    : QWidget(parent)
{
    buildUI();
    rebuildCanvas();
    updateDisplay();
}

// =========================================================
// UI (only the two-image canvas; the controls live in the sidebar)
// =========================================================

void MatchingPanel::buildUI()
{
    // Same palette as the main window; the image box reuses its "imageBox" style.
    setStyleSheet("QLabel#matchTitle { color: #c9d1e0; font-size: 14px; font-weight: 700; }");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    titlesLayout = new QHBoxLayout();
    titlesLayout->setContentsMargins(0, 0, 0, 0);
    lblTitle1 = new QLabel("Image 1", this);
    lblTitle2 = new QLabel("Image 2", this);
    lblTitle1->setObjectName("matchTitle");
    lblTitle2->setObjectName("matchTitle");
    lblTitle1->setAlignment(Qt::AlignCenter);
    lblTitle2->setAlignment(Qt::AlignCenter);
    titlesLayout->addWidget(lblTitle1, 1);
    titlesLayout->addWidget(lblTitle2, 1);
    root->addLayout(titlesLayout);

    lblCanvas = new QLabel(this);
    lblCanvas->setObjectName("imageBox");
    lblCanvas->setAlignment(Qt::AlignCenter);
    lblCanvas->setMinimumSize(420, 300);
    lblCanvas->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    root->addWidget(lblCanvas, 1);
}

// =========================================================
// Sidebar widgets
// =========================================================

QPushButton *MatchingPanel::loadSecondButton(QWidget *parent)
{
    if (!btnLoadSecond) {
        btnLoadSecond = new QPushButton("  Load Second Image...", parent);
        btnLoadSecond->setObjectName("btnLoad");   // same look as "Load Image..."
        connect(btnLoadSecond, &QPushButton::clicked, this, &MatchingPanel::onLoadSecondClicked);
    }
    return btnLoadSecond;
}

QLabel *MatchingPanel::resultsLabel(QWidget *parent)
{
    if (!lblResults) {
        lblResults = new QLabel("Press Run Task to match.", parent);
        lblResults->setObjectName("results");      // same card as the main results box
        lblResults->setWordWrap(true);
        lblResults->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    }
    return lblResults;
}

// Block shown right below the SSD / NCC task tab (item widget of that task-list row).
// The top 44 px are a transparent spacer covering the tab itself (its blue pill and title are
// painted by MatchTabDelegate); the slider block underneath sits on the dark sidebar background.
// It is only shown while its tab is selected (showRatio), so unselected tabs keep their size.
QWidget *MatchingPanel::ratioWidget(bool ssd, QWidget *parent)
{
    const int idx = ssd ? 0 : 1;
    if (ratioInner[idx]) return ratioInner[idx]->parentWidget();

    QWidget *holder = new QWidget(parent);
    holder->setStyleSheet(R"(
        QLabel#ratioName  { color: #8b93a7; font-size: 12px; font-weight: 600; background: transparent; }
        QLabel#ratioValue { color: #4f8cff; font-size: 12px; font-weight: 700; background: transparent; }
        QSlider::groove:horizontal { height: 5px; background: #2f3546; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #4f8cff; border-radius: 2px; }
        QSlider::handle:horizontal {
            background: #e6e9f0; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px;
        }
    )");

    QVBoxLayout *v = new QVBoxLayout(holder);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    QWidget *spacer = new QWidget(holder);
    spacer->setFixedHeight(MatchTabDelegate::tabHeight());
    v->addWidget(spacer);

    QWidget *inner = new QWidget(holder);
    inner->setFixedHeight(ratioPanelHeight());
    QVBoxLayout *iv = new QVBoxLayout(inner);
    iv->setContentsMargins(14, 4, 14, 10);
    iv->setSpacing(6);

    QHBoxLayout *row = new QHBoxLayout();
    QLabel *name = new QLabel(ssd ? "SSD Ratio Test:" : "NCC Ratio Test:", inner);
    name->setObjectName("ratioName");
    QLabel *value = new QLabel("0.65", inner);
    value->setObjectName("ratioValue");
    row->addWidget(name, 1);
    row->addWidget(value);
    iv->addLayout(row);

    QSlider *slider = new QSlider(Qt::Horizontal, inner);
    slider->setRange(1, 100);
    slider->setValue(65);
    iv->addWidget(slider);
    connect(slider, &QSlider::valueChanged, this, [value](int v2) {
        value->setText(QString::number(v2 / 100.0, 'f', 2));
    });

    v->addWidget(inner);
    inner->hide();   // shown only while this tab is selected

    ratioInner[idx] = inner;
    ratioSlider[idx] = slider;
    return holder;
}

void MatchingPanel::setSidebarWidgetsVisible(bool visible)
{
    if (btnLoadSecond) btnLoadSecond->setVisible(visible);
    if (lblResults) lblResults->setVisible(visible);
}

void MatchingPanel::showRatio(bool ssd, bool visible)
{
    QWidget *inner = ratioInner[ssd ? 0 : 1];
    if (inner) inner->setVisible(visible);
}

float MatchingPanel::ratioValue(bool ssd) const
{
    const QSlider *s = ratioSlider[ssd ? 0 : 1];
    return s ? s->value() / 100.0f : 0.65f;
}

void MatchingPanel::setResultsText(const QString &text)
{
    if (lblResults) lblResults->setText(text);
}

// =========================================================
// Images
// =========================================================

QImage MatchingPanel::toQImage(const cv::Mat &bgr) const
{
    // rgbSwapped() returns a deep copy, so the QImage does not depend on the cv::Mat memory.
    return QImage(bgr.data, bgr.cols, bgr.rows, static_cast<int>(bgr.step), QImage::Format_RGB888)
        .rgbSwapped();
}

void MatchingPanel::setFirstImage(const ImageData &data)
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

void MatchingPanel::setMethod(bool ssd)
{
    if (methodSet && ssdActive == ssd) return;
    methodSet = true;
    ssdActive = ssd;

    // The drawn matches belong to the other method: start clean
    clearMatches();
    rebuildCanvas();
    updateDisplay();
    setResultsText(QString("%1 matching: press Run Task.").arg(QString(ssd ? "SSD" : "NCC")));
}

void MatchingPanel::clearMatches()
{
    haveResult = false;
    lastResult = MatchResult();
}

void MatchingPanel::onLoadSecondClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open Second Image", "", "Images (*.png *.jpg *.jpeg)");
    if (path.isEmpty()) return;

    const std::string p = path.toStdString();
    cv::Mat color = cv::imread(p, cv::IMREAD_COLOR);
    cv::Mat gray  = cv::imread(p, cv::IMREAD_GRAYSCALE);
    if (color.empty() || gray.empty()) {
        QMessageBox::warning(this, "Feature Matching", "Could not load the image.");
        return;
    }

    img2.filePath = p;
    img2.colorImage = color;
    img2.grayscaleImage = gray;
    img2.isLoaded = true;
    qimg2 = toQImage(color);
    feat2Ready = false;
    clearMatches();

    setResultsText("Image 2: " + QFileInfo(path).fileName() + "\nPress Run Task.");
    emit statusMessage("Loaded second image: " + QFileInfo(path).fileName());
    rebuildCanvas();
    updateDisplay();
}

// =========================================================
// Matching (called by the sidebar's "Run Task")
// =========================================================

void MatchingPanel::run()
{
    if (!img1.isLoaded) {
        QMessageBox::warning(this, "Feature Matching", "Load Image 1 first (Load Image... in the sidebar).");
        return;
    }
    if (!img2.isLoaded) {
        QMessageBox::warning(this, "Feature Matching", "Load the second image first (Load Second Image... in the sidebar).");
        return;
    }

    emit statusMessage("Running SIFT + matching...");
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
        setResultsText("SIFT produced no descriptors for one of the images.");
        emit statusMessage("Matching failed.");
        return;
    }

    const bool ssd = ssdActive;
    const float ratio = ratioValue(ssd);
        lastResult = ssd ? FeatureMatching::matchSSD(feat1, feat2, ratio)   // SSD + Lowe's ratio test
                         : FeatureMatching::matchNCC(feat1, feat2, ratio);  // NCC + Lowe's ratio test
    haveResult = !lastResult.matches.empty();

    QApplication::restoreOverrideCursor();

    rebuildCanvas();
    updateDisplay();

    const QString siftText = QString("SIFT keypoints: %1 / %2\nSIFT time: %3 / %4 ms")
                                 .arg(feat1.keypoints.size())
                                 .arg(feat2.keypoints.size())
                                 .arg(feat1.computationTimeMs, 0, 'f', 1)
                                 .arg(feat2.computationTimeMs, 0, 'f', 1);
    if (haveResult) {
        setResultsText(QString("%1\n%2%3: %4 matches\nMatching time: %5 ms")
                           .arg(siftText)
                           .arg(QString(ssd ? "SSD" : "NCC"))
                           .arg(QString(" (ratio %1)").arg(ratio, 0, 'f', 2))
                           .arg(lastResult.matches.size())
                           .arg(lastResult.matchingTimeMs, 0, 'f', 3));
        emit statusMessage(QString("Matching complete: %1 matches.").arg(lastResult.matches.size()));
    } 
     else {
        setResultsText(siftText + "\nNCC returned no matches (NCC is Task 4 and may not be implemented yet).");
        emit statusMessage("No matches found.");
    }
}

// =========================================================
// Drawing
// =========================================================

void MatchingPanel::rebuildCanvas()
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
    canvas.fill(QColor("#0e1015"));

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

void MatchingPanel::updateDisplay()
{
    if (canvasFull.isNull()) {
        lblCanvas->setPixmap(QPixmap());
        lblCanvas->setText("Load the second image to compare");
        titlesLayout->setContentsMargins(0, 0, 0, 0);
        titlesLayout->setStretch(0, 1);
        titlesLayout->setStretch(1, 1);
        return;
    }

    const QSize avail = lblCanvas->size() - QSize(24, 24);
    if (avail.width() <= 0 || avail.height() <= 0) return;

    const QPixmap shown = canvasFull.scaled(avail, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    lblCanvas->setText(QString());
    lblCanvas->setPixmap(shown);

    // Keep "Image 1" / "Image 2" centred over their own half of the picture
    const int pad = std::max(0, (lblCanvas->width() - shown.width()) / 2);
    titlesLayout->setContentsMargins(pad, 0, pad, 0);
    titlesLayout->setStretch(0, slotW1);
    titlesLayout->setStretch(1, slotW2);
}

void MatchingPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDisplay();
}
