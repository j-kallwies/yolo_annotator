#pragma once

#include <QAbstractListModel>

#include "annotationboundingbox.h"
#include "image_view.h"

class ImageView;

class AnnotationManager : public QAbstractListModel
{
public:
  enum Columns
  {
    LABEL_ID,
    WIDTH,
    HEIGHT,
    COUNT
  };

  AnnotationManager(ImageView* image_view, const QStringList& label_names);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  void loadFromFile(const QString& label_filename, const QSize& image_size, const bool auto_select_first_bbox);
  void saveToFile(const QString& label_filename);

  void setLabelOutputFilename(const QString& output_label_filename);

  void save();

  void clear();

  void add(AnnotationBoundingBox* new_bbox);
  void select(int bbox_index);
  void unselect(int bbox_index);
  void unselect();

  void selectPrevious();
  void selectNext();

  void activateLabel(const int label_id);
  int activeLabel() const;

  void removeSelectedBoundingBox();

  AnnotationBoundingBox* latest();

  std::optional<std::pair<int, BoundingBoxPart>> getBoundingBoxPartUnderCursor(const QPointF& cursor_position);

  AnnotationBoundingBox* getAnnotationBoundingBox(int bbox_index);

  void removeLatest();
  void remove(int bbox_index);

  std::optional<int> prefered_label_id_;

private:
  QVector<AnnotationBoundingBox*> annotation_bounding_boxes_;
  ImageView* image_view_;

  QString output_label_filename_;

  std::optional<int> selected_bbox_id_;

  int active_label_{0};

  bool cleared_{false};

  const QStringList& label_names_;
};
