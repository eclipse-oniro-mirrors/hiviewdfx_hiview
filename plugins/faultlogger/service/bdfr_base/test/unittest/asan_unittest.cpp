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

    static Json::Value GenerateAndParseSanitizerEvent(const std::string& buf)
    {
        char path[] = "faultlogger";
        auto strat = GetCurrentTimestampMs();
        std::vector<char> logbuf(buf.begin(), buf.end());
        logbuf.push_back('\0');
        WriteSanitizerLog(logbuf.data(), buf.length(), path);
        auto end = GetCurrentTimestampMs();

        std::string cmd = "hisysevent -l -o RELIABILITY -n ADDR_SANITIZER -s " +
            std::to_string(strat) + " -e " + std::to_string(end + 5);
        std::string result;
        EXPECT_TRUE(ExecuteCmd(cmd, result));
        GTEST_LOG_(INFO) << cmd;
        GTEST_LOG_(INFO) << result;

        Json::Reader reader;
        Json::Value sanitizerEvent;
        if (!reader.parse(result, sanitizerEvent)) {
            GTEST_LOG_(ERROR) << "Failed to parse JSON: " << reader.getFormattedErrorMessages();
        }
        return sanitizerEvent;
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
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(hwasanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/data/debugsantizer+0x1c7c");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/");
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
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(hwasanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/system/lib64/platformsdk/libeventhandler.z.so+0x1ed9c");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/system/bin/appspawn+0xb940");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/");
}

/**
 * @tc.name: AsanTest006
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest006, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char hwasanBuf[] =
        "==sa_main==1077==ERROR: HWAddressSanitizer: tag-mismatch on address 0x000200253d20 at pc 0x000000000000\n"
        "WRITE of size 4 at 0x000200253d20 tags: 8f/32 (ptr/mem) in thread 65095\n"
        "    <empty stack>\n"
        "\n"
        "[0x000200253d00,0x000200253d40) is a small unallocated heap chunk; size: 64 offset: 32, Allocated By 62463\n"
        "\n"
        "Potential Cause: use-after-free\n"
        "0x000200253d20 (rb[13340] tags:d3) is located 32 bytes inside of 48-byte region "
        "[0x000200253d00,0x000200253d30)\n"
        "freed by thread 1077 here:\n"
        "    #0 0x5ad4a6bc54  (/system/asan/lib64/libclang_rt.hwasan.so+0x2bc54) (BuildId: 3246022d31ed)\n"
        "    #1 0x6401a0a210  (/system/asan/lib64/platformsdk/libapp_manager.z.so+0x10a210) (BuildId: c5b6)\n"
        "    #2 0x6401a0a510  (/system/asan/lib64/platformsdk/libapp_manager.z.so+0x10a510) (BuildId: c5b6)\n"
        "==sa_main==1077==End Hwasan report\n";
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(hwasanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/system/asan/lib64/platformsdk/libapp_manager.z.so+0x10a210");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/system/asan/lib64/platformsdk/libapp_manager.z.so+0x10a510");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/");
}

/**
 * @tc.name: AsanTest007
 * @tc.desc: Test calling WriteSanitizerLog Func
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, WriteSanitizerLogTest007, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char hwasanBuf[] =
        "\"==appspawn==26514==ERROR: HWAddressSanitizer: tag-mismatch on address 0x00040099b0c0 at pc 0x0066e33a7ed4\n"
        "READ of size 4 at 0x00040099b0c0 tags: c6/7c (ptr/mem) in thread 27406\n"
        "    #0 0x66e33a7ed4  (/lib/ld-musl-aarch64-asan.so.1+0x1c4ed4) (BuildId: 9d85a942843298c4acb116342082d68b)\n"
        "    #1 0x66e4c3dca8  (/system/asan/lib64/libc++.so+0xfdca8) (BuildId: df08820831451dcb5f87265be7e5c85af7a7db68)\n"
        "[0x000100020000,0x000100020020) is a small unallocated heap chunk; size: 32 offset: 0, Allocated By 14705\n"
        "Potential Cause: use-after-free\n"
        "0x000100020000 (rb[0] tags:df) is located 0 bytes inside of 8-byte region [0x000100020000,0x000100020008)\n"
        "freed by thread 14705 here:\n"
        "    #0 0x66e466bc10  (/system/asan/lib64/libclang_rt.hwasan.so+0x2bc10) (BuildId: d3555fb4a9b8c0d25877121185b43107192f5531)\n"
        "    #1 0x711c275d98  (/system/asan/lib64/libsystemshare_base.z.so+0x35d98) (BuildId: ba7223db3af56974558c0d859434f979)\n"
        "Test HWASAN, End Hwasan report\n";
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(hwasanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "use-after-free");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/lib/ld-musl-aarch64-asan.so.1+0x1c4ed4");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/system/asan/lib64/libc++.so+0xfdca8");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/");
}

/**
 * @tc.name: GWPAsanTest001
 * @tc.desc: Test GWPAsan hisysevent
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, GWPAsanTest001, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char gwpAsanBuf[] =
        "*** GWP-ASan detected a memory error ***\n"
        "Buffer Underflow at 0x5b980a0ffc (4 bytes to the left...) by thread 32698 here:\n"
        " #0 0x5c9920c5f8  (/data/storage/el1/bundle/libs/arm64/libsample.so+0xc5f8) (BuildId: ee9b)\n"
        " #1 0x5c9920c230  (/data/storage/el1/bundle/libs/arm64/libsample.so+0xc230) (BuildId: ee9b)\n"
        " #2 0x5c9920c1c8  (/data/storage/el1/bundle/libs/arm64/libsample.so+0xc1c8) (BuildId: ee9b)\n"
        "0x5b980a0ffc was allocated by thread 32698 here:\n"
        " #0 0x5adc12b4d4  (/lib/ld-musl-aarch64.so.1+0x1484d4) (BuildId: d996)\n"
        " #1 0x5adc12ac10  (/lib/ld-musl-aarch64.so.1+0x147c10) (BuildId: d996)\n"
        "*** End GWP-ASan report ***\n";
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(gwpAsanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "GWP-ASAN");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/data/storage/el1/bundle/libs/arm64/libsample.so+0xc5f8");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/data/storage/el1/bundle/libs/arm64/libsample.so+0xc230");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/data/storage/el1/bundle/libs/arm64/libsample.so+0xc1c8");
}

/**
 * @tc.name: GWPAsanTest002
 * @tc.desc: Test GWPAsan hisysevent
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, GWPAsanTest002, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char gwpAsanBuf[] =
        "*** GWP-ASan detected a memory error ***\n"
        "Buffer Underflow at 0x5b980a0ffc (4 bytes to the left...) by thread 32698 here:\n"
        " #0 0x5c9920c5f8  (/system/asan/lib64/libclang_rt.hwasan.so+0x2b2fc) (BuildId: ee9b)\n"
        " #1 0x5c9920c230  (/lib/ld-musl-aarch64.so.1+0xc230) (BuildId: ee9b)\n"
        " #2 0x5c9920c1c8  (/lib/ld-musl-aarch64.so.1+0xc1c8) (BuildId: ee9b)\n"
        "0x5b980a0ffc was allocated by thread 32698 here:\n"
        " #0 0x5adc12b4d4  (/lib/ld-musl-aarch64.so.1+0x1484d4) (BuildId: d996)\n"
        " #1 0x5adc12ac10  (/lib/ld-musl-aarch64.so.1+0x147c10) (BuildId: d996)\n"
        "*** End GWP-ASan report ***\n";
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(gwpAsanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "GWP-ASAN");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/system/asan/lib64/libclang_rt.hwasan.so+0x2b2fc");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/lib/ld-musl-aarch64.so.1+0xc230");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/lib/ld-musl-aarch64.so.1+0xc1c8");
}

/**
 * @tc.name: UbsanTest001
 * @tc.desc: Test GWPAsan hisysevent
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, UbsanTest001, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char ubsanBuf[] =
        "napi_init.cpp:81:14: runtime error: store to misaligned address 0x005aef2dc361 "
        "for type 'int32_t' (aka 'int'), which requires 8 byte alignment\n"
        "0x005aef2dc361: note: pointer points here\n"
        " 00 00 00  00 00 00 00 00 00 00 00  d0 35 e0 d6 7e 00 00 00  "
        "01 00 00 00 00 00 00 00  38 cf 9f ee 9e\n"
        "              ^ \n"
        "    #0 0x5bf59756dc  (/data/storage/el1/bundle/libs/arm64/libentry.so+0xb56dc) (BuildId: ee9b)\n"
        "    #1 0x5ad57270a0  (/system/lib64/platformsdk/libace_napi.z.so+0x670a0) (BuildId: ee9b)\n"
        "    #2 0x7ed93b69ec  (/system/lib64/module/arkcompiler/stub.an+0xe0d9ec)\n"
        "    #3 0x7ed8a0d908  (/system/lib64/module/arkcompiler/stub.an+0x464908)\n"
        "\n"
        "SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior "
        "C:/Users/c00905970/DevEcoStudioProjects/"
        "ADDRESS_SANITIZER/ubsan/entry/src/main/cpp/napi_init.cpp:81:14 in \n"
        "==com.debug.ubsan==60853==End of process memory map.\n"
        "==com.debug.ubsan==60853==End Ubsan report\n";
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(ubsanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "UBSAN");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/data/storage/el1/bundle/libs/arm64/libentry.so+0xb56dc");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/system/lib64/platformsdk/libace_napi.z.so+0x670a0");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/system/lib64/module/arkcompiler/stub.an+0xe0d9ec");
}

/**
 * @tc.name: TsanTest001
 * @tc.desc: Test GWPAsan hisysevent
 * @tc.type: FUNC
 */
HWTEST_F(AsanUnittest, TsanTest001, testing::ext::TestSize.Level1)
{
    ClearAllLogs("/data/log/faultlog/faultlogger/");
    char tsanBuf[] =
        "==appspawn==37911==ThreadSanitizer: WARNING: unexpected format specifier "
        "in printf interceptor: %{ (reported once per process)\n"
        "==================\n"
        "WARNING: ThreadSanitizer: data race (pid=37911)\n"
        "  Write of size 4 at 0x0057103d2eac by main thread:\n"
        "    #0 0x57103c8eac  (/data/storage/el1/bundle/libs/arm64/libentry.so+0x8ea8) (BuildId: 4f61)\n"
        "    #1 0x57103ca1a8  (/data/storage/el1/bundle/libs/arm64/libentry.so+0xa1a4) (BuildId: 4f61)\n"
        "    #2 0x55ec6e70a4  (/system/lib64/platformsdk/libace_napi.z.so+0x670a0) (BuildId: 3604)\n"
        "\n"
        "  Previous write of size 1 at 0x0057103d2eac by thread T29:\n"
        "    #0 0x57103c8e30  (/data/storage/el1/bundle/libs/arm64/libentry.so+0x8e2c) (BuildId: 4f61)\n"
        "\n"
        "  Location is global '<null>' at 0x000000000000 (libentry.so+0x12eac)\n"
        "\n"
        "  Thread T29 (tid=38186, running) created by main thread at:\n"
        "    #0 0x5556db8c9c  (/system/lib64/libclang_rt.tsan.so+0x78c98) (BuildId: ab90)\n"
        "    #1 0x57103ca188  (/data/storage/el1/bundle/libs/arm64/libentry.so+0xa184) (BuildId: 4f61)\n"
        "\n"
        "SUMMARY: ThreadSanitizer: data race (/data/storage/el1/bundle/libs/arm64/libentry.so+0x8ea8) (BuildId: 4f61)\n"
        "==appspawn==37911==End of process memory map.\n"
        "==================\n"
        "End Tsan report\n";
    Json::Value sanitizerEvent = GenerateAndParseSanitizerEvent(tsanBuf);
    EXPECT_EQ(sanitizerEvent["MODULE"], "AsanUnittest");
    EXPECT_EQ(sanitizerEvent["REASON"], "TSAN");
    EXPECT_EQ(sanitizerEvent["FIRST_FRAME"], "/data/storage/el1/bundle/libs/arm64/libentry.so+0x8ea8");
    EXPECT_EQ(sanitizerEvent["SECOND_FRAME"], "/data/storage/el1/bundle/libs/arm64/libentry.so+0xa1a4");
    EXPECT_EQ(sanitizerEvent["LAST_FRAME"], "/system/lib64/platformsdk/libace_napi.z.so+0x670a0");
}
} // namespace HiviewDFX
} // namespace OHOS
