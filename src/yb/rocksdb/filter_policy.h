// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// The following only applies to changes made to this file as part of YugaByte development.
//
// Portions Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//
// A database can be configured with a custom FilterPolicy object.
// This object is responsible for creating a small filter from a set
// of keys.  These filters are stored in rocksdb and are consulted
// automatically by rocksdb to decide whether or not to read some
// information from disk. In many cases, a filter can cut down the
// number of disk seeks form a handful to a single disk seek per
// DB::Get() call.
//
// Most people will want to use the builtin bloom filter support (see
// NewBloomFilterPolicy() below).

#pragma once

#include <string>
#include <memory>

#include "yb/rocksdb/env.h"
#include "yb/util/slice.h"

namespace rocksdb {

// A class that takes a bunch of keys, then generates filter
class FilterBitsBuilder {
 public:
  virtual ~FilterBitsBuilder() {}

  // Add Key to filter, you could use any way to store the key.
  // Such as: storing hashes or original keys
  // Keys are in sorted order and duplicated keys are possible.
  virtual void AddKey(const Slice& key) = 0;

  // Generate the filter using the keys that are added
  // The return value of this function would be the filter bits,
  // The ownership of actual data is set to buf
  virtual Slice Finish(std::unique_ptr<const char[]>* buf) = 0;

  // Checks whether bits builder is full and we should start a new bloom filter block.
  virtual bool IsFull() const = 0;
};

// A class that checks if a key can be in filter
// It should be initialized by Slice generated by BitsBuilder
class FilterBitsReader {
 public:
  virtual ~FilterBitsReader() {}

  // Check if the entry match the bits in filter
  virtual bool MayMatch(const Slice& entry) = 0;
};

// We add a new format of filter block called full filter block
// This new interface gives you more space of customization
//
// For the full filter block, you can plug in your version by implement
// the FilterBitsBuilder and FilterBitsReader
//
// There are two sets of interface in FilterPolicy
// Set 1: CreateFilter, KeyMayMatch: used for blockbased filter
// Set 2: GetFilterBitsBuilder, GetFilterBitsReader, they are used for
// full filter and fixed-size filter.
// Either Set 1 or Set 2 MUST be implemented correctly.
// RocksDB would first try using functions in Set 2. if they return nullptr,
// it would use Set 1 instead.
// You can choose filter type in NewBloomFilterPolicy.
class FilterPolicy {
 public:
  virtual ~FilterPolicy();

  // Return the name of this policy.  Note that if the filter encoding
  // changes in an incompatible way, the name returned by this method
  // must be changed.  Otherwise, old incompatible filters may be
  // passed to methods of this type.
  virtual const char* Name() const = 0;

  // keys[0,n-1] contains a list of keys (potentially with duplicates)
  // that are ordered according to the user supplied comparator.
  // Append a filter that summarizes keys[0,n-1] to *dst.
  //
  // Warning: do not change the initial contents of *dst.  Instead,
  // append the newly constructed filter to *dst.
  virtual void CreateFilter(const Slice* keys, int n, std::string* dst)
      const = 0;

  // "filter" contains the data appended by a preceding call to
  // CreateFilter() on this class.  This method must return true if
  // the key was in the list of keys passed to CreateFilter().
  // This method may return true or false if the key was not on the
  // list, but it should aim to return false with a high probability.
  virtual bool KeyMayMatch(const Slice& key, const Slice& filter) const = 0;

  // Get the FilterBitsBuilder, which is used for full filter block and fixed size filter block.
  // It contains interface to take individual key, then generate filter.
  virtual FilterBitsBuilder* GetFilterBitsBuilder() const {
    return nullptr;
  }

  // Get the FilterBitsReader, which is used for full filter block and fixed size filter block.
  // It contains interface to tell if key can be in filter.
  // The input slice should NOT be deleted by FilterPolicy.
  virtual FilterBitsReader* GetFilterBitsReader(const Slice& contents) const {
    return nullptr;
  }

  // Filter type that will be used for this table.
  enum FilterType {
    // No filter is used.
    kNoFilter,

    // One monolithic full filter per SSTable, with keys buffering while building.
    kFullFilter,

    // Block based filter, with one filter block corresponding to each data block.
    kBlockBasedFilter,

    // Fixed size filter without key buffering.
    kFixedSizeFilter
  };

  // Returns filter type to be used based on this policy.
  virtual FilterType GetFilterType() const = 0;

  class KeyTransformer {
   public:
    virtual ~KeyTransformer() {}

    // Transform a key.
    virtual Slice Transform(Slice key) const = 0;
  };

  // Filter policy can optionally return key transformer to be used before writing key to filter or
  // testing key against filter and building/reading filter index based on keys (used for fixed-size
  // bloom filter). This method is used by BlockBasedTable(Reader)/BlockBasedTableBuilder.
  // If KeyTransformer returns empty filter key - this is treated as matching the filter by
  // BlockBasedTableReader. This allows us to support disabling bloom filters for old versions
  // of filter policies if they have bugs.
  // Because of this when BlockBasedTableBuilder is getting empty filter key - there is no need to
  // add it into the filter block.
  //
  // Requires: order of keys defined by BytewiseComparator shouldn't be broken by key transformer,
  // which means if `BytewiseComparator::Compare(a, b) <= 0` then for non-empty a_transformed and
  // b_transformed `BytewiseComparator::Compare(a_tranformed, b_transformed) <= 0` must hold.
  // Returning empty keys affecting the order is acceptable, for example: "AAA123" <= "BBB456",
  // but it is acceptable to transform it to: "AAA" > "", because we treat empty filter keys
  // specifically (see previous section).
  //
  // Actually we can use ColumnFamilyOptions::prefix_extractor instead and switch off whole key
  // filtering by setting BlockBasedTableOptions::whole_key_filtering to false. But we want
  // key transformation algorithm to be part of filter policy, so we can:
  // 1) detect which key transformation to use based on policy serialized into filter meta block.
  // 2) support multiple filter policies with their own key transformations.
  //
  // Note: in case prefix_extractor is also set, it is applied after key transformer. But for
  // block-based filter prefix_extractor should also be applicable to user key, because
  // block filter containing required key prefix is retrieved from data index based on user key.
  virtual const KeyTransformer* GetKeyTransformer() const { return nullptr; }

  static constexpr size_t kDefaultFixedSizeFilterBits = 65536;
  static constexpr double kDefaultFixedSizeFilterErrorRate = 0.01;
};

// Return a new filter policy that uses a bloom filter with approximately
// the specified number of bits per key.
//
// bits_per_key: bits per key in bloom filter. A good value for bits_per_key
// is 10, which yields a filter with ~ 1% false positive rate.
// use_block_based_builder: use block based filter rather than full fiter.
// If you want to builder full filter, it needs to be set to false.
//
// Callers must delete the result after any database that is using the
// result has been closed.
//
// Note: if you are using a custom comparator that ignores some parts
// of the keys being compared, you must not use NewBloomFilterPolicy()
// and must provide your own FilterPolicy that also ignores the
// corresponding parts of the keys.  For example, if the comparator
// ignores trailing spaces, it would be incorrect to use a
// FilterPolicy (like NewBloomFilterPolicy) that does not ignore
// trailing spaces in keys.
extern const FilterPolicy* NewBloomFilterPolicy(int bits_per_key,
    bool use_block_based_builder = true);

// Return a new filter policy that uses a bloom filter divided into fixed-size blocks with
// specified parameters:
//
// total_bits: purely size of bloom filter itself in each filter block, also each filter block has
// some metadata added.
// error_rate: expected false positive error rate to calculate maximum number of keys to store in
// each filter block. This is used to determine whether a filter block is full.
//
// Callers must delete the result after any database that is using the filter policy has been
// closed.
extern const FilterPolicy* NewFixedSizeFilterPolicy(size_t total_bits,
                                                    double error_rate,
                                                    Logger* logger);
}  // namespace rocksdb

