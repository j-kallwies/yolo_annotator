#pragma once

#include <QEvent>
#include <QGraphicsView>

#include "annotation_manager.h"

class AnnotationManager;

enum class TouchMode
{
  None,
  Moving,
  Zooming,
};

enum class BoundingBoxEditMode
{
  None,
  New,
  DragCorner,
  DragEdge,
  DragFullBox
};

class ImageView : public QGraphicsView
{
  Q_OBJECT

public:
  ImageView(QWidget* parent = nullptr);

  void init(AnnotationManager* annotation_manager);

  void setImage(const QImage& image, const bool fit_view);

  bool viewportEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private:
  AnnotationManager* annotation_manager_{nullptr};

  qreal totalScaleFactor = 1;
  QPointF total_movement_{0, 0};
  TouchMode touch_mode_{TouchMode::None};
  std::optional<QPointF> current_start_point_;

  QGraphicsPixmapItem* image_item_;
  QGraphicsLineItem* v_line_item_;
  QGraphicsLineItem* h_line_item_;

  BoundingBoxEditMode bbox_edit_mode_{BoundingBoxEditMode::None};
  std::optional<int> edit_bbox_id_;
  BoundingBoxPart edit_bbox_part_;
  QPointF edit_bbox_static_opposite_point_;
  QPointF edit_bbox_offset_;
};
