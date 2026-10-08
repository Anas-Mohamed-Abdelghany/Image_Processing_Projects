#pragma once

#include <QImage>
#include <QMap>
#include <QMainWindow>
#include <QStringList>

class QComboBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;

class ImageDisplayWidget;
class ImageProcessorController;

// ----------------------------------------------------------------------------
// MainWindow (ui)
// The complete control panel for all 10 tasks:
//
//   File group      -> open image / open 2nd image (Task 10) / save result
//   Task group      -> pick one of the 20 operations (Task 0 ... Task 10)
//   Parameters group-> only the parameters of the selected operation are shown
//   Action group    -> run the operation / show original / reset
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
    void onOperationChanged(int index);
    void onResultReady(const QImage& image);
    void onErrorOccurred(const QString& message);

private:
    struct ParameterRow {
        QLabel* label = nullptr;
        QWidget* control = nullptr;
    };

    // Every value here MUST also be added to m_operationCombo (see createOperationGroup()).
    enum class Operation {
        ShowOriginal,         // Task 0
        UniformNoise,         // Task 1
        GaussianNoise,        // Task 1
        SaltPepperNoise,      // Task 1
        AverageFilter,        // Task 2
        GaussianFilter,       // Task 2
        MedianFilter,         // Task 2
        SobelEdges,           // Task 3
        RobertsEdges,         // Task 3
        PrewittEdges,         // Task 3
        CannyEdges,           // Task 3
        Histogram,            // Task 4
        Equalization,         // Task 5
        Normalization,        // Task 6
        Thresholding,         // Task 7
        GrayscaleConversion,  // Task 8
        ChannelHistograms,    // Task 8
        FrequencyLowPass,     // Task 9
        FrequencyHighPass,    // Task 9
        HybridImage           // Task 10
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

    // --- core ---------------------------------------------------------------
    ImageDisplayWidget* m_display = nullptr;
    ImageProcessorController* m_controller = nullptr;

    // --- selectors ----------------------------------------------------------
    QComboBox* m_operationCombo = nullptr;
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

    // --- status -------------------------------------------------------------
    QString m_pendingStatus;
};
