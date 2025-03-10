/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef UTILITY_TIME_UTIL_H
#define UTILITY_TIME_UTIL_H

#include <cstdint>
#include <memory>
#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace TimeUtil {
constexpr int64_t SEC_TO_NANOSEC = 1000000000;
constexpr int64_t SEC_TO_MICROSEC = 1000000;
constexpr int64_t SEC_TO_MILLISEC = 1000;
constexpr int64_t MILLISEC_TO_NANOSEC = 1000000;
constexpr int64_t MICROSEC_TO_NANOSEC = 1000;
constexpr int SECONDS_PER_MINUTE = 60; // 60 seconds
constexpr int SECONDS_PER_HOUR = 3600; // 60 * 60
constexpr int SECONDS_PER_DAY = 86400; // 60 * 60 * 24
constexpr uint64_t MILLISECS_PER_DAY = 60 * 60 * 24 * 1000;
constexpr int MAX_TIME_BUFF = 64; // 64 : for example 2021-05-27-01-01-01
constexpr int MAX_BUFFER_SIZE = 80;
class TimeCalculator {
public:
    TimeCalculator(std::shared_ptr<uint64_t>& timePtr);
    ~TimeCalculator();

private:
    std::shared_ptr<uint64_t> time_;
    uint64_t startTime_;
    uint64_t endTime_;
};

uint64_t GetNanoTime();
uint64_t GetTimeOfDay();
uint64_t GenerateTimestamp();
uint64_t GetMilliseconds();
time_t StrToTimeStamp(const std::string &tmStr, const std::string& format);
void Sleep(unsigned int seconds);
int64_t GetSeconds();
std::string TimestampFormatToDate(time_t timeStamp, const std::string& format);
std::string GetTimeZone();
int64_t Get0ClockStampMs();
uint64_t GetSteadyClockTimeMs();
uint64_t GetBootTimeMs();
uint64_t GetMonotonicTimeMs();
std::string FormatTime(const int64_t timestamp, const std::string &format);

// time format as YYYYmmddHHMMSSxxx
// eg. 202404222319765
std::string GetFormattedTimestampEndWithMilli();
} // namespace TimeUtil
} // namespace HiviewDFX
} // namespace OHOS
#endif // UTILITY_TIME_UTIL_H