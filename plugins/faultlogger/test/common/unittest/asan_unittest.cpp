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
#include <gtest/gtest.h>
#include <unistd.h>

#include "file_util.h"
#include "gwpasan_collector.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
class AsanUnittest : public testing::Test {
public:
    void SetUp()
    {
        chmod("/data/log/faultlog/", 0777); // 0777: add other user write permission
        chmod("/data/log/faultlog/faultlogger/", 0777); // 0777: add other user write permission
        sleep(1);
    };
    void TearDown()
    {
        chmod("/data/log/faultlog/", 0770); // 0770: restore permission
        chmod("/data/log/faultlog/faultlogger/", 0770); // 0770: restore permission
    };

    static void ClearAllLogs(const std::string& path)
    {
        DIR* dir = opendir(path.c_str());
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string full_path = path + "/" + entry->d_name;
            remove(full_path.c_str());
        }
        closedir(dir);
    }

    static bool hasSanitizerLogs(std::string path, std::string type)
    {
        std::vector<std::string> files;
        FileUtil::GetDirFiles(path, files, false);
        bool hasLogs = false;
        for (const auto& file : files) {
            if (file.find(type) != std::string::npos) {
                hasLogs = true;
                break;
            }
        }
        return hasLogs;
    }
};

/**
 * @tc.name: AsanTest001
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest001, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char path[] = "faultlogger";
    char gwpAsanBuf[] = "Test GWP-ASAN, End GWP-ASan report";
    WriteSanitizerLog(gwpAsanBuf, strlen(gwpAsanBuf), path);
    char cfiBuf[] = "Test CFI, End CFI report";
    WriteSanitizerLog(cfiBuf, strlen(cfiBuf), path);
    char ubsanBuf[] = "Test UBSAN, End Ubsan report";
    WriteSanitizerLog(ubsanBuf, strlen(ubsanBuf), path);
    char tsanBuf[] = "Test TSAN, End Tsan report";
    WriteSanitizerLog(tsanBuf, strlen(tsanBuf), path);
    char hwasanBuf[] = "Test HWASAN, End Hwasan report";
    WriteSanitizerLog(hwasanBuf, strlen(hwasanBuf), path);
    char asanBuf[] = "Test ASAN, End Asan report";
    WriteSanitizerLog(asanBuf, strlen(asanBuf), path);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: AsanTest002
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest002, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char* buf = nullptr;
    size_t sz = 10;
    char path[] = "faultlogger";
    WriteSanitizerLog(buf, sz, path);
    bool result = hasSanitizerLogs("/data/log/faultlog/faultlogger/", "asan");
    ASSERT_FALSE(result);
}

/**
 * @tc.name: AsanTest003
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest003, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char path[] = "/data/sanitizer.log";
    char hwasanBuf[] = "Test HWASAN, End Hwasan report";
    WriteSanitizerLog(hwasanBuf, strlen(hwasanBuf), path);
    bool result = hasSanitizerLogs("/data/log/faultlog/faultlogger/", "hwasan");
    ASSERT_FALSE(result);
}
} // namespace HiviewDFX
} // namespace OHOS
