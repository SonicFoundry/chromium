// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_H_
#define CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_H_

#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_vector.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/WebIDBKey.h"

namespace content {

class CONTENT_EXPORT IndexedDBKey {
 public:
  typedef std::vector<IndexedDBKey> KeyArray;

  IndexedDBKey();  // Defaults to WebKit::WebIDBKey::InvalidType.
  IndexedDBKey(WebKit::WebIDBKey::Type);  // must be Null or Invalid
  explicit IndexedDBKey(const KeyArray& array);
  explicit IndexedDBKey(const string16& str);
  IndexedDBKey(double number,
               WebKit::WebIDBKey::Type type);  // must be date or number
  explicit IndexedDBKey(const WebKit::WebIDBKey& key);
  ~IndexedDBKey();

  bool IsValid() const;

  int Compare(const IndexedDBKey& other) const;
  bool IsLessThan(const IndexedDBKey& other) const;
  bool IsEqual(const IndexedDBKey& other) const;

  WebKit::WebIDBKey::Type type() const { return type_; }
  const std::vector<IndexedDBKey>& array() const { return array_; }
  const string16& string() const { return string_; }
  double date() const { return date_; }
  double number() const { return number_; }

  operator WebKit::WebIDBKey() const;
  size_t size_estimate() const { return size_estimate_; }

 private:
  WebKit::WebIDBKey::Type type_;
  std::vector<IndexedDBKey> array_;
  string16 string_;
  double date_;
  double number_;

  size_t size_estimate_;
};

}  // namespace content

#endif  // CONTENT_COMMON_INDEXED_DB_INDEXED_DB_KEY_H_
