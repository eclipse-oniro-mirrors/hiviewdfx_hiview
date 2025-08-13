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

#include "content_reader.h"

#include "base/raw_data_base_def.h"
#include "base/raw_data.h"
#include "encoded/encoded_param.h"
#include "hiview_logger.h"
#include "securec.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-ContentReader");
using namespace EventStore;
namespace {
constexpr uint32_t DATA_FORMAT_VERSION_OFFSET =
    sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t); // magicNumber + blockSize + pageSize

bool UpdateParamCnt(std::shared_ptr<RawData> rawData, int32_t addParamCnt)
{
    uint32_t pos = HIVIEW_BLOCK_SIZE + sizeof(struct EventRaw::HiSysEventHeader);
    auto header = *(reinterpret_cast<struct EventRaw::HiSysEventHeader*>(rawData->GetData() + HIVIEW_BLOCK_SIZE));
    if (header.isTraceOpened == 1) { // 1: include trace info, 0: exclude trace info
        pos += sizeof(struct EventRaw::TraceInfo);
    }
    int32_t paramCnt = *(reinterpret_cast<int32_t*>(rawData->GetData() + pos)) + addParamCnt;
    return rawData->Update(reinterpret_cast<uint8_t*>(&paramCnt), sizeof(int32_t), pos);
}

bool UpdateRealSize(std::shared_ptr<RawData> rawData)
{
    uint32_t realSize = rawData->GetDataLength();
    return rawData->Update(reinterpret_cast<uint8_t*>(&realSize), HIVIEW_BLOCK_SIZE, 0);
}
}

uint8_t ContentReader::ReadFmtVersion(std::ifstream& docStream)
{
    if (!docStream.is_open()) {
        return EventStore::EVENT_DATA_FORMATE_VERSION::INVALID;
    }
    auto curPos = docStream.tellg();
    docStream.seekg(DATA_FORMAT_VERSION_OFFSET, std::ios::beg);
    uint8_t dataFmtVersion = EventStore::EVENT_DATA_FORMATE_VERSION::INVALID;
    docStream.read(reinterpret_cast<char*>(&dataFmtVersion), sizeof(uint8_t));
    docStream.seekg(curPos, std::ios::beg);
    return dataFmtVersion;
}

std::shared_ptr<RawData> ContentReader::ReadRawData(const EventInfo& eventInfo, uint8_t* content, uint32_t contentSize)
{
    constexpr uint32_t expandSize = 128; // for seq, tag, level
    // the different size of event header between old and newer version.
    uint32_t eventSize = contentSize - GetContentHeaderSize() + sizeof(EventStore::ContentHeader) -
        SEQ_SIZE + MAX_DOMAIN_LEN + MAX_EVENT_NAME_LEN + expandSize;
    if (eventSize > MAX_NEW_SIZE) {
        HIVIEW_LOGE("invalid new event size=%{public}u", eventSize);
        return nullptr;
    }
    auto rawData = std::make_shared<RawData>(eventSize);
    if (!rawData->Append(reinterpret_cast<uint8_t*>(&eventSize), HIVIEW_BLOCK_SIZE)) {
        HIVIEW_LOGE("failed to copy event size to raw event");
        return nullptr;
    }
    if (AppendDomainAndName(rawData, eventInfo) != DOC_STORE_SUCCESS) {
        return nullptr;
    }
    if (AppendContentData(rawData, content, contentSize) != DOC_STORE_SUCCESS) {
        return nullptr;
    }
    if (AppendExtraInfo(rawData, eventInfo)) {
        return nullptr;
    }
    return rawData;
}

int ContentReader::AppendDomainAndName(std::shared_ptr<RawData> rawData, const EventInfo& eventInfo)
{
    if (!rawData->Append(reinterpret_cast<uint8_t*>(const_cast<char*>(eventInfo.domain)), MAX_DOMAIN_LEN)) {
        HIVIEW_LOGE("failed to copy domain to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    if (!rawData->Append(reinterpret_cast<uint8_t*>(const_cast<char*>(eventInfo.name)), MAX_EVENT_NAME_LEN)) {
        HIVIEW_LOGE("failed to copy name to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    return DOC_STORE_SUCCESS;
}

int ContentReader::AppendContentData(std::shared_ptr<RawData> rawData, uint8_t* content, uint32_t contentSize)
{
    EventStore::ContentHeader contentHeader;
    if (GetContentHeader(content, contentHeader) != DOC_STORE_SUCCESS) {
        HIVIEW_LOGE("failed to get header of content");
        return DOC_STORE_ERROR_MEMORY;
    }
    if (!rawData->Append(reinterpret_cast<uint8_t*>(&contentHeader) + SEQ_SIZE,
        sizeof(EventStore::ContentHeader) - SEQ_SIZE)) {
        HIVIEW_LOGE("failed to copy content header to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    size_t contentPos = HIVIEW_BLOCK_SIZE + GetContentHeaderSize();
    if (!rawData->Append(content + contentPos, contentSize - contentPos - CRC_SIZE)) {
        HIVIEW_LOGE("failed to copy content data to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    return DOC_STORE_SUCCESS;
}

int ContentReader::AppendExtraInfo(std::shared_ptr<RawData> rawData, const EventInfo& eventInfo)
{
    uint32_t addParamCnt = 2; // for seq, level
    if (AppendTag(rawData, eventInfo.tag) == DOC_STORE_SUCCESS) {
        addParamCnt++;
    }
    if (auto ret = AppendLevel(rawData, eventInfo.level); ret != DOC_STORE_SUCCESS) {
        return ret;
    }
    if (auto ret = AppendSeq(rawData, eventInfo.seq); ret != DOC_STORE_SUCCESS) {
        return ret;
    }
    if (!UpdateParamCnt(rawData, addParamCnt)) {
        HIVIEW_LOGE("failed to update paramCnt to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    if (!UpdateRealSize(rawData)) {
        HIVIEW_LOGE("failed to update realSize of raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    return DOC_STORE_SUCCESS;
}

int ContentReader::AppendTag(std::shared_ptr<RawData> rawData, const std::string& tag)
{
    if (tag.empty()) {
        return DOC_STORE_ERROR_INVALID;
    }
    auto tagParam = std::make_shared<EventRaw::StringEncodedParam>(EventCol::TAG, tag);
    auto& tagRawData = tagParam->GetRawData();
    if (!rawData->Append(tagRawData.GetData(), tagRawData.GetDataLength())) {
        HIVIEW_LOGE("failed to copy tag to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    return DOC_STORE_SUCCESS;
}

int ContentReader::AppendLevel(std::shared_ptr<RawData> rawData, const std::string& level)
{
    auto levelParam = std::make_shared<EventRaw::StringEncodedParam>(EventCol::LEVEL, level);
    auto& levelRawData = levelParam->GetRawData();
    if (!rawData->Append(levelRawData.GetData(), levelRawData.GetDataLength())) {
        HIVIEW_LOGE("failed to copy level to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    return DOC_STORE_SUCCESS;
}

int ContentReader::AppendSeq(std::shared_ptr<RawData> rawData, int64_t seq)
{
    auto seqParam = std::make_shared<EventRaw::SignedVarintEncodedParam<int64_t>>(EventCol::SEQ, seq);
    auto& seqRawData = seqParam->GetRawData();
    if (!rawData->Append(seqRawData.GetData(), seqRawData.GetDataLength())) {
        HIVIEW_LOGE("failed to copy seq to raw event");
        return DOC_STORE_ERROR_MEMORY;
    }
    return DOC_STORE_SUCCESS;
}

bool ContentReader::GetDataString(std::ifstream& docStream, std::string& value)
{
    uint32_t valueSize = 0;
    docStream.read(reinterpret_cast<char*>(&valueSize), sizeof(uint32_t));
    if (valueSize > (MAX_VERSION_LENG + 1) || valueSize == 0) { // end with '\0'
        HIVIEW_LOGE("version size is invalid, size=%{public}" PRIu32 "", valueSize);
        return false;
    }

    char valueStr[valueSize];
    docStream.read(valueStr, valueSize);
    value = std::string(valueStr);
    return true;
}
} // HiviewDFX
} // OHOS
