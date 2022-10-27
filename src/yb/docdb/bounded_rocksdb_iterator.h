// Copyright (c) YugaByte, Inc.
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

#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "yb/docdb/docdb_fwd.h"

#include "yb/rocksdb/iterator.h"
#include "yb/rocksdb/options.h"

namespace yb {
namespace docdb {

class BoundedRocksDbIterator : public rocksdb::Iterator {
 public:
  BoundedRocksDbIterator() = default;

  BoundedRocksDbIterator(
      rocksdb::DB* rocksdb, const rocksdb::ReadOptions& read_opts, const KeyBounds* key_bounds);

  BoundedRocksDbIterator(const BoundedRocksDbIterator& other) = delete;
  void operator=(const BoundedRocksDbIterator& other) = delete;

  BoundedRocksDbIterator(BoundedRocksDbIterator&&) = default;
  BoundedRocksDbIterator& operator=(BoundedRocksDbIterator&&) = default;

  bool Initialized() const { return iterator_ != nullptr; }

  bool Valid() const override;

  void SeekToFirst() override;

  void SeekToLast() override;

  void Seek(const Slice& target) override;

  void Next() override;

  void Prev() override;

  Slice key() const override;

  Slice value() const override;

  Status status() const override;

  Status GetProperty(std::string prop_name, std::string* prop) override {
    return iterator_->GetProperty(prop_name, prop);
  }

  void RegisterCleanup(CleanupFunction function, void* arg1, void* arg2) {
    iterator_->RegisterCleanup(function, arg1, arg2);
  }

  void RevalidateAfterUpperBoundChange() override {
    iterator_->RevalidateAfterUpperBoundChange();
  }

  void Reset() {
    iterator_.reset();
  }

 private:
  std::unique_ptr<rocksdb::Iterator> iterator_;
  const KeyBounds* key_bounds_;
};

} // namespace docdb
} // namespace yb

