/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef HIVIEW_BASE_EVENT_STORE_INCLUDE_BASE_DEF_H
#define HIVIEW_BASE_EVENT_STORE_INCLUDE_BASE_DEF_H

#include <string>

namespace OHOS {
namespace HiviewDFX {
namespace EventStore {
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define MAGIC_NUM (0x894556454E541a0a & ~1) // set the first low bit to 0
#elif __BYTE_ORDER == __BIG_ENDIAN
#define MAGIC_NUM (0x894556454E541a0a | 1)  // set the first low bit to 1
#else
#error "ERROR: No BIG_LITTLE_ENDIAN defines."
#endif

#define NUM_OF_BYTES_IN_KB 1024
#define NUM_OF_BYTES_IN_MB (1024 * 1024)
#define CRC_SIZE sizeof(uint32_t)
#define HIVIEW_BLOCK_SIZE sizeof(uint32_t)
#define SEQ_SIZE sizeof(int64_t)

#define MAX_DOMAIN_LEN 17
#define MAX_EVENT_NAME_LEN 33
#define MAX_TAG_LEN 17

#define MAX_NEW_SIZE (386 * 1024)

#define INVALID_VALUE_INT (-1)

#define DOC_STORE_SUCCESS 0
#define DOC_STORE_NEW_FILE 1
#define DOC_STORE_READ_EMPTY 2
#define DOC_STORE_ERROR_NULL (-1)
#define DOC_STORE_ERROR_IO (-2)
#define DOC_STORE_ERROR_MEMORY (-3)
#define DOC_STORE_ERROR_INVALID (-4)

#define MAX_VERSION_LENG 1000

enum EVENT_DATA_FORMATE_VERSION {
    INVALID = 0x0,
    VERSION1 = 0x1,
    VERSION2 = 0x2,    // add log label into event header
    VERSION3 = 0x3,    // remove crc and append system version into file header
    VERSION4 = 0x4,    // append patch version into file header
    CURRENT = VERSION4,
};

#pragma pack(1)
/* File header of the binary storage file */
struct DocHeader {
    /* Magic number */
    uint64_t magicNum = 0;

    /* Block size */
    uint32_t blockSize = 0;

    /* Page size */
    uint8_t pageSize = 0;

    /* Version number */
    uint8_t version = 0;

    /* Event tag */
    char tag[MAX_TAG_LEN] = {0};
};

struct ContentHeader {
    /* event seqno */
    uint64_t seq;

    /* Event timestamp */
    uint64_t timestamp;

    /* Time zone */
    uint8_t timeZone;

    /* User id */
    uint32_t uid;

    /* Process id */
    uint32_t pid;

    /* Thread id */
    uint32_t tid;

    /* Event hash code*/
    uint64_t id;

    /* Event type */
    uint8_t type : 2;

    /* Trace info flag*/
    uint8_t isTraceOpened : 1;

    /* Log packing flag */
    uint8_t log;
};
using DocHeader = struct DocHeader;

#pragma pack()

} // EventStore
} // HiviewDFX
} // OHOS
#endif // HIVIEW_BASE_EVENT_STORE_INCLUDE_BASE_DEF_H
