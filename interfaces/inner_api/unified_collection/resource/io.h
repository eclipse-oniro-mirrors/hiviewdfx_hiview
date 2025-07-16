/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNER_API_UNIFIED_COLLECTION_RESOURCE_IO_H
#define INTERFACES_INNER_API_UNIFIED_COLLECTION_RESOURCE_IO_H
#include <cinttypes>
#include <string>

namespace OHOS {
namespace HiviewDFX {
struct ProcessIo {
    int32_t pid;
    std::string name;
    uint64_t rchar = 0;
    uint64_t wchar = 0;
    uint64_t syscr = 0;
    uint64_t syscw = 0;
    uint64_t readBytes = 0;
    uint64_t writeBytes = 0;
    uint64_t cancelledWriteBytes = 0;
};

struct DiskData {
    uint64_t sectorRead = 0;  /* number of sectors read */
    uint64_t sectorWrite = 0; /* number of sectors written */
    uint64_t operRead = 0;    /* number of reads merged */
    uint64_t operWrite = 0;   /* number of writes merged */
    uint64_t readTime = 0;    /* number of time spent reading (ms) */
    uint64_t writeTime = 0;   /* number of time spent writing (ms) */
    uint64_t ioWait = 0;      /* number of I/Os currently in progress */
};

struct DiskStats {
    std::string deviceName;
    double sectorReadRate = 0.0;  /* number of sectors read persecond */
    double sectorWriteRate = 0.0; /* number of sectors written persecond */
    double operReadRate = 0.0;    /* number of reads merged persecond */
    double operWriteRate = 0.0;   /* number of writes merged persecond */
    double readTimeRate = 0.0;    /* number of time spent reading (ms) persecond */
    double writeTimeRate = 0.0;   /* number of time spent writing (ms) persecond */
    uint64_t ioWait = 0;        /* number of I/Os currently in progress */
};

struct DiskStatsDevice {
    DiskData preData;
    DiskStats stats;
    uint64_t collectTime = 0;
};

struct EMMCInfo {
    std::string name;
    std::string manfid;
    std::string csd;
    std::string type;
    int64_t size;
};

struct ProcessIoStats {
    int32_t pid;
    std::string name;
    int32_t ground;
    double rcharRate;
    double wcharRate;
    double syscrRate;
    double syscwRate;
    double readBytesRate;
    double writeBytesRate;
};

struct SysIoStats {
    double rcharRate = 0.0;
    double wcharRate = 0.0;
    double syscrRate = 0.0;
    double syscwRate = 0.0;
    double readBytesRate = 0.0;
    double writeBytesRate = 0.0;
};

struct ProcessIoStatsInfo {
    ProcessIo preData;
    ProcessIoStats stats;
    uint64_t collectTime = 0;
};
} // HiviewDFX
} // OHOS
#endif // INTERFACES_INNER_API_UNIFIED_COLLECTION_RESOURCE_IO_H