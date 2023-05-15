/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEWDFX_DATA_SHARE_DAO_H
#define OHOS_HIVIEWDFX_DATA_SHARE_DAO_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "data_share_common.h"

namespace OHOS {
namespace HiviewDFX {

class DataShareStore;

class DataShareDao {
public:
    DataShareDao(std::shared_ptr<DataShareStore> store);
    ~DataShareDao() {}
    int SaveSubscriberInfo(int32_t uid, const std::string &events);
    int DeleteSubscriberInfo(int32_t uid);
    bool IsUidExists(int32_t uid);
    int GetEventListByUid(int32_t uid, std::string &events);
    int GetUidByBundleName(const std::string &bundleName, int32_t &uid);
    int GetTotalSubscriberInfo(std::map<int, std::string> &map);

private:
    std::shared_ptr<DataShareStore> store_;
    std::string eventTable_;
};

}  // namespace HiviewDFX
}  // namespace OHOS

#endif  // OHOS_HIVIEWDFX_DATA_SHARE_DAO_H
