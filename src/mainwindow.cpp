#include <QGraphicsPixmapItem>

#include <memory>

#include "annotationboundingbox.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  ui->image_view->init(&annotation_bounding_boxes_, &selected_bbox_);

  ui->image_view->setScene(scene_);
  ui->image_view->setInteractive(true);

  ui->image_view->setImage(
      QImage("/Users/jan/Library/Mobile Documents/com~apple~CloudDocs/Documents/yolo_data/IMG_0916.jpeg"));
}

MainWindow::~MainWindow()
{
}
