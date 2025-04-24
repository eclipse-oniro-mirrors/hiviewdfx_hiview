/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "sys_event_doc_reader.h"

#include <cinttypes>

#include "event_db_file_util.h"
#include "hiview_logger.h"
#include "securec.h"
#include "string_util.h"
#include "sys_event_query.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
DEFINE_LOG_TAG("HiView-SysEventDocReader");
using OHOS::HiviewDFX::EventRaw::RawData;
namespace {
template<typename T>
int32_t GetNegativeNum(T num)
{
    return static_cast<int32_t>(~(num - 1)); // 1 for binary conversion
}

template<typename T>
void ReadValueAndReset(std::ifstream& in, T& value)
{
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    in.seekg(GetNegativeNum(sizeof(T)), std::ios::cur);
}
}

SysEventDocReader::SysEventDocReader(const std::string& path): EventDocReader(path)
{
    Init(path);
}

SysEventDocReader::~SysEventDocReader()
{
    if (in_.is_open()) {
        in_.close();
    }
}

void SysEventDocReader::Init(const std::string& path)
{
    in_.open(path, std::ios::binary);

    // get domain, name and level from file path
    InitEventInfo(path);

    // get file size and data format version
    if (in_.is_open()) {
        auto curPos = in_.tellg();
        in_.seekg(0, std::ios::end);
        fileSize_ = in_.tellg();
        in_.seekg(curPos, std::ios::beg);

        dataFmtVersion_ = ContentReader::ReadFmtVersion(in_);
    }
}

void SysEventDocReader::InitEventInfo(const std::string& path)
{
    std::vector<std::string> dirNames;
    StringUtil::SplitStr(path, "/", dirNames);
    constexpr size_t domainOffset = 2;
    if (dirNames.size() < domainOffset) {
        HIVIEW_LOGW("invalid size=%{public}zu of dir names", dirNames.size());
        return;
    }

    // init domain
    const std::string domainStr = dirNames[dirNames.size() - domainOffset];
    if (memcpy_s(info_.domain, MAX_DOMAIN_LEN, domainStr.c_str(), domainStr.length()) != EOK) {
        HIVIEW_LOGE("failed to copy domain to EventInfo");
        return;
    }
    std::string file = dirNames.back();
    SplitedEventInfo eventInfo;
    if (!EventDbFileUtil::ParseEventInfoFromDbFileName(file, eventInfo, NAME_ONLY | LEVEL_ONLY)) {
        HIVIEW_LOGW("failed to parse event info from %{public}s", file.c_str());
        return;
    }

    // init name
    if (memcpy_s(info_.name, MAX_EVENT_NAME_LEN, eventInfo.name.c_str(), eventInfo.name.length()) != EOK) {
        HIVIEW_LOGE("failed to copy name to EventInfo");
        return;
    }

    // init level
    info_.level = eventInfo.level;
}

int SysEventDocReader::Read(const DocQuery& query, EntryQueue& entries, int& num)
{
    auto saveFunc = [this, &query, &entries, &num](uint8_t* content, uint32_t& contentSize) {
        if (content == nullptr) {
            return false;
        }
        TryToAddEntry(content, contentSize, query, entries, num);
        return true;
    };
    return Read(saveFunc);
}

int SysEventDocReader::Read(ReadCallback callback)
{
    // read the header
    DocHeader header;
    if (auto ret = ReadHeader(header); ret != DOC_STORE_SUCCESS) {
        return ret;
    }

    if (!IsValidHeader(header)) {
        return DOC_STORE_ERROR_INVALID;
    }

    // set event tag if have
    if (info_.tag.empty() && strlen(header.tag) != 0) {
        info_.tag = header.tag;
    }

    // set page size
    pageSize_ = header.pageSize * NUM_OF_BYTES_IN_KB;

    // read the events
    if (pageSize_ == 0) {
        uint8_t* content = nullptr;
        uint32_t contentSize = 0;
        uint32_t pageIndex = 0;
        if (auto ret = ReadContent(&content, contentSize, pageIndex); ret != DOC_STORE_SUCCESS) {
            return ret;
        }
        callback(content, contentSize);
        delete[] content;
        return DOC_STORE_SUCCESS;
    }
    return ReadPages(callback);
}

int SysEventDocReader::ReadHeader(DocHeader& header)
{
    auto reader = ContentReaderFactory::GetInstance().Get(dataFmtVersion_);
    if (reader == nullptr) {
        HIVIEW_LOGE("reader is nullptr, version:%{public}d", dataFmtVersion_);
        return DOC_STORE_ERROR_IO;
    }
    return reader->ReadDocDetails(in_, header, docHeaderSize_, headExtra_);
}

int SysEventDocReader::ReadHeader(DocHeader& header, HeadExtraInfo& headExtra)
{
    ReadHeader(header);
    headExtra = headExtra_;
    return DOC_STORE_SUCCESS;
}

int SysEventDocReader::ReadPages(ReadCallback callback)
{
    uint32_t pageIndex = 0;
    while (!HasReadFileEnd()) {
        uint8_t* content = nullptr;
        uint32_t contentSize = 0;
        if (ReadContent(&content, contentSize, pageIndex) != DOC_STORE_SUCCESS) {
            pageIndex++;
            if (SeekgPage(pageIndex) != DOC_STORE_SUCCESS) {
                HIVIEW_LOGD("end to seekg the next page index=%{public}" PRIu32 ", file=%{public}s",
                    pageIndex, docPath_.c_str());
                break;
            }
            HIVIEW_LOGD("read the next page index=%{public}" PRIu32 ", file=%{public}s", pageIndex, docPath_.c_str());
            continue;
        }
        callback(content, contentSize);
        delete[] content;
    }
    return DOC_STORE_SUCCESS;
}

bool SysEventDocReader::HasReadFileEnd()
{
    if (!in_.is_open()) {
        return true;
    }
    return (static_cast<int>(in_.tellg()) < 0) || in_.eof();
}

bool SysEventDocReader::HasReadPageEnd(uint32_t pageIndex)
{
    if (HasReadFileEnd()) {
        return true;
    }
    uint32_t curPos = static_cast<uint32_t>(in_.tellg());
    if (curPos <= docHeaderSize_) {
        return false;
    }
    if (curPos == docHeaderSize_ + pageSize_ * (pageIndex + 1)) {
        /* if no byte of current page is filled with '\0' charactor,
         * when position is at the end of current page,
         * we should check whether page is end by page index increment.
         */
        return true;
    }
    return ((curPos - docHeaderSize_) % pageSize_ + HIVIEW_BLOCK_SIZE) >= pageSize_;
}

int SysEventDocReader::ReadContent(uint8_t** content, uint32_t& contentSize, uint32_t pageIndex)
{
    if (HasReadPageEnd(pageIndex)) {
        HIVIEW_LOGD("end to read the page, file=%{public}s", docPath_.c_str());
        return DOC_STORE_READ_EMPTY;
    }
    ReadValueAndReset(in_, contentSize);
    constexpr uint32_t minContentSize = HIVIEW_BLOCK_SIZE + sizeof(ContentHeader) + CRC_SIZE;
    if (contentSize < minContentSize) {
        HIVIEW_LOGD("invalid content size=%{public}u, file=%{public}s", contentSize, docPath_.c_str());
        return DOC_STORE_READ_EMPTY;
    }
    if (contentSize > MAX_NEW_SIZE) {
        HIVIEW_LOGE("invalid content size=%{public}u", contentSize);
        return DOC_STORE_ERROR_MEMORY;
    }
    *content = new(std::nothrow) uint8_t[contentSize];
    if (*content == nullptr) {
        HIVIEW_LOGE("failed to new memory for content, size=%{public}u", contentSize);
        return DOC_STORE_ERROR_MEMORY;
    }
    in_.read(reinterpret_cast<char*>(*content), contentSize);
    return DOC_STORE_SUCCESS;
}

int SysEventDocReader::ReadFileSize()
{
    return fileSize_;
}

int SysEventDocReader::ReadPageSize(uint32_t& pageSize)
{
    DocHeader header;
    if (int ret = ReadHeader(header); ret != DOC_STORE_SUCCESS) {
        return ret;
    }
    pageSize = header.pageSize * NUM_OF_BYTES_IN_KB;
    return DOC_STORE_SUCCESS;
}

bool SysEventDocReader::IsValidHeader(const DocHeader& header)
{
    auto reader = ContentReaderFactory::GetInstance().Get(dataFmtVersion_);
    if (reader == nullptr) {
        HIVIEW_LOGE("reader nullptr, version:%{public}d", dataFmtVersion_);
        return false;
    }
    if (!reader->IsValidMagicNum(header.magicNum)) {
        HIVIEW_LOGE("invalid magic number of file=%{public}s", docPath_.c_str());
        return false;
    }
    return true;
}

int SysEventDocReader::SeekgPage(uint32_t pageIndex)
{
    if (HasReadFileEnd()) {
        return DOC_STORE_ERROR_IO;
    }
    auto seekSize = docHeaderSize_ + pageSize_ * pageIndex;
    if (static_cast<int>(seekSize) < ReadFileSize()) {
        in_.seekg(seekSize, std::ios::beg);
        return DOC_STORE_SUCCESS;
    }
    in_.setstate(std::ios::eofbit);
    return DOC_STORE_ERROR_IO;
}

std::shared_ptr<RawData> SysEventDocReader::BuildRawData(uint8_t* content, uint32_t contentSize)
{
    auto reader = ContentReaderFactory::GetInstance().Get(dataFmtVersion_);
    if (reader == nullptr) {
        HIVIEW_LOGE("reader nullptr, version:%{public}d", dataFmtVersion_);
        return nullptr;
    }
    return reader->ReadRawData(info_, content, contentSize);
}

void SysEventDocReader::TryToAddEntry(uint8_t* content, uint32_t contentSize, const DocQuery& query,
    EntryQueue& entries, int& num)
{
    if (!CheckEventInfo(content)) {
        return;
    }

    // check inner condition
    if (!query.IsContainInnerConds(content)) {
        return;
    }
    // build raw data
    auto rawData = BuildRawData(content, contentSize);
    if (rawData == nullptr) {
        return;
    }
    // check extra condition
    if (!query.IsContainExtraConds(rawData->GetData(), rawData->GetDataLength())) {
        return;
    }
    // add to entry queue
    num++;
    entries.emplace(info_.seq, info_.timestamp, rawData, headExtra_.sysVersion, headExtra_.patchVersion);
}

bool SysEventDocReader::CheckEventInfo(uint8_t* content)
{
    if (strlen(info_.domain) == 0 || strlen(info_.name) == 0 || info_.level.empty()) {
        HIVIEW_LOGE("domain=%{public}s or name=%{public}s or level=%{public}s is empty",
            info_.domain, info_.name, info_.level.c_str());
        return false;
    }

    int64_t seq = *(reinterpret_cast<int64_t*>(content + HIVIEW_BLOCK_SIZE));
    if (seq < 0) {
        HIVIEW_LOGE("event seq is invalid, seq=%{public}" PRId64, seq);
        return false;
    }
    info_.seq = seq;

    int64_t timestamp = *(reinterpret_cast<int64_t*>(content + HIVIEW_BLOCK_SIZE + SEQ_SIZE));
    if (timestamp < 0) {
        HIVIEW_LOGE("event seq is invalid, timestamp=%{public}" PRId64, timestamp);
        return false;
    }
    info_.timestamp = timestamp;

    return true;
}

int64_t SysEventDocReader::ReadMaxEventSequence()
{
    int64_t maxSeq = 0;
    auto callback = [&maxSeq] (uint8_t* content, uint32_t& contentSize) {
        if (content == nullptr || contentSize < HIVIEW_BLOCK_SIZE + sizeof(int64_t)) {
            HIVIEW_LOGE("invalid event, content size is %{public}" PRIu32 "", contentSize);
            return false;
        }
        int64_t seq = *(reinterpret_cast<int64_t*>(content + HIVIEW_BLOCK_SIZE));
        if (maxSeq < seq) {
            maxSeq = seq;
        }
        return true;
    };
    (void)Read(callback);
    return maxSeq;
}
} // EventStore
} // HiviewDFX
} // OHOS
