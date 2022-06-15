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

#include "dfx_node_manager.h"
#include "media_log.h"
#include "string_ex.h"
#include "param_wrapper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "DfxNodeManager"};
enum DfxLevel : uint32_t {
    DFX_LEVEL0,
    DFX_LEVEL1,
    DFX_LEVEL2,
    DFX_LEVEL3,
};
}

namespace OHOS {
namespace Media {
void DfxNode::UpdateFuncTag(const std::string &funcTag, bool insert) {
    std::unique_lock<std::mutex> lock(funcMutex_);
    if (insert) {
        funcSet_.insert(funcTag);
        funcTags_.push_back(funcTag);
        if (funcTags_.size() > FUNC_TAG_SIZE) {
            funcTags_.pop_front();
        }
    } else {
        funcSet_.erase(funcTag);
    }
}

void DfxNode::UpdateClassTag(void *thiz, const std::string &classTag, bool insert) {
    std::unique_lock<std::mutex> lock(classMutex_);
    if (insert) {
        classMap_[thiz] = classTag;
    } else {
        classMap_.erase(thiz);
    }
}

std::shared_ptr<DfxNode> DfxNode::GetChildNode(const std::string &name) {
    std::unique_lock<std::mutex> lock(childMutex_);
    std::shared_ptr<DfxNode> node = std::make_shared<DfxNode>(name);
    node->parent = weak_from_this();
    childs.push_back(node);
    return node;
}

void DfxNode::DumpInfo(std::string &dumpString) {
    dumpString += (name_ + "::DumpInfo\n");
    decltype(valMap_) valTemp;
    {
        std::unique_lock<std::mutex> lock(valMutex_);
        valTemp = valMap_;
    }
    dumpString += "ValueList\n";
    for (auto iter = valTemp.begin(); iter != valTemp.end(); iter++) {
        dumpString += (iter->first + ":\n");
        for (auto str : iter->second) {
            dumpString += (str + " ");
        }
        dumpString += "\n";
    }
    decltype(funcTags_) funcTemp;
    decltype(funcSet_) funcSetTemp;
    {
        std::unique_lock<std::mutex> lock(funcMutex_);
        funcTemp = funcTags_;
        funcSetTemp = funcSet_;
    }
    dumpString += "Function History:\n";
    for (auto str : funcTemp) {
        dumpString += (str + "\n");
    }
    dumpString += "Function No Exit:\n";
    for (auto str : funcSetTemp) {
        dumpString += (str + "\n");
    }
    dumpString += "\n";
    decltype(classMap_) classTemp;
    {
        std::unique_lock<std::mutex> lock(classMutex_);
        classTemp = classMap_;
    }
    dumpString += "Class No Exit:\n";
    for (auto iter : classTemp) {
        dumpString += iter.second;
        dumpString += std::to_string(FAKE_POINTER(iter.first));
        dumpString += "\n";
    }
    dumpString += "\n";
    decltype(childs) childsTemp;
    {
        std::unique_lock<std::mutex> lock(classMutex_);
        childsTemp = childs;
    }
    for (auto child : childsTemp) {
        child->DumpInfo(dumpString);
    }
}

DfxNodeManager &DfxNodeManager::GetInstance()
{
    static DfxNodeManager dfxNodeManager;
    return dfxNodeManager;
}

DfxNodeManager::DfxNodeManager() : cleanNodeTask_("cleanNodeTask")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    cleanNodeTask_.Start();
}

DfxNodeManager::~DfxNodeManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    cleanNodeTask_.Stop();
}

std::shared_ptr<DfxNode> DfxNodeManager::CreateDfxNode(std::string name)
{
    GetDfxLevel();
    if (level_ == DFX_LEVEL0) {
        return nullptr;
    }
    return std::make_shared<DfxNode>(name);
}

std::shared_ptr<DfxNode> DfxNodeManager::CreateChildDfxNode(const std::shared_ptr<DfxNode> &parentNode, std::string name)
{
    if (parentNode == nullptr) {
        return nullptr;
    }
    return parentNode->GetChildNode(name);
}

void DfxNodeManager::CleanTask()
{
    std::unique_lock<std::mutex> lock(mutex_);
    static constexpr int32_t timeout = 300;
    cond_.wait_for(lock, std::chrono::seconds(timeout));
}

void DfxNodeManager::SaveErrorDfxNode(const std::shared_ptr<DfxNode> &node)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (node == nullptr) {
        return;
    }
    GetDfxLevel();
    if (level_ < DFX_LEVEL1) {
        return;
    }
    errorNodeVec_.push_back(node);
    auto task = std::make_shared<TaskHandler<void>>([this] {
        CleanTask();
    });
    (void)cleanNodeTask_.EnqueueTask(task);
}

void DfxNodeManager::SaveFinishDfxNode(const std::shared_ptr<DfxNode> &node)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (node == nullptr) {
        return;
    }
    GetDfxLevel();
    if (level_ < DFX_LEVEL2) {
        return;
    }
    finishNodeVec_.push_back(node);
    auto task = std::make_shared<TaskHandler<void>>([this] {
        CleanTask();
    });
    (void)cleanNodeTask_.EnqueueTask(task);
}

void DfxNodeManager::DumpDfxNode(const std::shared_ptr<DfxNode> &node, std::string &str)
{
    if (node == nullptr) {
        return;
    }
    node->DumpInfo(str);
}

void DfxNodeManager::DumpErrorDfxNode(std::string &str)
{
    decltype(errorNodeVec_) temp;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        temp = errorNodeVec_;
    }
    for (auto node : temp) {
        node->DumpInfo(str);
    }
}

void DfxNodeManager::DumpFinishDfxNode(std::string &str)
{
    decltype(finishNodeVec_) temp;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        temp = finishNodeVec_;
    }
    for (auto node : temp) {
        node->DumpInfo(str);
    }
}

void DfxNodeManager::GetDfxLevel()
{
    std::string levelStr;
    int32_t level;
    int32_t res = OHOS::system::GetStringParameter("sys.media.dfx.node.level", levelStr, "");
    if (res == 0 && !levelStr.empty()) {
        OHOS::StrToInt(levelStr, level);
        level_ = level;
    } else {
        level_ = DFX_LEVEL1;
    }
}
} // namespace Media
} // namespace OHOS