/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef NDK_AV_MEMORY_H
#define NDK_AV_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct AVMemory AVMemory; 

enum AVMemoryFlags {
    /**
     * This flag bit indicates that the remote process is allowed to read and write
     * the shared memory. If no flags are specified, this is the default memory
     * sharing policy. If the FLAGS_READ_ONLY bit is set, this flag bit is ignored.
     */
    FLAGS_READ_WRITE = 0x1,
    /**
      * For platforms that support multiple processes, this flag bit indicates that the
      * remote process can only read data in the shared memory. If this flag is not set,
      * the remote process has both read and write permissions by default. Adding this
      * flag does not affect the process that creates the memory, which always has the
      * read and write permission on the shared memory. For platforms that do not support
      * multi-processes, the memory read and write permission control capability may
      * not be available. In this case, this flag is invalid.
      */
    FLAGS_READ_ONLY = 0x2,
};

/**
 * @brief Get the memory's virtual address
 * @param mem Encapsulate AVMemory structure instance pointer
 * @return the memory's virtual address if the memory is valid, otherwise nullptr.
 * @since 3.2
 * @version 3.2
 */
uint8_t* OH_AV_MemoryGetAddr(struct AVMemory *mem);

/**
 * @brief Get the memory's size
 * @param mem Encapsulate AVMemory structure instance pointer
 * @return the memory's size if the memory is valid, otherwise -1.
 * @since 3.2
 * @version 3.2
 */
int32_t OH_AV_MemoryGetSize(struct AVMemory *mem);

/**
 * @brief Get the memory's flags set by the creator, refer to {@AVMemoryFlags}
 * @param mem Encapsulate AVMemory structure instance pointer
 * @return the memory's flags if the memory is valid, otherwise 0.
 * @since 3.2
 * @version 3.2
 */
uint32_t OH_AV_MemoryGetFlags(struct AVMemory *mem);

#ifdef __cplusplus
}
#endif

#endif // NDK_AV_MEMORY_H