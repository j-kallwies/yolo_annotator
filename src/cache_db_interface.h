#pragma once

#include <QCache>
#include <QDir>

#include <string>

#include "sqlite_orm.h"

struct DBPreviewImage
{
  int id;
  std::string md5_hash;
  int filesize;
  std::vector<char> preview_image;
  int preview_width;
  int preview_height;
};

class CacheDBConnection
{
public:
  CacheDBConnection(const QDir& root_path);

  void storePreviewImage(const QString& md5_hash, const int filesize, const QImage& image);
  std::optional<QImage> getPreviewImage(const QString& md5_hash, const int filesize) const;

  using StorageType = sqlite_orm::internal::storage_t<sqlite_orm::internal::table_t<
      DBPreviewImage,
      false,
      sqlite_orm::internal::column_t<int DBPreviewImage::*,
                                     sqlite_orm::internal::empty_setter,
                                     sqlite_orm::internal::primary_key_with_autoincrement<sqlite_orm::internal::primary_key_t<>>>,
      sqlite_orm::internal::column_t<std::string DBPreviewImage::*, sqlite_orm::internal::empty_setter>,
      sqlite_orm::internal::column_t<int DBPreviewImage::*, sqlite_orm::internal::empty_setter>,
      sqlite_orm::internal::column_t<std::vector<char> DBPreviewImage::*, sqlite_orm::internal::empty_setter>,
      sqlite_orm::internal::column_t<int DBPreviewImage::*, sqlite_orm::internal::empty_setter>,
      sqlite_orm::internal::column_t<int DBPreviewImage::*, sqlite_orm::internal::empty_setter>>>;

  std::unique_ptr<StorageType> storage_;

  mutable QMap<QString, QImage> preview_image_cache_;
};
