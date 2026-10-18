#include "ui/MainWindow.h"

#include "backend/ImageProcessorController.h"
#include "services/TaskTypes.h"
#include "ui/HybridPanel.h"
#include "ui/ImageDisplayWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
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
    // These are LOGICAL pixels and Qt reports this desktop as 150%, so the
    // window paints at 1.5x: 840x470 becomes ~1260x705 physical, which just
    // fits a 1280x720 screen. Anything larger clips the control panel.
    resize(840, 470);

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

    // --- the three previews -------------------------------------------------
    m_originalBox = new ImageDisplayWidget(tr("Original"),
                                           tr("Open an image"), central);
    m_resultBox2 = new ImageDisplayWidget(tr("Box 2"),
                                          tr("Run a task,\nthen click\n\"Show in Box 2\""),
                                          central);
    m_resultBox3 = new ImageDisplayWidget(tr("Box 3"),
                                          tr("Run a task,\nthen click\n\"Show in Box 3\""),
                                          central);

    // Mark the default target box so it is obvious where a result will go.
    m_resultBox2->setTitle(tr("Box 2  (active)"));

    auto* previewArea = new QWidget(central);
    auto* previewLayout = new QHBoxLayout(previewArea);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);

    previewLayout->addWidget(m_originalBox, 1);
    previewLayout->addWidget(m_resultBox2, 1);
    previewLayout->addWidget(m_resultBox3, 1);

    auto* panel = new QWidget(central);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(10);

    panelLayout->addWidget(createFileGroup());
    panelLayout->addWidget(createOperationGroup());
    panelLayout->addWidget(createParameterGroup());
    panelLayout->addWidget(createActionGroup());
    panelLayout->addStretch(1);

    // The preview area has two pages: the three boxes above, and the hybrid
    // page that replaces them while "Hybrid Image" is the selected task. The
    // hybrid page sits in a scroll area so a small window can still reach all
    // of it, while a large window lets it grow.
    m_hybridPanel = new HybridPanel(m_controller, central);
    auto* hybridScroll = new QScrollArea(central);
    hybridScroll->setWidgetResizable(true);
    hybridScroll->setFrameShape(QFrame::NoFrame);
    hybridScroll->setWidget(m_hybridPanel);

    m_previewStack = new QStackedWidget(central);
    m_previewStack->addWidget(previewArea);   // page 0
    m_previewStack->addWidget(hybridScroll);  // page 1
    connect(m_hybridPanel, &HybridPanel::statusMessage, this, [this](const QString& text) {
        statusBar()->showMessage(text, 6000);
    });

    mainLayout->addWidget(m_previewStack, 1);
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
    auto* group = new QGroupBox(tr("Tasks"), this);
    auto* grid = new QGridLayout(group);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(4);

    // One drop down per task group. The short label keeps the panel narrow,
    // the task number is shown inside the entries ("Sobel").
    struct Group {
        QString label;
        QVector<QPair<QString, Operation>> entries;
    };

    const QVector<Group> groups = {
        {tr("Noise"), {
            {tr("Uniform "), Operation::UniformNoise},
            {tr("Gaussian "), Operation::GaussianNoise},
            {tr("Salt & Pepper "), Operation::SaltPepperNoise}}},
        {tr("Filter"), {
            {tr("Average "), Operation::AverageFilter},
            {tr("Gaussian "), Operation::GaussianFilter},
            {tr("Median "), Operation::MedianFilter}}},
        {tr("Edges"), {
            {tr("Sobel "), Operation::SobelEdges},
            {tr("Roberts "), Operation::RobertsEdges},
            {tr("Prewitt "), Operation::PrewittEdges},
            {tr("Canny "), Operation::CannyEdges}}},
        {tr("Color"), {
            {tr("RGB -> Gray"), Operation::GrayscaleConversion},
            {tr("Channel Hist + CDF"), Operation::ChannelHistograms}}},
        {tr("Frequency"), {
            {tr("Low Pass"), Operation::FrequencyLowPass},
            {tr("High Pass"), Operation::FrequencyHighPass}}},
        {tr("Other"), {
            {tr("Histogram"), Operation::Histogram},
            {tr("CDF Curve"), Operation::CumulativeCurve},
            {tr("Equalization"), Operation::Equalization},
            {tr("Normalization"), Operation::Normalization},
            {tr("Thresholding"), Operation::Thresholding},
            {tr("Hybrid Image"), Operation::HybridImage}}},
    };

    for (int i = 0; i < groups.size(); ++i) {
        const int row = i;
        const Group& entry = groups.at(i);

        auto* label = new QLabel(entry.label, group);
        auto* combo = new QComboBox(group);

        // Index 0 is the "nothing selected" placeholder.
        combo->addItem(tr("(select a task)"), -1);
        for (const auto& item : entry.entries) {
            combo->addItem(item.first, static_cast<int>(item.second));
        }
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        grid->addWidget(label, row, 0);
        grid->addWidget(combo, row, 1);

        m_taskCombos.append(combo);
        connect(combo, &QComboBox::currentIndexChanged,
                this, &MainWindow::onTaskSelectionChanged);
    }

    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);
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
    m_directionCombo->addItem(tr("X,Y directions"), static_cast<int>(Direction::Both));
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

    // Which preview box receives the next result.
    m_showInBox2Button = new QPushButton(tr("Show Result in Box 2"), group);
    m_showInBox3Button = new QPushButton(tr("Show Result in Box 3"), group);
    m_showInBox2Button->setCheckable(true);
    m_showInBox3Button->setCheckable(true);
    m_showInBox2Button->setChecked(true);

    // Every operation starts from the loaded image again, so two results can
    // be compared fairly (uncheck to chain the operations instead).
    m_resetBeforeOpCheck = new QCheckBox(tr("Start each operation from the original"), group);
    m_resetBeforeOpCheck->setChecked(true);

    layout->addWidget(m_runButton, 0, 0, 1, 2);
    layout->addWidget(m_originalButton, 1, 0);
    layout->addWidget(m_resetButton, 1, 1);

    auto* targetLabel = new QLabel(tr("Next result goes to:"), group);
    layout->addWidget(targetLabel, 2, 0, 1, 2);
    layout->addWidget(m_showInBox2Button, 3, 0);
    layout->addWidget(m_showInBox3Button, 3, 1);

    layout->addWidget(m_resetBeforeOpCheck, 4, 0, 1, 2);

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

    // Destination selector: only one of the two boxes can be the target.
    // The buttons are checkable, so a click on the active one keeps it.
    connect(m_showInBox2Button, &QPushButton::clicked, this, [this]() {
        showResultInBox2();
    });
    connect(m_showInBox3Button, &QPushButton::clicked, this, [this]() {
        showResultInBox3();
    });
    connect(m_resetBeforeOpCheck, &QCheckBox::toggled, this, [this](bool checked) {
        statusBar()->showMessage(checked
            ? tr("Each operation now starts from the original image.")
            : tr("Operations are now chained onto the previous result."), 4000);
    });

    // The task drop downs are connected inside createOperationGroup().

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
    switch (m_activeOperation) {
    case Operation::None:
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
    case Operation::CumulativeCurve:
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
        showParameters({QStringLiteral("cutoff")});
        break;
    case Operation::HybridImage:
        showParameters({}); // the hybrid page has its own cutoff per image
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

    // The freshly loaded image belongs into the original box.
    m_routeToOriginal = true;
    if (!m_controller->loadImage(path)) {
        m_pendingStatus.clear();
    } else {
        m_hybridPanel->setImage1(m_originalBox->image());
    }
    m_routeToOriginal = false;
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
        m_hybridPanel->setImage2(m_controller->secondImagePreview());
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
    const Operation op = m_activeOperation;

    if (op == Operation::None) {
        statusBar()->showMessage(tr("Pick a task from one of the drop downs first."), 5000);
        return;
    }

    if (op == Operation::ShowOriginal) {
        showOriginal();
        return;
    }

    if (op == Operation::HybridImage) {
        // The hybrid page has its own filter / cutoff controls per image.
        m_hybridPanel->createHybrid();
        return;
    }

    // If the controller fails it emits errorOccurred first, which clears this
    // again - so the status only ever shows success.
    QString operationName;
    for (QComboBox* combo : m_taskCombos) {
        const int index = combo->findData(static_cast<int>(op));
        if (index >= 0) {
            operationName = combo->itemText(index);
            break;
        }
    }
    m_pendingStatus = tr("Applied: %1").arg(operationName);

    // Comparing two variants (e.g. Sobel X vs Sobel Y) only makes sense when
    // both are computed from the original image, so by default the chain is
    // discarded before every operation.
    if (m_resetBeforeOpCheck->isChecked() && m_controller->hasImage()) {
        // The reset publishes the original image again - keep it in box 1.
        m_routeToOriginal = true;
        m_controller->resetToOriginal();
        m_routeToOriginal = false;
    }

    const int ksize = m_kernelCombo->currentData().toInt();
    const Direction direction = static_cast<Direction>(m_directionCombo->currentData().toInt());

    switch (op) {
    case Operation::None:
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

    case Operation::CumulativeCurve:
        m_controller->showCumulativeCurve();
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
    m_routeToOriginal = true;
    m_controller->showOriginal();
    m_routeToOriginal = false;
}

void MainWindow::resetToOriginal()
{
    m_pendingStatus = tr("Reset - all operations discarded");
    m_routeToOriginal = true;
    m_controller->resetToOriginal();
    m_routeToOriginal = false;
}

// ---------------------------------------------------------------------------
// Result boxes
// ---------------------------------------------------------------------------

void MainWindow::showResultInBox2()
{
    m_activeResultBox = ResultBox::Box2;

    // Exactly one of the two buttons is checked at a time.
    m_showInBox2Button->setChecked(true);
    m_showInBox3Button->setChecked(false);
    m_resultBox2->setTitle(tr("Box 2  (active)"));
    m_resultBox3->setTitle(tr("Box 3"));

    if (m_lastResult.isNull()) {
        statusBar()->showMessage(tr("No result yet - run a task first."), 4000);
    }

    displayResultInActiveBox();
}

void MainWindow::showResultInBox3()
{
    m_activeResultBox = ResultBox::Box3;

    m_showInBox2Button->setChecked(false);
    m_showInBox3Button->setChecked(true);
    m_resultBox2->setTitle(tr("Box 2"));
    m_resultBox3->setTitle(tr("Box 3  (active)"));

    if (m_lastResult.isNull()) {
        statusBar()->showMessage(tr("No result yet - run a task first."), 4000);
    }

    displayResultInActiveBox();
}

void MainWindow::displayResultInActiveBox()
{
    if (m_lastResult.isNull()) {
        return;
    }

    (m_activeResultBox == ResultBox::Box2 ? m_resultBox2 : m_resultBox3)
        ->setImage(m_lastResult);
}

void MainWindow::onTaskSelectionChanged(int index)
{
    auto* combo = qobject_cast<QComboBox*>(sender());
    if (combo == nullptr) {
        return;
    }

    const int value = combo->itemData(index).toInt();
    if (value < 0) {
        return; // "(select a task)" placeholder - nothing becomes active
    }

    selectOperation(static_cast<Operation>(value), combo);
}

void MainWindow::selectOperation(Operation operation, QComboBox* source)
{
    m_activeOperation = operation;

    // Hybrid Image gets its own page, every other task the three previews.
    if (m_previewStack != nullptr) {
        m_previewStack->setCurrentIndex(operation == Operation::HybridImage ? 1 : 0);
    }

    // Exactly one drop down may show the active task: reset the others back
    // to the placeholder. Signals are blocked so these resets do not come
    // back into onTaskSelectionChanged().
    for (QComboBox* combo : m_taskCombos) {
        if (combo == source) {
            continue;
        }
        const QSignalBlocker blocker(combo);
        combo->setCurrentIndex(0);
    }

    updateParameterVisibility();
}

void MainWindow::onResultReady(const QImage& image)
{
    if (m_routeToOriginal) {
        // Loaded / "Show Original" / "Reset" - box 1 keeps the source image,
        // which is also the chain's starting point again.
        m_originalBox->setImage(image);
    } else {
        // A task result: remember it and show it in the selected box.
        m_lastResult = image;
        displayResultInActiveBox();

        if (!m_pendingStatus.isEmpty()) {
            statusBar()->showMessage(m_pendingStatus + tr("  (shown in %1)")
                                                     .arg(m_activeResultBox == ResultBox::Box2
                                                              ? tr("Box 2")
                                                              : tr("Box 3")),
                                     4000);
            m_pendingStatus.clear();
        }
        return;
    }

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
