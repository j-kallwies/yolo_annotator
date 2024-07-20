#include "annotationboundingbox.h"
#include "label_colors.h"

#include <QPainter>
#include <QPen>

AnnotationBoundingBox::AnnotationBoundingBox(const QRectF& rect,
                                             const int label_id,
                                             const QSize& image_size,
                                             const QStringList& label_names)
    : image_size_(image_size),
      label_names_(label_names)
{
  this->setRect(rect);
  this->setLabelID(label_id);

  updateColors();

  this->setZValue(100);
}

AnnotationBoundingBox::AnnotationBoundingBox(const QStringList& yolo_fields,
                                             const QSize& image_size,
                                             const QStringList& label_names)
    : image_size_(image_size),
      label_names_(label_names)
{
  if (!std::isfinite(yolo_fields[1].toFloat()) || !std::isfinite(yolo_fields[2].toFloat()) ||
      !std::isfinite(yolo_fields[3].toFloat()) || !std::isfinite(yolo_fields[4].toFloat()))
  {
    qDebug() << "Found invalid bounding box!";

    qDebug() << yolo_fields;

    std::terminate();
  }

  const float x_center = yolo_fields[1].toFloat() * image_size.width();
  const float y_center = yolo_fields[2].toFloat() * image_size.height();
  const float box_width = yolo_fields[3].toFloat() * image_size.width();
  const float box_height = yolo_fields[4].toFloat() * image_size.height();

  this->setRect(QRectF(QPointF(x_center - box_width / 2.f, y_center - box_height / 2.f), QSizeF(box_width, box_height)));

  this->setLabelID(yolo_fields[0].toInt());
}

QPointF AnnotationBoundingBox::center() const
{
  return rect().center();
}

float AnnotationBoundingBox::width() const
{
  return rect().width();
}

float AnnotationBoundingBox::height() const
{
  return rect().height();
}

void AnnotationBoundingBox::setCenter(const QPointF& center)
{
  this->setRect(QRectF(center.x() - rect().width() / 2.f, center.y() - rect().height() / 2.f, rect().width(), rect().height()));
}

void AnnotationBoundingBox::setCornerPoints(const QPointF& p1, const QPointF& p2)
{
  this->setRect(QRectF(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()), std::abs(p1.x() - p2.x()), std::abs(p1.y() - p2.y())));
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

  this->updateColors();
}

void AnnotationBoundingBox::unselect()
{
  selected_ = false;

  this->updateColors();
}

void AnnotationBoundingBox::setLabelID(int new_label_id)
{
  label_id_ = new_label_id;
  updateColors();
}

int AnnotationBoundingBox::labelID() const
{
  return label_id_;
}

void AnnotationBoundingBox::updateColors()
{
  QPen pen;
  pen.setWidth(5);
  pen.setCosmetic(true);

  QColor color = LabelColors::colorForLabelId(label_id_);

  pen.setColor(color);
  this->setPen(pen);

  default_brush_.setStyle(Qt::NoBrush);
  selected_brush_.setStyle(Qt::SolidPattern);

  color.setAlpha(100);
  selected_brush_.setColor(color);

  if (selected_)
  {
    this->setBrush(selected_brush_);
  }
  else
  {
    this->setBrush(default_brush_);
  }
}

QString AnnotationBoundingBox::toString() const
{
  return QString("%1 %2 %3 %4 %5")
      .arg(label_id_)
      .arg(center().x() / float(image_size_.width()))
      .arg(center().y() / float(image_size_.height()))
      .arg(width() / float(image_size_.width()))
      .arg(height() / float(image_size_.height()));
}

void AnnotationBoundingBox::setRect(const QRectF& r)
{
  QGraphicsRectItem::setRect(r.intersected(QRectF(QPointF(0, 0), image_size_)));
}

QRectF AnnotationBoundingBox::boundingRect() const
{
  QRectF box_rect = QGraphicsRectItem::boundingRect();

  QRectF text_rect;
  if (label_names_.size() > label_id_)
  {
    text_rect = QFontMetrics(label_text_font_).boundingRect(label_names_.at(label_id_));
    text_rect.translate(QPointF(this->rect().topLeft()));
  }

  return box_rect.united(text_rect);
}

void AnnotationBoundingBox::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  QColor color = LabelColors::colorForLabelId(label_id_);
  if (this->selected_)
  {
    color.setAlphaF(1.0);
  }
  else
  {
    color.setAlphaF(0.7);
  }
  painter->setPen(color);
  painter->setFont(label_text_font_);

  if (label_names_.size() > label_id_)
  {
    painter->drawText(this->rect().topLeft(), label_names_.at(label_id_));
  }

  QGraphicsRectItem::paint(painter, option, widget);
}
