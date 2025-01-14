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

#ifndef HIVIEW_BASE_EVENT_STORE_UTILITY_SYS_EVENT_DOC_READER_H
#define HIVIEW_BASE_EVENT_STORE_UTILITY_SYS_EVENT_DOC_READER_H

#include <fstream>
#include <functional>
#include <string>

#include "content_reader_factory.h"
#include "event_doc_reader.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
using ReadCallback = std::function<bool(uint8_t* content, uint32_t& contentSize)>;
class SysEventDocReader : public EventDocReader {
public:
    SysEventDocReader(const std::string& path);
    ~SysEventDocReader();
    int Read(const DocQuery& query, EntryQueue& entries, int& num) override;
    int ReadFileSize();
    int ReadPageSize(uint32_t& pageSize);
    int ReadHeader(DocHeader& header);
    int ReadHeader(DocHeader& header, HeadExtraInfo& headExtra);

private:
    void Init(const std::string& path);
    void InitEventInfo(const std::string& path);
    int Read(ReadCallback callback);
    int ReadContent(uint8_t** content, uint32_t& contentSize, uint32_t pageIndex);
    int ReadPages(ReadCallback callback);
    bool HasReadFileEnd();
    bool HasReadPageEnd(uint32_t pageIndex);
    bool IsValidHeader(const DocHeader& header);
    bool CheckEventInfo(uint8_t* content);
    int SeekgPage(uint32_t pageIndex);
    std::shared_ptr<RawData> BuildRawData(uint8_t* content, uint32_t contentSize);
    void TryToAddEntry(uint8_t* content, uint32_t contentSize, const DocQuery& query,
        EntryQueue& entries, int& num);

private:
    std::ifstream in_;
    int fileSize_ = 0;
    uint32_t pageSize_ = 0;
    uint8_t dataFmtVersion_ = 0;
    uint64_t docHeaderSize_ = 0;
    HeadExtraInfo headExtra_;
    EventInfo info_;
}; // EventDocWriter
} // EventStore
} // HiviewDFX
} // OHOS
#endif // HIVIEW_BASE_EVENT_STORE_UTILITY_SYS_EVENT_DOC_READER_H
