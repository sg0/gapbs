// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef BITMAP_H_
#define BITMAP_H_

#include <algorithm>
#include <cinttypes>

#include "platform_atomics.h"


/*
GAP Benchmark Suite
Class:  Bitmap
Author: Scott Beamer

Parallel bitmap that is thread-safe
 - Can set bits in parallel (set_bit_atomic) unlike std::vector<bool>
*/
#if defined(ZFILL_CACHE_LINES) && defined(__ARM_ARCH) && __ARM_ARCH >= 8
#ifndef CACHE_LINE_SIZE_BYTES
#define CACHE_LINE_SIZE_BYTES   256
#endif
/* The zfill distance must be large enough to be ahead of the L2 prefetcher */
static const int ZFILL_DISTANCE = 100;

/* x-byte cache lines */
static const int I32_ELEMS_PER_CACHE_LINE = CACHE_LINE_SIZE_BYTES / sizeof(int32_t);
static const int I64_ELEMS_PER_CACHE_LINE = CACHE_LINE_SIZE_BYTES / sizeof(int64_t);
static const int U64_ELEMS_PER_CACHE_LINE = CACHE_LINE_SIZE_BYTES / sizeof(uint64_t);
static const int FLT_ELEMS_PER_CACHE_LINE = CACHE_LINE_SIZE_BYTES / sizeof(float);
static const int DBL_ELEMS_PER_CACHE_LINE = CACHE_LINE_SIZE_BYTES / sizeof(double);

/* Offset from a[j] to zfill */
static const int I32_ZFILL_OFFSET = ZFILL_DISTANCE * I32_ELEMS_PER_CACHE_LINE;
static const int I64_ZFILL_OFFSET = ZFILL_DISTANCE * I64_ELEMS_PER_CACHE_LINE;
static const int U64_ZFILL_OFFSET = ZFILL_DISTANCE * U64_ELEMS_PER_CACHE_LINE;
static const int FLT_ZFILL_OFFSET = ZFILL_DISTANCE * FLT_ELEMS_PER_CACHE_LINE;
static const int DBL_ZFILL_OFFSET = ZFILL_DISTANCE * DBL_ELEMS_PER_CACHE_LINE;

static inline void zfill_i32(int32_t * a) 
{ asm volatile("dc zva, %0": : "r"(a)); }

static inline void zfill_i64(int64_t * a) 
{ asm volatile("dc zva, %0": : "r"(a)); }

static inline void zfill_u64(uint64_t * a) 
{ asm volatile("dc zva, %0": : "r"(a)); }

static inline void zfill_flt(float * a) 
{ asm volatile("dc zva, %0": : "r"(a)); }

static inline void zfill_dbl(double * a) 
{ asm volatile("dc zva, %0": : "r"(a)); }
#endif

class Bitmap {
 public:
  explicit Bitmap(size_t size) {
    uint64_t num_words = (size + kBitsPerWord - 1) / kBitsPerWord;
    start_ = new uint64_t[num_words];
    end_ = start_ + num_words;
  }

  ~Bitmap() {
    delete[] start_;
  }

  void reset() {
    std::fill(start_, end_, 0);
  }
#if defined(ZFILL_CACHE_LINES) && defined(__ARM_ARCH) && __ARM_ARCH >= 8
  void zero(size_t beg, size_t end) {
    uint64_t * const zfill_limit = start_ + word_offset(end) - U64_ZFILL_OFFSET;
    uint64_t * const ustart = start_ + word_offset(beg);

    if (ustart + U64_ZFILL_OFFSET < zfill_limit)
       zfill_u64(ustart + U64_ZFILL_OFFSET);
  }
#endif

  void set_bit(size_t pos) {
    start_[word_offset(pos)] |= ((uint64_t) 1l << bit_offset(pos));
  }

  void set_bit_atomic(size_t pos) {
    uint64_t old_val, new_val;
    do {
      old_val = start_[word_offset(pos)];
      new_val = old_val | ((uint64_t) 1l << bit_offset(pos));
    } while (!compare_and_swap(start_[word_offset(pos)], old_val, new_val));
  }

  bool get_bit(size_t pos) const {
    return (start_[word_offset(pos)] >> bit_offset(pos)) & 1l;
  }

  void swap(Bitmap &other) {
    std::swap(start_, other.start_);
    std::swap(end_, other.end_);
  }

 private:
  uint64_t *start_;
  uint64_t *end_;

  static const uint64_t kBitsPerWord = 64;
  static uint64_t word_offset(size_t n) { return n / kBitsPerWord; }
  static uint64_t bit_offset(size_t n) { return n & (kBitsPerWord - 1); }
};

#endif  // BITMAP_H_
