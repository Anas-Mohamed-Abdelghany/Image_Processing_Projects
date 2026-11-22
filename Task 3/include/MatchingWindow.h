#ifndef MATCHINGWINDOW_H
#define MATCHINGWINDOW_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include "DataStructures.h"

class QLabel;
class QPushButton;
class QRadioButton;
class QSlider;
class QComboBox;
class QHBoxLayout;

// Two-image Feature Matching window (Image 1 | Image 2 with the matches drawn between them).
// Pipeline: FeatureDescription::computeSIFT (both images) -> FeatureMatching::matchSSD / matchNCC.
class MatchingWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MatchingWindow(QWidget *parent = nullptr);

    // Image 1 comes from the main window (ignored if it is already the current Image 1).
    void setFirstImage(const ImageData &data);
    // Pre-select the matching method radio button.
    void useSSD(bool ssd);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onLoadFirstClicked();
    void onLoadSecondClicked();
    void onApplyClicked();

private:
    void buildUI();
    void applyStyles();
    bool loadImage(const QString &path, ImageData &dst, QImage &qimg);
    QImage toQImage(const cv::Mat &bgr) const;
    void clearMatches();
    void rebuildCanvas();   // compose Image1 | Image2 (+ match lines) at full resolution
    void updateDisplay();   // scale the composed canvas into the label

    // Widgets
    QPushButton *btnLoadFirst = nullptr;
    QPushButton *btnLoadSecond = nullptr;
    QPushButton *btnApply = nullptr;
    QComboBox *comboMode = nullptr;
    QRadioButton *radioSSD = nullptr;
    QRadioButton *radioNCC = nullptr;
    QSlider *sliderSSD = nullptr;
    QSlider *sliderNCC = nullptr;
    QLabel *lblSSDValue = nullptr;
    QLabel *lblNCCValue = nullptr;
    QLabel *lblTitle1 = nullptr;
    QLabel *lblTitle2 = nullptr;
    QHBoxLayout *titlesLayout = nullptr;
    QLabel *lblCanvas = nullptr;
    QLabel *lblResults = nullptr;

    // Data
    ImageData img1, img2;
    QImage qimg1, qimg2;
    FeatureSet feat1, feat2;
    bool feat1Ready = false;
    bool feat2Ready = false;
    MatchResult lastResult;
    bool haveResult = false;

    QPixmap canvasFull;     // composed canvas (full resolution)
    int slotW1 = 1;         // width of the Image 1 slot in the canvas
    int slotW2 = 1;         // width of the Image 2 slot in the canvas
};

#endif // MATCHINGWINDOW_H
