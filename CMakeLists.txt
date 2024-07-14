# Reference: https://doc.qt.io/qt-6/cmake-manual.html

cmake_minimum_required(VERSION 3.16)

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include Qt basic functions
include(QtCommon)

# Basic information about project

project(yolo_annotator VERSION 1.0)

# Set PROJECT_VERSION_PATCH and PROJECT_VERSION_TWEAK to 0 if not present, needed by add_project_meta
fix_project_version()

set(SOURCE_FILES
    src/main.cpp
    src/mainwindow.cpp
    src/image_view.cpp
    src/annotationboundingbox.cpp
    src/label_colors.cpp
    src/annotation_manager.cpp
    src/image_list_model.cpp
    src/image_list_model.cpp
    src/image_sort_filter_proxy_model.cpp
    src/cache_db_interface.cpp
)

set(HEADER_FILES
    src/mainwindow.h
    src/image_view.h
    src/annotationboundingbox.h
    src/label_colors.h
    src/annotation_manager.h
    src/image_list_model.h
    src/image_list_model.h
    src/image_sort_filter_proxy_model.h
    src/cache_db_interface.h
)

add_project_meta(META_FILES_TO_INCLUDE)

set(RESOURCE_FILES yolo_annotator.qrc)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(SQLite3)

qt_standard_project_setup()

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${META_FILES_TO_INCLUDE}
    ${RESOURCE_FILES}
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

target_precompile_headers(${PROJECT_NAME} INTERFACE QtWidgets.h)

target_link_libraries(
  ${PROJECT_NAME}
  Qt6::Widgets
  SQLite::SQLite3
)

install(
    TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION /Applications
)
