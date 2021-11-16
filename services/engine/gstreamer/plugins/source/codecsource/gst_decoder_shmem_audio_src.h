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

#ifndef __GST_DECODER_SHMEM_AUDIO_SRC_H__
#define __GST_DECODER_SHMEM_AUDIO_SRC_H__

#include "gst_shmem_pool_src.h"

G_BEGIN_DECLS

#define GST_TYPE_DECODER_SHMEM_AUDIO_SRC (gst_decoder_shmem_audio_src_get_type())
#define GST_DECODER_SHMEM_AUDIO_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DECODER_SHMEM_AUDIO_SRC, GstDecoderShmemAudioSrc))
#define GST_DECODER_SHMEM_AUDIO_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_DECODER_SHMEM_AUDIO_SRC, GstDecoderShmemAudioSrcClass))
#define GST_IS_DECODER_SHMEM_AUDIO_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_DECODER_SHMEM_AUDIO_SRC))
#define GST_IS_DECODER_SHMEM_AUDIO_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_DECODER_SHMEM_AUDIO_SRC))
#define GST_DECODER_SHMEM_AUDIO_SRC_CAST(obj) ((GstDecoderShmemAudioSrc*)(obj))

typedef struct _GstDecoderShmemAudioSrc GstDecoderShmemAudioSrc;
typedef struct _GstDecoderShmemAudioSrcClass GstDecoderShmemAudioSrcClass;

struct _GstDecoderShmemAudioSrc {
    GstShmemPoolSrc shmemPoolSrc;
    GstCaps *newCaps;
};

struct _GstDecoderShmemAudioSrcClass {
    GstShmemPoolSrcClass parent_class;
};

G_GNUC_INTERNAL GType gst_decoder_shmem_audio_src_get_type(void);

G_END_DECLS
#endif