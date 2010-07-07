// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/compressor_zlib.h"

#if defined(USE_SYSTEM_ZLIB)
#include <zlib.h>
// The code below uses the MOZ_Z_ forms of these functions in order that things
// should work on Windows. In order to make this code cross platform, we map
// back to the normal functions here in the case that we are using the system
// zlib.
#define MOZ_Z_deflate deflate
#define MOZ_Z_deflateEnd deflateEnd
#define MOZ_Z_deflateInit_ deflateInit_
#else
#include "third_party/zlib/zlib.h"
#endif
#include "base/logging.h"

namespace remoting {

CompressorZlib::CompressorZlib() {
  stream_.reset(new z_stream());

  stream_->next_in = Z_NULL;
  stream_->zalloc = Z_NULL;
  stream_->zfree = Z_NULL;
  stream_->opaque = Z_NULL;

  deflateInit(stream_.get(), Z_BEST_SPEED);
}

CompressorZlib::~CompressorZlib() {
  deflateEnd(stream_.get());
}

bool CompressorZlib::Process(const uint8* input_data, int input_size,
                             uint8* output_data, int output_size,
                             int* consumed, int* written) {
  DCHECK_GT(output_size, 0);

  // Setup I/O parameters.
  stream_->avail_in = input_size;
  stream_->next_in = (Bytef*)input_data;
  stream_->avail_out = output_size;
  stream_->next_out = (Bytef*)output_data;

  int ret = deflate(stream_.get(), input_size ? Z_NO_FLUSH : Z_FINISH);
  if (ret == Z_STREAM_ERROR) {
    NOTREACHED() << "zlib compression failed";
  }

  *consumed = input_size - stream_->avail_in;
  *written = output_size - stream_->avail_out;

  // Return true when we get Z_BUF_ERROR, this way we can feed more data
  // to zlib.
  return ret == Z_OK || ret == Z_BUF_ERROR;
}

}  // namespace remoting
