#include "image_sort_filter_proxy_model.h"
#include "image_list_model.h"

ImageSortFilterProxy::ImageSortFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
  this->setDynamicSortFilter(true);
}

int ImageSortFilterProxy::mapRowToSource(int row) const
{
  return mapToSource(this->index(row, 0)).row();
}

bool ImageSortFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
  QModelIndex filename_index = sourceModel()->index(sourceRow, ImageListModel::Columns::IMAGE_FILE_NAME, sourceParent);

  QString image_filename = sourceModel()->data(filename_index).toString();
  const int num_objects =
      sourceModel()->data(sourceModel()->index(sourceRow, ImageListModel::Columns::NUM_OBJECTS, sourceParent)).toInt();
  const float min_rel_object_size =
      sourceModel()->data(sourceModel()->index(sourceRow, ImageListModel::Columns::MIN_REL_OBJECT_SIZE, sourceParent)).toFloat();
  const float max_rel_object_size =
      sourceModel()->data(sourceModel()->index(sourceRow, ImageListModel::Columns::MAX_REL_OBJECT_SIZE, sourceParent)).toFloat();

  bool use_image = true;

  // Filter by filename
  if (filter_by_filename_pattern_)
  {
    use_image &= image_filename.contains(filter_by_filename_pattern_.value());
  }

  // Filter by rel. object size
  if (filter_by_rel_object_size_)
  {
    use_image &= min_rel_object_size >= filter_by_rel_object_size_.value().first;
    use_image &= max_rel_object_size <= filter_by_rel_object_size_.value().second;

    if (filter_by_rel_object_size_.value().first > 0.f)
    {
      use_image &= num_objects > 0;
    }
  }

  // Filter by num. objects
  if (filter_by_num_objects_)
  {
    use_image &= num_objects >= filter_by_num_objects_.value().first;
    use_image &= num_objects <= filter_by_num_objects_.value().second;
  }

  return use_image;
}

void ImageSortFilterProxy::setFilterByFilename(const QString& filename_pattern, const bool enabled)
{
  if (enabled)
  {
    filter_by_filename_pattern_ = filename_pattern;
  }
  else
  {
    filter_by_filename_pattern_.reset();
  }

  this->invalidateRowsFilter();
}

void ImageSortFilterProxy::setFilterRelObjectSize(const float& min_object_size, const float& max_object_size, const bool enabled)
{
  if (enabled)
  {
    filter_by_rel_object_size_ = QPair<float, float>(min_object_size, max_object_size);
  }
  else
  {
    filter_by_rel_object_size_.reset();
  }

  this->invalidateRowsFilter();
}

void ImageSortFilterProxy::setFilterByNumObjects(const int& min_num_objects, const int& max_num_objects, const bool enabled)
{
  if (enabled)
  {
    filter_by_num_objects_ = QPair<int, int>(min_num_objects, max_num_objects);
  }
  else
  {
    filter_by_num_objects_.reset();
  }

  this->invalidateRowsFilter();
}
