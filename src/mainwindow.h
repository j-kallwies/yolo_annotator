#pragma once

#include <QGraphicsScene>
#include <QMainWindow>
#include <QScopedPointer>
#include <QSettings>

#include "annotationboundingbox.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget* parent = 0);
  virtual ~MainWindow();

private:
  QScopedPointer<Ui::MainWindow> ui;

  QSettings settings_{"YOLO", "Annotator"};

  QGraphicsScene* scene_{new QGraphicsScene(this)};
  QVector<std::shared_ptr<AnnotationBoundingBox>> annotation_bounding_boxes_;
  AnnotationBoundingBox* selected_bbox_{nullptr};

  void closeEvent(QCloseEvent* event) override;
};
