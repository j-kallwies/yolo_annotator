#pragma once

#include <QFileSystemModel>
#include <QGraphicsScene>
#include <QMainWindow>
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
  MainWindow(QWidget* parent = 0);
  virtual ~MainWindow();

private:
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

  void openFolder(const QString& folder);

  void loadImage(const QString& image_filename);

  void closeEvent(QCloseEvent* event) override;

private slots:
  void onLoadImage(int idx);
  void onPrevImage();
  void onNextImage();
};
