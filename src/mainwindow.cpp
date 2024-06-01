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

  ui->image_view->init(&annotation_bounding_boxes_, &selected_bbox_id_);

  ui->image_view->setScene(scene_);
  ui->image_view->setInteractive(true);

  ui->image_view->setImage(
      QImage("/Users/jan/Library/Mobile Documents/com~apple~CloudDocs/Documents/yolo_data/IMG_0916.jpeg"));

  // Restore the previous state
  this->restoreGeometry(settings_.value("window/geometry").toByteArray());
  this->restoreState(settings_.value("window/state").toByteArray());
  ui->splitter->restoreState(settings_.value("splitter/state").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  settings_.setValue("window/geometry", saveGeometry());
  settings_.setValue("window/state", saveState());
  settings_.setValue("splitter/state", ui->splitter->saveState());

  QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
}
