#include "annotationboundingbox.h"

#include <QPen>

AnnotationBoundingBox::AnnotationBoundingBox()
{
  QPen pen;
  pen.setWidth(5);
  pen.setCosmetic(true);
  pen.setColor(QColor(Qt::red));
  this->setPen(pen);
  this->setZValue(100);

  default_brush_.setStyle(Qt::NoBrush);
  selected_brush_.setStyle(Qt::SolidPattern);
  selected_brush_.setColor(QColor(255, 0, 0, 128));
}

void AnnotationBoundingBox::setCenter(const QPointF& center)
{
  setRect(QRectF(center.x() - rect().width() / 2.f, center.y() - rect().height() / 2.f, rect().width(), rect().height()));
}

void AnnotationBoundingBox::setCornerPoints(const QPointF& p1, const QPointF& p2)
{
  this->setRect(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()), std::abs(p1.x() - p2.x()), std::abs(p1.y() - p2.y()));
}

void AnnotationBoundingBox::setXMin(const float x_min)
{
  QPointF p1 = this->rect().topLeft();
  QPointF p2 = this->rect().bottomRight();

  p1.setX(x_min);

  setCornerPoints(p1, p2);
}

void AnnotationBoundingBox::setXMax(const float x_max)
{
  QPointF p1 = this->rect().topLeft();
  QPointF p2 = this->rect().bottomRight();

  p2.setX(x_max);

  setCornerPoints(p1, p2);
}

void AnnotationBoundingBox::setYMin(const float y_min)
{
  QPointF p1 = this->rect().topLeft();
  QPointF p2 = this->rect().bottomRight();

  p1.setY(y_min);

  setCornerPoints(p1, p2);
}

void AnnotationBoundingBox::setYMax(const float y_max)
{
  QPointF p1 = this->rect().topLeft();
  QPointF p2 = this->rect().bottomRight();

  p2.setY(y_max);

  setCornerPoints(p1, p2);
}

float AnnotationBoundingBox::xMin() const
{
  return this->rect().topLeft().x();
}

float AnnotationBoundingBox::xMax() const
{
  return this->rect().bottomRight().x();
}

float AnnotationBoundingBox::yMin() const
{
  return this->rect().topLeft().y();
}

float AnnotationBoundingBox::yMax() const
{
  return this->rect().bottomRight().y();
}

std::optional<BoundingBoxPart> AnnotationBoundingBox::getPart(const QPointF& cursor_position)
{
  const float catch_radius = 20.0;

  float current_best_dist = std::numeric_limits<float>::infinity();
  std::optional<BoundingBoxPart> current_best_part;

  float dist;

  if (cursor_position.x() >= xMin() && cursor_position.y() >= yMin() && cursor_position.x() <= xMax() &&
      cursor_position.y() <= yMax())
  {
    current_best_part = BoundingBoxPart::CentralArea;
    current_best_dist = 0.f;
  }

  dist = QLineF(this->rect().topLeft(), cursor_position).length();
  if (dist < catch_radius && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::CornerUpperLeft;
    current_best_dist = dist;
  }

  dist = QLineF(this->rect().topRight(), cursor_position).length();
  if (dist < catch_radius && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::CornerUpperRight;
    current_best_dist = dist;
  }

  dist = QLineF(this->rect().bottomLeft(), cursor_position).length();
  if (dist < catch_radius && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::CornerLowerLeft;
    current_best_dist = dist;
  }

  dist = QLineF(this->rect().bottomRight(), cursor_position).length();
  if (dist < catch_radius && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::CornerLowerRight;
    current_best_dist = dist;
  }

  dist = QLineF(((this->rect().topLeft() + this->rect().bottomLeft()) / 2.), cursor_position).length();
  if (std::abs(xMin() - cursor_position.x()) < catch_radius / 2. && cursor_position.y() >= yMin() &&
      cursor_position.y() <= yMax() && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::EdgeLeft;
    current_best_dist = dist;
  }

  dist = QLineF(((this->rect().topRight() + this->rect().bottomRight()) / 2.), cursor_position).length();
  if (std::abs(xMax() - cursor_position.x()) < catch_radius / 2. && cursor_position.y() >= yMin() &&
      cursor_position.y() <= yMax() && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::EdgeRight;
    current_best_dist = dist;
  }

  dist = QLineF(((this->rect().topLeft() + this->rect().topRight()) / 2.), cursor_position).length();
  if (std::abs(yMin() - cursor_position.y()) < catch_radius / 2. && cursor_position.x() >= xMin() &&
      cursor_position.x() <= xMax() && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::EdgeTop;
    current_best_dist = dist;
  }

  dist = QLineF(((this->rect().bottomLeft() + this->rect().bottomRight()) / 2.), cursor_position).length();
  if (std::abs(yMax() - cursor_position.y()) < catch_radius / 2. && cursor_position.x() >= xMin() &&
      cursor_position.x() <= xMax() && dist < current_best_dist)
  {
    current_best_part = BoundingBoxPart::EdgeBottom;
    // current_best_dist = dist;
  }

  return current_best_part;
}

void AnnotationBoundingBox::select()
{
  selected_ = true;

  this->setBrush(selected_brush_);
}

void AnnotationBoundingBox::unselect()
{
  selected_ = false;

  this->setBrush(default_brush_);
}
