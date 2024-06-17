#include "annotation_manager.h"

AnnotationManager::AnnotationManager()
{
}

void AnnotationManager::add(std::shared_ptr<AnnotationBoundingBox> new_bbox)
{
  annotation_bounding_boxes_.push_back(new_bbox);
}

void AnnotationManager::clear()
{
  annotation_bounding_boxes_.clear();
}

void AnnotationManager::select(int bbox_index)
{
  annotation_bounding_boxes_[bbox_index]->select();
}

void AnnotationManager::unselect(int bbox_index)
{
  annotation_bounding_boxes_[bbox_index]->unselect();
}

AnnotationBoundingBox* AnnotationManager::latest()
{
  if (annotation_bounding_boxes_.empty())
  {
    return nullptr;
  }
  else
  {
    return annotation_bounding_boxes_.back().get();
  }
}

std::optional<std::pair<int, BoundingBoxPart>> AnnotationManager::getBoundingBoxPartUnderCursor(const QPointF& cursor_position)
{
  int bbox_id = 0;
  for (std::shared_ptr<AnnotationBoundingBox>& bbox : annotation_bounding_boxes_)
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
  annotation_bounding_boxes_.pop_back();
}

void AnnotationManager::remove(int bbox_index)
{
  annotation_bounding_boxes_.remove(bbox_index);
}

std::shared_ptr<AnnotationBoundingBox> AnnotationManager::getAnnotationBoundingBox(int bbox_index)
{
  return annotation_bounding_boxes_[bbox_index];
}
