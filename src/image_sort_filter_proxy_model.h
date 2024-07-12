#pragma once

#include <QSortFilterProxyModel>

class ImageSortFilterProxy : public QSortFilterProxyModel
{
public:
  ImageSortFilterProxy(QObject* parent = nullptr);

  void setFilterByFilename(const QString& filename_pattern, const bool enabled);
  void setFilterRelObjectSize(const float& min_object_size, const float& max_object_size, const bool enabled);
  void setFilterByNumObjects(const int& min_num_objects, const int& max_num_objects, const bool enabled);
  void setFilterByLabelId(const int& label_id, const bool enabled);

  int mapRowToSource(int row) const;

  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

private:
  std::optional<QString> filter_by_filename_pattern_;
  std::optional<QPair<float, float>> filter_by_rel_object_size_;
  std::optional<QPair<int, int>> filter_by_num_objects_;
  std::optional<int> filter_by_label_id_;
};
