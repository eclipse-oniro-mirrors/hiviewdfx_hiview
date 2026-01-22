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

#ifndef OHOS_HIVIEWDFX_DATA_SHARE_STORE_H
#define OHOS_HIVIEWDFX_DATA_SHARE_STORE_H

#include <memory>
#include <mutex>
#include <string>

#include "rdb_store.h"

#include "data_share_common.h"

namespace OHOS {
namespace HiviewDFX {
class DataShareStore {
public:
    DataShareStore(std::string dirPath): dbStore_(nullptr), dirPath_(dirPath){};
    ~DataShareStore() {}
    std::shared_ptr<NativeRdb::RdbStore> GetDbStore();
    int CreateTable(const std::string &name);
    int DropTable(const std::string &name);
    int DestroyDbStore();

private:
    std::shared_ptr<NativeRdb::RdbStore> CreateDbStore();

private:
    std::shared_ptr<NativeRdb::RdbStore> dbStore_;
    const std::string dirPath_;
    std::mutex dbMutex_;
};

}  // namespace HiviewDFX
}  // namespace OHOS

#endif  // OHOS_HIVIEWDFX_DATA_SHARE_STORE_H
