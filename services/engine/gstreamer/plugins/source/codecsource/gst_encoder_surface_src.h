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

#ifndef __GST_ENCODER_SURFACE_SRC_H__
#define __GST_ENCODER_SURFACE_SRC_H__

#include "gst_surface_pool_src.h"

G_BEGIN_DECLS

#define GST_TYPE_ENCODER_SURFACE_SRC (gst_encoder_surface_src_get_type())
#define GST_ENCODER_SURFACE_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_ENCODER_SURFACE_SRC, GstEncoderSurfaceSrc))
#define GST_ENCODER_SURFACE_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_ENCODER_SURFACE_SRC, GstEncoderSurfaceSrcClass))
#define GST_IS_ENCODER_SURFACE_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_ENCODER_SURFACE_SRC))
#define GST_IS_ENCODER_SURFACE_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_ENCODER_SURFACE_SRC))
#define GST_ENCODER_SURFACE_SRC_CAST(obj) ((GstEncoderSurfaceSrc*)(obj))

typedef struct _GstEncoderSurfaceSrc GstEncoderSurfaceSrc;
typedef struct _GstEncoderSurfaceSrcClass GstEncoderSurfaceSrcClass;

struct _GstEncoderSurfaceSrc {
    GstSurfacePoolSrc basesrc;
    GstCaps *newCaps;
};

struct _GstEncoderSurfaceSrcClass {
    GstSurfacePoolSrcClass parent_class;
};

G_GNUC_INTERNAL GType gst_encoder_surface_src_get_type(void);

G_END_DECLS
#endif