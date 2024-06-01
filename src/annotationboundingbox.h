#pragma once

#include <QBrush>
#include <QGraphicsRectItem>

enum class BoundingBoxPart
{
  CentralArea,
  CornerUpperLeft,
  CornerUpperRight,
  CornerLowerRight,
  CornerLowerLeft,
  EdgeLeft,
  EdgeRight,
  EdgeTop,
  EdgeBottom,
};

class AnnotationBoundingBox : public QGraphicsRectItem
{
public:
  AnnotationBoundingBox();

  void setCenter(const QPointF& center);
  void setCornerPoints(const QPointF& p1, const QPointF& p2);
  void setXMin(const float x_min);
  void setXMax(const float x_max);
  void setYMin(const float y_min);
  void setYMax(const float y_max);

  float xMin() const;
  float xMax() const;
  float yMin() const;
  float yMax() const;

  std::optional<BoundingBoxPart> getPart(const QPointF& cursor_position);

  void select();
  void unselect();

private:
  bool selected_{false};
  QBrush default_brush_;
  QBrush selected_brush_;
};
