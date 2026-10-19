#include "ui/HybridPanel.h"

#include "backend/ImageProcessorController.h"
#include "ui/ImageDisplayWidget.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

namespace {

// Cutoff in frequency samples of the image's longer side (see Task 9).
const int kMaxCutoff = 200;
const int kDefaultCutoff = 12;

// The boxes of this page may grow with the window (the default cap of
// ImageDisplayWidget is meant for the small three box layout).
void letBoxGrow(ImageDisplayWidget* box)
{
    box->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

} // namespace

HybridPanel::HybridPanel(ImageProcessorController* controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    for (int i = 0; i < 2; ++i) {
        m_input[i] = new ImageDisplayWidget(
            tr("Image %1").arg(i + 1),
            i == 0 ? tr("Open an image") : tr("Open the second image"),
            this);
        m_filtered[i] = new ImageDisplayWidget(
            tr("Image %1 - filtered").arg(i + 1),
            tr("Choose a filter,\nthen click\n\"Apply Filter\""),
            this);
        letBoxGrow(m_input[i]);
        letBoxGrow(m_filtered[i]);
    }

    // Row 0: the two inputs, row 1: their filter settings, row 2: the two
    // filtered images.
    layout->addWidget(m_input[0], 0, 0);
    layout->addWidget(m_input[1], 0, 1);
    layout->addWidget(createSettingsGroup(0, false), 1, 0); // image 1: low pass by default
    layout->addWidget(createSettingsGroup(1, true), 1, 1);  // image 2: high pass by default
    layout->addWidget(m_filtered[0], 2, 0);
    layout->addWidget(m_filtered[1], 2, 1);

    // Row 3: the button, row 4: the hybrid.
    auto* createButton = new QPushButton(tr("Create Hybrid Image"), this);
    layout->addWidget(createButton, 3, 0, 1, 2);

    m_result = new ImageDisplayWidget(tr("Hybrid Result"),
                                      tr("Click \"Create Hybrid Image\""), this);
    letBoxGrow(m_result);
    layout->addWidget(m_result, 4, 0, 1, 2);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
    layout->setRowStretch(0, 3);
    layout->setRowStretch(2, 3);
    layout->setRowStretch(4, 4);

    connect(createButton, &QPushButton::clicked, this, &HybridPanel::createHybrid);
}

QWidget* HybridPanel::createSettingsGroup(int index, bool highPassByDefault)
{
    auto* group = new QGroupBox(tr("Image %1 Filter Settings").arg(index + 1), this);
    auto* grid = new QGridLayout(group);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(4);

    m_filterCombo[index] = new QComboBox(group);
    m_filterCombo[index]->addItem(tr("Low Pass"), false);
    m_filterCombo[index]->addItem(tr("High Pass"), true);
    m_filterCombo[index]->setCurrentIndex(highPassByDefault ? 1 : 0);

    m_cutoffSlider[index] = new QSlider(Qt::Horizontal, group);
    m_cutoffSlider[index]->setRange(1, kMaxCutoff);
    m_cutoffSlider[index]->setValue(kDefaultCutoff);

    m_cutoffLabel[index] = new QLabel(QString::number(kDefaultCutoff), group);
    m_cutoffLabel[index]->setMinimumWidth(30);
    m_cutoffLabel[index]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* applyButton = new QPushButton(tr("Apply Filter"), group);

    grid->addWidget(new QLabel(tr("Filter Type:"), group), 0, 0);
    grid->addWidget(m_filterCombo[index], 0, 1, 1, 2);
    grid->addWidget(new QLabel(tr("Cutoff:"), group), 1, 0);
    grid->addWidget(m_cutoffSlider[index], 1, 1);
    grid->addWidget(m_cutoffLabel[index], 1, 2);
    grid->addWidget(applyButton, 2, 0, 1, 3);
    grid->setColumnStretch(1, 1);

    connect(m_cutoffSlider[index], &QSlider::valueChanged, this, [this, index](int value) {
        m_cutoffLabel[index]->setText(QString::number(value));
    });
    connect(applyButton, &QPushButton::clicked, this, [this, index]() {
        applyFilter(index);
    });

    return group;
}

bool HybridPanel::isHighPass(int index) const
{
    return m_filterCombo[index]->currentData().toBool();
}

int HybridPanel::cutoffValue(int index) const
{
    return m_cutoffSlider[index]->value();
}

void HybridPanel::applyFilter(int index)
{
    const int cutoff = cutoffValue(index);

    // The controller reports a missing image / bad value itself.
    const QImage result = m_controller->previewHybridFilter(index + 1, isHighPass(index), cutoff);
    if (result.isNull()) {
        return;
    }

    const QString text = tr("Image %1: %2 (cutoff %3)")
                             .arg(index + 1)
                             .arg(m_filterCombo[index]->currentText())
                             .arg(cutoff);
    m_filtered[index]->setTitle(text);
    m_filtered[index]->setImage(result);
    emit statusMessage(text);
}

void HybridPanel::createHybrid()
{
    const bool highPass1 = isHighPass(0);
    const bool highPass2 = isHighPass(1);

    const QImage hybrid = m_controller->createHybridFromFilters(highPass1, cutoffValue(0),
                                                                highPass2, cutoffValue(1));
    if (hybrid.isNull()) {
        return;
    }

    // Show the two filtered images the hybrid was built from, then the hybrid.
    applyFilter(0);
    applyFilter(1);
    m_result->setImage(hybrid);

    if (highPass1 == highPass2) {
        emit statusMessage(tr("Hybrid created - tip: use Low Pass on one image and High Pass "
                              "on the other for the classic hybrid effect."));
    } else {
        emit statusMessage(tr("Hybrid image created - double click a box to see it at full size."));
    }
}

void HybridPanel::setImage1(const QImage& image)
{
    m_input[0]->setImage(image);
    m_filtered[0]->clearImage();
    m_filtered[0]->setTitle(tr("Image 1 - filtered"));
    m_result->clearImage();
}

void HybridPanel::setImage2(const QImage& image)
{
    m_input[1]->setImage(image);
    m_filtered[1]->clearImage();
    m_filtered[1]->setTitle(tr("Image 2 - filtered"));
    m_result->clearImage();
}
