/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "native_leak_config.h"

#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>

#include "logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("NativeLeakConfig");

using std::string;
using std::regex;
using std::smatch;
using std::regex_match;
using std::unordered_map;
using std::ifstream;

namespace {
const string NATIVE_THRESHOLD_PATH = "/system/etc/hiview/memory_leak_threshold";

enum RecordItem {
    ITEM_NAME = 1,
    ITEM_VALUE
};
}

NativeLeakConfig::NativeLeakConfig() {};

NativeLeakConfig::~NativeLeakConfig() {};

bool NativeLeakConfig::GetThresholdList(unordered_map<string, uint64_t> &list)
{
    ifstream fin;
    const string &path = NATIVE_THRESHOLD_PATH;
    fin.open(path.c_str());
    if (!fin.is_open()) {
        HIVIEW_LOGI("open file failed. path:%{public}s", path.c_str());
        return false;
    }
    string line;
    while (getline(fin, line)) {
        regex recordRegex("^(.+)\\s+(.+)$");
        smatch matches;
        if (!regex_match(line, matches, recordRegex)) {
            continue;
        }
        list.insert(make_pair(matches[ITEM_NAME].str(), stoull(matches[ITEM_VALUE])));
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
