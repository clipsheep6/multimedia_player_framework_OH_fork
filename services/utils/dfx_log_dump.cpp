/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "dfx_log_dump.h"
#include <fstream>
#include <unistd.h>
#include "securec.h"

namespace {
constexpr int32_t FILE_MAX = 100;
constexpr int32_t FILE_LINE_MAX = 1000000;
}
namespace OHOS {
namespace Media {
DfxLogDump &DfxLogDump::GetInstance()
{
    static DfxLogDump dfxLogDump;
    return dfxLogDump;
}

DfxLogDump::DfxLogDump()
{
    thread_ = std::make_unique<std::thread>(&DfxLogDump::TaskProcessor, this);
}

DfxLogDump::~DfxLogDump()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isExit_ = true;
        cond_.notify_all();
    }
    if (thread_ != nullptr && thread_->joinable()) {
        thread_->join();
    }
}

void DfxLogDump::DumpLog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    isDump_ = true;
    cond_.notify_all();
}

void DfxLogDump::SaveLog(const char *level, const OHOS::HiviewDFX::HiLogLabel &label, const char *fmt, ...)
{
    std::string temp = "";
    std::string fmtStr = fmt;
    int32_t srcPos = 0;
    auto dtsPos = fmtStr.find("{public}", srcPos);
    const int32_t pubLen = 8;
    while (dtsPos != std::string::npos) {
        temp += fmtStr.substr(srcPos, dtsPos - srcPos);
        srcPos = dtsPos + pubLen;
        dtsPos = fmtStr.find("{public}", srcPos);
    }
    temp += fmtStr.substr(srcPos);

    va_list ap;
    va_start(ap, fmt);
    constexpr uint8_t maxLogLen = 255;
    char logBuf[maxLogLen];
    (void)vsnprintf_s(logBuf, maxLogLen, maxLogLen - 1, temp.c_str(), ap);
    va_end(ap);

    std::unique_lock<std::mutex> lock(mutex_);
    logString_ += level;
    logString_ += " uid:";
    logString_ += std::to_string(getuid());
    logString_ += " ";
    logString_ += label.tag;
    logString_ += ":";
    logString_ += logBuf;
    logString_ += "\n";
    lineCount++;
    if (lineCount >= FILE_LINE_MAX) {
        cond_.notify_all();
    }
}

void DfxLogDump::TaskProcessor()
{
    std::string temp;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return isExit_ || isDump_ || lineCount >= FILE_LINE_MAX; });
        if (isExit_) {
            return;
        }
        isDump_ = false;
        lineCount = 0;
        swap(logString_, temp);
    }

    std::string file = "/data/media/hilog_media.log";
    file += std::to_string(fileCount++);
    std::ofstream ofStream(file);
    if (!ofStream.is_open()) {
        return;
    }
    ofStream.write(temp.c_str(), temp.size());
    ofStream.close();
    fileCount = fileCount > FILE_MAX ? 0 : fileCount;
}
} // namespace Media
} // namespace OHOS