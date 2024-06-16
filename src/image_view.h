#pragma once

#include <QEvent>
#include <QGraphicsView>

#include "annotationboundingbox.h"

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

  void init(QVector<std::shared_ptr<AnnotationBoundingBox>>* annotation_bounding_boxes, std::optional<int>* selected_bbox_id);

  void setImage(const QImage& image);

  bool viewportEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

  int active_label_{0};

private:
  QList<std::shared_ptr<AnnotationBoundingBox>>* annotation_bounding_boxes_{nullptr};
  std::optional<int>* selected_bbox_id_{nullptr};

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

  std::optional<std::pair<int, BoundingBoxPart>> getBoundingBoxPartUnderCursor(const QPointF& cursor_position);
};
