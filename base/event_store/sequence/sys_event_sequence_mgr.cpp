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

#include "sys_event_sequence_mgr.h"

#include <fstream>

#include "file_util.h"
#include "hiview_logger.h"
#include "sys_event_dao.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
namespace {
DEFINE_LOG_TAG("HiView-SysEventSeqMgr");
constexpr char SEQ_PERSISTS_FILE_NAME[] = "event_sequence";

bool SaveStringToFile(const std::string& filePath, const std::string& content)
{
    std::ofstream file;
    file.open(filePath.c_str(), std::ios::in | std::ios::out);
    if (!file.is_open()) {
        file.open(filePath.c_str(), std::ios::out);
        if (!file.is_open()) {
            return false;
        }
    }
    file.seekp(0);
    file.write(content.c_str(), content.length() + 1);
    bool ret = !file.fail();
    file.close();
    return ret;
}
}

SysEventSequenceManager& SysEventSequenceManager::GetInstance()
{
    static SysEventSequenceManager instance;
    return instance;
}

SysEventSequenceManager::SysEventSequenceManager()
{
    if (!FileUtil::FileExists(GetSequenceFile())) {
        EventStore::SysEventDao::Restore();
    }
    int64_t seq = 0;
    ReadSeqFromFile(seq);
    startSeq_ = seq + 1; // if sysevent restored, the start sequence must plus one for export
    HIVIEW_LOGI("start seq=%{public}" PRId64, startSeq_);
    curSeq_.store(startSeq_, std::memory_order_release);
}

void SysEventSequenceManager::SetSequence(int64_t seq)
{
    curSeq_.store(seq, std::memory_order_release);
    WriteSeqToFile(seq);
}

int64_t SysEventSequenceManager::GetSequence()
{
    return curSeq_.load(std::memory_order_acquire);
}

int64_t SysEventSequenceManager::GetStartSequence()
{
    return startSeq_;
}

void SysEventSequenceManager::WriteSeqToFile(int64_t seq)
{
    std::string seqFile(GetSequenceFile());
    std::string content(std::to_string(seq));
    if (!SaveStringToFile(seqFile, content)) {
        HIVIEW_LOGE("failed to write sequence %{public}s.", content.c_str());
    }
}

void SysEventSequenceManager::ReadSeqFromFile(int64_t& seq)
{
    std::string content;
    std::string seqFile = GetSequenceFile();
    if (!FileUtil::LoadStringFromFile(seqFile, content)) {
        HIVIEW_LOGE("failed to read sequence value from %{public}s.", seqFile.c_str());
        return;
    }
    seq = static_cast<int64_t>(strtoll(content.c_str(), nullptr, 0));
    HIVIEW_LOGI("read sequence successful, value is %{public}" PRId64 ".", seq);
}

std::string SysEventSequenceManager::GetSequenceFile() const
{
    return EventStore::SysEventDao::GetDatabaseDir() + SEQ_PERSISTS_FILE_NAME;
}
} // EventStore
} // HiviewDFX
} // OHOS