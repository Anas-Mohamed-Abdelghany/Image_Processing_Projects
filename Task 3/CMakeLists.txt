cmake_minimum_required(VERSION 3.16)

project(FeatureExtractionApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt moc/uic/rcc (required for Q_OBJECT classes such as TaskManager)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Find OpenCV (Required for Harris/SIFT logic later)
find_package(OpenCV REQUIRED)

set(PROJECT_SOURCES
    main.cpp
    include/TaskManager.h
    include/FeatureLogic.h
    include/DataStructures.h
    src/TaskManager.cpp
    src/FeatureDetection.cpp
    src/FeatureDescription.cpp
    src/FeatureMatching.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Link Qt6 and OpenCV
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets
    ${OpenCV_LIBS}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)