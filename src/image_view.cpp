#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QPointF>

#include "image_view.h"

ImageView::ImageView(QWidget* parent)
    : QGraphicsView(parent)
{
  viewport()->setAttribute(Qt::WA_AcceptTouchEvents);

  setMouseTracking(true);
}

void ImageView::init(QVector<std::shared_ptr<AnnotationBoundingBox>>* annotation_bounding_boxes,
                     std::optional<int>* selected_bbox_id)
{
  annotation_bounding_boxes_ = annotation_bounding_boxes;
  selected_bbox_id_ = selected_bbox_id;
}

void ImageView::setImage(const QImage& image)
{
  scene()->clear();

  image_item_ = new QGraphicsPixmapItem();
  image_item_->setPixmap(QPixmap::fromImage(image));

  scene()->addItem(image_item_);

  {
    QPen pen;
    pen.setWidth(2);
    QColor color(255, 255, 0, 128);
    pen.setColor(color);
    pen.setCosmetic(true);

    v_line_item_ = new QGraphicsLineItem();
    v_line_item_->setPen(pen);
    v_line_item_->setLine(QLineF(QPointF(0, 0), QPointF(0, image.height())));
    scene()->addItem(v_line_item_);

    h_line_item_ = new QGraphicsLineItem();
    h_line_item_->setPen(pen);
    h_line_item_->setLine(QLineF(QPointF(0, 0), QPointF(image.width(), 0)));
    scene()->addItem(h_line_item_);
  }
}

std::optional<std::pair<int, BoundingBoxPart>> ImageView::getBoundingBoxPartUnderCursor(const QPointF& cursor_position)
{
  int bbox_id = 0;
  for (std::shared_ptr<AnnotationBoundingBox>& bbox : *annotation_bounding_boxes_)
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

bool ImageView::viewportEvent(QEvent* event)
{
  switch (event->type())
  {
  case QEvent::TouchBegin:
  case QEvent::TouchUpdate:
  case QEvent::TouchEnd:
  {
    QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);

    QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->points();

    if (touchPoints.count() == 2)
    {
      // determine scale factor
      const QTouchEvent::TouchPoint& touchPoint0 = touchPoints.first();
      const QTouchEvent::TouchPoint& touchPoint1 = touchPoints.last();
      qreal currentScaleFactor = QLineF(touchPoint0.position(), touchPoint1.position()).length() /
                                 QLineF(touchPoint0.pressPosition(), touchPoint1.pressPosition()).length();

      auto movement_p1 = touchPoint1.position() - touchPoint1.pressPosition();
      auto movement_p0 = touchPoint0.position() - touchPoint0.pressPosition();

      auto mean_movement = (movement_p0 + movement_p1) / 2.0;
      auto zoom_movement = QLineF(touchPoint0.position(), touchPoint1.position()).length() -
                           QLineF(touchPoint0.pressPosition(), touchPoint1.pressPosition()).length();

      // qDebug() << "movement_p0=" << movement_p0;
      // qDebug() << "movement_p1=" << movement_p1;
      // qDebug() << "pan_movement=" << mean_movement.manhattanLength();
      // qDebug() << "zoom_movement=" << zoom_movement;
      // qDebug() << "totalScaleFactor=" << totalScaleFactor;
      // qDebug() << "currentScaleFactor=" << currentScaleFactor;

      // Start new touch action
      if (touch_mode_ == TouchMode::None)
      {
        if (abs(currentScaleFactor - 1.0) > 0.2)
        {
          touch_mode_ = TouchMode::Zooming;
        }

        if (mean_movement.manhattanLength() > 15.0)
        {
          touch_mode_ = TouchMode::Moving;
        }
      }

      // qDebug() << "touch_mode_=" << int(touch_mode_);

      if (touch_mode_ != TouchMode::Zooming)
      {
        currentScaleFactor = 1.0;
      }

      if (touchEvent->touchPointStates() & Qt::TouchPointReleased)
      {
        // qDebug() << "TouchPointReleased!";

        // if one of the fingers is released, remember the current scale
        // factor so that adding another finger later will continue zooming
        // by adding new scale factor to the existing remembered value.
        totalScaleFactor *= currentScaleFactor;
        currentScaleFactor = 1;

        touch_mode_ = TouchMode::None;
      }
      setTransformationAnchor(AnchorUnderMouse);
      setTransform(QTransform::fromScale(totalScaleFactor * currentScaleFactor, totalScaleFactor * currentScaleFactor));

      // Update
      // qDebug() << "Current scale: " << transform().m11();
    }
    return true;
  }

  case QEvent::Wheel:
  {
    // qDebug() << "QEvent::Wheel";
    return QGraphicsView::viewportEvent(event);

    // Disable wheel events
    if (touch_mode_ != TouchMode::Moving)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

    // case QEvent::MouseButtonPress:
    // {
    //   qDebug() << "QEvent::MouseButtonPress";

    //   QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);

    //   if (bbox_edit_mode_ == BoundingBoxEditMode::None)
    //   {
    //     bbox_edit_mode_ = BoundingBoxEditMode::New;

    //     annotation_bounding_boxes_->push_back(std::shared_ptr<AnnotationBoundingBox>(new AnnotationBoundingBox()));
    //     annotation_bounding_boxes_->back()->setRect(
    //         QRectF(mapToScene(mouse_event->position().x(), mouse_event->position().y()), QSizeF(100, 100)));

    //     // Show item!
    //     scene()->addItem(annotation_bounding_boxes_->back().get());
    //   }

    //   return QGraphicsView::viewportEvent(event);
    // }

  case QEvent::MouseButtonRelease:
  {
    if (bbox_edit_mode_ == BoundingBoxEditMode::New)
    {
      if (annotation_bounding_boxes_->back()->rect().width() <= 1 || annotation_bounding_boxes_->back()->rect().height() <= 1)
      {
        scene()->removeItem(annotation_bounding_boxes_->back().get());
        annotation_bounding_boxes_->pop_back();
      }
    }

    // End editing
    bbox_edit_mode_ = BoundingBoxEditMode::None;

    return QGraphicsView::viewportEvent(event);
  }

  default:
    break;
  }
  return QGraphicsView::viewportEvent(event);
}

void ImageView::mousePressEvent(QMouseEvent* event)
{
  // qDebug() << "QEvent::mousePressEvent";

  const QPointF cursor_position = mapToScene(event->position().x(), event->position().y());

  if (event->button() == Qt::LeftButton && this->dragMode() != QGraphicsView::DragMode::ScrollHandDrag)
  {
    if (bbox_edit_mode_ == BoundingBoxEditMode::None)
    {
      const auto bbox_part_under_cursor = getBoundingBoxPartUnderCursor(cursor_position);

      // Case A: No bounding box under the cursor => Start new BBox!
      if (!bbox_part_under_cursor)
      {
        if (*selected_bbox_id_)
        {
          int bbox_id = *(*selected_bbox_id_);
          annotation_bounding_boxes_->at(bbox_id)->unselect();
          *selected_bbox_id_ = {};
        };

        bbox_edit_mode_ = BoundingBoxEditMode::New;

        annotation_bounding_boxes_->push_back(std::shared_ptr<AnnotationBoundingBox>(new AnnotationBoundingBox()));
        annotation_bounding_boxes_->back()->setRect(QRectF(cursor_position, QSizeF(0, 0)));

        current_start_point_ = cursor_position;

        // Show item!
        scene()->addItem(annotation_bounding_boxes_->back().get());
      }
      // Case B: We found a bounding box under the cursor => Start editing / dragging!
      else
      {
        edit_bbox_id_ = bbox_part_under_cursor->first;
        edit_bbox_part_ = bbox_part_under_cursor->second;

        AnnotationBoundingBox& edit_bbox = *((*annotation_bounding_boxes_)[*edit_bbox_id_]);

        switch (bbox_part_under_cursor->second)
        {
        case BoundingBoxPart::CentralArea:
          bbox_edit_mode_ = BoundingBoxEditMode::DragFullBox;
          edit_bbox_offset_ = cursor_position - edit_bbox.rect().center();

          // Unselect previous BBox
          if (*selected_bbox_id_)
          {
            qDebug() << "Unselect " << (*selected_bbox_id_).value();
            (*annotation_bounding_boxes_)[(*selected_bbox_id_).value()]->unselect();
          }

          // Select new BBox
          *selected_bbox_id_ = edit_bbox_id_;
          (*annotation_bounding_boxes_)[*edit_bbox_id_]->select();
          break;

        case BoundingBoxPart::CornerLowerLeft:
        case BoundingBoxPart::CornerLowerRight:
        case BoundingBoxPart::CornerUpperLeft:
        case BoundingBoxPart::CornerUpperRight:
          bbox_edit_mode_ = BoundingBoxEditMode::DragCorner;
          switch (edit_bbox_part_)
          {
          case BoundingBoxPart::CornerUpperLeft:
            edit_bbox_static_opposite_point_ = edit_bbox.rect().bottomRight();
            break;

          case BoundingBoxPart::CornerUpperRight:
            edit_bbox_static_opposite_point_ = edit_bbox.rect().bottomLeft();
            break;

          case BoundingBoxPart::CornerLowerRight:
            edit_bbox_static_opposite_point_ = edit_bbox.rect().topLeft();
            break;

          case BoundingBoxPart::CornerLowerLeft:
            edit_bbox_static_opposite_point_ = edit_bbox.rect().topRight();
            break;
          }
          break;

        case BoundingBoxPart::EdgeLeft:
        case BoundingBoxPart::EdgeRight:
        case BoundingBoxPart::EdgeTop:
        case BoundingBoxPart::EdgeBottom:
          bbox_edit_mode_ = BoundingBoxEditMode::DragEdge;
          break;
        }
      }
    }
  }

  else if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton)
  {
    setCursor(Qt::DragMoveCursor);

    // temporarly enable dragging mode
    this->setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
    // emit a left mouse click (the default button for the drag mode)
    QMouseEvent* fake_press_event = new QMouseEvent(QEvent::GraphicsSceneMousePress,
                                                    event->position(),
                                                    event->globalPosition(),
                                                    Qt::MouseButton::LeftButton,
                                                    Qt::MouseButton::LeftButton,
                                                    Qt::KeyboardModifier::NoModifier);
    this->mousePressEvent(fake_press_event);
  }

  QGraphicsView::mousePressEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
  // qDebug() << "mouseMoveEvent()";
  // qDebug() << "bbox_edit_mode_=" << int(bbox_edit_mode_);

  const QPointF cursor_position = mapToScene(event->position().x(), event->position().y());

  auto bbox_under_cursor = getBoundingBoxPartUnderCursor(cursor_position);

  if (bbox_under_cursor)
  {
    switch (bbox_under_cursor->second)
    {
    case BoundingBoxPart::CentralArea:
      this->setCursor(QCursor(Qt::PointingHandCursor));
      break;

    case BoundingBoxPart::EdgeLeft:
    case BoundingBoxPart::EdgeRight:
      this->setCursor(QCursor(Qt::SizeHorCursor));
      break;

    case BoundingBoxPart::EdgeTop:
    case BoundingBoxPart::EdgeBottom:
      this->setCursor(QCursor(Qt::SizeVerCursor));
      break;

    case BoundingBoxPart::CornerUpperLeft:
    case BoundingBoxPart::CornerLowerRight:
      this->setCursor(QCursor(Qt::SizeFDiagCursor));
      break;

    case BoundingBoxPart::CornerUpperRight:
    case BoundingBoxPart::CornerLowerLeft:
      this->setCursor(QCursor(Qt::SizeBDiagCursor));
      break;
    }
  }
  else
  {
    this->setCursor(QCursor(Qt::CrossCursor));
  }

  v_line_item_->setPos(cursor_position.x(), 0);
  h_line_item_->setPos(0, cursor_position.y());

  AnnotationBoundingBox& edit_bbox = *((*annotation_bounding_boxes_)[*edit_bbox_id_]);

  switch (bbox_edit_mode_)
  {
  case BoundingBoxEditMode::New:
    if (!current_start_point_)
    {
      current_start_point_ = cursor_position;
    }

    annotation_bounding_boxes_->back()->setCornerPoints(*current_start_point_, cursor_position);
    break;

  case BoundingBoxEditMode::DragFullBox:
    edit_bbox.setCenter(cursor_position - edit_bbox_offset_);
    this->setCursor(QCursor(Qt::SizeAllCursor));
    break;

  case BoundingBoxEditMode::DragEdge:
    switch (edit_bbox_part_)
    {
    case BoundingBoxPart::EdgeLeft:
      edit_bbox.setXMin(cursor_position.x());
      break;

    case BoundingBoxPart::EdgeRight:
      edit_bbox.setXMax(cursor_position.x());
      break;

    case BoundingBoxPart::EdgeTop:
      edit_bbox.setYMin(cursor_position.y());
      break;

    case BoundingBoxPart::EdgeBottom:
      edit_bbox.setYMax(cursor_position.y());
      break;
    }
    break;

  case BoundingBoxEditMode::DragCorner:
    edit_bbox.setCornerPoints(edit_bbox_static_opposite_point_, cursor_position);
    break;
  }

  QGraphicsView::mouseMoveEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent* event)
{
  // disable drag mode if dragging is finished
  this->setDragMode(QGraphicsView::DragMode::NoDrag);

  // unsetCursor();
  QGraphicsView::mouseReleaseEvent(event);
}

void ImageView::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Backspace)
  {
    if (*selected_bbox_id_)
    {
      scene()->removeItem(annotation_bounding_boxes_->at((*selected_bbox_id_).value()).get());
      annotation_bounding_boxes_->remove((*selected_bbox_id_).value());
      *selected_bbox_id_ = {};
    }
  }
}
