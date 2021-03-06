// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_service_factory.h"

#include "chrome/browser/download/download_service.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/browser_context_keyed_service/browser_context_dependency_manager.h"

// static
DownloadService* DownloadServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<DownloadService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
DownloadServiceFactory* DownloadServiceFactory::GetInstance() {
  return Singleton<DownloadServiceFactory>::get();
}

DownloadServiceFactory::DownloadServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "DownloadService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HistoryServiceFactory::GetInstance());
}

DownloadServiceFactory::~DownloadServiceFactory() {
}

BrowserContextKeyedService* DownloadServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  DownloadService* service =
      new DownloadService(static_cast<Profile*>(profile));

  // No need for initialization; initialization can be done on first
  // use of service.

  return service;
}

content::BrowserContext* DownloadServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}
