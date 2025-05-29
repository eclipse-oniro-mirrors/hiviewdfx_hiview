/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <map>
#include <sys/stat.h>

#ifdef __LP64__
constexpr size_t BASELINE_BBOX_DETECTOR_PIPELINE = 4;
constexpr size_t BASELINE_BDFR_PLUGIN_CONFIG = 4;
constexpr size_t BASELINE_COMPOSE_RULE = 4;
constexpr size_t BASELINE_CRASHVALIDATOR_PLUGIN_CONFIG = 4;
constexpr size_t BASELINE_EXTRACT_RULE = 12;
constexpr size_t BASELINE_FAULTLOGGER_PIPELINE = 4;

constexpr size_t BASELINE_LIBBDFR = 528;
constexpr size_t BASELINE_LIBCRASHVALIDATOR = 52;
constexpr size_t BASELINE_LIBFAULTLOGGER = 64;
constexpr size_t BASELINE_LIBFAULTLOGGER_NAPI = 52;
#else
constexpr size_t BASELINE_BBOX_DETECTOR_PIPELINE = 4;
constexpr size_t BASELINE_BDFR_PLUGIN_CONFIG = 4;
constexpr size_t BASELINE_COMPOSE_RULE = 4;
constexpr size_t BASELINE_CRASHVALIDATOR_PLUGIN_CONFIG = 4;
constexpr size_t BASELINE_EXTRACT_RULE = 12;
constexpr size_t BASELINE_FAULTLOGGER_PIPELINE = 4;

constexpr size_t BASELINE_LIBBDFR = 432;
constexpr size_t BASELINE_LIBCRASHVALIDATOR = 72;
constexpr size_t BASELINE_LIBFAULTLOGGER = 48;
constexpr size_t BASELINE_LIBFAULTLOGGER_NAPI = 40;
#endif

class FaultloggerRomBaseLine : public testing::Test {
public:
    void SetUp() {};
    void TearDown() {};

    const std::map<std::string, size_t> dfrBaseline_ = {
        {GetSystemLibPath("libbdfr.z.so"), BASELINE_LIBBDFR},
        {GetSystemLibPath("chipset-pub-sdk/libfaultlogger.z.so"), BASELINE_LIBFAULTLOGGER},
        {GetSystemLibPath("module/libfaultlogger_napi.z.so"), BASELINE_LIBFAULTLOGGER_NAPI},
        {GetSystemLibPath("libcrashvalidator.z.so"), BASELINE_LIBCRASHVALIDATOR},
        {"/system/etc/hiview/bdfr_plugin_config", BASELINE_BDFR_PLUGIN_CONFIG},
        {"/system/etc/hiview/compose_rule.json", BASELINE_COMPOSE_RULE},
        {"/system/etc/hiview/extract_rule.json", BASELINE_EXTRACT_RULE},
        {"/system/etc/hiview/crashvalidator_plugin_config", BASELINE_CRASHVALIDATOR_PLUGIN_CONFIG},
        {"/system/etc/hiview/dispatch_rule/BBoxDetectorPipeline", BASELINE_BBOX_DETECTOR_PIPELINE},
        {"/system/etc/hiview/dispatch_rule/faultloggerPipeline", BASELINE_FAULTLOGGER_PIPELINE},
    };

    static std::string GetSystemLibPath(std::string filePath)
    {
#ifdef __LP64__
        return "/system/lib64/" + filePath;
#else
        return "/system/lib/" + filePath;
#endif
    }

    double GetFilesExpansionRatio(const std::map<std::string, size_t>& fileList)
    {
        size_t baselineTotalSize = 0;
        size_t realTotalSize = 0;
        constexpr size_t byteConvertKb = 1024;
        constexpr size_t alignLen = 4;
        for (auto &[filePath, baselineSize] : fileList) {
            struct stat fileStat;
            int ret = stat(filePath.c_str(), &fileStat);
            if (ret != 0) {
                GTEST_LOG_(ERROR) << "Get " << filePath << "stat failed.";
                return -1.0f;
            }
            auto fileSizeAlign = (fileStat.st_size + byteConvertKb * alignLen - 1) / byteConvertKb;
            fileSizeAlign = fileSizeAlign / alignLen * alignLen;
            if (fileSizeAlign > baselineSize) {
                double fileSize = fileStat.st_size / static_cast<double>(byteConvertKb);
                constexpr size_t precision = 2;
                GTEST_LOG_(WARNING) << std::fixed << std::setprecision(precision)
                    << filePath << " exceeds the baseline. ("
                    << fileSize << "KB/" << fileSizeAlign << "KB/" << baselineSize << "KB)";
            }
            baselineTotalSize += baselineSize;
            realTotalSize += fileSizeAlign;
        }
        GTEST_LOG_(INFO)
            << "Current 4KB alignment size:" << realTotalSize << "KB/" << baselineTotalSize << "KB)";
        return static_cast<double>(realTotalSize) / static_cast<double>(baselineTotalSize);
    }
};

/**
 * @tc.name: FaultloggerBaseline001
 * @tc.desc: check ROM size
 * @tc.type: FUNC
 */
HWTEST_F(FaultloggerRomBaseLine, FaultloggerBaseline001, testing::ext::TestSize.Level3)
{
    // To ensure that the use case passes the tentative 1.15 and is later changed to 1.05
    constexpr double maxRatio = 1.15f;
    double ratioBaseline = GetFilesExpansionRatio(dfrBaseline_);
    ASSERT_LT(ratioBaseline, maxRatio);
}
