#include <QCryptographicHash>
#include <QDirIterator>
#include <QImage>

#include "image_list_model.h"

ImageListModel::ImageListModel(QObject* parent)
    : QAbstractListModel{parent}
{
}

void ImageListModel::openFolder(const QString& folder, const Mode& folder_mode)
{
  folder_mode_ = folder_mode;

  this->beginResetModel();

  qDebug() << "=================================================";
  qDebug() << "=================================================";
  qDebug() << "openFolder(" << folder << ")";

  // A "normal" folder
  // -> Load images from the folder itself
  // -> Primary annotations are right next to the image folder
  if (!folder.contains("pred"))
  {
    current_image_folder_ = QDir(folder);
    primary_annotations_folder_ = QDir(folder);
  }

  // A "pred" folder
  // -> Load images from the parent folder
  // -> Primary annotations are:
  //    -> in the parent folder [annotation mode]
  //    -> in the selected folder [review mode]
  else
  {
    current_image_folder_ = QDir::cleanPath(folder + "/../");

    if (folder_mode == Mode::ANNOTATION)
    {
      primary_annotations_folder_ = current_image_folder_;
    }

    else if (folder_mode == Mode::REVIEW)
    {
      primary_annotations_folder_ = QDir(folder);
    }
  }

  // All subdirectories are potential folders for secondary annotions
  secondary_annotations_folders_.clear();
  {
    QDirIterator it(
        current_image_folder_.path(), QStringList() << "*", QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
      const QString new_folder = it.next();

      // qDebug() << "found subfolder as secondary annotation folder: " << new_folder;

      if (new_folder.startsWith(folder))
      {
        // qDebug() << "Prepending " << new_folder;
        secondary_annotations_folders_.prepend(new_folder);
      }
      else if (folder_mode == Mode::ANNOTATION)
      {
        // qDebug() << "Appending " << new_folder;
        secondary_annotations_folders_.append(new_folder);
      }
    }

    // qDebug() << "secondary_annotations_folders_:";
    // for (const auto& x : secondary_annotations_folders_)
    // {
    //   qDebug() << x.path();
    // }
  }

  QStringList all_image_file_names = current_image_folder_.entryList(image_filename_filter_, QDir::Filter::Files, QDir::Name);

  image_data_.clear();

  for (const QString& image_filename : all_image_file_names)
  {
    ImageData new_elem;
    new_elem.image_filename = image_filename;

    // Load image data
    QFile image_file(current_image_folder_.absoluteFilePath(new_elem.image_filename));
    new_elem.filesize = image_file.size();

    // Hash the first 5kByte of image data
    image_file.open(QIODevice::ReadOnly);
    new_elem.md5_hash = QCryptographicHash::hash(image_file.read(1024 * 5), QCryptographicHash::Algorithm::Md5);

    // Load annotation data
    const QString label_filename = getLabelFilename(image_filename);

    QFile file(current_image_folder_.absoluteFilePath(label_filename));
    if (file.open(QIODevice::ReadOnly))
    {
      QTextStream in(&file);

      while (!in.atEnd())
      {
        QString line = in.readLine();

        QStringList fields = line.split(" ");

        if (fields.size() >= 5)
        {
          new_elem.num_objects++;

          const float rel_box_width = fields[3].toFloat();
          const float rel_box_height = fields[4].toFloat();

          new_elem.min_rel_objet_size = std::min(rel_box_width, std::min(rel_box_height, new_elem.min_rel_objet_size));
          new_elem.max_rel_objet_size = std::max(rel_box_width, std::max(rel_box_height, new_elem.max_rel_objet_size));

          new_elem.label_ids.insert(fields[0].toInt());
        }
      }

      file.close();
    }

    image_data_.push_back(new_elem);
  }


  this->endResetModel();
}

void ImageListModel::removeImage(const int image_idx)
{
  emit layoutAboutToBeChanged();

  this->image_data_.remove(image_idx);
  this->removeRow(image_idx);

  emit layoutChanged();
}

int ImageListModel::rowCount(const QModelIndex& parent) const
{
  return image_data_.size();
}

int ImageListModel::columnCount(const QModelIndex& parent) const
{
  return Columns::COUNT;
}

QVariant ImageListModel::data(const QModelIndex& index, int role) const
{
  // qDebug() << "ImageListModel::data(" << index.row() << "; " << index.column() << ")";
  // return QVariant(image_file_names_.at(index.row()));

  if (!index.isValid())
  {
    return QVariant();
  }

  const QString image_filename = image_data_.at(index.row()).image_filename;

  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
    case Columns::IMAGE_FILE_NAME:
      return image_filename;

    case Columns::ANNOTATION_INPUT_FILENAME:
      return getLabelFilename(image_filename);

    case Columns::ANNOTATION_OUTPUT_FILENAME:
    {
      if (folder_mode_ == Mode::ANNOTATION)
      {
        return current_image_folder_.absoluteFilePath(imageFilenameToLabelFilename(image_filename));
      }
      else
      {
        return "";
      }
    }

    case Columns::NUM_OBJECTS:
      return image_data_.at(index.row()).num_objects;

    case Columns::MIN_REL_OBJECT_SIZE:
      return image_data_.at(index.row()).min_rel_objet_size;

    case Columns::MAX_REL_OBJECT_SIZE:
      return image_data_.at(index.row()).max_rel_objet_size;

    case Columns::LABEL_IDS:
    {
      QList<QVariant> label_ids;
      for (const auto& id : image_data_.at(index.row()).label_ids)
      {
        label_ids.push_back(id);
      }
      return label_ids;
    }

    case Columns::MD5_HASH:
      return image_data_.at(index.row()).md5_hash.toHex();

    case Columns::FILESIZE:
      return image_data_.at(index.row()).filesize;
    }
  }

  if (index.column() == Columns::IMAGE)
  {
    switch (role)
    {
      // Thumbnail image with annotation overlays
    case Qt::DecorationRole:
      // TODO: Add annotation overlays!
      return QImage(current_image_folder_.absoluteFilePath(image_data_.at(index.row()).image_filename))
          .scaled(128, 128, Qt::KeepAspectRatio, Qt::FastTransformation);

      // Raw image in full resolution (for annotation purposeses)
    case Qt::UserRole:
      return QImage(current_image_folder_.absoluteFilePath(image_data_.at(index.row()).image_filename));
    }
  }

  return QVariant();
}

QVariant ImageListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Orientation::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case Columns::IMAGE:
      return "Image";

    case Columns::IMAGE_FILE_NAME:
      return "Image Filename";

    case Columns::ANNOTATION_INPUT_FILENAME:
      return "Annotation Input Filename";

    case Columns::ANNOTATION_OUTPUT_FILENAME:
      return "Annotation Output Filename";

    case Columns::NUM_OBJECTS:
      return "Num. Objects";

    case MIN_REL_OBJECT_SIZE:
      return "Min. rel. object size";

    case MAX_REL_OBJECT_SIZE:
      return "Max. rel. object size";

    case LABEL_IDS:
      return "Label IDs";

    case Columns::MD5_HASH:
      return "MD5 Hash";

    case Columns::FILESIZE:
      return "Filesize";
    }
  }

  // Fallback
  return QAbstractListModel::headerData(section, orientation, role);
}

QString ImageListModel::getImageFilename(const int image_idx) const
{
  return this->data(this->index(image_idx, Columns::IMAGE_FILE_NAME), Qt::DisplayRole).value<QString>();
}

QString ImageListModel::getFullImagePath(const int image_idx) const
{
  return current_image_folder_.absoluteFilePath(
      this->data(this->index(image_idx, Columns::IMAGE_FILE_NAME), Qt::DisplayRole).value<QString>());
}

QImage ImageListModel::getFullResImage(const int image_idx) const
{
  return this->data(this->index(image_idx, Columns::IMAGE), Qt::UserRole).value<QImage>();
}

QString ImageListModel::getAnnotationInputFilename(const int image_idx) const
{
  return this->data(this->index(image_idx, Columns::ANNOTATION_INPUT_FILENAME), Qt::DisplayRole).value<QString>();
}

QString ImageListModel::getAnnotationOutputFilename(const int image_idx) const
{
  return this->data(this->index(image_idx, Columns::ANNOTATION_OUTPUT_FILENAME), Qt::DisplayRole).value<QString>();
}

QString ImageListModel::imageFilenameToLabelFilename(const QString& image_filename)
{
  return QFileInfo(image_filename).completeBaseName() + ".txt";
}

QString ImageListModel::getLabelFilename(const QString& image_filename) const
{
  // Try primary labelfile
  const QString label_filename = imageFilenameToLabelFilename(image_filename);

  if (QFile(primary_annotations_folder_.absoluteFilePath(label_filename)).exists())
  {
    return primary_annotations_folder_.absoluteFilePath(label_filename);
  }

  for (const QDir& annotation_folder : secondary_annotations_folders_)
  {
    if (QFile(annotation_folder.absoluteFilePath(label_filename)).exists())
    {
      return annotation_folder.absoluteFilePath(label_filename);
    }
  }

  // Fallback: No annotation file is available
  return "";
}

QDir& ImageListModel::currentImageFolder()
{
  return current_image_folder_;
}
