// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
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

#pragma once

#include <string>
#include <unordered_map>

namespace rocksdb {
// This enum defines the RocksDB options sanity level.
enum OptionsSanityCheckLevel : unsigned char {
  // Performs no sanity check at all.
  kSanityLevelNone = 0x00,
  // Performs minimum check to ensure the RocksDB instance can be
  // opened without corrupting / mis-interpreting the data.
  kSanityLevelLooselyCompatible = 0x01,
  // Perform exact match sanity check.
  kSanityLevelExactMatch = 0xFF,
};

// The sanity check level for DB options
static const std::unordered_map<std::string, OptionsSanityCheckLevel>
    sanity_level_db_options {};

// The sanity check level for column-family options
static const std::unordered_map<std::string, OptionsSanityCheckLevel>
    sanity_level_cf_options = {
        {"comparator", kSanityLevelLooselyCompatible},
        {"prefix_extractor", kSanityLevelLooselyCompatible},
        {"table_factory", kSanityLevelLooselyCompatible},
        {"merge_operator", kSanityLevelLooselyCompatible}};

// The sanity check level for block-based table options
static const std::unordered_map<std::string, OptionsSanityCheckLevel>
    sanity_level_bbt_options {};

OptionsSanityCheckLevel DBOptionSanityCheckLevel(
    const std::string& options_name);
OptionsSanityCheckLevel CFOptionSanityCheckLevel(
    const std::string& options_name);
OptionsSanityCheckLevel BBTOptionSanityCheckLevel(
    const std::string& options_name);

}  // namespace rocksdb
