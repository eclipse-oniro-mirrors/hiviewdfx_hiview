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

#include "time_util.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>

namespace OHOS {
namespace HiviewDFX {
namespace TimeUtil {
namespace {
constexpr int8_t MILLISECONDS_LENGTH = 3;
}
time_t StrToTimeStamp(const std::string& tmStr, const std::string& format)
{
    std::string stTime = tmStr;
    struct tm tmFormat { 0 };
    strptime(stTime.c_str(), format.c_str(), &tmFormat);
    tmFormat.tm_isdst = -1;
    return mktime(&tmFormat);
}

uint64_t GenerateTimestamp()
{
    struct timeval now;
    if (gettimeofday(&now, nullptr) == -1) {
        return 0;
    }
    return (now.tv_sec * SEC_TO_MICROSEC + now.tv_usec);
}

void Sleep(unsigned int seconds)
{
    sleep(seconds);
}

int64_t GetSeconds()
{
    auto now = std::chrono::system_clock::now();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    return secs.count();
}

uint64_t GetMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

std::string TimestampFormatToDate(time_t timeStamp, const std::string& format)
{
    char date[MAX_TIME_BUFF] = {0};
    struct tm result {};
    if (localtime_r(&timeStamp, &result) != nullptr) {
        strftime(date, MAX_TIME_BUFF, format.c_str(), &result);
    }
    return std::string(date);
}

std::string GetTimeZone()
{
    struct timeval currentTime;
    if (gettimeofday(&currentTime, nullptr) != 0) {
        return "";
    }
    time_t systemSeconds = currentTime.tv_sec;
    struct tm tmLocal;
    if (localtime_r(&systemSeconds, &tmLocal) == nullptr) {
        return "";
    }
    int tzBufSize = 20;
    char tz[tzBufSize];
    auto ret = strftime(tz, tzBufSize, "%z", &tmLocal);
    if (ret > 0) {
        return std::string(tz);
    }
    return std::string("+0000");
}

int64_t Get0ClockStampMs()
{
    time_t now = std::time(nullptr);
    int64_t zero = now;
    struct tm *l = std::localtime(&now);
    if (l != nullptr) {
        l->tm_hour = 0;
        l->tm_min = 0;
        l->tm_sec = 0;
        zero = std::mktime(l) * SEC_TO_MILLISEC;  // time is 00:00:00
    }
    return zero > 0 ? zero : 0; // result of mktime is negative, when the date is 19700101
}

uint64_t GetSteadyClockTimeMs()
{
    auto now = std::chrono::steady_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

TimeCalculator::TimeCalculator(std::shared_ptr<uint64_t>& timePtr)
{
    this->time_ = timePtr;
    this->startTime_ = GenerateTimestamp();
    this->endTime_ = 0;
}

TimeCalculator::~TimeCalculator()
{
    this->endTime_ = GenerateTimestamp();
    if (this->time_ != nullptr && this->endTime_ > this->startTime_) {
        *(this->time_) += this->endTime_ - this->startTime_;
    }
}

std::string FormatTime(const int64_t timestamp, const std::string &format)
{
    std::time_t tt = static_cast<std::time_t>(timestamp / SEC_TO_MILLISEC);
    std::tm t = *std::localtime(&tt);
    char buffer[MAX_BUFFER_SIZE] = {0};
    std::strftime(buffer, sizeof(buffer), format.c_str(), &t);
    return std::string(buffer);
}

uint64_t GetNanoTime()
{
    auto nanoNow = std::chrono::steady_clock::now().time_since_epoch();
    return nanoNow.count();
}

uint64_t GetBootTimeMs()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
        return static_cast<uint64_t>(ts.tv_sec * SEC_TO_MILLISEC + ts.tv_nsec / MILLISEC_TO_NANOSEC);
    }
    return 0;
}

uint64_t GetMonotonicTimeMs()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return static_cast<uint64_t>(ts.tv_sec * SEC_TO_MILLISEC + ts.tv_nsec / MILLISEC_TO_NANOSEC);
    }
    return 0;
}

std::string GetFormattedTimestampEndWithMilli()
{
    auto milliSeconds = GetMilliseconds();
    auto seconds = milliSeconds / SEC_TO_MILLISEC;
    std::string formattedTimeStamp = TimestampFormatToDate(seconds, "%Y%m%d%H%M%S");
    std::stringstream ss;
    ss << formattedTimeStamp;
    milliSeconds = milliSeconds % SEC_TO_MILLISEC;
    ss << std::setfill('0') << std::setw(MILLISECONDS_LENGTH) << milliSeconds;
    return ss.str();
}
} // namespace TimeUtil
} // namespace HiviewDFX
} // namespace OHOS
