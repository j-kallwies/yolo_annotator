#include "label_colors.h"

QColor LabelColors::colorForLabelId(const int label_id)
{
  switch (label_id)
  {
  case 0:
    return QColor(0.894 * 255, 0.102 * 255, 0.110 * 255);

  case 1:
    return QColor(0.216 * 255, 0.494 * 255, 0.722 * 255);

  case 2:
    return QColor(0.302 * 255, 0.686 * 255, 0.290 * 255);

  case 3:
    return QColor(0.596 * 255, 0.306 * 255, 0.639 * 255);

  case 4:
    return QColor(1.00 * 255, 0.498 * 255, 0.0 * 255);

  case 5:
    return QColor(0.651 * 255, 0.337 * 255, 0.157 * 255);

  case 6:
    return QColor(0.969 * 255, 0.506 * 255, 0.749 * 255);

  case 7:
    return QColor(0.40 * 255, 0.761 * 255, 0.647 * 255);

  case 8:
  default:
    return QColor(0, 0, 0);
  }
}
