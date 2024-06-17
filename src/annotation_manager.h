#pragma once

#include "annotationboundingbox.h"

class AnnotationManager
{
public:
  AnnotationManager();

  void clear();

  void add(std::shared_ptr<AnnotationBoundingBox> new_bbox);
  void select(int bbox_index);
  void unselect(int bbox_index);

  AnnotationBoundingBox* latest();

  std::optional<std::pair<int, BoundingBoxPart>> getBoundingBoxPartUnderCursor(const QPointF& cursor_position);

  std::shared_ptr<AnnotationBoundingBox> getAnnotationBoundingBox(int bbox_index);

  void removeLatest();
  void remove(int bbox_index);

private:
  QVector<std::shared_ptr<AnnotationBoundingBox>> annotation_bounding_boxes_;
};
