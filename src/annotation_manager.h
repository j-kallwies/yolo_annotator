#pragma once

#include "annotationboundingbox.h"
#include "image_view.h"

class ImageView;

class AnnotationManager
{
public:
  AnnotationManager(ImageView* image_view);

  void loadFromFile(const QString& label_filename, const QSize& image_size);
  void saveToFile(const QString& label_filename);

  void clear();

  void add(AnnotationBoundingBox* new_bbox);
  void select(int bbox_index);
  void unselect(int bbox_index);

  AnnotationBoundingBox* latest();

  std::optional<std::pair<int, BoundingBoxPart>> getBoundingBoxPartUnderCursor(const QPointF& cursor_position);

  AnnotationBoundingBox* getAnnotationBoundingBox(int bbox_index);

  void removeLatest();
  void remove(int bbox_index);

private:
  QVector<AnnotationBoundingBox*> annotation_bounding_boxes_;
  ImageView* image_view_;
};
