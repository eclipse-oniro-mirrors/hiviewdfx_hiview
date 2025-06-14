/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#include "adapter_utility_ohos_test.h"

#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include "ash_memory_utils.h"
#include "cjson_util.h"
#include "common_utils.h"
#include "file_util.h"
#include "freeze_json_util.h"
#include "hiview_config_util.h"
#include "hiview_db_util.h"
#include "hiview_zip_util.h"
#include "securec.h"
#include "time_util.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr char LOG_FILE_PATH[] = "/data/test/adapter_utility_test/";
constexpr char TEST_JSON_FILE_PATH[] = "/data/test/test_data/test.json";
constexpr char FREEZE_JSON_FILE[] = "/data/test/test_data/0-0-123456";
constexpr char FREEZE_JSON_FILE_EMPTY[] = "/data/test/test_data/0-1-123456";
constexpr char TEST_ZIP_FILE[] = "/data/test/test_data/test_pack.zip";
const std::string SOURCE_PATH = "/data/test/test_data/";
const std::string ZIP_DES_PATH = "/data/test/test_data/zip_des/";
const std::string DB_PATH = "/data/test/test_data/db/";
const std::string UPLOAD_PATH = "/data/test/test_data/upload/";
constexpr char STRING_VAL[] = "OpenHarmony is a better choice for you.";
constexpr char STRING_ARR_FIRST_VAL[] = "3.1 release";
constexpr int64_t INT_VAL = 2024;
constexpr int64_t INT_VAL_DEFAULT = -1;
constexpr double DOU_VAL = 2024.0;
constexpr double DOU_VAL_DEFAULT = -1.0;
constexpr size_t TEST_ARRAY_SIZE = 2;
constexpr int SUFFIX_0 = 0;
constexpr int SUFFIX_1 = 1;

std::string GetLogDir(std::string& testCaseName)
{
    std::string workPath = std::string(LOG_FILE_PATH);
    if (workPath.back() != '/') {
        workPath = workPath + "/";
    }
    workPath.append(testCaseName);
    workPath.append("/");
    std::string logDestDir = workPath;
    if (!FileUtil::FileExists(logDestDir)) {
        FileUtil::ForceCreateDirectory(logDestDir, FileUtil::FILE_PERM_770);
    }
    return logDestDir;
}

std::string GenerateLogFileName(std::string& testCaseName, int index)
{
    return GetLogDir(testCaseName) + "testFile" + std::to_string(index);
}

struct AshMemTestStruct {
    explicit AshMemTestStruct(std::string data) : data(data) {};

    void* GetData(uint32_t& dataSize) const
    {
        auto dataLen = data.length() + 1;
        char* buff = reinterpret_cast<char *>(malloc(dataLen));
        if (buff == nullptr) {
            return nullptr;
        }
        auto ret = memset_s(buff, dataLen, 0, dataLen);
        if (ret != EOK) {
            return nullptr;
        }

        ret = memcpy_s(buff, dataLen, data.c_str(), data.length());
        if (ret != EOK) {
            return nullptr;
        }
        dataSize = dataLen;
        return buff;
    }

    static AshMemTestStruct ParseData(const char* dataInput, const uint32_t dataSize)
    {
        if (dataInput == nullptr || dataInput[dataSize - 1] != 0) {
            return AshMemTestStruct("");
        }
        return AshMemTestStruct(dataInput);
    }
    std::string data;
};
}

void AdapterUtilityOhosTest::SetUpTestCase() {}

void AdapterUtilityOhosTest::TearDownTestCase() {}

void AdapterUtilityOhosTest::SetUp() {}

void AdapterUtilityOhosTest::TearDown()
{
    (void)FileUtil::ForceRemoveDirectory(LOG_FILE_PATH);
}

/**
 * @tc.name: CommonUtilsOhosTest001
 * @tc.desc: Test GetPidByName defined in namespace CommonUtils
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, CommonUtilsOhosTest001, testing::ext::TestSize.Level3)
{
    std::vector<std::string> cmdRet;
    auto hiviewProcessId = CommonUtils::GetPidByName("hiview");
    ASSERT_TRUE(hiviewProcessId > 0);
}

/**
 * @tc.name: CommonUtilsOhosTest002
 * @tc.desc: Test GetProcFullNameByPid defined in namespace CommonUtils
 * @tc.type: FUNC
 * @tc.require: issueI97MDA
 */
HWTEST_F(AdapterUtilityOhosTest, CommonUtilsOhosTest002, testing::ext::TestSize.Level3)
{
    auto ret = CommonUtils::GetProcFullNameByPid(1); // 1 is pid of init process
    ASSERT_EQ(ret, "init");
    ret = CommonUtils::GetProcFullNameByPid(-1); // -1 is a invalid pid value just for test
    ASSERT_EQ(ret, "");
}

/**
 * @tc.name: TimeUtilOhosTest001
 * @tc.desc: Test Sleep/GetSeconds defined in namespace TimeUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, TimeUtilOhosTest001, testing::ext::TestSize.Level3)
{
    auto time = TimeUtil::GetSeconds();
    ASSERT_GE(time, 0);
    int sleepSecs = 1;
    TimeUtil::Sleep(sleepSecs);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: TimeUtilOhosTest002
 * @tc.desc: Test Get0ClockStampMs/GetSteadyClockTimeMs defined in namespace TimeUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, TimeUtilOhosTest002, testing::ext::TestSize.Level3)
{
    auto time1 = TimeUtil::Get0ClockStampMs();
    auto time2 = TimeUtil::GetSteadyClockTimeMs();
    ASSERT_GE(time1, 0);
    ASSERT_GE(time2, 0);
}

/**
 * @tc.name: TimeUtilOhosTest003
 * @tc.desc: Test GetBootTimeMs defined in namespace TimeUtil
 * @tc.type: FUNC
 * @tc.require: issueI9DYOQ
 */
HWTEST_F(AdapterUtilityOhosTest, TimeUtilOhosTest003, testing::ext::TestSize.Level3)
{
    auto time1 = TimeUtil::GetBootTimeMs();
    ASSERT_GT(time1, 0);

    TimeUtil::Sleep(1); // 1s

    auto time2 = TimeUtil::GetBootTimeMs();
    ASSERT_GT(time2, time1);
}

/**
 * @tc.name: FileUtilOhosTest001
 * @tc.desc: Test LoadBufferFromFile/SaveBufferToFile defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest001, testing::ext::TestSize.Level3)
{
    std::string caseName("FileUtilOhosTest001");
    std::vector<char> content;
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_0), "123");
    (void)FileUtil::LoadBufferFromFile(GenerateLogFileName(caseName, SUFFIX_0), content);
    ASSERT_TRUE(true);
    (void)FileUtil::SaveBufferToFile(GenerateLogFileName(caseName, SUFFIX_0), content, true);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: FileUtilOhosTest002
 * @tc.desc: Test ExtractFilePath/ExtractFileName defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest002, testing::ext::TestSize.Level3)
{
    std::string path = "ohos/test/123.txt";
    auto dir = FileUtil::ExtractFilePath(path);
    ASSERT_EQ("ohos/test/", dir);
    auto name = FileUtil::ExtractFileName(path);
    ASSERT_EQ("123.txt", name);
}

/**
 * @tc.name: FileUtilOhosTest003
 * @tc.desc: Test ExcludeTrailingPathDelimiter/IncludeTrailingPathDelimiter defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest003, testing::ext::TestSize.Level3)
{
    auto excludeRet = FileUtil::ExcludeTrailingPathDelimiter("ohos/test/123.txt");
    ASSERT_EQ("ohos/test/123.txt", excludeRet);
    auto name = FileUtil::IncludeTrailingPathDelimiter("ohos/test/123.txt/");
    ASSERT_EQ("ohos/test/123.txt/", name);
}

/**
 * @tc.name: FileUtilOhosTest004
 * @tc.desc: Test ChangeModeFile defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest004, testing::ext::TestSize.Level3)
{
    std::string caseName("FileUtilOhosTest004");
    (void)FileUtil::ChangeModeFile(GenerateLogFileName(caseName, SUFFIX_0), FileUtil::FILE_PERM_755);
    ASSERT_TRUE(true);
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_0), "test content");
    (void)FileUtil::ChangeModeFile(GenerateLogFileName(caseName, SUFFIX_0), FileUtil::FILE_PERM_660);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: FileUtilOhosTest005
 * @tc.desc: Test Umask defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest005, testing::ext::TestSize.Level3)
{
    auto umask = FileUtil::Umask(FileUtil::FILE_PERM_755);
    auto expectedRet = 18;
    ASSERT_EQ(expectedRet, umask);
}

/**
 * @tc.name: FileUtilOhosTest006
 * @tc.desc: Test FormatPath2UnixStyle/RemoveFolderBeginWith defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest006, testing::ext::TestSize.Level3)
{
    std::string path = std::string(LOG_FILE_PATH);
    FileUtil::FormatPath2UnixStyle(path);
    ASSERT_TRUE(true);
    FileUtil::RemoveFolderBeginWith(path, "data");
    ASSERT_TRUE(true);
}

/**
 * @tc.name: FileUtilOhosTest007
 * @tc.desc: Test WriteBufferToFd defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest007, testing::ext::TestSize.Level3)
{
    std::string caseName("FileUtilOhosTest007");
    auto fd = -1;
    std::string writeContent = "write content";
    auto ret = FileUtil::WriteBufferToFd(fd, writeContent.c_str(), writeContent.size());
    ASSERT_TRUE(!ret);
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_0), "test");
    fd = FileUtil::Open(GenerateLogFileName(caseName, SUFFIX_0),
        O_CREAT | O_WRONLY | O_TRUNC, FileUtil::FILE_PERM_770);
    ret = FileUtil::WriteBufferToFd(fd, nullptr, writeContent.size());
    ASSERT_TRUE(!ret);
    (void)FileUtil::WriteBufferToFd(fd, "", writeContent.size());
    ASSERT_TRUE(true);
    (void)FileUtil::WriteBufferToFd(fd, writeContent.c_str(), writeContent.size());
    ASSERT_TRUE(true);
}

/**
 * @tc.name: FileUtilOhosTest008
 * @tc.desc: Test CreateFile defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest008, testing::ext::TestSize.Level3)
{
    std::string caseName("FileUtilOhosTest008");
    caseName.append("_").append(std::to_string(getpid()));
    std::string fileName = GenerateLogFileName(caseName, SUFFIX_0);
    int ret = FileUtil::CreateFile(fileName);
    if (FileUtil::FileExists(fileName)) {
        ret = FileUtil::CreateFile(fileName);
        ASSERT_EQ(ret, 0); // 0 means the file is succeed to be created
        ASSERT_TRUE(FileUtil::SaveStringToFile(fileName, "1111"));
    }
}

/**
 * @tc.name: FileUtilOhosTest009
 * @tc.desc: Test CopyFile defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest009, testing::ext::TestSize.Level3)
{
    std::string caseName("FileUtilOhosTest009");
    int expectedFailedRet = -1;
    auto ret = FileUtil::CopyFile("//...../invalid_dest_file", GenerateLogFileName(caseName, SUFFIX_1));
    ASSERT_EQ(expectedFailedRet, ret);
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_0), "test");
    ret = FileUtil::CopyFile(GenerateLogFileName(caseName, SUFFIX_0), "//...../invalid_dest_file");
    ASSERT_EQ(expectedFailedRet, ret);
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_1), "test");
    (void)FileUtil::CopyFile(GenerateLogFileName(caseName, SUFFIX_0), GenerateLogFileName(caseName, SUFFIX_1));
    ASSERT_TRUE(true);
}

/**
 * @tc.name: FileUtilOhosTest010
 * @tc.desc: Test GetLastLine defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest010, testing::ext::TestSize.Level3)
{
    std::string caseName("FileUtilOhosTest010");
    ifstream in(GenerateLogFileName(caseName, SUFFIX_0));
    std::string line;
    int invalidMaxLen = 5;
    auto ret = FileUtil::GetLastLine(in, line, invalidMaxLen);
    ASSERT_TRUE(!ret);
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_1), "line1");
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_1), "\nline2");
    (void)FileUtil::SaveStringToFile(GenerateLogFileName(caseName, SUFFIX_1), "\nline3");
    ifstream in1(GenerateLogFileName(caseName, SUFFIX_1));
    ret = FileUtil::GetLastLine(in1, line, invalidMaxLen);
    ASSERT_TRUE(!ret);
    int validMaxLen = 2;
    ret = FileUtil::GetLastLine(in1, line, validMaxLen);
    ASSERT_TRUE(!ret);
    validMaxLen = 3;
    ret = FileUtil::GetLastLine(in1, line, validMaxLen);
    ASSERT_TRUE(!ret);
}

/**
 * @tc.name: FileUtilOhosTest011
 * @tc.desc: Test GetParentDir defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest011, testing::ext::TestSize.Level3)
{
    auto ret = FileUtil::GetParentDir("/");
    ASSERT_EQ("", ret);
    ret = FileUtil::GetParentDir("123/345/789");
    ASSERT_EQ("123/345", ret);
}

/**
 * @tc.name: FileUtilOhosTest012
 * @tc.desc: Test IslegalPath defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest012, testing::ext::TestSize.Level3)
{
    auto ret = FileUtil::IsLegalPath("aa/../bb");
    ASSERT_TRUE(!ret);
    ret = FileUtil::IsLegalPath("aa/./bb");
    ASSERT_TRUE(!ret);
    ret = FileUtil::IsLegalPath("aa/bb/");
    ASSERT_TRUE(ret);
    ret = FileUtil::IsLegalPath("aa/bb/cc");
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: FileUtilOhosTest013
 * @tc.desc: Test RenameFile defined in namespace FileUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, FileUtilOhosTest013, testing::ext::TestSize.Level3)
{
    std::string caseName("FileUtilOhosTest013");
    caseName.append("_").append(std::to_string(getpid()));
    std::string fileName = GenerateLogFileName(caseName, SUFFIX_0);
    std::string fileNameOther = GenerateLogFileName(caseName, SUFFIX_1);
    bool ret = FileUtil::RenameFile(fileName, fileNameOther);
    ASSERT_TRUE(!ret);
    (void)FileUtil::SaveStringToFile(fileName, "line1");
    if (FileUtil::FileExists(fileName)) {
        ret = FileUtil::RenameFile(fileName, fileNameOther);
        ASSERT_EQ(ret, FileUtil::FileExists(fileNameOther));
    }
}

/**
 * @tc.name: AshMemoryUtilsOhosTest001
 * @tc.desc: Test AshMemoryUtil
 * @tc.type: FUNC
 * @tc.require: issueI65DUW
 */
HWTEST_F(AdapterUtilityOhosTest, AshMemoryUtilsOhosTest001, testing::ext::TestSize.Level3)
{
    std::vector<AshMemTestStruct> dataIn = {
        AshMemTestStruct("testData1"),
        AshMemTestStruct("testData2"),
        AshMemTestStruct("testData3"),
        AshMemTestStruct("testData4"),
    };
    const uint32_t memSize = 256;
    auto ashmem = AshMemoryUtils::GetAshmem("ashMemTest", memSize);
    if (ashmem == nullptr) {
        ASSERT_TRUE(false);
    }
    std::vector<uint32_t> allSize;
    bool retIn = AshMemoryUtils::WriteBulkData<AshMemTestStruct>(dataIn, ashmem, memSize, allSize);
    ASSERT_TRUE(retIn);
    std::vector<AshMemTestStruct> dataOut;
    bool retOut = AshMemoryUtils::ReadBulkData<AshMemTestStruct>(ashmem, allSize, dataOut);
    ASSERT_TRUE(retOut);
    ASSERT_TRUE(dataIn.size() == dataOut.size());
    for (size_t i = 0; i < dataIn.size(); i++) {
        ASSERT_EQ(dataIn[i].data, dataOut[i].data);
    }
}

/**
 * @tc.name: CJsonUtilTest001
 * @tc.desc: Test api of CJsonUtil
 * @tc.type: FUNC
 * @tc.require: issueI9E8HA
 */
HWTEST_F(AdapterUtilityOhosTest, CJsonUtilTest001, testing::ext::TestSize.Level3)
{
    auto jsonRoot = CJsonUtil::ParseJsonRoot(TEST_JSON_FILE_PATH);
    ASSERT_NE(jsonRoot, nullptr);
    std::vector<std::string> strArr;
    CJsonUtil::GetStringMemberArray(jsonRoot, "STRING_ARRAY", strArr);
    ASSERT_EQ(strArr.size(), TEST_ARRAY_SIZE);
    ASSERT_EQ(strArr[0], STRING_ARR_FIRST_VAL);
    std::vector<std::string> strArrNullptr;
    CJsonUtil::GetStringMemberArray(nullptr, "STRING_ARRAY", strArrNullptr);
    ASSERT_EQ(strArrNullptr.size(), 0);
    std::vector<std::string> strArrNotSet;
    CJsonUtil::GetStringMemberArray(jsonRoot, "STRING_ARRAY_NOT_SET", strArrNotSet);
    ASSERT_EQ(strArrNotSet.size(), 0);
    ASSERT_NE(CJsonUtil::GetArrayMember(jsonRoot, "STRING_ARRAY"), nullptr);
    ASSERT_EQ(CJsonUtil::GetArrayMember(nullptr, "STRING_ARRAY"), nullptr);
    ASSERT_EQ(CJsonUtil::GetArrayMember(jsonRoot, "STRING_ARRAY_NOT_SET"), nullptr);
    ASSERT_EQ(CJsonUtil::GetStringMemberValue(jsonRoot, "STRING"), STRING_VAL);
    ASSERT_EQ(CJsonUtil::GetStringMemberValue(jsonRoot, "STRING_NOT_SET"), "");
    ASSERT_EQ(CJsonUtil::GetInt64MemberValue(jsonRoot, "INT"), INT_VAL);
    ASSERT_EQ(CJsonUtil::GetDoubleMemberValue(jsonRoot, "DOUBLE"), DOU_VAL);
    ASSERT_EQ(CJsonUtil::GetInt64MemberValue(jsonRoot, "INT_NOT_SET", INT_VAL_DEFAULT), INT_VAL_DEFAULT);
    ASSERT_EQ(CJsonUtil::GetDoubleMemberValue(jsonRoot, "DOUBLE_NOT_SET", DOU_VAL_DEFAULT), DOU_VAL_DEFAULT);
    ASSERT_EQ(CJsonUtil::GetStringMemberValue(nullptr, "STRING"), "");
    ASSERT_EQ(CJsonUtil::GetInt64MemberValue(nullptr, "INT", INT_VAL_DEFAULT), INT_VAL_DEFAULT);
    ASSERT_EQ(CJsonUtil::GetDoubleMemberValue(nullptr, "DOUBLE", DOU_VAL_DEFAULT), DOU_VAL_DEFAULT);
    bool boolValue = false;
    ASSERT_TRUE(CJsonUtil::GetBoolMemberValue(jsonRoot, "BOOL", boolValue));
    ASSERT_TRUE(boolValue);
    ASSERT_FALSE(CJsonUtil::GetBoolMemberValue(nullptr, "BOOL", boolValue));
    ASSERT_FALSE(CJsonUtil::GetBoolMemberValue(jsonRoot, "BOOL_NOT_SET", boolValue));
}

/**
 * @tc.name: FreezeJsonUtilTest001
 * @tc.desc: Test api of FreezeJsonUtil
 * @tc.type: FUNC
 * @tc.require: issueI9E8HA
 */
HWTEST_F(AdapterUtilityOhosTest, FreezeJsonUtilTest001, testing::ext::TestSize.Level3)
{
    FreezeJsonUtil::FreezeJsonCollector jsonCollector;
    FreezeJsonUtil::LoadCollectorFromFile(FREEZE_JSON_FILE, jsonCollector);
    ASSERT_NE(jsonCollector.domain, "");
    ASSERT_NE(jsonCollector.stringId, "");
}

/**
 * @tc.name: FreezeJsonUtilTest002
 * @tc.desc: Test api of FreezeJsonUtil
 * @tc.type: FUNC
 */
HWTEST_F(AdapterUtilityOhosTest, FreezeJsonUtilTest002, testing::ext::TestSize.Level3)
{
    FreezeJsonUtil::FreezeJsonCollector jsonCollector;
    FreezeJsonUtil::LoadCollectorFromFile(FREEZE_JSON_FILE_EMPTY, jsonCollector);
    ASSERT_EQ(jsonCollector.domain, "");
    ASSERT_EQ(jsonCollector.stringId, "");
}

/**
 * @tc.name: ZipUtilTest001
 * @tc.desc: Test api of ZipUtil
 * @tc.type: FUNC
 * @tc.require: issueI9E8HA
 */
HWTEST_F(AdapterUtilityOhosTest, ZipUtilTest001, testing::ext::TestSize.Level3)
{
    std::string testSourceFile1 = SOURCE_PATH + "zip_test_file1.txt";
    std::string testSourceFile2 = SOURCE_PATH + "zip_test_file2.txt";
    FileUtil::SaveStringToFile(testSourceFile1, "zip_test_content1", true);
    FileUtil::SaveStringToFile(testSourceFile2, "zip_test_content2", true);
    {
        HiviewZipUnit zipUnit(TEST_ZIP_FILE);
        ASSERT_EQ(zipUnit.AddFileInZip(testSourceFile1, ZipFileLevel::KEEP_NONE_PARENT_PATH), 0);
        ASSERT_EQ(zipUnit.AddFileInZip(testSourceFile2, ZipFileLevel::KEEP_ONE_PARENT_PATH), 0);
    }
    HiviewUnzipUnit unzipUnit(TEST_ZIP_FILE, ZIP_DES_PATH);
    unzipUnit.UnzipFile();
    const int waitUnzip = 2;
    sleep(waitUnzip);
    ASSERT_TRUE(FileUtil::FileExists(ZIP_DES_PATH + "zip_test_file1.txt"));
    ASSERT_TRUE(FileUtil::FileExists(ZIP_DES_PATH + "test_data/zip_test_file2.txt"));
}

/**
 * @tc.name: DbUtilTest001
 * @tc.desc: Test api of DbUtil
 * @tc.type: FUNC
 */
HWTEST_F(AdapterUtilityOhosTest, DbUtilTest001, testing::ext::TestSize.Level3)
{
    int pid = getpid();
    std::string uploadPath = UPLOAD_PATH + std::to_string(pid) + "/";
    std::string dbPath = DB_PATH + std::to_string(pid) + "/";
    ASSERT_TRUE(HiviewDbUtil::InitDbUploadPath("", uploadPath));
    uploadPath = "";
    ASSERT_FALSE(HiviewDbUtil::InitDbUploadPath("", uploadPath));
    ASSERT_TRUE(HiviewDbUtil::InitDbUploadPath(dbPath, uploadPath));
    std::string dbFile = HiviewDbUtil::CreateFileNameByDate("test");
    std::string otherFile = dbPath + "testOther.db-shm";
    FileUtil::SaveStringToFile(dbPath + dbFile, "test db file", true);
    FileUtil::SaveStringToFile(otherFile, "test other file", true);

    std::string uploadDbFile = uploadPath + "/" + dbFile;
    HiviewDbUtil::MoveDbFilesToUploadDir(dbPath, uploadPath);
    ASSERT_FALSE(FileUtil::FileExists(otherFile));
    ASSERT_TRUE(FileUtil::FileExists(uploadDbFile));

    HiviewDbUtil::TryToAgeUploadDbFiles(uploadPath, 1); // 1 is the max file number
    ASSERT_TRUE(FileUtil::FileExists(uploadDbFile));
    HiviewDbUtil::TryToAgeUploadDbFiles(uploadPath, 0); // 0 is the max file number
    ASSERT_FALSE(FileUtil::FileExists(uploadDbFile));
}

/**
 * @tc.name: HiViewConfigUtilTest001
 * @tc.desc: Test api of HiViewConfigUtil
 * @tc.type: FUNC
 */
HWTEST_F(AdapterUtilityOhosTest, HiViewConfigUtilTest001, testing::ext::TestSize.Level3)
{
    std::string localVer;
    FileUtil::LoadStringFromFile("/system/etc/hiview/hiview_config_version", localVer);
    std::string cloudVer;
    FileUtil::LoadStringFromFile("/data/system/hiview/hiview_config_version", cloudVer);
    auto configPath = HiViewConfigUtil::GetConfigFilePath("test_file_name");
    if (localVer >= cloudVer) {
        ASSERT_EQ(configPath, "/system/etc/hiview/test_file_name");
    } else {
        ASSERT_EQ(configPath, "/data/system/hiview/test_file_name");
    }
    auto ret = HiViewConfigUtil::GetConfigFilePath("test_file_name", "data/test/", "test_file_name");
    ASSERT_EQ(ret, "/data/system/hiview/unzip_configs/data/test/test_file_name");
}

/**
 * @tc.name: CommonUtilsOhosTest003
 * @tc.desc: Test GetTransformedUid defined in namespace CommonUtils
 * @tc.type: FUNC
 * @tc.require: issueIC607P
 */
HWTEST_F(AdapterUtilityOhosTest, CommonUtilsOhosTest003, testing::ext::TestSize.Level3)
{
    auto ret = CommonUtils::GetTransformedUid(0); // 1 is a test uid value
    ASSERT_EQ(ret, 0); // 0 is expected transformed ret
    ret = CommonUtils::GetTransformedUid(1001); // 1001 is a test uid value
    ASSERT_EQ(ret, 0); // 0 is expected transformed ret
    ret = CommonUtils::GetTransformedUid(199999); // 19999 is a test uid value
    ASSERT_EQ(ret, 0); // 0 is expected transformed ret
    ret = CommonUtils::GetTransformedUid(200000); // 20000 is a test uid value
    ASSERT_EQ(ret, 1); // 1 is expected transformed ret
    ret = CommonUtils::GetTransformedUid(200001); // 20001 is a test uid value
    ASSERT_EQ(ret, 1); // 1 is expected transformed ret
    ret = CommonUtils::GetTransformedUid(1000000); // 100000 is a test uid value
    ASSERT_EQ(ret, 5); // 5 is expected transformed ret
    ret = CommonUtils::GetTransformedUid(1000001); // 20001 is a test uid value
    ASSERT_EQ(ret, 5); // 5 is expected transformed ret
}
} // namespace HiviewDFX
} // namespace OHOS