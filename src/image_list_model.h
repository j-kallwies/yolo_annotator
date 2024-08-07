#pragma once

#include <QAbstractItemModel>
#include <QDir>
#include <QImage>

#include "annotationboundingbox.h"
#include "cache_db_interface.h"

struct ImageData
{
  QString image_filename;
  QByteArray md5_hash;
  int filesize{0};
  float min_rel_objet_size{std::numeric_limits<float>::infinity()};
  float max_rel_objet_size{0.f};
  QSet<int> label_ids;
  QList<QStringList> annotations;
};

class ImageListModel : public QAbstractListModel
{
public:
  enum Columns
  {
    IMAGE,
    IMAGE_FILE_NAME,
    ANNOTATION_INPUT_FILENAME,
    ANNOTATION_OUTPUT_FILENAME,
    NUM_OBJECTS,
    MIN_REL_OBJECT_SIZE,
    MAX_REL_OBJECT_SIZE,
    LABEL_IDS,
    MD5_HASH,
    FILESIZE,
    COUNT
  };

  enum Mode
  {
    ANNOTATION,
    REVIEW
  };

  explicit ImageListModel(const QDir& root_path, QObject* parent = nullptr);

  void openFolder(const QString& folder, const Mode& folder_mode);

  void setFolderMode(const Mode& folder_mode);

  void removeImage(const int image_idx);

  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  int columnCount(const QModelIndex& parent = QModelIndex()) const;

  QImage getPreviewImage(const int image_idx) const;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  QString getImageFilename(const int image_idx) const;
  QString getFullImagePath(const int image_idx) const;
  QImage getFullResImage(const int image_idx) const;
  QString getAnnotationInputFilename(const int image_idx) const;
  QString getAnnotationOutputFilename(const int image_idx) const;

  QDir& currentImageFolder();

  Mode currentFolderMode();

private:
  QString opened_folder_;
  QDir current_image_folder_;
  QDir primary_annotations_folder_;
  QList<QDir> secondary_annotations_folders_;
  QList<ImageData> image_data_;

  Mode folder_mode_;

  const QStringList image_filename_filter_{"*.jpg", "*.jpeg", "*.png", "*.webp"};

  mutable CacheDBConnection cache_db_;

  static QString imageFilenameToLabelFilename(const QString& image_filename);
  QString getLabelFilename(const QString& image_filename) const;
};
