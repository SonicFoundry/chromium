// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sync_file_system_internals/sync_file_system_internals_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/sync_file_system_internals/extension_statuses_handler.h"
#include "chrome/browser/ui/webui/sync_file_system_internals/file_metadata_handler.h"
#include "chrome/browser/ui/webui/sync_file_system_internals/sync_file_system_internals_handler.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/sync_file_system_internals_resources.h"

namespace {

content::WebUIDataSource* CreateSyncFileSystemInternalsHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(
          chrome::kChromeUISyncFileSystemInternalsHost);
  source->SetJsonPath("strings.js");
  source->AddResourcePath(
      "extension_statuses.js",
      IDR_SYNC_FILE_SYSTEM_INTERNALS_EXTENSION_STATUSES_JS);
  source->AddResourcePath(
      "file_metadata.js", IDR_SYNC_FILE_SYSTEM_INTERNALS_FILE_METADATA_JS);
  source->AddResourcePath(
      "sync_service.js", IDR_SYNC_FILE_SYSTEM_INTERNALS_SYNC_SERVICE_JS);
  source->SetDefaultResource(IDR_SYNC_FILE_SYSTEM_INTERNALS_MAIN_HTML);
  return source;
}

}  // namespace

SyncFileSystemInternalsUI::SyncFileSystemInternalsUI(content::WebUI* web_ui)
    : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  web_ui->AddMessageHandler(
      new syncfs_internals::ExtensionStatusesHandler(profile));
  web_ui->AddMessageHandler(
      new syncfs_internals::FileMetadataHandler(profile));
  web_ui->AddMessageHandler(
      new syncfs_internals::SyncFileSystemInternalsHandler(profile));
  content::WebUIDataSource::Add(profile,
                                CreateSyncFileSystemInternalsHTMLSource());
}

SyncFileSystemInternalsUI::~SyncFileSystemInternalsUI() {}
