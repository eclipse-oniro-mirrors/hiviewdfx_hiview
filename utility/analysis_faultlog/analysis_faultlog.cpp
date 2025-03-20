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
#include <chrono>
#include <iostream>
#include <string>

#include "feature_analysis.h"
#include "file_util.h"
#include "hiview_logger.h"
#include "rule.h"
#include "smart_parser.h"
#include "tbox.h"

DEFINE_LOG_TAG("SmartParser");

static const char* const SMART_PARSER_PATH = "/system/etc/hiview/";

static void PrintEventInfo(const std::map<std::string, std::string>& eventInfo, const std::string& msg)
{
    HIVIEW_LOGI("%{public}s >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>", msg.c_str());
    HIVIEW_LOGI("eventInfo size : %{public}s", std::to_string(eventInfo.size()).c_str());
    for (auto &[key, val] : eventInfo) {
        std::istringstream iss(val);
        std::string line;

        while (std::getline(iss, line)) {
            HIVIEW_LOGI("[%{public}s] : %{public}s", key.c_str(), line.c_str());
        }
    }
}

static std::map<std::string, std::string> SmartParser(const std::string& eventPath, const std::string& eventType)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    auto eventInfos = OHOS::HiviewDFX::SmartParser::Analysis(eventPath, SMART_PARSER_PATH, eventType);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto diffTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "SmartParser::Analysis running time : " << diffTime << " ms" << std::endl;

    PrintEventInfo(eventInfos, "SmartParser::Analysis result:");

    return eventInfos;
}

static void Tbox(std::map<std::string, std::string>& eventInfo, std::string& eventType)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    OHOS::HiviewDFX::Tbox::FilterTrace(eventInfo, eventType);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto diffTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Tbox::FilterTrace running time : " << diffTime << " ms" << std::endl;

    PrintEventInfo(eventInfo, "Tbox::FilterTrace result:");
}

static bool SetCpuAffinity(uint16_t mask)
{
    cpu_set_t cpuset;
    pthread_t thread = pthread_self();

    CPU_ZERO(&cpuset);
    const int maxCores = 12;
    for (int j = 0; j < maxCores; j++) {
        if (mask & (1U << j)) {
            CPU_SET(j, &cpuset);
        }
    }

    int ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    return ret == 0;
}

int main(int argc, char *argv[])
{
    static uint16_t cpuMask = 0x0f;
    SetCpuAffinity(cpuMask);

    std::string eventType;
    std::string logPath;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-t") {
            if (i + 1 < argc) {
                eventType = argv[i + 1];
            }
        } else if (std::string(argv[i]) == "-f") {
            if (i + 1 < argc) {
                logPath = argv[i + 1];
            }
        }
    }

    if (eventType.empty() || logPath.empty()) {
        std::cout << "Usage:" << std::endl;
        std::cout << "\t" << argv[0] << " -t eventType -f filePath" << std::endl;
        std::cout << "\teventType\t" <<
            "The event name must match the event name configured in the configuration file." << std::endl;
        std::cout << "\tfilePath\t" <<
            "The parsed file path must match the path matching rule configured in the configuration file." << std::endl;
        return -1;
    }
    std::cout << ">>>>> eventType : " << eventType << std::endl;
    std::cout << ">>>>> logPath : " << logPath << std::endl;
    std::cout << std::endl;

    auto eventInfos = SmartParser(logPath, eventType);
    Tbox(eventInfos, eventType);

    return 0;
}
