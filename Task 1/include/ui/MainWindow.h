#pragma once

#include <QImage>
#include <QMap>
#include <QMainWindow>
#include <QStringList>
#include <QVector>

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QVBoxLayout;

class ImageDisplayWidget;
class ImageProcessorController;

// ----------------------------------------------------------------------------
// MainWindow (ui)
// The complete control panel for all 10 tasks:
//
//   File group      -> open image / open 2nd image (Task 10) / save result
//   Tasks group     -> one drop down per task group (noise / filter / edges ...)
//                      each listing that group's operations
//   Parameters group-> only the parameters of the selected operation are shown
//   Action group    -> run the operation / show original / reset
//
// The left side shows THREE previews side by side:
//
//   Box 1  Original     -> always the image that was loaded
//   Box 2  Result A     -> a result you sent there
//   Box 3  Result B     -> a result you sent there
//
// "Show Result in Box 2" / "Show Result in Box 3" put the latest result into
// the chosen box, so two variants of the same task (e.g. Sobel in X and in Y
// direction) can be compared next to each other and next to the original.
//
// Every button is already connected to ImageProcessorController, which in turn
// calls the Task 0-10 services. Teammates only implement their service bodies.
// ----------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void openImage();
    void openSecondImage();
    void saveResult();
    void runCurrentOperation();
    void showOriginal();
    void resetToOriginal();
    void onTaskSelectionChanged(int index);
    void onResultReady(const QImage& image);
    void onErrorOccurred(const QString& message);

    // Where the next result should be shown.
    void showResultInBox2();
    void showResultInBox3();

private:
    struct ParameterRow {
        QLabel* label = nullptr;
        QWidget* control = nullptr;
    };

    // Every value here MUST also be added to the task drop downs
    // (see createOperationGroup()). None means "no operation selected yet".
    enum class Operation {
        None,                // nothing selected
        ShowOriginal,        // Task 0
        UniformNoise,        // Task 1
        GaussianNoise,       // Task 1
        SaltPepperNoise,     // Task 1
        AverageFilter,       // Task 2
        GaussianFilter,      // Task 2
        MedianFilter,        // Task 2
        SobelEdges,          // Task 3
        RobertsEdges,        // Task 3
        PrewittEdges,        // Task 3
        CannyEdges,          // Task 3
        Histogram,           // Task 4
        CumulativeCurve,     // Task 4
        Equalization,        // Task 5
        Normalization,       // Task 6
        Thresholding,        // Task 7
        GrayscaleConversion, // Task 8
        ChannelHistograms,   // Task 8
        FrequencyLowPass,    // Task 9
        FrequencyHighPass,   // Task 9
        HybridImage          // Task 10
    };

    void buildUi();
    QWidget* createFileGroup();
    QWidget* createOperationGroup();
    QWidget* createParameterGroup();
    QWidget* createActionGroup();
    void connectSignals();

    void addParameterRow(QGridLayout* grid, int row, const QString& key,
                         const QString& labelText, QWidget* control);
    void showParameters(const QStringList& keys);
    void updateParameterVisibility();

    // Marks one task drop down as the active one (only one entry can be
    // selected at a time - picking an entry resets the other drop downs).
    void selectOperation(Operation operation, QComboBox* source);

    // Shows the latest result in the currently selected result box and marks
    // which box is the active one.
    void displayResultInActiveBox();

    // --- core ---------------------------------------------------------------
    ImageDisplayWidget* m_originalBox = nullptr;
    ImageDisplayWidget* m_resultBox2 = nullptr;
    ImageDisplayWidget* m_resultBox3 = nullptr;
    ImageProcessorController* m_controller = nullptr;

    // The two result boxes are a small stack: Box 2 is the default target,
    // Box 3 lets a second variant be compared side by side.
    enum class ResultBox { Box2, Box3 };
    ResultBox m_activeResultBox = ResultBox::Box2;

    QImage m_lastResult;  // latest result produced by the controller
    bool m_routeToOriginal = true; // next result goes to the original box

    // --- task selectors -------------------------------------------------------
    // One drop down per task group: the user picks e.g. "Task 3 - Sobel"
    // from the edge detection drop down, "Task 2 - Gaussian" from the filter
    // drop down, and so on. Picking an entry resets all the other drop downs
    // to "(select a task)" so exactly one operation is ever active.
    QVector<QComboBox*> m_taskCombos;
    Operation m_activeOperation = Operation::None;

    QComboBox* m_kernelCombo = nullptr;
    QComboBox* m_directionCombo = nullptr;

    // --- parameters ---------------------------------------------------------
    QSpinBox* m_amplitudeSpin = nullptr;
    QSpinBox* m_threshSpin = nullptr;
    QSpinBox* m_maxValSpin = nullptr;
    QSpinBox* m_cutoffSpin = nullptr;

    QDoubleSpinBox* m_meanSpin = nullptr;
    QDoubleSpinBox* m_stddevSpin = nullptr;
    QDoubleSpinBox* m_probSpin = nullptr;
    QDoubleSpinBox* m_sigmaSpin = nullptr;
    QDoubleSpinBox* m_cannyLowSpin = nullptr;
    QDoubleSpinBox* m_cannyHighSpin = nullptr;

    QMap<QString, ParameterRow> m_paramRows;

    // --- buttons ------------------------------------------------------------
    QPushButton* m_openButton = nullptr;
    QPushButton* m_openSecondButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_runButton = nullptr;
    QPushButton* m_originalButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_showInBox2Button = nullptr;
    QPushButton* m_showInBox3Button = nullptr;

    // When checked (default) every operation is applied to the ORIGINAL image
    // instead of chaining onto the previous result. Comparing Sobel in X with
    // Sobel in Y is only meaningful that way.
    QCheckBox* m_resetBeforeOpCheck = nullptr;

    // --- status -------------------------------------------------------------
    QString m_pendingStatus;
};
