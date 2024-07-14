#include <QFileIconProvider>
#include <QGraphicsPixmapItem>
#include <QProcess>
#include <QRandomGenerator>

#include <memory>

#include "annotationboundingbox.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(const QString& root_path, const QStringList& label_names, QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      root_path_(root_path),
      label_names_(label_names),
      image_list_model_(new ImageListModel(root_path_, this)),
      image_sort_filter_proxy_model_(new ImageSortFilterProxy(this))

{
  ui->setupUi(this);

  annotation_manager_ = std::make_unique<AnnotationManager>(ui->image_view, label_names_);

  ui->image_view->init(annotation_manager_.get(), &label_names_);

  ui->image_view->setScene(scene_);
  ui->image_view->setInteractive(true);

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

  ui->filter_by_label_combobox->clear();
  for (const auto& label_name : label_names)
  {
    ui->filter_by_label_combobox->addItem(label_name);
  }

  image_sort_filter_proxy_model_->setSourceModel(image_list_model_);

  ui->image_grid_view->setResizeMode(QListView::Adjust);
  ui->image_grid_view->setUniformItemSizes(true);
  ui->image_grid_view->setViewMode(QListView::ViewMode::IconMode);
  // ui->image_grid_view->setIconSize(QSize(64, 64));
  // ui->image_grid_view->setWordWrap(true);
  // ui->image_grid_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  // ui->image_grid_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  // ui->image_grid_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  ui->image_grid_view->setModel(image_sort_filter_proxy_model_);
  // ui->image_grid_view->showNormal();

  ui->images_table_view->setModel(image_sort_filter_proxy_model_);
  ui->images_table_view->horizontalHeader()->show();
  ui->images_table_view->verticalHeader()->show();

  // Restore the previous state
  this->restoreGeometry(settings_.value("window/geometry").toByteArray());
  this->restoreState(settings_.value("window/state").toByteArray());
  ui->splitter->restoreState(settings_.value("splitter/state").toByteArray());

  connect(ui->folder_tree_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onSelectFolder);

  connect(ui->image_slider, &QSlider::valueChanged, this, &MainWindow::onLoadImage);

  connect(&prev_image_shortcut_, &QShortcut::activated, this, &MainWindow::onPrevImage);
  connect(&next_image_shortcut_, &QShortcut::activated, this, &MainWindow::onNextImage);

  connect(&remove_image_shortcut_, &QShortcut::activated, this, &MainWindow::onRemoveImage);

  connect(&edit_image_shortcut_, &QShortcut::activated, this, &MainWindow::onEditImage);

  connect(&move_to_random_set_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToRandomSet);
  connect(&move_to_train_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToTrain);
  connect(&move_to_val_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToVal);
  connect(&move_to_test_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToTest);
  connect(&move_to_merge_shortcut_, &QShortcut::activated, this, &MainWindow::onMoveImageToMerge);

  connect(ui->predict_button, &QPushButton::clicked, this, &MainWindow::onStartPrediction);

  connect(ui->filter_by_num_objects, &QGroupBox::toggled, this, &MainWindow::onUpdateFiltering);
  connect(ui->min_num_objects, SIGNAL(valueChanged(int)), this, SLOT(onUpdateFiltering()));
  connect(ui->max_num_objects, SIGNAL(valueChanged(int)), this, SLOT(onUpdateFiltering()));
  connect(ui->min_rel_bbox_size, SIGNAL(valueChanged(double)), this, SLOT(onUpdateFiltering()));
  connect(ui->max_rel_bbox_size, SIGNAL(valueChanged(double)), this, SLOT(onUpdateFiltering()));
  connect(ui->filter_by_label, SIGNAL(toggled(bool)), this, SLOT(onUpdateFiltering()));
  connect(ui->filter_by_label_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(onUpdateFiltering()));
  connect(ui->filter_by_filename, SIGNAL(toggled(bool)), this, SLOT(onUpdateFiltering()));
  connect(ui->filter_by_filename_edit, SIGNAL(textChanged(QString)), this, SLOT(onUpdateFiltering()));

  connect(this->image_list_model_, SIGNAL(modelReset()), this, SLOT(onImageListModelReset()));

  connect(ui->folder_mode, SIGNAL(currentIndexChanged(int)), this, SLOT(onFolderModeChanged(int)));

  move_to_train_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);
  move_to_val_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);
  move_to_test_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);
  move_to_merge_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);

  prev_image_shortcut_.setAutoRepeat(true);
  prev_image_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);

  next_image_shortcut_.setAutoRepeat(true);
  next_image_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);

  connect(this->image_sort_filter_proxy_model_, &ImageSortFilterProxy::rowsRemoved, [this]() { this->onImageListModelReset(); });

  connect(this->image_sort_filter_proxy_model_, &ImageSortFilterProxy::rowsInserted, [this]() { this->onImageListModelReset(); });

  connect(ui->fit_view_button,
          &QPushButton::clicked,
          [this](const bool checked)
          {
            if (checked)
            {
              ui->image_view->fitViewToImage();
            }
          });

  connect(&predict_process_,
          &QProcess::readyRead,
          [this]()
          {
            if (predict_process_.canReadLine())
            {
              ui->predict_command_output->moveCursor(QTextCursor::End);
              ui->predict_command_output->insertPlainText(QString::fromStdString(predict_process_.readLine().toStdString()));
              ui->predict_command_output->moveCursor(QTextCursor::End);
            }
          });

  connect(&predict_process_, &QProcess::terminate, [this]() { qDebug() << "YOLO terminated."; });

  // Load CNN weight files
  {
    ui->cnn_model->clear();
    QStringList pt_files = QDir(root_path_).entryList({"*.pt"}, QDir::Filter::Files, QDir::Name);

    for (const auto& pt_filename : pt_files)
    {
      ui->cnn_model->addItem(pt_filename);
    }

    ui->cnn_model->setCurrentIndex(ui->cnn_model->count() - 1);
  }
}

void MainWindow::onImageListModelReset()
{
  ui->image_slider->setMinimum(1);
  ui->image_slider->setMaximum(image_sort_filter_proxy_model_->rowCount());

  annotation_manager_->clear();
  ui->image_view->clear();
  ui->image_index_label->setText("");

  // Load the first image of this folder
  if (ui->keep_image_index->isChecked() == false)
  {
    ui->image_slider->setValue(1);
    onLoadImage(1);
  }
  else
  {
    onLoadImage(ui->image_slider->value());
  }
}

void MainWindow::onFolderModeChanged(int folder_mode)
{
  switch (folder_mode)
  {
  case ImageListModel::Mode::ANNOTATION:
    this->ui->image_view->setEditingMode(true);
    break;

  case ImageListModel::Mode::REVIEW:
  default:
    this->ui->image_view->setEditingMode(false);
    break;
  }
}

void MainWindow::onLoadImage(int image_id)
{
  annotation_manager_->save();

  loadImage(image_id - 1);

  const QString image_filename = image_list_model_->getImageFilename(
      image_sort_filter_proxy_model_->mapToSource(image_sort_filter_proxy_model_->index(image_id - 1, 0)).row());
}

void MainWindow::onUpdateFiltering()
{
  // Filter by filename
  image_sort_filter_proxy_model_->setFilterByFilename(ui->filter_by_filename_edit->text(), ui->filter_by_filename->isChecked());

  // Rel. object size
  image_sort_filter_proxy_model_->setFilterRelObjectSize(
      ui->min_rel_bbox_size->value(), ui->max_rel_bbox_size->value(), ui->filter_by_rel_bbox_size->isChecked());

  // Num. objects
  image_sort_filter_proxy_model_->setFilterByNumObjects(
      ui->min_num_objects->value(), ui->max_num_objects->value(), ui->filter_by_num_objects->isChecked());

  // Filter by label id
  image_sort_filter_proxy_model_->setFilterByLabelId(ui->filter_by_label_combobox->currentIndex(),
                                                     ui->filter_by_label->isChecked());
}

void MainWindow::onSelectFolder(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (selected.size() == 1)
  {
    const auto selected_index = selected.at(0).indexes().at(0);
    image_list_model_->openFolder(folder_tree_model_.filePath(selected_index),
                                  ImageListModel::Mode(ui->folder_mode->currentIndex()));
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
            << "Affinity Photo 2.app"
            << image_list_model_->getFullImagePath(image_sort_filter_proxy_model_->mapRowToSource(ui->image_slider->value() - 1));

  qDebug() << arguments;

  image_editor_process->start("open", arguments);
}

void MainWindow::onRemoveImage()
{
  auto removeImage = [this](const int row)
  {
    const QString image_filename = image_list_model_->getFullImagePath(image_sort_filter_proxy_model_->mapRowToSource(row));
    const QString label_filename =
        image_list_model_->getAnnotationOutputFilename(image_sort_filter_proxy_model_->mapRowToSource(row));

    qDebug() << "Remove " << image_filename;
    qDebug() << "Remove " << label_filename;

    QFile(image_filename).moveToTrash();
    QFile(label_filename).moveToTrash();

    // Remove the image from the list
    image_list_model_->removeImage(image_sort_filter_proxy_model_->mapRowToSource(row));
    image_sort_filter_proxy_model_->removeRow(row);
  };

  // Image View: Remove the currently shown image
  if (ui->image_view_container->currentIndex() == 0)
  {
    removeImage(ui->image_slider->value() - 1);

    // "Reload" => Load next image
    ui->image_slider->setMaximum(image_sort_filter_proxy_model_->rowCount());
    onLoadImage(ui->image_slider->value());
  }
  // Grid View: Remove the currently shown image
  else if (ui->image_view_container->currentIndex() == 1)
  {
    std::vector<int> rows_to_remove;

    for (const auto& idx : ui->image_grid_view->selectionModel()->selectedIndexes())
    {
      rows_to_remove.push_back(idx.row());
    }

    for (auto it = rows_to_remove.rbegin(); it != rows_to_remove.rend(); it++)
    {
      removeImage(*it);
    }

    ui->image_grid_view->selectionModel()->clear();
  }
}

void MainWindow::moveCurrentImageToFolder(const QString& folder)
{
  // Make sure that the latest changes are saved.
  annotation_manager_->save();

  const int image_idx = ui->image_slider->value() - 1;

  const QString image_filename = image_list_model_->getImageFilename(image_sort_filter_proxy_model_->mapRowToSource(image_idx));
  const QString label_filename = image_list_model_->currentImageFolder().relativeFilePath(
      image_list_model_->getAnnotationOutputFilename(image_sort_filter_proxy_model_->mapRowToSource(image_idx)));

  // TODO: also make possible sibling folders! (e.g. train -> val)
  QString rel_new_folder = folder;

  QString new_image_filename = rel_new_folder + QDir::separator() + image_filename;
  QString new_label_filename = rel_new_folder + QDir::separator() + label_filename;

  image_list_model_->currentImageFolder().mkpath(rel_new_folder);

  qDebug() << "MOVE " << image_filename << " -> " << new_image_filename;
  qDebug() << "MOVE " << label_filename << " -> " << new_label_filename;

  // Move the image file
  image_list_model_->currentImageFolder().rename(image_filename, new_image_filename);

  // Move the label file
  image_list_model_->currentImageFolder().rename(label_filename, new_label_filename);
  annotation_manager_->clear();

  // Remove the image from the list
  image_list_model_->removeImage(image_sort_filter_proxy_model_->mapRowToSource(image_idx));
  image_sort_filter_proxy_model_->removeRow(image_idx);

  // "Reload" => Load next image
  ui->image_slider->setMaximum(image_sort_filter_proxy_model_->rowCount());
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

void MainWindow::onMoveImageToMerge()
{
  moveCurrentImageToFolder("merge");
}

void MainWindow::onStartPrediction(bool checked)
{
  qDebug() << "predict_process_ start!";

  predict_process_.setProgram("/opt/homebrew/anaconda3/bin/yolo");

  const QString folder_name = QDir(root_path_).relativeFilePath(this->image_list_model_->currentImageFolder().path());

  QStringList args;
  args.append("predict");
  args.append(QString("model=%1").arg(ui->cnn_model->currentText()));
  args.append(QString("source='%1'").arg(folder_name));
  args.append(QString("imgsz=%1").arg(ui->cnn_image_size->currentText()));
  args.append(QString("project='%1'").arg(folder_name));
  args.append(QString("name=pred_%1_%2").arg(ui->cnn_image_size->currentText()).arg(ui->cnn_model->currentText()));
  args.append("save_txt=True");
  args.append("save_conf=True");
  args.append("save=False");
  args.append("device=mps"); // TODO!

  predict_process_.setWorkingDirectory(root_path_);

  predict_process_.setArguments(args);

  predict_process_.start();

  ui->predict_command_output->clear();

  if (predict_process_.waitForStarted())
  {
    ui->predict_command_output->moveCursor(QTextCursor::End);
    ui->predict_command_output->insertPlainText("YOLO started...");
    ui->predict_command_output->moveCursor(QTextCursor::End);
  }
  else
  {
    ui->predict_command_output->moveCursor(QTextCursor::End);
    ui->predict_command_output->insertPlainText("COULD NOT START YOLO!");
    ui->predict_command_output->moveCursor(QTextCursor::End);
  }
}

void MainWindow::loadImage(const int image_idx)
{
  const QImage image = image_list_model_->getFullResImage(image_sort_filter_proxy_model_->mapRowToSource(image_idx));
  const QString input_label_filename =
      image_list_model_->getAnnotationInputFilename(image_sort_filter_proxy_model_->mapRowToSource(image_idx));
  const QString output_label_filename =
      image_list_model_->getAnnotationOutputFilename(image_sort_filter_proxy_model_->mapRowToSource(image_idx));

  if (ui->scale_to_cnn_resolution->isChecked())
  {
    const int cnn_image_size = QString(ui->cnn_image_size->currentText()).toInt();
    ui->image_view->setImage(image.scaled(cnn_image_size, cnn_image_size, Qt::KeepAspectRatio), ui->fit_view_button->isChecked());
  }
  else
  {
    ui->image_view->setImage(image, ui->fit_view_button->isChecked());
  }

  const QString image_filename = image_list_model_->getImageFilename(image_sort_filter_proxy_model_->mapRowToSource(image_idx));

  ui->image_index_label->setText(QString("Image %1 / %2: %3 (%4 x %5 px)")
                                     .arg(image_idx + 1)
                                     .arg(image_sort_filter_proxy_model_->rowCount())
                                     .arg(image_filename) // QDir(root_path_).relativeFilePath(
                                     .arg(image.width())
                                     .arg(image.height()));

  // Load the annotations
  {
    ui->annotation_filenames_label->setText(QString("📂 %1\n📝 %2")
                                                .arg(QDir(root_path_).relativeFilePath(input_label_filename),
                                                     QDir(root_path_).relativeFilePath(output_label_filename)));

    if (ui->filter_by_label->isChecked())
    {
      annotation_manager_->prefered_label_id_ = ui->filter_by_label_combobox->currentIndex();
    }
    else
    {
      annotation_manager_->prefered_label_id_.reset();
    }
    annotation_manager_->loadFromFile(input_label_filename, image.size());
    annotation_manager_->setLabelOutputFilename(output_label_filename);
  }
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
