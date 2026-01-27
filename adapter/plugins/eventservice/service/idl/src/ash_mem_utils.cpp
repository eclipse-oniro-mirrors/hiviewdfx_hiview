/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ash_mem_utils.h"

#include <codecvt>
#include <locale>
#include <string>

#include "hiview_logger.h"
#include "string_ex.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-SharedMemory-Util");
constexpr char ASH_MEM_NAME[] = "HiSysEventService SharedMemory";
constexpr int32_t ASH_MEM_SIZE = 1024 * 769; // 769k

void ParseAllStringItemSize(const std::vector<std::u16string>& data, std::vector<std::string>& translatedData,
    std::vector<uint32_t>& allSize)
{
    for (auto& item : data) {
        auto translated = Str16ToStr8(item);
        translatedData.emplace_back(translated);
        allSize.emplace_back(strlen(translated.c_str()) + 1);
    }
}
}

sptr<Ashmem> AshMemUtils::GetAshmem()
{
    auto ashmem = Ashmem::CreateAshmem(ASH_MEM_NAME, ASH_MEM_SIZE);
    if (ashmem == nullptr) {
        HIVIEW_LOGE("ashmem init failed.");
        return ashmem;
    }
    if (!ashmem->MapReadAndWriteAshmem()) {
        HIVIEW_LOGE("ashmem map failed.");
        return ashmem;
    }
    HIVIEW_LOGD("ashmem init succeed.");
    return ashmem;
}

void AshMemUtils::CloseAshmem(sptr<Ashmem> ashmem)
{
    if (ashmem != nullptr) {
        ashmem->UnmapAshmem();
        ashmem->CloseAshmem();
    }
    HIVIEW_LOGD("ashmem closed.");
}

sptr<Ashmem> AshMemUtils::WriteBulkData(MessageParcel& parcel, const std::vector<std::u16string>& src)
{
    std::vector<std::string> allData;
    std::vector<uint32_t> allSize;
    ParseAllStringItemSize(src, allData, allSize);
    if (!parcel.WriteUInt32Vector(allSize)) {
        HIVIEW_LOGE("writing allSize array failed.");
        return nullptr;
    }
    auto ashmem = GetAshmem();
    if (ashmem == nullptr) {
        return nullptr;
    }
    uint32_t offset = 0;
    for (uint32_t i = 0; i < allData.size(); i++) {
        auto translated = allData[i].c_str();
        if (!ashmem->WriteToAshmem(translated, strlen(translated), offset)) {
            HIVIEW_LOGE("writing ashmem failed.");
            CloseAshmem(ashmem);
            return nullptr;
        }
        offset += allSize[i];
    }
    if (!parcel.WriteAshmem(ashmem)) {
        HIVIEW_LOGE("writing ashmem failed.");
        CloseAshmem(ashmem);
        return nullptr;
    }
    return ashmem;
}

bool AshMemUtils::ReadBulkData(MessageParcel& parcel, std::vector<std::u16string>& dest)
{
    std::vector<uint32_t> allSize;
    if (!parcel.ReadUInt32Vector(&allSize)) {
        HIVIEW_LOGE("reading allSize array failed.");
        return false;
    }
    auto ashmem = parcel.ReadAshmem();
    if (ashmem == nullptr) {
        HIVIEW_LOGE("reading ashmem failed.");
        return false;
    }
    bool ret = ashmem->MapReadAndWriteAshmem();
    if (!ret) {
        HIVIEW_LOGE("mapping read only ashmem failed.");
        CloseAshmem(ashmem);
        return false;
    }
    uint32_t offset = 0;
    for (uint32_t i = 0; i < allSize.size(); i++) {
        auto origin = ashmem->ReadFromAshmem(allSize[i], offset);
        if (origin == nullptr) {
            HIVIEW_LOGE("invalid ash memory");
            CloseAshmem(ashmem);
            return false;
        }
        dest.emplace_back(Str8ToStr16(std::string(reinterpret_cast<const char*>(origin))));
        offset += allSize[i];
    }
    CloseAshmem(ashmem);
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
