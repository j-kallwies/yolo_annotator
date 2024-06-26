#pragma once

#include <QFileSystemModel>
#include <QGraphicsScene>
#include <QItemSelection>
#include <QMainWindow>
#include <QProcess>
#include <QScopedPointer>
#include <QSettings>
#include <QShortcut>

#include "annotation_manager.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(const QString& root_path, const QStringList& label_names, QWidget* parent = 0);
  virtual ~MainWindow();

private:
  const QString root_path_;

  QScopedPointer<Ui::MainWindow> ui;

  QSettings settings_{"YOLO", "Annotator"};

  QGraphicsScene* scene_{new QGraphicsScene(this)};
  std::unique_ptr<AnnotationManager> annotation_manager_;

  QFileSystemModel folder_tree_model_;

  const QStringList image_filename_filter_{"*.jpg", "*.jpeg", "*.png", "*.webp"};

  QDir current_folder_;
  QStringList image_file_names_;

  QShortcut prev_image_shortcut_{QKeySequence(Qt::Key_Left), this};
  QShortcut next_image_shortcut_{QKeySequence(Qt::Key_Right), this};

  QShortcut remove_image_shortcut_{QKeySequence{Qt::CTRL | Qt::Key_Backspace}, this};

  QShortcut edit_image_shortcut_{QKeySequence{Qt::CTRL | Qt::Key_E}, this};

  QShortcut move_to_random_set_shortcut_{QKeySequence(Qt::CTRL | Qt::Key_R), this};
  QShortcut move_to_train_shortcut_{QKeySequence(Qt::CTRL | Qt::Key_Return), this};
  QShortcut move_to_val_shortcut_{QKeySequence(Qt::CTRL | Qt::Key_V), this};
  QShortcut move_to_test_shortcut_{QKeySequence(Qt::CTRL | Qt::Key_T), this};
  QShortcut move_to_merge_shortcut_{QKeySequence(Qt::CTRL | Qt::Key_M), this};

  QProcess predict_process_{this};

  void openFolder(const QString& folder);

  void loadImage(const QString& image_filename);

  void closeEvent(QCloseEvent* event) override;

  void onSelectFolder(const QItemSelection& selected, const QItemSelection& deselected);

private slots:
  void onLoadImage(int idx);
  void onPrevImage();
  void onNextImage();

  void onEditImage();

  void onRemoveImage();

  void moveCurrentImageToFolder(const QString& folder);

  void onMoveImageToRandomSet();
  void onMoveImageToTrain();
  void onMoveImageToVal();
  void onMoveImageToTest();
  void onMoveImageToMerge();

  void onUpdateFiltering();

  void onStartPrediction(bool checked);

private:
  static QString getLabelFilename(const QString& image_filename);

  QStringList label_names_;
};
