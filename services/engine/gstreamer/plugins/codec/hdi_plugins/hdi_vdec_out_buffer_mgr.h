/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef HDI_VDEC_OUT_BUFFER_MGR_H
#define HDI_VDEC_OUT_BUFFER_MGR_H

#include "hdi_out_buffer_mgr.h"

namespace OHOS {
namespace Media {
class HdiVdecOutBufferMgr : public HdiOutBufferMgr, public std::enable_shared_from_this<HdiVdecOutBufferMgr> {
public:
    HdiVdecOutBufferMgr();
    ~HdiVdecOutBufferMgr() override;
    int32_t UseBuffers(std::vector<GstBuffer*> buffers) override;
    void SetOutputPool(GstBufferPool *pool) override;
    int32_t FreeBuffers() override;
    uint32_t GetWaitDisPlayBufNum() override;

protected:
    void UpdateCodecMeta(GstBufferTypeMeta *bufferType, std::shared_ptr<HdiBufferWrap> &codecBuffer) override;

private:
    bool enableNativeBuffer_ = false;
    int32_t UseHandleMems(std::vector<GstBuffer *> &buffers);
    GstBufferPool *pool_ = nullptr;
    std::vector<GstBuffer *> preBuffers_;
    bool isCallBackMode_ = false;
    static GstFlowReturn NewBuffer(GstBuffer *buffer, gpointer userData);
    int32_t OnNewBuffer(GstBuffer *buffer);
    void ClearPreBuffers();
};
} // namespace Media
} // namespace OHOS
#endif // HDI_VDEC_OUT_BUFFER_MGR_H