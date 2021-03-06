// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/audio_buffer.h"

#include "base/logging.h"
#include "media/base/audio_bus.h"
#include "media/base/buffers.h"
#include "media/base/limits.h"

namespace media {

// Alignment of each channel's data; use 8-byte alignment as that is bigger
// than maximum size of a sample, and the minimum alignment.
enum { kChannelAlignment = 8 };

AudioBuffer::AudioBuffer(SampleFormat sample_format,
                         int channel_count,
                         int frame_count,
                         const uint8* const* data,
                         const base::TimeDelta timestamp,
                         const base::TimeDelta duration)
    : sample_format_(sample_format),
      channel_count_(channel_count),
      frame_count_(frame_count),
      timestamp_(timestamp),
      duration_(duration) {
  CHECK_GE(channel_count, 0);
  CHECK_LE(channel_count, limits::kMaxChannels);
  CHECK_GE(frame_count, 0);
  int bytes_per_channel = SampleFormatToBytesPerChannel(sample_format);
  DCHECK_LE(bytes_per_channel, kChannelAlignment);
  int data_size = frame_count * bytes_per_channel;

  // Empty buffer?
  if (!data) {
    CHECK_EQ(frame_count, 0);
    return;
  }

  if (sample_format == kSampleFormatPlanarF32 ||
      sample_format == kSampleFormatPlanarS16) {
    // Planar data, so need to allocate buffer for each channel.
    // Determine per channel data size, taking into account alignment.
    int block_size_per_channel =
        (data_size + kChannelAlignment - 1) & ~(kChannelAlignment - 1);
    DCHECK_GE(block_size_per_channel, data_size);

    // Allocate a contiguous buffer for all the channel data.
    data_.reset(static_cast<uint8*>(base::AlignedAlloc(
        channel_count * block_size_per_channel, kChannelAlignment)));
    channel_data_.reserve(channel_count);

    // Copy each channel's data into the appropriate spot.
    for (int i = 0; i < channel_count; ++i) {
      channel_data_.push_back(data_.get() + i * block_size_per_channel);
      memcpy(channel_data_[i], data[i], data_size);
    }
    return;
  }

  // Remaining formats are interleaved data.
  DCHECK(sample_format_ == kSampleFormatU8 ||
         sample_format_ == kSampleFormatS16 ||
         sample_format_ == kSampleFormatS32 ||
         sample_format_ == kSampleFormatF32) << sample_format_;
  // Allocate our own buffer and copy the supplied data into it. Buffer must
  // contain the data for all channels.
  data_size *= channel_count;
  data_.reset(
      static_cast<uint8*>(base::AlignedAlloc(data_size, kChannelAlignment)));
  memcpy(data_.get(), data[0], data_size);
}

AudioBuffer::~AudioBuffer() {}

// static
scoped_refptr<AudioBuffer> AudioBuffer::CopyFrom(
    SampleFormat sample_format,
    int channel_count,
    int frame_count,
    const uint8* const* data,
    const base::TimeDelta timestamp,
    const base::TimeDelta duration) {
  // If you hit this CHECK you likely have a bug in a demuxer. Go fix it.
  CHECK(data[0]);
  return make_scoped_refptr(new AudioBuffer(
      sample_format, channel_count, frame_count, data, timestamp, duration));
}

// static
scoped_refptr<AudioBuffer> AudioBuffer::CreateEOSBuffer() {
  return make_scoped_refptr(new AudioBuffer(
      kUnknownSampleFormat, 1, 0, NULL, kNoTimestamp(), kNoTimestamp()));
}

// Convert int16 values in the range [kint16min, kint16max] to [-1.0, 1.0].
static inline float ConvertS16ToFloat(int16 value) {
  return value * (value < 0 ? -1.0f / kint16min : 1.0f / kint16max);
}

void AudioBuffer::ReadFrames(int frames_to_copy,
                             int source_frame_offset,
                             int dest_frame_offset,
                             AudioBus* dest) {
  // Deinterleave each channel (if necessary) and convert to 32bit
  // floating-point with nominal range -1.0 -> +1.0 (if necessary).

  // |dest| must have the same number of channels, and the number of frames
  // specified must be in range.
  DCHECK(!end_of_stream());
  DCHECK_EQ(dest->channels(), channel_count_);
  DCHECK_LE(source_frame_offset + frames_to_copy, frame_count_);
  DCHECK_LE(dest_frame_offset + frames_to_copy, dest->frames());

  if (sample_format_ == kSampleFormatPlanarF32) {
    // Format is planar float32. Copy the data from each channel as a block.
    for (int ch = 0; ch < channel_count_; ++ch) {
      const float* source_data =
          reinterpret_cast<const float*>(channel_data_[ch]) +
          source_frame_offset;
      memcpy(dest->channel(ch) + dest_frame_offset,
             source_data,
             sizeof(float) * frames_to_copy);
    }
    return;
  }

  if (sample_format_ == kSampleFormatPlanarS16) {
    // Format is planar signed16. Convert each value into float and insert into
    // output channel data.
    for (int ch = 0; ch < channel_count_; ++ch) {
      const int16* source_data =
          reinterpret_cast<const int16*>(channel_data_[ch]) +
          source_frame_offset;
      float* dest_data = dest->channel(ch) + dest_frame_offset;
      for (int i = 0; i < frames_to_copy; ++i) {
        dest_data[i] = ConvertS16ToFloat(source_data[i]);
      }
    }
    return;
  }

  if (sample_format_ == kSampleFormatF32) {
    // Format is interleaved float32. Copy the data into each channel.
    const float* source_data = reinterpret_cast<const float*>(data_.get()) +
                               source_frame_offset * channel_count_;
    for (int ch = 0; ch < channel_count_; ++ch) {
      float* dest_data = dest->channel(ch) + dest_frame_offset;
      for (int i = 0, offset = ch; i < frames_to_copy;
           ++i, offset += channel_count_) {
        dest_data[i] = source_data[offset];
      }
    }
    return;
  }

  // Remaining formats are integer interleaved data. Use the deinterleaving code
  // in AudioBus to copy the data.
  DCHECK(sample_format_ == kSampleFormatU8 ||
         sample_format_ == kSampleFormatS16 ||
         sample_format_ == kSampleFormatS32);
  int bytes_per_channel = SampleFormatToBytesPerChannel(sample_format_);
  int frame_size = channel_count_ * bytes_per_channel;
  const uint8* source_data = data_.get() + source_frame_offset * frame_size;
  dest->FromInterleavedPartial(
      source_data, dest_frame_offset, frames_to_copy, bytes_per_channel);
}

}  // namespace media
