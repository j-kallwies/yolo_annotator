#include "annotation_manager.h"

#include <QFile>

AnnotationManager::AnnotationManager(ImageView* image_view)
    : image_view_(image_view)
{
}

void AnnotationManager::loadFromFile(const QString& label_filename, const QSize& image_size)
{
  label_filename_ = label_filename;

  this->clear();

  QFile file(label_filename);
  if (file.open(QIODevice::ReadOnly))
  {
    QTextStream in(&file);

    while (!in.atEnd())
    {
      QString line = in.readLine();
      QStringList fields = line.split(" ");

      if (fields.size() == 5)
      {
        AnnotationBoundingBox* new_bbox = new AnnotationBoundingBox(image_size);

        const float x_center = fields[1].toFloat() * image_size.width();
        const float y_center = fields[2].toFloat() * image_size.height();
        const float box_width = fields[3].toFloat() * image_size.width();
        const float box_height = fields[4].toFloat() * image_size.height();

        new_bbox->setRect(
            QRectF(QPointF(x_center - box_width / 2.f, y_center - box_height / 2.f), QSizeF(box_width, box_height)));
        new_bbox->setLabelID(fields[0].toInt());

        this->add(new_bbox);
      }
    }

    file.close();
  }

  if (this->annotation_bounding_boxes_.size() > 0)
  {
    select(0);
  }

  this->cleared_ = false;
}

void AnnotationManager::saveToFile(const QString& label_filename)
{
  // Cancel saving in case of a cleared state!
  if (this->cleared_)
  {
    return;
  }

  QFile file(label_filename);
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
  this->saveToFile(label_filename_);
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
  annotation_bounding_boxes_[bbox_index]->select();
  selected_bbox_id_ = bbox_index;
}

void AnnotationManager::unselect(int bbox_index)
{
  annotation_bounding_boxes_[bbox_index]->unselect();

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
    int next_bbox_id = *selected_bbox_id_ + 1;

    if (next_bbox_id >= this->annotation_bounding_boxes_.size())
    {
      next_bbox_id = 0;
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
  image_view_->scene()->removeItem(annotation_bounding_boxes_.back());
  annotation_bounding_boxes_.pop_back();
}

void AnnotationManager::remove(int bbox_index)
{
  image_view_->scene()->removeItem(annotation_bounding_boxes_[bbox_index]);
  annotation_bounding_boxes_.remove(bbox_index);
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
  }
}

void AnnotationManager::activateLabel(const int label_id)
{
  if (selected_bbox_id_)
  {
    annotation_bounding_boxes_[*selected_bbox_id_]->setLabelID(label_id);
  }

  active_label_ = label_id;
}
