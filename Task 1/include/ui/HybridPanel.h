#pragma once

#include <QImage>
#include <QString>
#include <QWidget>

class QComboBox;
class QLabel;
class QSlider;

class ImageDisplayWidget;
class ImageProcessorController;

// ----------------------------------------------------------------------------
// HybridPanel (ui)
// The page MainWindow shows while "Hybrid Image" is the selected task:
//
//   +--------------------+--------------------+
//   | Image 1            | Image 2            |   the two input images
//   +--------------------+--------------------+
//   | filter type        | filter type        |   one filter + cutoff per image,
//   | cutoff  ----o--    | cutoff  ----o--    |   applied with its own button
//   | [Apply Filter]     | [Apply Filter]     |
//   +--------------------+--------------------+
//   | Image 1 filtered   | Image 2 filtered   |   each filter result, separately
//   +--------------------+--------------------+
//   |        [Create Hybrid Image]            |
//   |             Hybrid Result               |
//   +-----------------------------------------+
//
// Image 1 is the image loaded with "Open Image", image 2 the one loaded with
// "Open Second Image". All the work is done by the ImageProcessorController
// (Task 9 filters, Task 10 combination).
// ----------------------------------------------------------------------------
class HybridPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HybridPanel(ImageProcessorController* controller, QWidget* parent = nullptr);

    // A new first / second image was loaded: show it and clear what was
    // computed from the previous one.
    void setImage1(const QImage& image);
    void setImage2(const QImage& image);

public slots:
    // Builds the hybrid from the current filter settings of both images.
    void createHybrid();

signals:
    void statusMessage(const QString& text);

private:
    QWidget* createSettingsGroup(int index, bool highPassByDefault); // index 0 = image 1, 1 = image 2
    void applyFilter(int index);
    bool isHighPass(int index) const;
    int cutoffValue(int index) const;

    ImageProcessorController* m_controller = nullptr;

    ImageDisplayWidget* m_input[2] = {nullptr, nullptr};
    ImageDisplayWidget* m_filtered[2] = {nullptr, nullptr};
    QComboBox* m_filterCombo[2] = {nullptr, nullptr};
    QSlider* m_cutoffSlider[2] = {nullptr, nullptr};
    QLabel* m_cutoffLabel[2] = {nullptr, nullptr};
    ImageDisplayWidget* m_result = nullptr;
};
