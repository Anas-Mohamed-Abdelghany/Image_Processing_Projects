cmake_minimum_required(VERSION 3.16)

project(CV_Assignment LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

# ----------------------------------------------------------------------------
# Headers (listed so AUTOMOC processes the Q_OBJECT classes)
# ----------------------------------------------------------------------------
set(HEADERS
    include/backend/ImageProcessorController.h
    include/services/TaskTypes.h
    include/services/Task0_ImageIOService.h
    include/services/Task1_NoiseService.h
    include/services/Task2_LowPassFilterService.h
    include/services/Task3_EdgeDetectionService.h
    include/services/Task4_HistogramService.h
    include/services/Task5_EqualizationService.h
    include/services/Task6_NormalizationService.h
    include/services/Task7_ThresholdingService.h
    include/services/Task8_ColorTransformationService.h
    include/services/Task9_FrequencyFilterService.h
    include/services/Task10_HybridImageService.h
    include/ui/ImageDisplayWidget.h
    include/ui/MainWindow.h
    include/utils/QtCvBridge.h
    include/validation/InputValidator.h
)

# ----------------------------------------------------------------------------
# Sources
# ----------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/backend/ImageProcessorController.cpp
    src/services/Task0_ImageIOService.cpp
    src/services/Task1_NoiseService.cpp
    src/services/Task2_LowPassFilterService.cpp
    src/services/Task3_EdgeDetectionService.cpp
    src/services/Task4_HistogramService.cpp
    src/services/Task5_EqualizationService.cpp
    src/services/Task6_NormalizationService.cpp
    src/services/Task7_ThresholdingService.cpp
    src/services/Task8_ColorTransformationService.cpp
    src/services/Task9_FrequencyFilterService.cpp
    src/services/Task10_HybridImageService.cpp
    src/ui/ImageDisplayWidget.cpp
    src/ui/MainWindow.cpp
    src/utils/QtCvBridge.cpp
    src/validation/InputValidator.cpp
)

# WIN32 -> no console window on Windows (qDebug still shows in Qt Creator).
add_executable(CV_Assignment WIN32 ${HEADERS} ${SOURCES})

target_include_directories(CV_Assignment PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(CV_Assignment PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
)
