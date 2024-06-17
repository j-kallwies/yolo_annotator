#include <QFileIconProvider>
#include <QGraphicsPixmapItem>

#include <memory>

#include "annotationboundingbox.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  ui->image_view->init(&annotation_manager_, &selected_bbox_id_);

  ui->image_view->setScene(scene_);
  ui->image_view->setInteractive(true);

  // folder_tree_model_.setOption(QFileSystemModel::DontWatchForChanges);

  const QString rootPath = "/Users/jan/Library/Mobile Documents/com~apple~CloudDocs/Documents/yolo_data/";

  // QFileIconProvider iconProvider;
  // folder_tree_model_.setIconProvider(&iconProvider);
  folder_tree_model_.setRootPath(rootPath);
  // folder_tree_model_.setRootPath("/Users/jan/");

  folder_tree_model_.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

  ui->folder_tree_view->setModel(&folder_tree_model_);

  if (!rootPath.isEmpty())
  {
    const QModelIndex rootIndex = folder_tree_model_.index(QDir::cleanPath(rootPath));
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

  openFolder("/Users/jan/Library/Mobile Documents/com~apple~CloudDocs/Documents/yolo_data/");

  loadImage("/Users/jan/Library/Mobile Documents/com~apple~CloudDocs/Documents/yolo_data/IMG_0445.jpeg");

  connect(ui->image_slider, &QSlider::valueChanged, this, &MainWindow::onLoadImage);

  connect(&prev_image_shortcut_, &QShortcut::activated, this, &MainWindow::onPrevImage);
  connect(&next_image_shortcut_, &QShortcut::activated, this, &MainWindow::onNextImage);

  prev_image_shortcut_.setAutoRepeat(true);
  prev_image_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);

  next_image_shortcut_.setAutoRepeat(true);
  next_image_shortcut_.setContext(Qt::ShortcutContext::ApplicationShortcut);
}

void MainWindow::onLoadImage(int image_id)
{
  QString image_filename = image_file_names_.at(image_id - 1);

  ui->image_index_label->setText(QString("Image %1 / %2: %3").arg(image_id).arg(image_file_names_.size()).arg(image_filename));

  loadImage(current_folder_.filePath(image_filename));
}

void MainWindow::onPrevImage()
{
  ui->image_slider->setValue(ui->image_slider->value() - 1);
}

void MainWindow::onNextImage()
{
  ui->image_slider->setValue(ui->image_slider->value() + 1);
}

void MainWindow::openFolder(const QString& folder)
{
  current_folder_ = QDir(folder);
  image_file_names_ = current_folder_.entryList(image_filename_filter_, QDir::Filter::Files, QDir::Name);
  ui->image_slider->setMinimum(1);
  ui->image_slider->setMaximum(image_file_names_.size());
  ui->image_slider->setValue(1);
}

void MainWindow::loadImage(const QString& image_filename)
{
  annotation_manager_.clear();

  const QFileInfo f(image_filename);

  qDebug() << "f.filePath()=" << f.filePath();
  qDebug() << "f.dir().path()=" << f.dir().path();
  qDebug() << "f.baseName()=" << f.baseName();

  const QString label_filename = f.dir().path() + "/" + f.baseName() + ".txt";

  qDebug() << "label_filename=" << label_filename;

  const QImage image(image_filename);

  ui->image_view->setImage(image);

  QFile file(label_filename);
  if (file.open(QIODevice::ReadOnly))
  {
    QTextStream in(&file);

    while (!in.atEnd())
    {
      QString line = in.readLine();
      QStringList fields = line.split(" ");

      if (fields.size() == 5)
      {
        std::shared_ptr<AnnotationBoundingBox> new_bbox(new AnnotationBoundingBox(QSize(image.size())));

        const float x_center = fields[1].toFloat() * image.width();
        const float y_center = fields[2].toFloat() * image.height();
        const float box_width = fields[3].toFloat() * image.width();
        const float box_height = fields[4].toFloat() * image.height();

        new_bbox->setRect(
            QRectF(QPointF(x_center - box_width / 2.f, y_center - box_height / 2.f), QSizeF(box_width, box_height)));
        new_bbox->setLabelID(fields[0].toInt());

        // Show item!
        ui->image_view->scene()->addItem(new_bbox.get());

        annotation_manager_.add(new_bbox);
      }
    }

    file.close();
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
