// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/browser/fileapi/syncable/syncable_file_system_util.h"

#include "base/logging.h"
#include "base/message_loop.h"
#include "base/message_loop/message_loop_proxy.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/browser/fileapi/external_mount_points.h"
#include "webkit/browser/fileapi/syncable/canned_syncable_file_system.h"
#include "webkit/browser/fileapi/syncable/local_file_sync_context.h"
#include "webkit/common/fileapi/file_system_types.h"

using fileapi::ExternalMountPoints;
using fileapi::FileSystemURL;

namespace sync_file_system {

namespace {

const char kSyncableFileSystemRootURI[] =
    "filesystem:http://www.example.com/external/syncfs/";
const char kNonRegisteredFileSystemRootURI[] =
    "filesystem:http://www.example.com/external/non_registered/";
const char kNonSyncableFileSystemRootURI[] =
    "filesystem:http://www.example.com/temporary/";

const char kOrigin[] = "http://www.example.com/";
const base::FilePath::CharType kPath[] = FILE_PATH_LITERAL("dir/file");

FileSystemURL CreateFileSystemURL(const std::string& url) {
  return ExternalMountPoints::GetSystemInstance()->CrackURL(GURL(url));
}

base::FilePath CreateNormalizedFilePath(const base::FilePath::CharType* path) {
  return base::FilePath(path).NormalizePathSeparators();
}

}  // namespace

TEST(SyncableFileSystemUtilTest, GetSyncableFileSystemRootURI) {
  const GURL root = GetSyncableFileSystemRootURI(GURL(kOrigin));
  EXPECT_TRUE(root.is_valid());
  EXPECT_EQ(GURL(kSyncableFileSystemRootURI), root);
}

TEST(SyncableFileSystemUtilTest, CreateSyncableFileSystemURL) {
  RegisterSyncableFileSystem();

  const base::FilePath path(kPath);
  const FileSystemURL expected_url =
      CreateFileSystemURL(kSyncableFileSystemRootURI + path.AsUTF8Unsafe());
  const FileSystemURL url = CreateSyncableFileSystemURL(GURL(kOrigin), path);

  EXPECT_TRUE(url.is_valid());
  EXPECT_EQ(expected_url, url);

  RevokeSyncableFileSystem();
}

TEST(SyncableFileSystemUtilTest,
       SerializeAndDesirializeSyncableFileSystemURL) {
  RegisterSyncableFileSystem();

  const std::string expected_url_str = kSyncableFileSystemRootURI +
      CreateNormalizedFilePath(kPath).AsUTF8Unsafe();
  const FileSystemURL expected_url = CreateFileSystemURL(expected_url_str);
  const FileSystemURL url = CreateSyncableFileSystemURL(
      GURL(kOrigin), base::FilePath(kPath));

  std::string serialized;
  EXPECT_TRUE(SerializeSyncableFileSystemURL(url, &serialized));
  EXPECT_EQ(expected_url_str, serialized);

  FileSystemURL deserialized;
  EXPECT_TRUE(DeserializeSyncableFileSystemURL(serialized, &deserialized));
  EXPECT_TRUE(deserialized.is_valid());
  EXPECT_EQ(expected_url, deserialized);

  RevokeSyncableFileSystem();
}

TEST(SyncableFileSystemUtilTest,
     FailInSerializingAndDeserializingSyncableFileSystemURL) {
  RegisterSyncableFileSystem();

  const base::FilePath normalized_path = CreateNormalizedFilePath(kPath);
  const std::string non_registered_url =
      kNonRegisteredFileSystemRootURI + normalized_path.AsUTF8Unsafe();
  const std::string non_syncable_url =
      kNonSyncableFileSystemRootURI + normalized_path.AsUTF8Unsafe();

  // Expected to fail in serializing URLs of non-registered filesystem and
  // non-syncable filesystem.
  std::string serialized;
  EXPECT_FALSE(SerializeSyncableFileSystemURL(
      CreateFileSystemURL(non_registered_url), &serialized));
  EXPECT_FALSE(SerializeSyncableFileSystemURL(
      CreateFileSystemURL(non_syncable_url), &serialized));

  // Expected to fail in deserializing a string that represents URLs of
  // non-registered filesystem and non-syncable filesystem.
  FileSystemURL deserialized;
  EXPECT_FALSE(DeserializeSyncableFileSystemURL(
      non_registered_url, &deserialized));
  EXPECT_FALSE(DeserializeSyncableFileSystemURL(
      non_syncable_url, &deserialized));

  RevokeSyncableFileSystem();
}

TEST(SyncableFileSystemUtilTest, SerializeBeforeOpenFileSystem) {
  ScopedEnableSyncFSDirectoryOperation enable_directory_operation_;
  const std::string serialized = kSyncableFileSystemRootURI +
      CreateNormalizedFilePath(kPath).AsUTF8Unsafe();
  FileSystemURL deserialized;
  base::MessageLoop message_loop;

  // Setting up a full syncable filesystem environment.
  CannedSyncableFileSystem file_system(GURL(kOrigin),
                                       base::MessageLoopProxy::current().get(),
                                       base::MessageLoopProxy::current().get());
  file_system.SetUp();
  scoped_refptr<LocalFileSyncContext> sync_context =
      new LocalFileSyncContext(base::MessageLoopProxy::current().get(),
                               base::MessageLoopProxy::current().get());

  // Before calling initialization we would not be able to get a valid
  // deserialized URL.
  EXPECT_FALSE(DeserializeSyncableFileSystemURL(serialized, &deserialized));
  EXPECT_FALSE(deserialized.is_valid());

  ASSERT_EQ(sync_file_system::SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context.get()));

  // After initialization this should be ok (even before opening the file
  // system).
  EXPECT_TRUE(DeserializeSyncableFileSystemURL(serialized, &deserialized));
  EXPECT_TRUE(deserialized.is_valid());

  // Shutting down.
  file_system.TearDown();
  RevokeSyncableFileSystem();
  sync_context->ShutdownOnUIThread();
  sync_context = NULL;
  base::MessageLoop::current()->RunUntilIdle();
}

TEST(SyncableFileSystemUtilTest, SyncableFileSystemURL_IsParent) {
  RegisterSyncableFileSystem();

  const std::string root1 = sync_file_system::GetSyncableFileSystemRootURI(
      GURL("http://foo.com")).spec();
  const std::string root2 = sync_file_system::GetSyncableFileSystemRootURI(
      GURL("http://bar.com")).spec();

  const std::string parent("dir");
  const std::string child("dir/child");

  // True case.
  EXPECT_TRUE(CreateFileSystemURL(root1 + parent).IsParent(
      CreateFileSystemURL(root1 + child)));
  EXPECT_TRUE(CreateFileSystemURL(root2 + parent).IsParent(
      CreateFileSystemURL(root2 + child)));

  // False case: different origin.
  EXPECT_FALSE(CreateFileSystemURL(root1 + parent).IsParent(
      CreateFileSystemURL(root2 + child)));
  EXPECT_FALSE(CreateFileSystemURL(root2 + parent).IsParent(
      CreateFileSystemURL(root1 + child)));

  RevokeSyncableFileSystem();
}

}  // namespace sync_file_system
