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

  void save();

  void clear();

  void add(AnnotationBoundingBox* new_bbox);
  void select(int bbox_index);
  void unselect(int bbox_index);
  void unselect();

  void selectPrevious();
  void selectNext();

  void activateLabel(const int label_id);
  void removeSelectedBoundingBox();

  AnnotationBoundingBox* latest();

  std::optional<std::pair<int, BoundingBoxPart>> getBoundingBoxPartUnderCursor(const QPointF& cursor_position);

  AnnotationBoundingBox* getAnnotationBoundingBox(int bbox_index);

  void removeLatest();
  void remove(int bbox_index);

private:
  QVector<AnnotationBoundingBox*> annotation_bounding_boxes_;
  ImageView* image_view_;

  QString label_filename_;

  std::optional<int> selected_bbox_id_;

  int active_label_{0};

  bool cleared_{false};
};
