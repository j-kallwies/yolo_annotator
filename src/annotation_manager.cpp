#include "annotation_manager.h"

#include <QFile>

AnnotationManager::AnnotationManager(ImageView* image_view, const QStringList& label_names)
    : image_view_(image_view),
      label_names_(label_names)
{
}

int AnnotationManager::rowCount(const QModelIndex& parent) const
{
  return annotation_bounding_boxes_.size();
}

int AnnotationManager::columnCount(const QModelIndex& parent) const
{
  return Columns::COUNT;
}

QVariant AnnotationManager::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
  {
    return QVariant();
  }

  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
    case Columns::LABEL_ID:
      return label_names_.at(annotation_bounding_boxes_.at(index.row())->labelID());

    case Columns::WIDTH:
      return std::round(annotation_bounding_boxes_.at(index.row())->rect().width());

    case Columns::HEIGHT:
      return std::round(annotation_bounding_boxes_.at(index.row())->rect().height());
    }
  }

  // Fallback
  return {};
}

QVariant AnnotationManager::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Orientation::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case Columns::LABEL_ID:
      return "Label";

    case Columns::WIDTH:
      return "Width";

    case Columns::HEIGHT:
      return "Height";
    }
  }

  // Fallback
  return QAbstractListModel::headerData(section, orientation, role);
}

void AnnotationManager::loadFromFile(const QString& label_filename, const QSize& image_size, const bool auto_select_first_bbox)
{
  output_label_filename_ = "";

  this->clear();

  this->beginResetModel();

  bool annotations_updated = false;

  QFile file(label_filename);
  if (file.open(QIODevice::ReadOnly))
  {
    QTextStream in(&file);

    while (!in.atEnd())
    {
      QString line = in.readLine();

      QStringList fields = line.split(" ");

      if (!std::isfinite(fields[1].toFloat()) || !std::isfinite(fields[2].toFloat()) || !std::isfinite(fields[3].toFloat()))
      {
        qDebug() << "Found nan in " << label_filename;
        annotations_updated = true;
        std::terminate();
      }

      if (fields.size() >= 5)
      {
        this->add(new AnnotationBoundingBox(fields, image_size, label_names_));
      }
    }

    file.close();
  }

  // Select the first bounding box
  if (auto_select_first_bbox)
  {
    this->unselect();

    bool selected = false;

    if (prefered_label_id_)
    {
      for (int i = 0; i < annotation_bounding_boxes_.size(); i++)
      {
        if (annotation_bounding_boxes_.at(i)->labelID() == prefered_label_id_.value())
        {
          select(i);
          selected = true;
          break;
        }
      }
    }

    if (!selected)
    {
      select(0);
    }
  }

  this->cleared_ = false;

  this->endResetModel();

  // if (annotations_updated)
  // {
  //   this->save();
  // }
}

void AnnotationManager::setLabelOutputFilename(const QString& output_label_filename)
{
  output_label_filename_ = output_label_filename;
  // qDebug() << "output_label_filename_=" << output_label_filename_;
}

void AnnotationManager::saveToFile(const QString& label_filename)
{
  // Cancel saving in case of a cleared state!
  if (this->cleared_)
  {
    return;
  }

  QFile file(label_filename);

  // Do not create empty (useless) files
  if (!file.exists() && this->annotation_bounding_boxes_.size() == 0)
  {
    return;
  }

  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    QTextStream stream(&file);
    for (const auto* bbox : annotation_bounding_boxes_)
    {
      stream << bbox->toString() << Qt::endl;
    }

    file.close();
  }
}

void AnnotationManager::save()
{
  if (output_label_filename_.size() > 0)
  {
    this->saveToFile(output_label_filename_);
  }
}

void AnnotationManager::add(AnnotationBoundingBox* new_bbox)
{
  annotation_bounding_boxes_.push_back(new_bbox);

  if (new_bbox->labelID() == -1)
  {
    new_bbox->setLabelID(active_label_);
  }

  // Show item!
  image_view_->scene()->addItem(new_bbox);
}

void AnnotationManager::clear()
{
  annotation_bounding_boxes_.clear();
  this->cleared_ = true;
}

void AnnotationManager::select(int bbox_index)
{
  if (this->annotation_bounding_boxes_.size() > bbox_index)
  {
    annotation_bounding_boxes_[bbox_index]->select();
    selected_bbox_id_ = bbox_index;
  }
}

void AnnotationManager::unselect(int bbox_index)
{
  if (this->annotation_bounding_boxes_.size() > bbox_index)
  {
    annotation_bounding_boxes_[bbox_index]->unselect();
  }

  if (selected_bbox_id_ && selected_bbox_id_.value() == bbox_index)
  {
    selected_bbox_id_.reset();
  }
}

void AnnotationManager::selectPrevious()
{
  // If a bounding box is selected, move on to the next one
  if (selected_bbox_id_)
  {
    int next_bbox_id = *selected_bbox_id_ - 1;

    if (next_bbox_id < 0)
    {
      next_bbox_id = this->annotation_bounding_boxes_.size() - 1;
    }

    this->unselect(*selected_bbox_id_);
    this->select(next_bbox_id);
  }
  // If no bounding box is selected, select the first one
  else if (annotation_bounding_boxes_.size() > 0)
  {
    this->select(0);
  }
}

void AnnotationManager::selectNext()
{
  // If a bounding box is selected, move on to the next one
  if (selected_bbox_id_)
  {
    int next_bbox_id = -1;

    if (prefered_label_id_)
    {
      int num_bboxes_checked = 0;
      for (int i = selected_bbox_id_.value() + 1;; i++)
      {
        if (i >= annotation_bounding_boxes_.size())
        {
          i = 0;
        }

        if (annotation_bounding_boxes_.at(i)->labelID() == prefered_label_id_.value())
        {
          next_bbox_id = i;
          break;
        }

        num_bboxes_checked++;

        if (num_bboxes_checked >= annotation_bounding_boxes_.size())
        {
          break;
        }
      }
    }

    // No prefered label found => just use the next one!
    if (next_bbox_id == -1)
    {
      next_bbox_id = *selected_bbox_id_ + 1;
    }

    // Rotate at end of the indices
    if (next_bbox_id >= this->annotation_bounding_boxes_.size())
    {
      next_bbox_id = 0;
    }

    this->unselect(*selected_bbox_id_);
    this->select(next_bbox_id);
  }
  // If no bounding box is selected, select the first relevant one
  else if (annotation_bounding_boxes_.size() > 0)
  {
    int next_bbox_id = -1;

    if (prefered_label_id_)
    {
      for (int i = 0; i < annotation_bounding_boxes_.size(); i++)
      {
        if (annotation_bounding_boxes_.at(i)->labelID() == prefered_label_id_.value())
        {
          next_bbox_id = i;
          break;
        }
      }
    }

    if (next_bbox_id == -1)
    {
      next_bbox_id = 0;
    }

    this->select(next_bbox_id);
  }
}

void AnnotationManager::unselect()
{
  for (const auto& bbox : annotation_bounding_boxes_)
  {
    bbox->unselect();
  }

  selected_bbox_id_.reset();
}

AnnotationBoundingBox* AnnotationManager::latest()
{
  if (annotation_bounding_boxes_.empty())
  {
    return nullptr;
  }
  else
  {
    return annotation_bounding_boxes_.back();
  }
}

std::optional<std::pair<int, BoundingBoxPart>> AnnotationManager::getBoundingBoxPartUnderCursor(const QPointF& cursor_position)
{
  int bbox_id = 0;
  for (AnnotationBoundingBox* bbox : annotation_bounding_boxes_)
  {
    std::optional<BoundingBoxPart> part = bbox->getPart(cursor_position);

    if (part)
    {
      return std::make_pair(bbox_id, *part);
    }

    bbox_id++;
  }

  // Fallback
  return {};
}

void AnnotationManager::removeLatest()
{
  if (annotation_bounding_boxes_.size() > 0)
  {
    image_view_->scene()->removeItem(annotation_bounding_boxes_.back());
    annotation_bounding_boxes_.pop_back();
  }
}

void AnnotationManager::remove(int bbox_index)
{
  if (annotation_bounding_boxes_.size() > bbox_index)
  {
    image_view_->scene()->removeItem(annotation_bounding_boxes_[bbox_index]);
    annotation_bounding_boxes_.remove(bbox_index);
  }
}

AnnotationBoundingBox* AnnotationManager::getAnnotationBoundingBox(int bbox_index)
{
  if (annotation_bounding_boxes_.size() > bbox_index)
  {
    return annotation_bounding_boxes_[bbox_index];
  }
  else
  {
    return nullptr;
  }
}

void AnnotationManager::removeSelectedBoundingBox()
{
  if (selected_bbox_id_)
  {
    this->remove(selected_bbox_id_.value());
    selected_bbox_id_.reset();
  }

  selectNext();
}

void AnnotationManager::activateLabel(const int label_id)
{
  if (selected_bbox_id_ && annotation_bounding_boxes_.size() > *selected_bbox_id_)
  {
    annotation_bounding_boxes_[*selected_bbox_id_]->setLabelID(label_id);
  }

  active_label_ = label_id;
}

int AnnotationManager::activeLabel() const
{
  return active_label_;
}
