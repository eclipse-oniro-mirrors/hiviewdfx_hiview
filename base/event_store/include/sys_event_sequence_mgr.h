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

#ifndef BASE_EVENT_STORE_SYS_EVENT_SEQ_MGR_H
#define BASE_EVENT_STORE_SYS_EVENT_SEQ_MGR_H

#include <atomic>

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
constexpr char SEQ_PERSISTS_FILE_NAME[] = "event_sequence";
constexpr char SEQ_PERSISTS_BACKUP_FILE_NAME[] = "event_sequence_backup";

class SysEventSequenceManager {
public:
    static SysEventSequenceManager& GetInstance();
    void SetSequence(int64_t seq);
    int64_t GetSequence();
    int64_t GetStartSequence();

private:
   SysEventSequenceManager();
   ~SysEventSequenceManager() = default;

private:
    void WriteSeqToFile(int64_t seq);
    void ReadSeqFromFile(int64_t& seq);
    std::string GetSequenceFile() const;

private:
    std::atomic<int64_t> curSeq_ = 0;
    int64_t startSeq_ = 0; // keep the initial sequence when hiview started, for event restore use
};
} // namespace EventStore
} // namespace HiviewDFX
} // namespace OHOS

#endif // BASE_EVENT_STORE_SYS_EVENT_SEQ_MGR_H