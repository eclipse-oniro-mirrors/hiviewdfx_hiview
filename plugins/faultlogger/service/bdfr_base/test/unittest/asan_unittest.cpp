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
#include <chrono>
#include <securec.h>
#include "json/json.h"
#include "file_util.h"
#include "gwpasan_collector.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
constexpr const uint16_t EACH_LINE_LENGTH = 100;
constexpr const uint16_t TOTAL_LENGTH = 4096;
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
            (void)remove(full_path.c_str());
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

    static bool ExecuteCmd(const std::string &cmd, std::string &result)
    {
        char buff[EACH_LINE_LENGTH] = { 0x00 };
        char output[TOTAL_LENGTH] = { 0x00 };
        FILE *ptr = popen(cmd.c_str(), "r");
        if (ptr != nullptr) {
            while (fgets(buff, sizeof(buff), ptr) != nullptr) {
                if (strcat_s(output, sizeof(output), buff) != 0) {
                    pclose(ptr);
                    ptr = nullptr;
                    return false;
                }
            }
            pclose(ptr);
            ptr = nullptr;
        } else {
            return false;
        }
        result = std::string(output);
        return true;
    }
 
    static int64_t GetCurrentTimestampMs()
    {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return ms.count();
    }
};

/**
 * @tc.name: AsanTest001
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest001, testing::ext::TestSize.Level0)
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

/**
 * @tc.name: AsanTest004
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest004, testing::ext::TestSize.Level0)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char hwasanBuf[] =
        "\"==debugsantizer==14705==ERROR: HWAddressSanitizer:"
        " tag-mismatch on address 0x000100020000 at pc 0x005750422ec8\n"
        "READ of size 8 at 0x000100020000 tags: df/2f (ptr/mem) in thread 14705\n"
        "    #0 0x5750422ec8  (/data/libclang_rt.hwasan.so+0x1ec8) (BuildId: 123456abcd)\n"
        "    #1 0x599883b23c  (/lib/ld-musl-aarch64-asan.so.1+0xc623c) (BuildId: 123456abcd)\n"
        "    #2 0x5750422c7c  (/data/debugsantizer+0x1c7c) (BuildId: 123456abcd)\n"
        "[0x000100020000,0x000100020020) is a small unallocated heap chunk; size: 32 offset: 0, Allocated By 14705\n"
        "Potential Cause: use-after-free\n"
        "0x000100020000 (rb[0] tags:df) is located 0 bytes inside of 8-byte region [0x000100020000,0x000100020008)\n"
        "freed by thread 14705 here:\n"
        "    #0 0x5999c6b2fc  (/system/asan/lib64/libclang_rt.hwasan.so+0x2b2fc) (BuildId: 123456abcd)\n"
        "    #1 0x5999c6b2fc  (/system/asan/lib64/libclang_rt.hwasan.so+0x2b2fc) (BuildId: 123456abcd)\n"
        "Test HWASAN, End Hwasan report\n";
    char path[] = "faultlogger";
    auto strat = GetCurrentTimestampMs();
    WriteSanitizerLog(hwasanBuf, strlen(hwasanBuf), path);
    auto end = GetCurrentTimestampMs();
    std::string cmd = "hisysevent -l -o RELIABILITY -n ADDR_SANITIZER -s " +
        std::to_string(strat) + " -e " + std::to_string(end + 5);
    std::string result;
    auto ret = ExecuteCmd(cmd, result);
    EXPECT_TRUE(ret);
    GTEST_LOG_(INFO) << cmd;
    GTEST_LOG_(INFO) << result;
    Json::Reader reader;
    Json::Value sanitizerEvent;
    if (!reader.parse(result, sanitizerEvent)) {
        GTEST_LOG_(ERROR) << "Failed to parse JSON: " << reader.getFormattedErrorMessages();
    }
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "debugsantizer");
}
 
/**
 * @tc.name: AsanTest005
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest005, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char hwasanBuf[] =
        "Potential Cause: use-after-free\n"
        "ptrBeg was re-written after free 0x000c00036f80[0],"
        "0x000c00036f80 0000000000000000:5555555555555555, freed by:\n"
        "    #0 0x5a94feb2fc  (/system/asan/lib64/libclang_rt.hwasan.so+0x2b2fc) (BuildId: 123456abcd)\n"
        "    #1 0x5a9b860678  (/system/lib64/chipset-sdk-sp/libc++.so+0xcd630) (BuildId: 123456abcd)\n"
        "    #2 0x5a9b85ed9c  (/system/lib64/platformsdk/libeventhandler.z.so+0x1ed9c) (BuildId: 123456abcd)\n"
        "    #3 0x56934ca940  (/system/bin/appspawn+0xb940) (BuildId: 123456abcd)\n"
        "allocated by:\n"
        "    #0 0x5a94feb1a8  (/system/asan/lib64/libclang_rt.hwasan.so+0x2b1a8) (BuildId: 123456abcd)\n"
        "    #1 0x5a9b87e268  (/system/lib64/platformsdk/libeventhandler.z.so+0x3e268) (BuildId: 123456abcd)\n"
        "==appspawn==7133==End Hwasan report\n";
    char path[] = "faultlogger";
    auto strat = GetCurrentTimestampMs();
    WriteSanitizerLog(hwasanBuf, strlen(hwasanBuf), path);
    auto end = GetCurrentTimestampMs();
    std::string cmd = "hisysevent -l -o RELIABILITY -n ADDR_SANITIZER -s " +
        std::to_string(strat) + " -e " + std::to_string(end + 5);
    std::string result;
    auto ret = ExecuteCmd(cmd, result);
    EXPECT_TRUE(ret);
    GTEST_LOG_(INFO) << cmd;
    GTEST_LOG_(INFO) << result;
    Json::Reader reader;
    Json::Value sanitizerEvent;
    if (!reader.parse(result, sanitizerEvent)) {
        GTEST_LOG_(ERROR) << "Failed to parse JSON: " << reader.getFormattedErrorMessages();
    }
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "libeventhandler.z.so");
}
} // namespace HiviewDFX
} // namespace OHOS
