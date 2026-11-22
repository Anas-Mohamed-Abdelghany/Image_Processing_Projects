#ifndef MATCHINGPANEL_H
#define MATCHINGPANEL_H

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include "DataStructures.h"

class QLabel;
class QPushButton;
class QSlider;
class QHBoxLayout;

// Delegate for the task list. A tab that also holds a ratio slider is taller than a normal
// tab, but only its first 44 px (the normal tab "pill": blue highlight + title) is painted by
// the style. The slider block below it therefore sits on the plain dark sidebar background.
// It also lets the slider block use the full row rectangle (no item padding).
class MatchTabDelegate : public QStyledItemDelegate
{
public:
    explicit MatchTabDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    static int tabHeight() { return 44; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        if (opt.rect.height() > tabHeight()) opt.rect.setHeight(tabHeight());
        QStyledItemDelegate::paint(painter, opt, index);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        editor->setGeometry(option.rect);   // the slider block uses the whole row
    }
};

// Two-image Feature Matching view for tasks 3 (SSD) and 4 (NCC).
//
// The panel itself only draws "Image 1 | Image 2" with the matches between them.
// Its controls live in the main window's sidebar (the main window places them):
//   - loadSecondButton()  -> "Load Second Image..." under "Load Image..."
//   - ratioWidget(ssd)    -> the ratio slider shown right below the SSD / NCC tab
//                            (on the dark sidebar background, tab keeps its normal size)
//   - resultsLabel()      -> the matching results, below "Run Task"
// "Run Task" calls run(). Each method (SSD / NCC) is its own task tab.
//
// Pipeline: FeatureDescription::computeSIFT (both images) -> matchSSD / matchNCC.
class MatchingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MatchingPanel(QWidget *parent = nullptr);

    // Image 1 = the image loaded in the main window (ignored if it is already Image 1).
    void setFirstImage(const ImageData &data);
    // Active method: true = SSD tab, false = NCC tab. Switching clears the previous matches.
    void setMethod(bool ssd);
    // Run SIFT (cached) + the active matching method and draw the matches.
    void run();

    // Sidebar widgets (created on first call, parented/placed by the caller)
    QPushButton *loadSecondButton(QWidget *parent);
    QWidget *ratioWidget(bool ssd, QWidget *parent);
    QLabel *resultsLabel(QWidget *parent);

    void setSidebarWidgetsVisible(bool visible);   // load-second button + results label
    void showRatio(bool ssd, bool visible);        // slider block under the SSD / NCC tab
    static int ratioPanelHeight() { return 58; }   // extra height the tab needs for the slider

signals:
    void statusMessage(const QString &message);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onLoadSecondClicked();

private:
    void buildUI();
    QImage toQImage(const cv::Mat &bgr) const;
    float ratioValue(bool ssd) const;
    void setResultsText(const QString &text);
    void clearMatches();
    void rebuildCanvas();   // compose Image 1 | Image 2 (+ match lines) at full resolution
    void updateDisplay();   // scale the composed canvas into the label

    // Canvas
    QLabel *lblTitle1 = nullptr;
    QLabel *lblTitle2 = nullptr;
    QHBoxLayout *titlesLayout = nullptr;
    QLabel *lblCanvas = nullptr;

    // Sidebar widgets (index 0 = SSD, 1 = NCC)
    QPushButton *btnLoadSecond = nullptr;
    QLabel *lblResults = nullptr;
    QWidget *ratioInner[2] = {nullptr, nullptr};
    QSlider *ratioSlider[2] = {nullptr, nullptr};

    // Data
    bool ssdActive = true;
    bool methodSet = false;
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

#endif // MATCHINGPANEL_H
