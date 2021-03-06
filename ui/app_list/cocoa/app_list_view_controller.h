// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_COCOA_APP_LIST_VIEW_CONTROLLER_H_
#define UI_APP_LIST_COCOA_APP_LIST_VIEW_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "ui/app_list/app_list_export.h"
#import "ui/app_list/cocoa/apps_pagination_model_observer.h"
#import "ui/app_list/cocoa/apps_search_box_controller.h"
#import "ui/app_list/cocoa/apps_search_results_controller.h"

namespace app_list {
class AppListViewDelegate;
class AppListModel;
}

@class AppListPagerView;
@class AppsGridController;

// Controller for the top-level view of the app list UI. It creates and hosts an
// AppsGridController (displaying an AppListModel), pager control to navigate
// between pages in the grid, and search entry box with a pop up menu.
APP_LIST_EXPORT
@interface AppListViewController : NSViewController<AppsPaginationModelObserver,
                                                    AppsSearchBoxDelegate,
                                                    AppsSearchResultsDelegate> {
 @private
  base::scoped_nsobject<AppsGridController> appsGridController_;
  base::scoped_nsobject<AppListPagerView> pagerControl_;
  base::scoped_nsobject<AppsSearchBoxController> appsSearchBoxController_;
  base::scoped_nsobject<AppsSearchResultsController>
      appsSearchResultsController_;
  base::scoped_nsobject<NSView> contentsView_;
  scoped_ptr<app_list::AppListViewDelegate> delegate_;
  BOOL showingSearchResults_;
}

- (AppsGridController*)appsGridController;

- (NSSegmentedControl*)pagerControl;

- (app_list::AppListViewDelegate*)delegate;

- (void)setDelegate:(scoped_ptr<app_list::AppListViewDelegate>)newDelegate;

@end

@interface AppListViewController (TestingAPI)

- (void)setDelegate:(scoped_ptr<app_list::AppListViewDelegate>)newDelegate
      withTestModel:(scoped_ptr<app_list::AppListModel>)newModel;

@end

#endif  // UI_APP_LIST_COCOA_APP_LIST_VIEW_CONTROLLER_H_
