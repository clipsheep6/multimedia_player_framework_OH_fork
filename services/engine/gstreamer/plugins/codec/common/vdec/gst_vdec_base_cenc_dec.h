/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef GST_VDEC_BASE_CENC_DEC_H
#define GST_VDEC_BASE_CENC_DEC_H

#include "gst_vdec_base.h"

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

G_BEGIN_DECLS

GST_API_EXPORT GstFlowReturn gst_vdec_base_drm_cenc_decrypt(GstVideoDecoder *decoder, GstBuffer *buf);

G_END_DECLS

#endif /* GST_VDEC_BASE_CENC_DEC_H */
