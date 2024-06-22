#include <QFileIconProvider>
#include <QGraphicsPixmapItem>
#include <QProcess>
#include <QRandomGenerator>

#include <memory>

#include "annotationboundingbox.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  annotation_manager_ = std::make_unique<AnnotationManager>(ui->image_view);

  ui->image_view->init(annotation_manager_.get());

  ui->image_view->setScene(scene_);
  ui->image_view->setInteractive(true);

  folder_tree_model_.setOption(QFileSystemModel::DontWatchForChanges);

  // QFileIconProvider iconProvider;
  // folder_tree_model_.setIconProvider(&iconProvider);
  folder_tree_model_.setRootPath(root_path_);

  folder_tree_model_.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

  ui->folder_tree_view->setModel(&folder_tree_model_);

  if (!root_path_.isEmpty())
  {
    const QModelIndex rootIndex = folder_tree_model_.index(QDir::cleanPath(root_path_));
    if (rootIndex.isValid())
      ui->folder_tree_view->setRootIndex(rootIndex);
  }

  ui->folder_tree_view->hideColumn(1);
  ui->folder_tree_view->hideColumn(2);
  ui->folder_tree_view->hideColumn(3);

  // Restore the previous state
  this->restoreGeometry(settings_.value("window/geometry").toByteArray());
  this->restoreState(settings_.value("window/state").toByteArray());
  ui->splitter->restoreState(settings_.value("splitter/state").toByteArray());

  connect(ui->folder_tree_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onSelectFolder);

  connect(ui->image_slider, &QSlider::valueChanged, this, &MainWindow::onLoadImage);
  openFolder("/Users/jan/yolo_data/");

  connect(ui->image_slider, &QSlider::valueChanged, this, &MainWindow::onLoadImage);

  connect(&prev_image_shortcut_, &QShortcut::activated, this, &MainWindow::onPrevImage);
  connect(&next_image_shortcut_, &QShortcut::activated, this, &MainWindow::onNextImage);

  connect(&remove_image_shortcut_, &QShortcut::activated, this, &MainWindow::onRemoveImage);

  connect(&edit_image_shortcut_, &QShortcut::activated, this, &MainWindow::onEditImage);

  connect(&move_to_random_set_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToRandomSet);
  connect(&move_to_train_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToTrain);
  connect(&move_to_val_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToVal);
  connect(&move_to_test_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToTest);

  move_to_train_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);
  move_to_val_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);
  move_to_test_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);

  prev_image_shortcut_.setAutoRepeat(true);
  prev_image_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);

  next_image_shortcut_.setAutoRepeat(true);
  next_image_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);

  connect(ui->fit_view_button,
          &QPushButton::clicked,
          [this](const bool checked)
          {
            if (checked)
            {
              ui->image_view->fitViewToImage();
            }
          });

  // Load CNN weight files
  {
    ui->cnn_model->clear();
    QStringList pt_files = QDir(root_path_).entryList({"*.pt"}, QDir::Filter::Files, QDir::Name);

    for (const auto pt_filename : pt_files)
    {
      ui->cnn_model->addItem(pt_filename);
    }
  }
}

void MainWindow::onLoadImage(int image_id)
{
  if (image_file_names_.size() > 0)
  {
    QString image_filename = image_file_names_.at(image_id - 1);

    ui->image_index_label->setText(QString("Image %1 / %2: %3").arg(image_id).arg(image_file_names_.size()).arg(image_filename));

    annotation_manager_->save();

    loadImage(current_folder_.filePath(image_filename));
  }
}

void MainWindow::onSelectFolder(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (selected.size() == 1)
  {
    const auto selected_index = selected.at(0).indexes().at(0);
    openFolder(folder_tree_model_.filePath(selected_index));
  }
}

void MainWindow::onPrevImage()
{
  ui->image_slider->setValue(ui->image_slider->value() - 1);
}

void MainWindow::onNextImage()
{
  ui->image_slider->setValue(ui->image_slider->value() + 1);
}

void MainWindow::onEditImage()
{
  QProcess* image_editor_process = new QProcess(this);

  QStringList arguments;
  arguments << "-a"
            << "Affinity Photo 2.app" << current_folder_.absoluteFilePath(image_file_names_.at(ui->image_slider->value() - 1));

  image_editor_process->start("open", arguments);
}

void MainWindow::onRemoveImage()
{
  const QString image_filename = image_file_names_.at(ui->image_slider->value() - 1);
  const QString label_filename = getLabelFilename(image_filename);

  qDebug() << "Remove " << QFile(current_folder_.absoluteFilePath(image_filename)).fileName();
  qDebug() << "Remove " << QFile(current_folder_.absoluteFilePath(label_filename)).fileName();

  QFile(current_folder_.absoluteFilePath(image_filename)).moveToTrash();
  QFile(current_folder_.absoluteFilePath(label_filename)).moveToTrash();

  // Remove the image from the list
  image_file_names_.removeAt(ui->image_slider->value() - 1);

  // "Reload" => Load next image
  ui->image_slider->setMaximum(image_file_names_.size());
  onLoadImage(ui->image_slider->value());
}

void MainWindow::moveCurrentImageToFolder(const QString& folder)
{
  // Make sure that the latest changes are saved.
  annotation_manager_->save();

  const QString image_filename = image_file_names_.at(ui->image_slider->value() - 1);
  const QString label_filename = getLabelFilename(image_filename);

  // TODO: also make possible sibling folders! (e.g. train -> val)
  QString rel_new_folder = folder;

  QString new_image_filename = rel_new_folder + QDir::separator() + image_filename;
  QString new_label_filename = rel_new_folder + QDir::separator() + label_filename;

  current_folder_.mkpath(rel_new_folder);

  // Move the image file
  current_folder_.rename(image_filename, new_image_filename);

  // Move the label file
  current_folder_.rename(label_filename, new_label_filename);
  annotation_manager_->clear();

  // Remove the image from the list
  image_file_names_.removeAt(ui->image_slider->value() - 1);

  // "Reload" => Load next image
  ui->image_slider->setMaximum(image_file_names_.size());
  onLoadImage(ui->image_slider->value());
}

void MainWindow::onMoveImageToRandomSet()
{
  const int v = QRandomGenerator::system()->bounded(0, 10);

  switch (v)
  {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
    moveCurrentImageToFolder("train");
    break;

  case 8:
    moveCurrentImageToFolder("val");
    break;

  case 9:
    moveCurrentImageToFolder("test");
    break;
  }
}

void MainWindow::onMoveImageToTrain()
{
  moveCurrentImageToFolder("train");
}

void MainWindow::onMoveImageToVal()
{
  moveCurrentImageToFolder("val");
}

void MainWindow::onMoveImageToTest()
{
  moveCurrentImageToFolder("test");
}

void MainWindow::openFolder(const QString& folder)
{
  current_folder_ = QDir(folder);
  image_file_names_ = current_folder_.entryList(image_filename_filter_, QDir::Filter::Files, QDir::Name);
  ui->image_slider->setMinimum(1);
  ui->image_slider->setMaximum(image_file_names_.size());
  ui->image_slider->setValue(1);

  // Load the first image of this folder
  onLoadImage(1);
}

QString MainWindow::getLabelFilename(const QString& image_filename)
{
  const QFileInfo f(image_filename);
  return f.dir().path() + "/" + f.completeBaseName() + ".txt";
}

void MainWindow::loadImage(const QString& image_filename)
{
  const QString label_filename = getLabelFilename(image_filename);

  // Load the image
  QImage image(image_filename);

  if (ui->scale_to_cnn_resolution->isChecked())
  {
    const int cnn_image_size = QString(ui->cnn_image_size->currentText()).toInt();
    image = image.scaled(cnn_image_size, cnn_image_size, Qt::KeepAspectRatio);
  }

  ui->image_view->setImage(image, ui->fit_view_button->isChecked());

  // Load the annotations
  annotation_manager_->loadFromFile(label_filename, image.size());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  settings_.setValue("window/geometry", saveGeometry());
  settings_.setValue("window/state", saveState());
  settings_.setValue("splitter/state", ui->splitter->saveState());

  QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
}
