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

#ifndef GST_CODEC_BIN_H
#define GST_CODEC_BIN_H

G_BEGIN_DECLS

GType gst_codec_bin_get_type(void);
#define GST_TYPE_CODEC_BIN \
    (gst_codec_bin_get_type())
#define GST_CODEC_BIN(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_CODEC_BIN, GstCodecBin))
#define GST_CODEC_BIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_CODEC_BIN, GstCodecBinClass))
#define GST_IS_CODEC_BIN(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_CODEC_BIN))
#define GST_IS_CODEC_BIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_CODEC_BIN))
#define GST_CODEC_BIN_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_CODEC_BIN, GstCodecBinClass))

enum GstCodecBinType {
    CODEC_BIN_TYPE_VIDEO_DECODER = 0,
    CODEC_BIN_TYPE_VIDEO_ENCODER,
    CODEC_BIN_TYPE_AUDIO_DECODER,
    CODEC_BIN_TYPE_AUDIO_ENCODER
    CODEC_BIN_TYPE_UNKNOWN,
}

typedef struct _GstCodecBin {
    GstBin parent;

    /* private */
    GstElement *src;
    GstElement *parser;
    GstElement *coder;
    GstElement *sink;

    GstCodecBinType type;
    gboolean use_software;
    gchar *plugin_name;
    gboolean stream_input;
} GstCodecBin;

typedef struct _GstCodecBinClass {
    GstBinClass parent_class;
} GstCodecBinClass;

G_END_DECLS

#endif // GST_CODEC_BIN_H
