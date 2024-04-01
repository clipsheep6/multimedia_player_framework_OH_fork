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

#include "engine_factory_repo.h"
#include <limits>
#include <cinttypes>
#include <dlfcn.h>
#include "directory_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_utils.h"

namespace {
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "EngineFactoryRepo"};
#if (defined(__aarch64__) || defined(__x86_64__))
    static const std::string MEDIA_ENGINE_LIB_PATH = "/system/lib64/media";
#else
    static const std::string MEDIA_ENGINE_LIB_PATH = "/system/lib/media";
#endif
    static const std::string MEDIA_ENGINE_LIB_NAME_HISTREAMER = "libmedia_engine_histreamer.z.so";
    static const std::string MEDIA_ENGINE_ENTRY_SYMBOL = "CreateEngineFactory";
}

namespace OHOS {
namespace Media {
using CreateFactoryFunc = IEngineFactory *(*)();

EngineFactoryRepo &EngineFactoryRepo::Instance()
{
    static EngineFactoryRepo inst;
    return inst;
}

EngineFactoryRepo::~EngineFactoryRepo()
{
    UnloadLib();
}

void __attribute__((no_sanitize("cfi"))) EngineFactoryRepo::UnloadLib()
{
    factorys_.clear();
    for (auto &lib : factoryLibs_) {
        if (lib != nullptr) {
            (void)dlclose(lib);
            lib = nullptr;
        }
    }
}

int32_t __attribute__((no_sanitize("cfi"))) EngineFactoryRepo::LoadLib(const std::string &libPath)
{
    void *handle = dlopen(libPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        MEDIA_LOGE("failed to dlopen %{public}s, errno:%{public}d, errormsg:%{public}s",
            libPath.c_str(), errno, dlerror());
        return MSERR_OPEN_FILE_FAILED;
    }

    CreateFactoryFunc entry = reinterpret_cast<CreateFactoryFunc>(dlsym(handle, MEDIA_ENGINE_ENTRY_SYMBOL.c_str()));
    if (entry == nullptr) {
        MEDIA_LOGE("failed to dlsym %{public}s for lib %{public}s, errno:%{public}d, errormsg:%{public}s",
            MEDIA_ENGINE_ENTRY_SYMBOL.c_str(), libPath.c_str(), errno, dlerror());
        (void)dlclose(handle);
        return MSERR_OPEN_FILE_FAILED;
    }

    std::shared_ptr<IEngineFactory> factory = std::shared_ptr<IEngineFactory>(entry());
    if (factory == nullptr) {
        MEDIA_LOGE("failed to create engine factory for lib: %{public}s", libPath.c_str());
        (void)dlclose(handle);
        return MSERR_OPEN_FILE_FAILED;
    }

    factoryLibs_.push_back(handle);
    factorys_.push_back(factory);
    return MSERR_OK;
}

int32_t EngineFactoryRepo::LoadHistreamerEngine(const int32_t& appUid)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLoadHistreamer_) {
        MEDIA_LOGD("Histreamer is enabled");
        return MSERR_OK;
    }

    std::string bundleName = GetClientBundleName(appUid);
    (void) bundleName;

    MEDIA_LOGI("LoadHistreamerEngine succeed!");
    std::vector<std::string> allFiles;
    GetDirFiles(MEDIA_ENGINE_LIB_PATH, allFiles);
    for (auto &file : allFiles) {
        std::string::size_type namePos = file.find(MEDIA_ENGINE_LIB_NAME_HISTREAMER);
        if (namePos == std::string::npos) {
            continue;
        } else {
            CHECK_AND_RETURN_RET_LOG(LoadLib(file) == MSERR_OK, MSERR_OPEN_FILE_FAILED, "LoadLib failed");
            isLoadHistreamer_ = true;
            break;
        }
    }

    return MSERR_OK;
}

std::shared_ptr<IEngineFactory> EngineFactoryRepo::GetEngineFactory(
    IEngineFactory::Scene scene, const int32_t& appUid, const std::string &uri)
{
    std::string bundleName = GetClientBundleName(appUid);
    (void)LoadHistreamerEngine(appUid);

    if (factorys_.empty()) {
        isLoadHistreamer_ = false;
        MEDIA_LOGE("Failed to load media engine library");
        return nullptr;
    }

    int32_t maxScore = std::numeric_limits<int32_t>::min();
    std::shared_ptr<IEngineFactory> target = nullptr;
    for (auto &factory : factorys_) {
        int32_t score = factory->Score(scene, appUid, uri);
        if (maxScore < score) {
            maxScore = score;
            target = factory;
        }
    }
    if (target == nullptr && !factorys_.empty()) {
        target = factorys_.front();
    }

    MEDIA_LOGD("Selected factory: 0x%{public}06" PRIXPTR ", score: %{public}d,"
        "appUid: %{public}d", FAKE_POINTER(target.get()), maxScore, appUid);
    return target;
}
} // namespace Media
} // namespace OHOS
