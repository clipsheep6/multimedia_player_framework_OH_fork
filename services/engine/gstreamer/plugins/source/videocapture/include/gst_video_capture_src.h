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

#ifndef GST_VIDEO_CAPTURE_SRC_H
#define GST_VIDEO_CAPTURE_SRC_H

#include <memory>
#include "gst_surface_src.h"
#include "common_utils.h"

G_BEGIN_DECLS

#define GST_TYPE_VIDEO_CAPTURE_SRC \
    (gst_video_capture_src_get_type())
#define GST_VIDEO_CAPTURE_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VIDEO_CAPTURE_SRC, GstVideoCaptureSrc))
#define GST_VIDEO_CAPTURE_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VIDEO_CAPTURE_SRC, GstVideoCaptureSrcClass))
#define GST_IS_VIDEO_CAPTURE_SRC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VIDEO_CAPTURE_SRC))
#define GST_IS_VIDEO_CAPTURE_SRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VIDEO_CAPTURE_SRC))
#define GST_VIDEO_CAPTURE_SRC_CAST(obj) ((GstVideoCaptureSrc *)obj)

struct _GstVideoCaptureSrc {
    GstSurfaceSrc element;

    /* private */
    VideoStreamType stream_type;
    GstCaps *src_caps;
    guint video_width;
    guint video_height;
    guint video_frame_rate;
    gboolean is_start;
    gboolean is_first_buffer;
};

struct _GstVideoCaptureSrcClass {
    GstSurfaceSrcClass parent_class;
};

using GstVideoCaptureSrc = struct _GstVideoCaptureSrc;
using GstVideoCaptureSrcClass = struct _GstVideoCaptureSrcClass;

GType gst_video_capture_src_get_type(void);

G_END_DECLS
#endif /* GST_VIDEO_CAPTURE_SRC_H */
