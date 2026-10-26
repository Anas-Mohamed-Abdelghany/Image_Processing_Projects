cmake_minimum_required(VERSION 3.16)
project(ComputerVisionApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/ImageUtils.cpp
    src/CannyEdgeDetector.cpp
    src/HoughTransform.cpp
    src/EllipseDetector.cpp
    src/ActiveContour.cpp
    src/MainWindow.cpp
)

set(HEADERS
    include/Types.h
    include/ImageUtils.h
    include/CannyEdgeDetector.h
    include/HoughTransform.h
    include/EllipseDetector.h
    include/ActiveContour.h
    include/MainWindow.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)