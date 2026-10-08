#include "ui/MainWindow.h"

#include "backend/ImageProcessorController.h"
#include "services/TaskTypes.h"
#include "ui/ImageDisplayWidget.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_controller(new ImageProcessorController(this))
{
    setWindowTitle(tr("CV Assignment - Image Processing Tasks"));
    resize(1240, 780);

    buildUi();
    connectSignals();
    updateParameterVisibility();

    statusBar()->showMessage(tr("Open an image to begin."));
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    m_display = new ImageDisplayWidget(central);

    auto* panel = new QWidget(central);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(10);

    panelLayout->addWidget(createFileGroup());
    panelLayout->addWidget(createOperationGroup());
    panelLayout->addWidget(createParameterGroup());
    panelLayout->addWidget(createActionGroup());
    panelLayout->addStretch(1);

    mainLayout->addWidget(m_display, 1);
    mainLayout->addWidget(panel, 0);

    setCentralWidget(central);
}

QWidget* MainWindow::createFileGroup()
{
    auto* group = new QGroupBox(tr("File"), this);
    auto* layout = new QGridLayout(group);

    m_openButton = new QPushButton(tr("Open Image..."), group);
    m_openSecondButton = new QPushButton(tr("Open Second Image (Task 10)..."), group);
    m_saveButton = new QPushButton(tr("Save Result..."), group);

    layout->addWidget(m_openButton, 0, 0);
    layout->addWidget(m_openSecondButton, 0, 1);
    layout->addWidget(m_saveButton, 1, 0, 1, 2);

    return group;
}

QWidget* MainWindow::createOperationGroup()
{
    auto* group = new QGroupBox(tr("Task / Operation"), this);
    auto* layout = new QVBoxLayout(group);

    m_operationCombo = new QComboBox(group);
    m_operationCombo->addItem(tr("Task 0  -  Read / Show Image"), static_cast<int>(Operation::ShowOriginal));
    m_operationCombo->addItem(tr("Task 1  -  Uniform Noise"), static_cast<int>(Operation::UniformNoise));
    m_operationCombo->addItem(tr("Task 1  -  Gaussian Noise"), static_cast<int>(Operation::GaussianNoise));
    m_operationCombo->addItem(tr("Task 1  -  Salt and Pepper Noise"), static_cast<int>(Operation::SaltPepperNoise));
    m_operationCombo->addItem(tr("Task 2  -  Average Filter"), static_cast<int>(Operation::AverageFilter));
    m_operationCombo->addItem(tr("Task 2  -  Gaussian Filter"), static_cast<int>(Operation::GaussianFilter));
    m_operationCombo->addItem(tr("Task 2  -  Median Filter"), static_cast<int>(Operation::MedianFilter));
    m_operationCombo->addItem(tr("Task 3  -  Sobel Edges"), static_cast<int>(Operation::SobelEdges));
    m_operationCombo->addItem(tr("Task 3  -  Roberts Edges"), static_cast<int>(Operation::RobertsEdges));
    m_operationCombo->addItem(tr("Task 3  -  Prewitt Edges"), static_cast<int>(Operation::PrewittEdges));
    m_operationCombo->addItem(tr("Task 3  -  Canny Edges"), static_cast<int>(Operation::CannyEdges));
    m_operationCombo->addItem(tr("Task 4  -  Histogram + CDF"), static_cast<int>(Operation::Histogram));
    m_operationCombo->addItem(tr("Task 5  -  Histogram Equalization"), static_cast<int>(Operation::Equalization));
    m_operationCombo->addItem(tr("Task 6  -  Normalization"), static_cast<int>(Operation::Normalization));
    m_operationCombo->addItem(tr("Task 7  -  Thresholding"), static_cast<int>(Operation::Thresholding));
    m_operationCombo->addItem(tr("Task 8  -  RGB to Grayscale"), static_cast<int>(Operation::GrayscaleConversion));
    m_operationCombo->addItem(tr("Task 8  -  R/G/B Channel Histograms + CDF"), static_cast<int>(Operation::ChannelHistograms));
    m_operationCombo->addItem(tr("Task 9  -  Frequency Low Pass"), static_cast<int>(Operation::FrequencyLowPass));
    m_operationCombo->addItem(tr("Task 9  -  Frequency High Pass"), static_cast<int>(Operation::FrequencyHighPass));
    m_operationCombo->addItem(tr("Task 10 -  Hybrid Image"), static_cast<int>(Operation::HybridImage));

    layout->addWidget(m_operationCombo);
    return group;
}

QWidget* MainWindow::createParameterGroup()
{
    auto* group = new QGroupBox(tr("Parameters"), this);
    auto* grid = new QGridLayout(group);

    // Row 0 - noise amplitude (Task 1, uniform)
    m_amplitudeSpin = new QSpinBox(group);
    m_amplitudeSpin->setRange(1, 255);
    m_amplitudeSpin->setValue(50);
    addParameterRow(grid, 0, QStringLiteral("amplitude"), tr("Noise amplitude"), m_amplitudeSpin);

    // Row 1 - Gaussian mean (Task 1)
    m_meanSpin = new QDoubleSpinBox(group);
    m_meanSpin->setRange(-255.0, 255.0);
    m_meanSpin->setDecimals(2);
    m_meanSpin->setValue(0.0);
    addParameterRow(grid, 1, QStringLiteral("mean"), tr("Gaussian mean"), m_meanSpin);

    // Row 2 - Gaussian stddev (Task 1)
    m_stddevSpin = new QDoubleSpinBox(group);
    m_stddevSpin->setRange(0.0, 255.0);
    m_stddevSpin->setDecimals(2);
    m_stddevSpin->setValue(25.0);
    addParameterRow(grid, 2, QStringLiteral("stddev"), tr("Gaussian stddev"), m_stddevSpin);

    // Row 3 - salt & pepper probability (Task 1)
    m_probSpin = new QDoubleSpinBox(group);
    m_probSpin->setRange(0.0, 1.0);
    m_probSpin->setDecimals(3);
    m_probSpin->setSingleStep(0.01);
    m_probSpin->setValue(0.05);
    addParameterRow(grid, 3, QStringLiteral("probability"), tr("Noise probability"), m_probSpin);

    // Row 4 - kernel size (Task 2)
    m_kernelCombo = new QComboBox(group);
    m_kernelCombo->addItem(tr("3 x 3"), 3);
    m_kernelCombo->addItem(tr("5 x 5"), 5);
    m_kernelCombo->addItem(tr("7 x 7"), 7);
    m_kernelCombo->addItem(tr("9 x 9"), 9);
    addParameterRow(grid, 4, QStringLiteral("kernel"), tr("Kernel size"), m_kernelCombo);

    // Row 5 - Gaussian sigma (Task 2)
    m_sigmaSpin = new QDoubleSpinBox(group);
    m_sigmaSpin->setRange(0.1, 100.0);
    m_sigmaSpin->setDecimals(1);
    m_sigmaSpin->setValue(1.0);
    addParameterRow(grid, 5, QStringLiteral("sigma"), tr("Gaussian sigma"), m_sigmaSpin);

    // Row 6 - gradient direction (Task 3, Sobel / Roberts / Prewitt)
    m_directionCombo = new QComboBox(group);
    m_directionCombo->addItem(tr("X direction"), static_cast<int>(Direction::X));
    m_directionCombo->addItem(tr("Y direction"), static_cast<int>(Direction::Y));
    addParameterRow(grid, 6, QStringLiteral("direction"), tr("Direction"), m_directionCombo);

    // Row 7 - Canny low threshold (Task 3)
    m_cannyLowSpin = new QDoubleSpinBox(group);
    m_cannyLowSpin->setRange(0.0, 255.0);
    m_cannyLowSpin->setDecimals(0);
    m_cannyLowSpin->setValue(50.0);
    addParameterRow(grid, 7, QStringLiteral("cannyLow"), tr("Canny low threshold"), m_cannyLowSpin);

    // Row 8 - Canny high threshold (Task 3)
    m_cannyHighSpin = new QDoubleSpinBox(group);
    m_cannyHighSpin->setRange(0.0, 255.0);
    m_cannyHighSpin->setDecimals(0);
    m_cannyHighSpin->setValue(150.0);
    addParameterRow(grid, 8, QStringLiteral("cannyHigh"), tr("Canny high threshold"), m_cannyHighSpin);

    // Row 9 - threshold value (Task 7)
    m_threshSpin = new QSpinBox(group);
    m_threshSpin->setRange(0, 255);
    m_threshSpin->setValue(127);
    addParameterRow(grid, 9, QStringLiteral("threshold"), tr("Threshold value"), m_threshSpin);

    // Row 10 - maximum value (Task 7)
    m_maxValSpin = new QSpinBox(group);
    m_maxValSpin->setRange(0, 255);
    m_maxValSpin->setValue(255);
    addParameterRow(grid, 10, QStringLiteral("maxValue"), tr("Maximum value"), m_maxValSpin);

    // Row 11 - frequency cut-off radius (Task 9 / Task 10)
    m_cutoffSpin = new QSpinBox(group);
    m_cutoffSpin->setRange(1, 10000);
    m_cutoffSpin->setValue(30);
    addParameterRow(grid, 11, QStringLiteral("cutoff"), tr("Cutoff radius"), m_cutoffSpin);

    grid->setColumnStretch(1, 1);
    return group;
}

QWidget* MainWindow::createActionGroup()
{
    auto* group = new QGroupBox(tr("Actions"), this);
    auto* layout = new QGridLayout(group);

    m_runButton = new QPushButton(tr("Run Operation"), group);
    m_runButton->setDefault(true);

    m_originalButton = new QPushButton(tr("Show Original"), group);
    m_resetButton = new QPushButton(tr("Reset to Original"), group);

    layout->addWidget(m_runButton, 0, 0, 1, 2);
    layout->addWidget(m_originalButton, 1, 0);
    layout->addWidget(m_resetButton, 1, 1);

    return group;
}

void MainWindow::connectSignals()
{
    // Buttons -> this window
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(m_openSecondButton, &QPushButton::clicked, this, &MainWindow::openSecondImage);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveResult);
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::runCurrentOperation);
    connect(m_originalButton, &QPushButton::clicked, this, &MainWindow::showOriginal);
    connect(m_resetButton, &QPushButton::clicked, this, &MainWindow::resetToOriginal);

    // Operation selector -> parameter visibility
    connect(m_operationCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onOperationChanged);

    // Controller -> this window (errors + status) and the display (results)
    connect(m_controller, &ImageProcessorController::resultReady,
            this, &MainWindow::onResultReady);
    connect(m_controller, &ImageProcessorController::errorOccurred,
            this, &MainWindow::onErrorOccurred);
}

// ---------------------------------------------------------------------------
// Parameter rows
// ---------------------------------------------------------------------------

void MainWindow::addParameterRow(QGridLayout* grid, int row, const QString& key,
                                 const QString& labelText, QWidget* control)
{
    auto* label = new QLabel(labelText, grid->parentWidget());
    label->setBuddy(control);

    grid->addWidget(label, row, 0);
    grid->addWidget(control, row, 1);

    m_paramRows.insert(key, ParameterRow{label, control});
}

void MainWindow::showParameters(const QStringList& keys)
{
    for (auto it = m_paramRows.cbegin(); it != m_paramRows.cend(); ++it) {
        const bool visible = keys.contains(it.key());
        it.value().label->setVisible(visible);
        it.value().control->setVisible(visible);
    }
}

void MainWindow::updateParameterVisibility()
{
    const Operation op = static_cast<Operation>(m_operationCombo->currentData().toInt());

    switch (op) {
    case Operation::ShowOriginal:
        showParameters({});
        break;
    case Operation::UniformNoise:
        showParameters({QStringLiteral("amplitude")});
        break;
    case Operation::GaussianNoise:
        showParameters({QStringLiteral("mean"), QStringLiteral("stddev")});
        break;
    case Operation::SaltPepperNoise:
        showParameters({QStringLiteral("probability")});
        break;
    case Operation::AverageFilter:
    case Operation::MedianFilter:
        showParameters({QStringLiteral("kernel")});
        break;
    case Operation::GaussianFilter:
        showParameters({QStringLiteral("kernel"), QStringLiteral("sigma")});
        break;
    case Operation::SobelEdges:
    case Operation::RobertsEdges:
    case Operation::PrewittEdges:
        showParameters({QStringLiteral("direction")});
        break;
    case Operation::CannyEdges:
        showParameters({QStringLiteral("cannyLow"), QStringLiteral("cannyHigh")});
        break;
    case Operation::Histogram:
    case Operation::Equalization:
    case Operation::Normalization:
    case Operation::GrayscaleConversion:
    case Operation::ChannelHistograms:
        showParameters({});
        break;
    case Operation::Thresholding:
        showParameters({QStringLiteral("threshold"), QStringLiteral("maxValue")});
        break;
    case Operation::FrequencyLowPass:
    case Operation::FrequencyHighPass:
    case Operation::HybridImage:
        showParameters({QStringLiteral("cutoff")});
        break;
    }
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void MainWindow::openImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Image"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All Files (*)"));
    if (path.isEmpty()) {
        return;
    }

    m_pendingStatus = tr("Loaded: %1").arg(path);
    if (!m_controller->loadImage(path)) {
        m_pendingStatus.clear();
    }
}

void MainWindow::openSecondImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Second Image (for Task 10 - Hybrid)"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All Files (*)"));
    if (path.isEmpty()) {
        return;
    }

    if (m_controller->loadSecondImage(path)) {
        statusBar()->showMessage(tr("Second image loaded: %1").arg(path), 4000);
    }
}

void MainWindow::saveResult()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Result"), QString(),
        tr("PNG Image (*.png);;JPEG Image (*.jpg);;Bitmap (*.bmp)"));
    if (path.isEmpty()) {
        return;
    }

    if (m_controller->saveImage(path)) {
        statusBar()->showMessage(tr("Saved: %1").arg(path), 4000);
    }
}

void MainWindow::runCurrentOperation()
{
    const Operation op = static_cast<Operation>(m_operationCombo->currentData().toInt());

    if (op == Operation::ShowOriginal) {
        showOriginal();
        return;
    }

    // If the controller fails it emits errorOccurred first, which clears this
    // again - so the status only ever shows success.
    m_pendingStatus = tr("Applied: %1").arg(m_operationCombo->currentText());

    const int ksize = m_kernelCombo->currentData().toInt();
    const Direction direction = static_cast<Direction>(m_directionCombo->currentData().toInt());

    switch (op) {
    case Operation::ShowOriginal:
        break; // handled above

    // Task 1
    case Operation::UniformNoise:
        m_controller->addUniformNoise(m_amplitudeSpin->value());
        break;
    case Operation::GaussianNoise:
        m_controller->addGaussianNoise(m_meanSpin->value(), m_stddevSpin->value());
        break;
    case Operation::SaltPepperNoise:
        m_controller->addSaltAndPepperNoise(m_probSpin->value());
        break;

    // Task 2
    case Operation::AverageFilter:
        m_controller->applyAverageFilter(ksize);
        break;
    case Operation::GaussianFilter:
        m_controller->applyGaussianFilter(ksize, m_sigmaSpin->value());
        break;
    case Operation::MedianFilter:
        m_controller->applyMedianFilter(ksize);
        break;

    // Task 3
    case Operation::SobelEdges:
        m_controller->applyEdges(EdgeMethod::Sobel, direction);
        break;
    case Operation::RobertsEdges:
        m_controller->applyEdges(EdgeMethod::Roberts, direction);
        break;
    case Operation::PrewittEdges:
        m_controller->applyEdges(EdgeMethod::Prewitt, direction);
        break;
    case Operation::CannyEdges:
        m_controller->applyCanny(m_cannyLowSpin->value(), m_cannyHighSpin->value());
        break;

    // Task 4
    case Operation::Histogram:
        m_controller->showHistogram();
        break;

    // Task 5
    case Operation::Equalization:
        m_controller->applyEqualization();
        break;

    // Task 6
    case Operation::Normalization:
        m_controller->applyNormalization();
        break;

    // Task 7
    case Operation::Thresholding:
        m_controller->applyThreshold(m_threshSpin->value(), m_maxValSpin->value());
        break;

    // Task 8
    case Operation::GrayscaleConversion:
        m_controller->applyGrayscaleConversion();
        break;
    case Operation::ChannelHistograms:
        m_controller->showChannelHistograms();
        break;

    // Task 9
    case Operation::FrequencyLowPass:
        m_controller->applyFrequencyLowPass(m_cutoffSpin->value());
        break;
    case Operation::FrequencyHighPass:
        m_controller->applyFrequencyHighPass(m_cutoffSpin->value());
        break;

    // Task 10
    case Operation::HybridImage:
        m_controller->applyHybridImage(m_cutoffSpin->value());
        break;
    }
}

void MainWindow::showOriginal()
{
    m_pendingStatus = tr("Showing the original image");
    m_controller->showOriginal();
}

void MainWindow::resetToOriginal()
{
    m_pendingStatus = tr("Reset - all operations discarded");
    m_controller->resetToOriginal();
}

void MainWindow::onOperationChanged(int index)
{
    Q_UNUSED(index)
    updateParameterVisibility();
}

void MainWindow::onResultReady(const QImage& image)
{
    m_display->setImage(image);

    if (!m_pendingStatus.isEmpty()) {
        statusBar()->showMessage(m_pendingStatus, 4000);
        m_pendingStatus.clear();
    }
}

void MainWindow::onErrorOccurred(const QString& message)
{
    m_pendingStatus.clear();
    statusBar()->showMessage(tr("Error: %1").arg(message), 8000);
}
