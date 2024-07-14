#include <QDebug>
#include <QElapsedTimer>
#include <QImage>
#include <QString>

#include "cache_db_interface.h"

using namespace sqlite_orm;

CacheDBConnection::CacheDBConnection(const QDir& root_path)
{
  storage_ =
      std::make_unique<StorageType>(make_storage(root_path.absoluteFilePath("cache.sqlite").toStdString(),
                                                 make_table("preview_images",
                                                            make_column("id", &DBPreviewImage::id, primary_key().autoincrement()),
                                                            make_column("md5_hash", &DBPreviewImage::md5_hash),
                                                            make_column("filesize", &DBPreviewImage::filesize),
                                                            make_column("preview_image", &DBPreviewImage::preview_image),
                                                            make_column("preview_width", &DBPreviewImage::preview_width),
                                                            make_column("preview_height", &DBPreviewImage::preview_height))));

  storage_->sync_schema(false);

  QElapsedTimer timer;
  timer.start();

  auto all_db_preview_images = storage_->get_all<DBPreviewImage>();
  for (const DBPreviewImage& db_image : all_db_preview_images)
  {
    preview_image_cache_.insert(
        QString::fromStdString(db_image.md5_hash),
        QImage((uchar*)db_image.preview_image.data(), db_image.preview_width, db_image.preview_height, QImage::Format_RGB888)
            .copy());
  }

  qDebug() << "Loading " << all_db_preview_images.size() << " preview images from the SQLite Database took " << timer.elapsed()
           << "ms";
}

void CacheDBConnection::storePreviewImage(const QString& md5_hash, const int filesize, const QImage& image)
{
  // TODO: Also check filesize!

  // auto existing_elements =
  // storage_->get_all<DBPreviewImage>(sqlite_orm::where(sqlite_orm::c(&DBPreviewImage::md5_hash) == md5_hash.toStdString()));

  // qDebug() << "found " << existing_elements.size() << " with hash=" << md5_hash;

  DBPreviewImage db_image;
  db_image.filesize = filesize;
  db_image.md5_hash = md5_hash.toStdString();

  db_image.preview_image.resize(image.sizeInBytes());
  std::memcpy(db_image.preview_image.data(), (const char*)image.constBits(), image.sizeInBytes());

  // db_image.preview_image = std::vector<char>(char(255), image.sizeInBytes());

  db_image.preview_width = image.width();
  db_image.preview_height = image.height();

  auto insertedId = storage_->insert(db_image);
  qDebug() << "inserted image with id=" << insertedId << ", hash=" << md5_hash << ", bytes=" << image.sizeInBytes()
           << ", image_size=" << image.width() << "x" << image.height();
}

std::optional<QImage> CacheDBConnection::getPreviewImage(const QString& md5_hash, const int filesize) const
{
  if (preview_image_cache_.contains(md5_hash))
  {
    return preview_image_cache_[md5_hash];
  }

  // TODO: Also check filesize!
  auto existing_elements =
      storage_->get_all<DBPreviewImage>(sqlite_orm::where(sqlite_orm::c(&DBPreviewImage::md5_hash) == md5_hash.toStdString()));

  // qDebug() << "found " << existing_elements.size() << " with hash=" << md5_hash;

  if (existing_elements.size() > 0)
  {
    const DBPreviewImage& db_image = existing_elements[0];
    // qDebug() << "db_image.preview_image.size()=" << db_image.preview_image.size();

    QImage output_image =
        QImage((uchar*)db_image.preview_image.data(), db_image.preview_width, db_image.preview_height, QImage::Format_RGB888);

    preview_image_cache_.insert(md5_hash, output_image.copy());

    return preview_image_cache_[md5_hash];
  }

  return {};
}
