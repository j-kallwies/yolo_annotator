#pragma once

#include <QBrush>
#include <QFont>
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
  AnnotationBoundingBox(const QRectF& rect, const int label_id, const QSize& image_size, const QStringList& label_names);
  AnnotationBoundingBox(const QStringList& yolo_fields, const QSize& image_size, const QStringList& label_names);

  QPointF center() const;
  float width() const;
  float height() const;

  void setRect(const QRectF& r);

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
  void setLabelID(int new_label_id);
  int labelID() const;

  QString toString() const;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  QRectF boundingRect() const override;

private:
  bool selected_{false};
  QBrush default_brush_;
  QBrush selected_brush_;
  int label_id_{-1};

  const QSize image_size_;

  const QStringList& label_names_;

  const QFont label_text_font_{"Arial", 24, 3};

  void updateColors();
};
