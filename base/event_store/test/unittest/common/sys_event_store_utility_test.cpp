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

#include "sys_event_store_utility_test.h"

#include <gmock/gmock.h>

#include "base_def.h"
#include "content_reader.h"
#include "content_reader_factory.h"
#include "content_reader_version_1.h"
#include "content_reader_version_2.h"
#include "content_reader_version_3.h"
#include "hiview_logger.h"
#include "sys_event_doc_reader.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("SysEventStoreUtilityTest");
using namespace  OHOS::HiviewDFX::EventStore;
namespace {
constexpr int8_t TEST_READER_VERSION = 100;
constexpr uint32_t TEST_EVENT_SIZE = 200;
constexpr int32_t TEST_EVENT_TYPE = 4;
const std::string TEST_DB_VERSION1_FILE = "/data/test/TEST_DOMAIN/TEST_VERSION1-1-CRITICAL-1.db";
const std::string TEST_DB_VERSION2_FILE = "/data/test/TEST_DOMAIN/TEST_VERSION2-1-CRITICAL-1.db";
const std::string TEST_DB_VERSION3_FILE = "/data/test/TEST_DOMAIN/TEST_VERSION3-1-CRITICAL-1.db";
class ContentReaderVersionTest : public ContentReader {
public:
    int ReadDocDetails(std::ifstream& docStream, EventStore::DocHeader& header,
        uint64_t& docHeaderSize, std::string& sysVersion) override
    {
        if (!docStream.is_open()) {
            return DOC_STORE_ERROR_IO;
        }
        docHeaderSize = 0;
        sysVersion = "";
        return DOC_STORE_SUCCESS;
    }

    bool IsValidMagicNum(const uint64_t magicNum) override
    {
        HIVIEW_LOGD("magicNum is %{public}" PRIu64 ".", magicNum);
        return true;
    }

protected:
    int GetContentHeader(uint8_t* content, EventStore::ContentHeader& header) override
    {
        if (content == nullptr) {
            return DOC_STORE_ERROR_INVALID;
        }
        HIVIEW_LOGD("seq is %{public}" PRId64 ".", header.seq);
        return DOC_STORE_SUCCESS;
    }

    size_t GetContentHeaderSize()
    {
        return sizeof(EventStore::ContentHeader);
    }
};

bool CompareSeqFuncGreater(const Entry& entryA, const Entry& entryB)
{
    return entryA.id > entryB.id;
}

void TestFileSizeOfDocReader(const std::string& path)
{
    SysEventDocReader reader(path);
    ASSERT_GT(reader.ReadFileSize(), 0);
}

void TestPageSizeOfDocReader(const std::string& path)
{
    SysEventDocReader reader(path);
    uint32_t pageSize = 0;
    ASSERT_EQ(reader.ReadPageSize(pageSize), DOC_STORE_SUCCESS);
    ASSERT_GT(pageSize, 0);
}

void TestHeaderOfDocReader(const std::string& path)
{
    SysEventDocReader reader(path);
    DocHeader header;
    ASSERT_EQ(reader.ReadHeader(header), DOC_STORE_SUCCESS);
    ASSERT_GT(header.magicNum, 0);
    ASSERT_GT(header.blockSize, 0);
    ASSERT_GT(header.pageSize, 0);
    ASSERT_GT(header.version, 0);
}

void TestSystemVersionOfDocReader(const std::string& path, bool isVersionEmpty = false)
{
    SysEventDocReader reader(path);
    DocHeader header;
    std::string sysVersion;
    ASSERT_EQ(reader.ReadHeader(header, sysVersion), DOC_STORE_SUCCESS);
    ASSERT_EQ(sysVersion.empty(), isVersionEmpty);
}

void CheckEvent(SysEvent& event)
{
    ASSERT_TRUE(event.domain_ == "TEST_DOMAIN");
    ASSERT_TRUE(event.eventName_.find("TEST_VERSION") == 0);
    ASSERT_EQ(event.eventType_, TEST_EVENT_TYPE);
    ASSERT_GT(event.happenTime_, 0);
    ASSERT_GE(event.GetTz(), 0);
    ASSERT_GE(event.GetPid(), 0);
    ASSERT_GE(event.GetTid(), 0);
    ASSERT_GE(event.GetUid(), 0);
    ASSERT_GE(event.GetSeq(), 0);

    std::string eventJsonStr = event.AsJsonStr();
    HIVIEW_LOGI("event=%{public}s", eventJsonStr.c_str());
    ASSERT_FALSE(eventJsonStr.empty());
    ASSERT_TRUE(eventJsonStr.find(EventCol::DOMAIN) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::NAME) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::TYPE) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::TS) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::TZ) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::PID) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::TID) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::UID) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::LEVEL) != std::string::npos);
    ASSERT_TRUE(eventJsonStr.find(EventCol::SEQ) != std::string::npos);
}

void TestEventsOfDocReader(const std::string& path, bool isVersionEmpty = false)
{
    SysEventDocReader reader(path);
    DocQuery query;
    EntryQueue entries(CompareSeqFuncGreater);
    int num = 0;
    ASSERT_EQ(reader.Read(query, entries, num), DOC_STORE_SUCCESS);
    ASSERT_GT(num, 0);

    while (!entries.empty()) {
        auto& entry = entries.top();
        SysEvent event("", nullptr, entry.data, entry.id);
        CheckEvent(event);
        entries.pop();
    }
}
}

void SysEventStoreUtilityTest::SetUpTestCase()
{
}

void SysEventStoreUtilityTest::TearDownTestCase()
{
}

void SysEventStoreUtilityTest::SetUp()
{
}

void SysEventStoreUtilityTest::TearDown()
{
}

/**
 * @tc.name: SysEventStoreUtilityTest001
 * @tc.desc: ContentReaderFactory test
 * @tc.type: FUNC
 * @tc.require: issueI9DJP3
 */
HWTEST_F(SysEventStoreUtilityTest, SysEventStoreUtilityTest001, testing::ext::TestSize.Level3)
{
    std::shared_ptr<ContentReader> reader = std::make_shared<ContentReaderVersionTest>();
    ContentReaderFactory::GetInstance().Register(TEST_READER_VERSION, reader);
    auto regReader = ContentReaderFactory::GetInstance().Get(TEST_READER_VERSION);
    ASSERT_EQ(reader, regReader);
    auto invalidReader = ContentReaderFactory::GetInstance().Get(EventStore::EVENT_DATA_FORMATE_VERSION::INVALID);
    ASSERT_EQ(invalidReader, nullptr);
}

/**
 * @tc.name: SysEventStoreUtilityTest002
 * @tc.desc: ContentReader test
 * @tc.type: FUNC
 * @tc.require: issueI9DJP3
 */
HWTEST_F(SysEventStoreUtilityTest, SysEventStoreUtilityTest002, testing::ext::TestSize.Level3)
{
    std::ifstream dbFileStream;
    dbFileStream.open(TEST_DB_VERSION1_FILE, std::ios::binary);
    ASSERT_TRUE(dbFileStream.is_open());
    auto version = ContentReader::ReadFmtVersion(dbFileStream);
    ASSERT_EQ(version, EventStore::EVENT_DATA_FORMATE_VERSION::VERSION1);
    std::shared_ptr<ContentReader> reader = ContentReaderFactory::GetInstance().Get(version);
    ASSERT_NE(reader, nullptr);
    ASSERT_TRUE(reader->IsValidMagicNum(MAGIC_NUM_VERSION1));
    EventStore::DocHeader header;
    uint64_t docHeaderSize;
    std::string sysVersion;
    reader->ReadDocDetails(dbFileStream, header, docHeaderSize, sysVersion);
    ASSERT_EQ(header.version, EventStore::EVENT_DATA_FORMATE_VERSION::VERSION1);
    ASSERT_EQ(sysVersion.size(), 0);
    dbFileStream.close();
    dbFileStream.open(TEST_DB_VERSION2_FILE, std::ios::binary);
    ASSERT_TRUE(dbFileStream.is_open());
    version = ContentReader::ReadFmtVersion(dbFileStream);
    ASSERT_EQ(version, EventStore::EVENT_DATA_FORMATE_VERSION::VERSION2);
    reader = ContentReaderFactory::GetInstance().Get(version);
    ASSERT_NE(reader, nullptr);
    ASSERT_TRUE(reader->IsValidMagicNum(MAGIC_NUM_VERSION2));
    reader->ReadDocDetails(dbFileStream, header, docHeaderSize, sysVersion);
    ASSERT_EQ(header.version, EventStore::EVENT_DATA_FORMATE_VERSION::VERSION2);
    ASSERT_EQ(sysVersion.size(), 0);
    dbFileStream.close();
}

/**
 * @tc.name: SysEventStoreUtilityTest003
 * @tc.desc: ContentReader test
 * @tc.type: FUNC
 * @tc.require: issueI9DJP3
 */
HWTEST_F(SysEventStoreUtilityTest, SysEventStoreUtilityTest003, testing::ext::TestSize.Level3)
{
    std::ifstream dbFileStream;
    dbFileStream.open(TEST_DB_VERSION3_FILE, std::ios::binary);
    auto version = ContentReader::ReadFmtVersion(dbFileStream);
    ASSERT_EQ(version, EventStore::EVENT_DATA_FORMATE_VERSION::VERSION3);
    auto reader = ContentReaderFactory::GetInstance().Get(version);
    ASSERT_NE(reader, nullptr);
    ASSERT_TRUE(reader->IsValidMagicNum(MAGIC_NUM_VERSION3));
    EventStore::DocHeader header;
    uint64_t docHeaderSize = 0;
    std::string sysVersion;
    reader->ReadDocDetails(dbFileStream, header, docHeaderSize, sysVersion);
    ASSERT_EQ(header.version, EventStore::EVENT_DATA_FORMATE_VERSION::VERSION3);
    ASSERT_GT(sysVersion.size(), 0);
    uint8_t content[TEST_EVENT_SIZE] = {0};
    dbFileStream.seekg(docHeaderSize, std::ios::beg);
    dbFileStream.read(reinterpret_cast<char*>(&content), TEST_EVENT_SIZE);
    uint32_t contentSize = TEST_EVENT_SIZE;
    EventInfo info;
    auto ret = reader->ReadRawData(info, content, contentSize);
    ASSERT_NE(ret, nullptr);
    dbFileStream.close();
}

/**
 * @tc.name: SysEventStoreUtilityTest004
 * @tc.desc: SysEventDocReader test
 * @tc.type: FUNC
 * @tc.require: issueI9DJP3
 */
HWTEST_F(SysEventStoreUtilityTest, SysEventStoreUtilityTest004, testing::ext::TestSize.Level3)
{
    const std::vector<std::string> dbVersionPaths = {
        TEST_DB_VERSION1_FILE,
        TEST_DB_VERSION2_FILE,
        TEST_DB_VERSION3_FILE
    };
    for (const auto& dbPath : dbVersionPaths) {
        TestFileSizeOfDocReader(dbPath);
        TestPageSizeOfDocReader(dbPath);
        TestHeaderOfDocReader(dbPath);

        // version1 and version2 do not support the sys version
        if (dbPath == TEST_DB_VERSION1_FILE || dbPath == TEST_DB_VERSION2_FILE) {
            TestSystemVersionOfDocReader(dbPath, true);
        } else {
            TestSystemVersionOfDocReader(dbPath, false);
        }

        TestEventsOfDocReader(dbPath);
    }
}
} // namespace HiviewDFX
} // namespace OHOS