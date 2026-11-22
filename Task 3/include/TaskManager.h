#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QResizeEvent>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include "DataStructures.h"

class MatchingPanel;   // two-image matching view (MatchingPanel.h)

class TaskManager : public QMainWindow
{
    Q_OBJECT

public:
    TaskManager(QWidget *parent = nullptr);
    ~TaskManager();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // Layout roots
    QWidget *centralWidget;
    QHBoxLayout *mainLayout;

    // Sidebar
    QWidget *sidebar;
    QVBoxLayout *sidebarLayout;
    QPushButton *btnLoadImage;
    QListWidget *taskList;
    QPushButton *btnRun;

    // Content area
    QWidget *contentWidget;
    QVBoxLayout *contentLayout;
    QLabel *lblTaskTitle;
    QLabel *lblStatus;
    QLabel *lblImageBox;
    QLabel *lblResults;
    MatchingPanel *matchingPanel = nullptr;

    // Detection parameters (Task 1) that Task 2 consumes
    QDoubleSpinBox *spinSigma;
    QSpinBox *spinMaxKeypoints;

    void setupUI();
    void setupSidebar();
    void setupContent();
    void applyStyles();

    // UI helpers
    void updateTaskTitle();
    void refreshImageDisplay();
    void updateStatus(const QString &message);
    void updateResults(const QString &message);
    void setTaskControlsEnabled(bool enabled);
    void runSelectedTask();
    void setupMatchingUI();      // tasks 3 & 4: two-image panel + its sidebar controls
    void updateMatchingView();   // switch between the normal view and the matching view

    // Image loading
    bool loadSelectedImage(const QString &filePath);

    // Data
    ImageData currentImageData;
    QPixmap originalPixmap;
    // Cache results to pass between tasks
    FeatureSet cachedFeatures;
    MatchResult cachedMatchesSSD;
    MatchResult cachedMatchesNCC;
    bool overlayKeypoints = false;

private slots:
    void onLoadImageClicked();
    void runHarrisTask();
    void runSIFTTask();
    void runSSDTask();
    void runNCCTask();
};

#endif // TASKMANAGER_H