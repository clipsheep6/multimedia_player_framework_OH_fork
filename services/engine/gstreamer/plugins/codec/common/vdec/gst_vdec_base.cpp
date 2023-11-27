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

#include "gst_vdec_base.h"
#include <vector>
#include <iostream>
#include <malloc.h>
#include <unistd.h>
#include <gst/gstprotection.h>
#include <gst/base/gstbytereader.h>
#include <sys/mman.h>
#include "ashmem.h"
#include "securec.h"
#include "buffer_type_meta.h"
#include "scope_guard.h"
#include "gst_codec_video_common.h"
#include "param_wrapper.h"
#include "media_dfx.h"
#include "media_log.h"

using namespace OHOS;
using namespace OHOS::Media;
GST_DEBUG_CATEGORY_STATIC(gst_vdec_base_debug_category);
#define GST_CAT_DEFAULT gst_vdec_base_debug_category
#define gst_vdec_base_parent_class parent_class
#define GST_VDEC_BASE_SUPPORTED_FORMATS "{ NV12, NV21 }"
#define DEFAULT_MAX_QUEUE_SIZE 30
#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080
#define DEFAULT_SEEK_FRAME_RATE 1000
#define BLOCKING_ACQUIRE_BUFFER_THRESHOLD 5
#define DRAIN_TIME_OUT_MIN (G_TIME_SPAN_SECOND * 2)
#define DRAIN_TIME_OUT_MAX (G_TIME_SPAN_SECOND * 5)
#define DRAIN_TIME_OUT_BY_FRAMERATE(rate) (static_cast<gint64>(G_TIME_SPAN_SECOND * 30 / (rate)))

#define DRM_VIDEO_FRAME_ARR_LEN            3
#define DRM_LEGACY_LEN                     3
#define DRM_IV_SIZE                        16
#define DRM_KEY_ID_SIZE                    16
#define DRM_AES_BLOCK_SIZE                 16
#define DRM_MAX_SUB_SAMPLE_NUM             64
#define DRM_TS_FLAG_CRYPT_BYTE_BLOCK       (2)
#define DRM_CRYPT_BYTE_BLOCK               (1 + 2) // 2: DRM_TS_FLAG_CRYPT_BYTE_BLOCK
#define DRM_SKIP_BYTE_BLOCK                (9)
#define DRM_H264_VIDEO_SKIP_BYTES          (32 + 3) // 32: bytes 3:3bytes
#define DRM_H264_VIDEO_NAL_TYPE_UMASK_NUM  (0x1f)
#define DRM_H264_VIDEO_START_NAL_TYPE      (1)
#define DRM_H264_VIDEO_END_NAL_TYPE        (5)
#define DRM_TS_SUB_SAMPLE_NUM              (2)
#define DRM_ASHMEM_INVALID_FD              (-1)

const char *g_drm_ashmem_inbuf_name = "drm_ashmem_in";
const char *g_drm_ashmem_outbuf_name = "drm_ashmem_out";

const guint8 g_video_frame_arr[DRM_VIDEO_FRAME_ARR_LEN] = { 0x00, 0x00, 0x01 };

typedef enum {
    DRM_ALG_CENC_UNENCRYPTED = 0x0,
    DRM_ALG_CENC_AES_CTR = 0x1,
    DRM_ALG_CENC_AES_WV = 0x2,
    DRM_ALG_CENC_AES_CBC = 0x3,
    DRM_ALG_CENC_SM4_CBC = 0x4,
    DRM_ALG_CENC_SM4_CTR,
} DRM_CencAlgorithm;

typedef enum {
    DRM_ARR_SUBSCRIPT_ZERO = 0,
    DRM_ARR_SUBSCRIPT_ONE,
    DRM_ARR_SUBSCRIPT_TWO,
    DRM_ARR_SUBSCRIPT_THREE,
} DRM_ArrSubscriptCollection;

typedef struct {
    guint clear_header_len;
    guint pay_load_len;
} DRM_SubSample;

static void gst_vdec_base_class_install_property(GObjectClass *gobject_class);
static void gst_vdec_base_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static gboolean gst_vdec_base_open(GstVideoDecoder *decoder);
static gboolean gst_vdec_base_close(GstVideoDecoder *decoder);
static gboolean gst_vdec_base_start(GstVideoDecoder *decoder);
static gboolean gst_vdec_base_stop(GstVideoDecoder *decoder);
static gboolean gst_vdec_base_set_format(GstVideoDecoder *decoder, GstVideoCodecState *state);
static gboolean gst_vdec_base_flush(GstVideoDecoder *decoder);
static GstFlowReturn gst_vdec_base_handle_frame(GstVideoDecoder *decoder, GstVideoCodecFrame *frame);
static GstFlowReturn gst_vdec_base_drain(GstVideoDecoder *decoder);
static void gst_vdec_base_finalize(GObject *object);
static GstFlowReturn gst_vdec_base_finish(GstVideoDecoder *decoder);
static void gst_vdec_base_loop(GstVdecBase *self);
static void gst_vdec_base_pause_loop(GstVdecBase *self);
static gboolean gst_vdec_base_event(GstVideoDecoder *decoder, GstEvent *event);
static gboolean gst_vdec_base_decide_allocation(GstVideoDecoder *decoder, GstQuery *query);
static gboolean gst_vdec_base_propose_allocation(GstVideoDecoder *decoder, GstQuery *query);
static gboolean gst_vdec_base_check_allocate_input(GstVdecBase *self);
static gboolean gst_codec_return_is_ok(const GstVdecBase *decoder, gint ret,
    const char *error_name, gboolean need_report);
static GstStateChangeReturn gst_vdec_base_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_vdec_base_update_out_port_def(GstVdecBase *self, guint *size);
static void gst_vdec_base_update_out_pool(GstVdecBase *self, GstBufferPool **pool, GstCaps *outcaps, gint size);
static void gst_vdec_base_set_flushing(GstVdecBase *self, const gboolean flushing);
static gboolean gst_vdec_base_allocate_in_buffers(GstVdecBase *self);
static gboolean gst_vdec_base_allocate_out_buffers(GstVdecBase *self);
static GstBufferPool *gst_vdec_base_new_in_shmem_pool(GstVdecBase *self, GstCaps *outcaps, gint size,
    guint min_buffer_cnt, guint max_buffer_cnt);
static void gst_vdec_base_post_resolution_changed_message(GstVdecBase *self, gboolean need_post);

enum {
    PROP_0,
    PROP_VENDOR,
    PROP_SURFACE_POOL,
    PROP_SINK_CAPS,
    PROP_PERFORMANCE_MODE,
    PROP_PLAYER_SCENE,
    PROP_SEEK,
    PROP_PLAYER_MODE,
    PROP_METADATA_MODE,
    PROP_FREE_CODEC_BUFFERS,
    PROP_RECOVER_CODEC_BUFFERS,
    PROP_STOP_FORMAT_CHANGED,
    PROP_SVP_MODE,
};

enum {
    SIGNAL_CAPS_FIX_ERROR,
    SIGNAL_ON_MEDIA_DECRYPT,
    SIGNAL_LAST
};

enum SvpMode : int32_t {
    SVP_CLEAR = -1, /* it's not a protection video */
    SVP_FALSE, /* it's a protection video but not need secure decoder */
    SVP_TRUE, /* it's a protection video and need secure decoder */
};

static guint signals[SIGNAL_LAST];

G_DEFINE_ABSTRACT_TYPE(GstVdecBase, gst_vdec_base, GST_TYPE_VIDEO_DECODER);

static void gst_vdec_base_drm_get_skip_clear_bytes(guint8 stream_type, guint *skip_bytes)
{
    (void)stream_type;
    *skip_bytes = (guint)DRM_H264_VIDEO_SKIP_BYTES;
    return;
}

static gint gst_vdec_base_drm_get_nal_type_and_index(guint8 *data, guint data_size, guint8 stream_type,
    guint8 *nal_type, guint *pos_index)
{
    guint i;
    *nal_type = 0;
    (void)stream_type;
    for (i = *pos_index; (i + (guint)DRM_LEGACY_LEN) < data_size; i++) {
        if ((data[i] != g_video_frame_arr[DRM_ARR_SUBSCRIPT_ZERO]) ||
            (data[i + DRM_ARR_SUBSCRIPT_ONE] != g_video_frame_arr[DRM_ARR_SUBSCRIPT_ONE]) ||
            (data[i + DRM_ARR_SUBSCRIPT_TWO] != g_video_frame_arr[DRM_ARR_SUBSCRIPT_TWO])) {
            continue;
        }
        *nal_type = data[i + DRM_ARR_SUBSCRIPT_THREE] & DRM_H264_VIDEO_NAL_TYPE_UMASK_NUM;
        if ((*nal_type == (guint8)DRM_H264_VIDEO_START_NAL_TYPE) ||
            (*nal_type == (guint8)DRM_H264_VIDEO_END_NAL_TYPE)) {
            *pos_index = i;
            return 0;
        }
        *nal_type = 0;
    }
    *pos_index = i;
    return -1;
}

static void gst_vdec_base_drm_get_sync_header_index(guint8 *data, guint data_size, guint *pos_index)
{
    guint i;
    for (i = *pos_index; (i + (guint)DRM_LEGACY_LEN) < data_size; i++) {
        if ((data[i] != g_video_frame_arr[DRM_ARR_SUBSCRIPT_ZERO]) ||
            (data[i + DRM_ARR_SUBSCRIPT_ONE] != g_video_frame_arr[DRM_ARR_SUBSCRIPT_ONE]) ||
            (data[i + DRM_ARR_SUBSCRIPT_TWO] != g_video_frame_arr[DRM_ARR_SUBSCRIPT_TWO])) {
            continue;
        }
        *pos_index = i;
        return;
    }
    *pos_index = data_size;
    return;
}

static guint8 gst_vdec_base_drm_get_final_nal_type_and_index(guint8 *data, guint data_size, guint8 stream_type,
    guint *pos_start_index, guint *pos_end_index)
{
    gint ret;
    guint skip_bytes = 0;
    guint8 tmp_nal_type = 0;
    guint tmp_pos_index = 0;
    guint8 nal_type = 0;
    *pos_start_index = 0;
    *pos_end_index = data_size;
    gst_vdec_base_drm_get_skip_clear_bytes(stream_type, &skip_bytes);
    while (1) { // 1 true
        ret = gst_vdec_base_drm_get_nal_type_and_index(data, data_size, stream_type, &tmp_nal_type, &tmp_pos_index);
        if (ret == 0) {
            nal_type = tmp_nal_type;
            *pos_start_index = tmp_pos_index;
            tmp_pos_index += (guint)DRM_LEGACY_LEN;
            gst_vdec_base_drm_get_sync_header_index(data, data_size, &tmp_pos_index);
            *pos_end_index = tmp_pos_index;
            if (tmp_pos_index > *pos_start_index + skip_bytes + (guint)DRM_AES_BLOCK_SIZE) {
                break;
            } else {
                nal_type = 0;
                *pos_start_index = data_size;
                *pos_end_index = data_size;
            }
        } else {
            nal_type = 0;
            *pos_start_index = data_size;
            *pos_end_index = data_size;
            break;
        }
    }
    return nal_type;
}

static void gst_vdec_base_drm_modify_cenc_info(guint8 stream_type, guint8 *data, guint data_size,
    DRM_SubSample *subsample_info)
{
    guint8 nal_type;
    guint pos_start_index;
    guint pos_end_index;
    guint skip_bytes = 0;
    guint del_len = 0;
    guint i;
    gst_vdec_base_drm_get_skip_clear_bytes(stream_type, &skip_bytes);
    nal_type = gst_vdec_base_drm_get_final_nal_type_and_index(data, data_size, stream_type, &pos_start_index,
        &pos_end_index);
    for (i = pos_end_index - 1; i > 0; i--) {
        if (data[i] != 0) {
            break;
        }
        del_len++;
    }
    subsample_info[0].clear_header_len = data_size;
    subsample_info[0].pay_load_len = 0;
    subsample_info[1].clear_header_len = 0;
    subsample_info[1].pay_load_len = 0;
    if ((nal_type == (guint8)DRM_H264_VIDEO_START_NAL_TYPE) || (nal_type == (guint8)DRM_H264_VIDEO_END_NAL_TYPE)) {
        guint clear_header_len = pos_start_index + skip_bytes;
        guint pay_load_len =
            (pos_end_index > (clear_header_len + del_len)) ? (pos_end_index - clear_header_len - del_len) : 0;
        if (pay_load_len > 0) {
            guint last_clear_len = (pay_load_len % (guint)DRM_AES_BLOCK_SIZE == 0) ? (guint)DRM_AES_BLOCK_SIZE
                                    : (pay_load_len % (guint)DRM_AES_BLOCK_SIZE);
            pay_load_len = pay_load_len - last_clear_len;
            subsample_info[0].clear_header_len = clear_header_len;
            subsample_info[0].pay_load_len = pay_load_len;
            subsample_info[1].clear_header_len = last_clear_len + del_len + (data_size - pos_end_index);
            subsample_info[1].pay_load_len = 0;
        }
    }
    return;
}

static void gst_drm_get_algo(const gchar *mode, guint *algo)
{
    if ((strcmp (mode, "sm4c") == 0) || (strcmp (mode, "sm4s") == 0)) {
        *algo = (guint)DRM_ALG_CENC_SM4_CBC;
    } else if ((strcmp (mode, "cbc1") == 0) || (strcmp (mode, "cbcs") == 0)) {
        *algo = (guint)DRM_ALG_CENC_AES_CBC;
    } else if ((strcmp (mode, "cenc") == 0) || (strcmp (mode, "cens") == 0)) {
        *algo = (guint)DRM_ALG_CENC_AES_CTR;
    } else if ((strcmp (mode, "sm4t") == 0) || (strcmp (mode, "sm4r") == 0)) {
        *algo = (guint)DRM_ALG_CENC_SM4_CTR;
    }
    return;
}

static void gst_vdec_base_class_init(GstVdecBaseClass *kclass)
{
    GST_DEBUG_OBJECT(kclass, "Class init");
    g_return_if_fail(kclass != nullptr);
    GObjectClass *gobject_class = G_OBJECT_CLASS(kclass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(kclass);
    GstVideoDecoderClass *video_decoder_class = GST_VIDEO_DECODER_CLASS(kclass);
    GST_DEBUG_CATEGORY_INIT (gst_vdec_base_debug_category, "vdecbase", 0, "video decoder base class");
    gobject_class->set_property = gst_vdec_base_set_property;
    gobject_class->finalize = gst_vdec_base_finalize;
    video_decoder_class->open = gst_vdec_base_open;
    video_decoder_class->close = gst_vdec_base_close;
    video_decoder_class->start = gst_vdec_base_start;
    video_decoder_class->stop = gst_vdec_base_stop;
    video_decoder_class->flush = gst_vdec_base_flush;
    video_decoder_class->set_format = gst_vdec_base_set_format;
    video_decoder_class->handle_frame = gst_vdec_base_handle_frame;
    video_decoder_class->finish = gst_vdec_base_finish;
    video_decoder_class->sink_event = gst_vdec_base_event;
    video_decoder_class->drain = gst_vdec_base_drain;
    video_decoder_class->decide_allocation = gst_vdec_base_decide_allocation;
    video_decoder_class->propose_allocation = gst_vdec_base_propose_allocation;
    element_class->change_state = gst_vdec_base_change_state;

    gst_vdec_base_class_install_property(gobject_class);

    signals[SIGNAL_CAPS_FIX_ERROR] =
        g_signal_new ("caps-fix-error", G_TYPE_FROM_CLASS (kclass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0, G_TYPE_NONE);

    signals[SIGNAL_ON_MEDIA_DECRYPT] =
        g_signal_new ("media-decrypt", G_TYPE_FROM_CLASS (kclass),
        G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL,
        G_TYPE_INT, 13, G_TYPE_INT64, G_TYPE_INT64, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_UINT, // 13:parameter nums
        G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);

    const gchar *src_caps_string = GST_VIDEO_CAPS_MAKE(GST_VDEC_BASE_SUPPORTED_FORMATS);
    GST_DEBUG_OBJECT(kclass, "Pad template caps %s", src_caps_string);

    GstCaps *src_caps = gst_caps_from_string(src_caps_string);
    if (src_caps != nullptr) {
        GstPadTemplate *src_templ = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
        gst_element_class_add_pad_template(element_class, src_templ);
        gst_caps_unref(src_caps);
    }
}

static void gst_vdec_base_class_install_property(GObjectClass *gobject_class)
{
    g_object_class_install_property(gobject_class, PROP_VENDOR,
        g_param_spec_pointer("vendor", "Vendor property", "Vendor property",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SURFACE_POOL,
        g_param_spec_pointer("surface-pool", "Surface pool", "Surface pool",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SINK_CAPS,
        g_param_spec_boxed("sink-caps", "Sink Caps",
            "The caps use with surface pool", GST_TYPE_CAPS,
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_PERFORMANCE_MODE,
        g_param_spec_boolean("performance-mode", "performance mode", "performance mode",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_PLAYER_SCENE,
        g_param_spec_boolean("player-scene", "player scene", "player scene",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_PLAYER_MODE,
        g_param_spec_boolean("player-mode", "player mode", "player mode",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SEEK,
        g_param_spec_boolean("seeking", "Seeking", "Whether the decoder is in seek",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_METADATA_MODE,
        g_param_spec_boolean("metadata-mode", "Metadata-mode", "Metadata Mode",
        FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_FREE_CODEC_BUFFERS,
        g_param_spec_boolean("free-codec-buffers", "Free-codec-buffers", "Free Codec Buffers",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_RECOVER_CODEC_BUFFERS,
        g_param_spec_boolean("recover-codec-buffers", "Recover-codec-buffers", "Recover Codec Buffers",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_STOP_FORMAT_CHANGED,
        g_param_spec_boolean("stop-format-change", "stop-format-change", "stop-format-change",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SVP_MODE,
        g_param_spec_int("svp-mode", "svp-mode", "Svp Mode decides whether need a secure decoder or not",
        -1, G_MAXINT32, SVP_CLEAR, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
}

static gboolean gst_vdec_base_free_codec_buffers(GstVdecBase *self)
{
    g_return_val_if_fail(self != nullptr && self->decoder != nullptr, FALSE);
    gint ret;
    GST_DEBUG_OBJECT(self, "enter free buffers, is_free_codec_buffers:%d is_eos_state:%d",
        self->is_free_codec_buffers, self->is_eos_state);
    if (!self->is_free_codec_buffers) {
        if (!self->is_eos_state) {
            GST_VIDEO_DECODER_STREAM_LOCK(self);
            (void)self->decoder->Flush(GST_CODEC_ALL);
            gst_vdec_base_set_flushing(self, TRUE);
            GST_VIDEO_DECODER_STREAM_UNLOCK(self);
            g_return_val_if_fail(gst_pad_push_event(GST_VIDEO_DECODER_SRC_PAD(self),
                gst_event_new_flush_start()), FALSE);
            g_return_val_if_fail(gst_pad_push_event(GST_VIDEO_DECODER_SRC_PAD(self),
                gst_event_new_flush_stop(FALSE)), FALSE);
            self->is_eos_state = FALSE;
        }
        if (self->decoder_start) {
            ret = self->decoder->Stop();
            g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "DecoderStop", TRUE), FALSE);
            self->decoder_start = FALSE;
        }
        GstPad *pad = GST_VIDEO_DECODER_SRC_PAD(self);
        if (gst_pad_get_task_state(pad) != GST_TASK_STOPPED) {
            g_return_val_if_fail(gst_pad_stop_task(pad), FALSE);
        }
        if (self->outpool) {
            g_return_val_if_fail(gst_buffer_pool_set_active(self->outpool, FALSE), FALSE);
        }
        if (self->input.allocator) {
            gst_object_unref(self->input.allocator);
            self->input.allocator = nullptr;
        }
        if (self->inpool) {
            g_return_val_if_fail(gst_buffer_pool_set_active(self->inpool, FALSE), FALSE);
            gst_object_unref(self->inpool);
            self->inpool = nullptr;
            self->input.av_shmem_pool = nullptr;
        }
        ret = self->decoder->FreeInputBuffers();
        g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "FreeInput", TRUE), FALSE);
        ret = self->decoder->FreeOutputBuffers();
        g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "FreeOutput", TRUE), FALSE);
        self->is_free_codec_buffers = TRUE;
    }
    return TRUE;
}

static gboolean gst_vdec_base_recover_codec_buffers(GstVdecBase *self)
{
    g_return_val_if_fail(self != nullptr, FALSE);
    if (self->is_free_codec_buffers) {
        mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
        mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
        self->input.allocator = gst_shmem_allocator_new();
        self->inpool = gst_vdec_base_new_in_shmem_pool(self, self->input_state->caps, self->input.buffer_size,
            self->input.buffer_cnt, self->input.buffer_cnt);
        g_return_val_if_fail(self->inpool != nullptr, FALSE);
        g_return_val_if_fail(gst_buffer_pool_set_active(self->inpool, TRUE), FALSE);

        if (self->outpool) {
            self->decoder->SetOutputPool(self->outpool);
            g_return_val_if_fail(gst_buffer_pool_set_active(self->outpool, TRUE), FALSE);
        }
        g_return_val_if_fail(gst_vdec_base_allocate_in_buffers(self), FALSE);
        g_return_val_if_fail(gst_vdec_base_allocate_out_buffers(self), FALSE);
        self->is_free_codec_buffers = FALSE;
    }
    return TRUE;
}

static inline void gst_vdec_reset_sink_caps(GstVdecBase *self)
{
    if (self->sink_caps != nullptr) {
        gst_caps_unref(self->sink_caps);
        self->sink_caps = nullptr;
    }
}

static inline void gst_vdec_reset_inpool(GstVdecBase *self)
{
    if (self->inpool) {
        gst_object_unref(self->inpool);
        self->inpool = nullptr;
    }
}

static inline void gst_vdec_reset_outpool(GstVdecBase *self)
{
    if (self->outpool) {
        gst_buffer_pool_set_active(self->outpool, FALSE);
        gst_object_unref(self->outpool);
        self->outpool = nullptr;
    }
}

static void gst_vdec_base_prob_seek(GstVdecBase *self, const GValue *value)
{
    GST_OBJECT_LOCK(self);
    if (self->decoder != nullptr) {
        self->seek_frame_rate = g_value_get_boolean(value) ? DEFAULT_SEEK_FRAME_RATE : self->frame_rate;
        self->decoder->SetParameter(GST_DYNAMIC_FRAME_RATE, GST_ELEMENT(self));
    }
    GST_OBJECT_UNLOCK(self);
}

static void gst_vdec_base_prob_surface_pool(GstVdecBase *self, const GValue *value)
{
    gst_vdec_reset_outpool(self);
    self->outpool = static_cast<GstBufferPool *>(g_value_get_pointer(value));
    gst_object_ref(self->outpool);
}

static void gst_vdec_base_prob_sink_caps(GstVdecBase *self, const GValue *value)
{
    const GstCaps *caps = gst_value_get_caps(value);
    gst_vdec_reset_sink_caps(self);
    self->sink_caps = gst_caps_copy(caps);
}

static void gst_vdec_base_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    (void)pspec;
    g_return_if_fail(object != nullptr && value != nullptr);
    GstVdecBase *self = GST_VDEC_BASE(object);
    GST_INFO_OBJECT(object, "Prop id %u", prop_id);

    switch (prop_id) {
        case PROP_SURFACE_POOL:
            gst_vdec_base_prob_surface_pool(self, value);
            break;
        case PROP_SINK_CAPS:
            gst_vdec_base_prob_sink_caps(self, value);
            break;
        case PROP_PERFORMANCE_MODE:
            self->performance_mode = g_value_get_boolean(value);
            break;
        case PROP_PLAYER_SCENE:
            self->player_scene = g_value_get_boolean(value);
            break;
        case PROP_SEEK:
            gst_vdec_base_prob_seek(self, value);
            break;
        case PROP_PLAYER_MODE:
            self->player_mode = g_value_get_boolean(value);
            break;
        case PROP_METADATA_MODE:
            self->metadata_mode = g_value_get_boolean(value);
            break;
        case PROP_FREE_CODEC_BUFFERS:
            (void)gst_vdec_base_free_codec_buffers(self);
            break;
        case PROP_RECOVER_CODEC_BUFFERS:
            (void)gst_vdec_base_recover_codec_buffers(self);
            break;
        case PROP_STOP_FORMAT_CHANGED:
            g_mutex_lock(&self->format_changed_lock);
            self->unsupport_format_changed = g_value_get_boolean(value);
            g_mutex_unlock(&self->format_changed_lock);
            break;
        case PROP_SVP_MODE:
            self->svp_mode = g_value_get_int(value);
            break;
        default:
            break;
    }
}

static void gst_vdec_base_check_input_need_copy(GstVdecBase *self)
{
    GstVdecBaseClass *base_class = GST_VDEC_BASE_GET_CLASS(self);
    g_return_if_fail(base_class != nullptr);
    if (base_class->input_need_copy != nullptr) {
        self->input_need_ashmem = base_class->input_need_copy();
    }
}

static void gst_vdec_base_check_support_swap_width_height(GstVdecBase *self)
{
    GstVdecBaseClass *base_class = GST_VDEC_BASE_GET_CLASS(self);
    g_return_if_fail(base_class != nullptr);
    GstElementClass *element_class = GST_ELEMENT_CLASS(base_class);
    if (base_class->support_swap_width_height != nullptr) {
        self->is_support_swap_width_height = base_class->support_swap_width_height(element_class);
    }
}

static void gst_vdec_base_property_init(GstVdecBase *self)
{
    g_mutex_init(&self->lock);

    g_mutex_init(&self->drain_lock);
    g_cond_init(&self->drain_cond);
    self->draining = FALSE;
    self->flushing = FALSE;
    self->prepared = FALSE;
    self->idrframe = FALSE;
    self->width = DEFAULT_WIDTH;
    self->height = DEFAULT_HEIGHT;
    self->frame_rate = 0;
    self->seek_frame_rate = 0;
    self->memtype = GST_MEMTYPE_INVALID;
    (void)memset_s(&self->input, sizeof(GstVdecBasePort), 0, sizeof(GstVdecBasePort));
    (void)memset_s(&self->output, sizeof(GstVdecBasePort), 0, sizeof(GstVdecBasePort));
    self->coding_outbuf_cnt = 0;
    self->input_state = nullptr;
    self->output_state = nullptr;
    self->last_pts = GST_CLOCK_TIME_NONE;
    self->flushing_stoping = FALSE;
    self->decoder_start = FALSE;
    self->stride = 0;
    self->stride_height = 0;
    self->real_stride = 0;
    self->real_stride_height = 0;
    (void)memset_s(&self->rect, sizeof(DisplayRect), 0, sizeof(DisplayRect));
    self->inpool = nullptr;
    self->outpool = nullptr;
    self->out_buffer_max_cnt = DEFAULT_MAX_QUEUE_SIZE;
    self->pre_init_pool = FALSE;
    self->performance_mode = FALSE;
    self->player_scene = FALSE;
    self->resolution_changed = FALSE;
    self->input_need_ashmem = FALSE;
    self->has_set_format = FALSE;
    self->player_mode = FALSE;
    self->metadata_mode = FALSE;
    self->is_eos_state = FALSE;
    self->is_support_swap_width_height = FALSE;
    g_mutex_init(&self->format_changed_lock);
    self->unsupport_format_changed = FALSE;
    self->svp_mode = SVP_CLEAR;
}

static void gst_vdec_base_init(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Init, id = %d", static_cast<int32_t>(FAKE_POINTER(self)));
    g_return_if_fail(self != nullptr && GST_VIDEO_DECODER_SINK_PAD(self) != nullptr);
    // The upstreamer must after parser.
    gst_video_decoder_set_packetized(GST_VIDEO_DECODER(self), TRUE);
    // Use accept caps from default.
    gst_video_decoder_set_use_default_pad_acceptcaps(GST_VIDEO_DECODER_CAST (self), TRUE);
    GST_PAD_SET_ACCEPT_TEMPLATE(GST_VIDEO_DECODER_SINK_PAD(self));

    gst_vdec_base_property_init(self);
    self->input.allocator = gst_shmem_allocator_new();
    self->output.allocator = gst_shmem_allocator_new();
    gst_allocation_params_init(&self->input.allocParams);
    gst_allocation_params_init(&self->output.allocParams);
    self->input.frame_cnt = 0;
    self->input.first_frame_time = 0;
    self->input.last_frame_time = 0;
    self->input.enable_dump = FALSE;
    self->input.dump_file = nullptr;
    self->output.frame_cnt = 0;
    self->output.first_frame_time = 0;
    self->output.last_frame_time = 0;
    self->output.enable_dump = FALSE;
    self->output.dump_file = nullptr;
    self->input.first_frame = TRUE;
    self->output.first_frame = TRUE;
    self->codec_data_update = FALSE;
    self->codec_data = nullptr;
    self->crypto_info = nullptr;
    self->drm_ashmem_infd = (gint)DRM_ASHMEM_INVALID_FD;
    self->drm_ashmem_outfd = (gint)DRM_ASHMEM_INVALID_FD;
}

static void gst_vdec_base_finalize(GObject *object)
{
    GST_DEBUG_OBJECT(object, "Finalize");
    g_return_if_fail(object != nullptr);
    GstVdecBase *self = GST_VDEC_BASE(object);
    g_mutex_clear(&self->drain_lock);
    g_cond_clear(&self->drain_cond);
    g_mutex_clear(&self->lock);
    if (self->input.allocator) {
        gst_object_unref(self->input.allocator);
        self->input.allocator = nullptr;
    }
    if (self->output.allocator) {
        gst_object_unref(self->output.allocator);
        self->output.allocator = nullptr;
    }
    gst_vdec_reset_inpool(self);
    gst_vdec_reset_outpool(self);
    gst_vdec_reset_sink_caps(self);

    if (self->drm_ashmem_infd > 0) {
        (void)close(self->drm_ashmem_infd);
        self->drm_ashmem_infd = (gint)DRM_ASHMEM_INVALID_FD;
    }
    if (self->drm_ashmem_outfd > 0) {
        (void)close(self->drm_ashmem_outfd);
        self->drm_ashmem_outfd = (gint)DRM_ASHMEM_INVALID_FD;
    }

    g_mutex_clear(&self->format_changed_lock);
    std::list<GstClockTime> tempList;
    tempList.swap(self->pts_list);
    std::vector<GstVideoFormat> tempVec;
    tempVec.swap(self->formats);

    self->input.av_shmem_pool = nullptr;
    self->output.av_shmem_pool = nullptr;
    self->decoder = nullptr;
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static gboolean gst_vdec_base_open(GstVideoDecoder *decoder)
{
    GST_DEBUG_OBJECT(decoder, "Open");
    g_return_val_if_fail(decoder != nullptr, FALSE);
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    GstVdecBaseClass *base_class = GST_VDEC_BASE_GET_CLASS(self);
    g_return_val_if_fail(base_class != nullptr && base_class->create_codec != nullptr, FALSE);
    self->decoder = base_class->create_codec(reinterpret_cast<GstElementClass*>(base_class));
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    gst_vdec_base_check_input_need_copy(self);
    gst_vdec_base_check_support_swap_width_height(self);
    return TRUE;
}

static gboolean gst_vdec_base_is_flushing(GstVdecBase *self)
{
    g_return_val_if_fail(self != nullptr, FALSE);
    GST_OBJECT_LOCK(self);
    gboolean flushing = self->flushing;
    GST_OBJECT_UNLOCK(self);
    GST_DEBUG_OBJECT(self, "Flushing %d", flushing);
    return flushing;
}

static void gst_vdec_base_set_flushing(GstVdecBase *self, const gboolean flushing)
{
    GST_DEBUG_OBJECT(self, "Set flushing %d", flushing);
    g_return_if_fail(self != nullptr);
    GST_OBJECT_LOCK(self);
    self->flushing = flushing;
    GST_OBJECT_UNLOCK(self);
}

static GstStateChangeReturn gst_vdec_base_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstVdecBase *self = GST_VDEC_BASE(element);

    GST_DEBUG_OBJECT(element, "change state %d", transition);
    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: stop start");
            gst_buffer_pool_set_active(self->outpool, FALSE);
            GST_VIDEO_DECODER_STREAM_LOCK(self);
            if (self->decoder != nullptr) {
                (void)self->decoder->Flush(GST_CODEC_ALL);
            }
            gst_vdec_base_set_flushing(self, TRUE);

            GST_VIDEO_DECODER_STREAM_UNLOCK(self);
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: close start");
            break;
        default:
            break;
    }

    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: stop end");
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: close end");
            break;
        default:
            break;
    }
    return ret;
}

static gboolean gst_vdec_base_close(GstVideoDecoder *decoder)
{
    GST_DEBUG_OBJECT(decoder, "Close");
    g_return_val_if_fail(decoder != nullptr, FALSE);
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    self->decoder->Deinit();
    self->decoder = nullptr;
    return TRUE;
}

void gst_vdec_base_dump_from_sys_param(GstVdecBase *self)
{
    std::string dump_vdec;
    self->input.enable_dump = FALSE;
    self->output.enable_dump = FALSE;
    int32_t res = OHOS::system::GetStringParameter("sys.media.dump.codec.vdec", dump_vdec, "");
    if (res != 0 || dump_vdec.empty()) {
        GST_DEBUG_OBJECT(self, "sys.media.dump.codec.vdec");
        return;
    }
    GST_DEBUG_OBJECT(self, "sys.media.dump.codec.vdec=%s", dump_vdec.c_str());

    if (dump_vdec == "INPUT" || dump_vdec == "ALL") {
        self->input.enable_dump = TRUE;
    }
    if (dump_vdec == "OUTPUT" || dump_vdec == "ALL") {
        self->output.enable_dump = TRUE;
    }
}

static gboolean gst_vdec_base_start(GstVideoDecoder *decoder)
{
    GST_DEBUG_OBJECT(decoder, "Start");
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    g_return_val_if_fail(self != nullptr, FALSE);
    self->input.frame_cnt = 0;
    self->input.first_frame_time = 0;
    self->input.last_frame_time = 0;
    self->output.frame_cnt = 0;
    self->output.first_frame_time = 0;
    self->output.last_frame_time = 0;
    std::list<GstClockTime> empty;
    self->pts_list.swap(empty);
    self->last_pts = GST_CLOCK_TIME_NONE;
    gst_vdec_base_dump_from_sys_param(self);
    return TRUE;
}

static void gst_vdec_base_stop_after(GstVdecBase *self)
{
    g_return_if_fail(self != nullptr);
    self->prepared = FALSE;
    self->idrframe = FALSE;
    self->input.first_frame = TRUE;
    self->output.first_frame = TRUE;
    self->decoder_start = FALSE;
    self->pre_init_pool = FALSE;
    self->performance_mode = FALSE;
    self->resolution_changed = FALSE;
    self->has_set_format = FALSE;
    gst_vdec_base_set_flushing(self, FALSE);
    if (self->input.dump_file != nullptr) {
        fclose(self->input.dump_file);
        self->input.dump_file = nullptr;
    }
    if (self->output.dump_file != nullptr) {
        fclose(self->output.dump_file);
        self->output.dump_file = nullptr;
    }
    gst_vdec_reset_sink_caps(self);
    self->codec_data_update = FALSE;
    if (self->codec_data) {
        gst_buffer_unref(self->codec_data);
        self->codec_data = nullptr;
    }
}

static gboolean gst_vdec_base_stop(GstVideoDecoder *decoder)
{
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    GST_DEBUG_OBJECT(self, "Stop decoder start");

    g_mutex_lock(&self->drain_lock);
    self->draining = FALSE;
    g_cond_broadcast(&self->drain_cond);
    g_mutex_unlock(&self->drain_lock);

    gint ret = self->decoder->Stop();
    (void)gst_codec_return_is_ok(self, ret, "Stop", TRUE);
    if (self->input_state) {
        gst_video_codec_state_unref(self->input_state);
    }
    if (self->output_state) {
        gst_video_codec_state_unref(self->output_state);
    }
    self->input_state = nullptr;
    self->output_state = nullptr;

    gst_pad_stop_task(GST_VIDEO_DECODER_SRC_PAD(decoder));
    ret = self->decoder->FreeInputBuffers();
    (void)gst_codec_return_is_ok(self, ret, "FreeInput", TRUE);
    ret = self->decoder->FreeOutputBuffers();
    (void)gst_codec_return_is_ok(self, ret, "FreeOutput", TRUE);
    gst_vdec_reset_inpool(self);
    gst_vdec_reset_outpool(self);
    gst_vdec_base_stop_after(self);
    GST_DEBUG_OBJECT(self, "Stop decoder end");
    return TRUE;
}

static gboolean gst_codec_return_is_ok(const GstVdecBase *decoder, gint ret,
    const char *error_name, gboolean need_report)
{
    if (ret == GST_CODEC_OK) {
        return TRUE;
    }
    if (need_report) {
        GST_ELEMENT_ERROR(decoder, STREAM, DECODE, ("hardware decoder error!"), ("%s", error_name));
    } else {
        GST_ERROR_OBJECT(decoder, "hardware decoder error %s", error_name);
    }
    return FALSE;
}

static gboolean gst_vdec_base_flush(GstVideoDecoder *decoder)
{
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    g_return_val_if_fail(self != nullptr, FALSE);

    g_mutex_lock(&self->format_changed_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&self->format_changed_lock); };

    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    GST_DEBUG_OBJECT(self, "Flush start");

    if (!self->flushing_stoping) {
        gst_vdec_base_set_flushing(self, TRUE);
        gint ret = self->decoder->Flush(GST_CODEC_ALL);
        (void)gst_codec_return_is_ok(self, ret, "flush", FALSE);
        gst_vdec_base_set_flushing(self, FALSE);
    }

    GST_DEBUG_OBJECT(self, "Flush end");
    return TRUE;
}

static gboolean update_caps_format(GstVdecBase *self, GstCaps *caps)
{
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    gint ret = self->decoder->GetParameter(GST_VIDEO_FORMAT, GST_ELEMENT(self));
    (void)gst_codec_return_is_ok(self, ret, "flush", FALSE);
    GST_DEBUG_OBJECT(self, "update_caps_format");

    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "flush", FALSE), FALSE);
    GValue value_list = G_VALUE_INIT;
    GValue value = G_VALUE_INIT;

    if (!self->formats.empty()) {
        g_value_init(&value_list, GST_TYPE_LIST);
        g_value_init(&value, G_TYPE_STRING);
        for (auto format : self->formats) {
            if (format != GST_VIDEO_FORMAT_UNKNOWN) {
                g_value_set_string(&value, gst_video_format_to_string(format));
                gst_value_list_append_value(&value_list, &value);
            }
        }
        gst_caps_set_value(caps, "format", &value_list);
        g_value_unset(&value);
        g_value_unset(&value_list);
    }

    return TRUE;
}

static gboolean get_memtype(GstVdecBase *self, const GstStructure *structure)
{
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    g_return_val_if_fail(structure != nullptr, FALSE);
    const gchar *memtype = gst_structure_get_string(structure, "memtype");
    g_return_val_if_fail(memtype != nullptr, FALSE);

    if (strcmp("surface", memtype) == 0) {
        self->memtype = GST_MEMTYPE_SURFACE;
    } else if (strcmp("avshmem", memtype) == 0) {
        self->memtype = GST_MEMTYPE_AVSHMEM;
    }

    if (self->memtype == GST_MEMTYPE_SURFACE) {
        gint ret = self->decoder->SetParameter(GST_VIDEO_SURFACE_INIT, GST_ELEMENT(self));
        g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "GST_VIDEO_SURFACE_INIT", TRUE), FALSE);
    }

    return TRUE;
}

static gboolean gst_vdec_base_negotiate_format(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Negotiate format");
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    g_return_val_if_fail(GST_VIDEO_DECODER_SRC_PAD(self) != nullptr, FALSE);

    GST_DEBUG_OBJECT(self, "Trying to negotiate a video format with downstream");
    if (self->performance_mode && self->pre_init_pool) {
        return TRUE;
    }
    GstCaps *templ_caps = gst_pad_get_pad_template_caps(GST_VIDEO_DECODER_SRC_PAD(self));
    g_return_val_if_fail(templ_caps != nullptr, FALSE);

    (void)update_caps_format(self, templ_caps);
    GstCaps *intersection = gst_pad_peer_query_caps(GST_VIDEO_DECODER_SRC_PAD(self), templ_caps);
    gst_caps_unref(templ_caps);
    g_return_val_if_fail(intersection != nullptr, FALSE);
    // We need unref at end.
    ON_SCOPE_EXIT(0) { gst_caps_unref(intersection); };
    g_return_val_if_fail(gst_caps_is_empty(intersection) == FALSE, FALSE);

    intersection = gst_caps_truncate(intersection);
    intersection = gst_caps_fixate(intersection);

    GstStructure *structure = gst_caps_get_structure(intersection, 0);
    const gchar *format_str = gst_structure_get_string(structure, "format");
    g_return_val_if_fail(format_str != nullptr, FALSE);

    GstVideoFormat format = gst_video_format_from_string(format_str);
    g_return_val_if_fail(format != GST_VIDEO_FORMAT_UNKNOWN, FALSE);

    self->format = format;
    gint ret = self->decoder->SetParameter(GST_VIDEO_FORMAT, GST_ELEMENT(self));
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "setparameter", TRUE), FALSE);
    return TRUE;
}

static gboolean gst_vdec_base_set_outstate(GstVdecBase *self)
{
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(GST_VIDEO_DECODER_SRC_PAD(self) != nullptr, FALSE);
    GstVideoDecoder *decoder = GST_VIDEO_DECODER(self);

    GST_DEBUG_OBJECT(self, "Setting output state: format %s, width %u, height %u",
        gst_video_format_to_string(self->format), self->width, self->height);
    GstVideoCodecState *out_state = gst_video_decoder_set_output_state(GST_VIDEO_DECODER(self),
        self->format, self->width, self->height, self->input_state);
    g_return_val_if_fail(out_state != nullptr, FALSE);
    if (self->output_state != nullptr) {
        gst_video_codec_state_unref(self->output_state);
    }
    self->output_state = out_state;
    if (GST_VIDEO_INFO_MULTIVIEW_MODE(&out_state->info) == GST_VIDEO_MULTIVIEW_MODE_NONE) {
        GST_VIDEO_INFO_MULTIVIEW_MODE(&out_state->info) = GST_VIDEO_MULTIVIEW_MODE_MONO;
        GST_VIDEO_INFO_MULTIVIEW_FLAGS(&out_state->info) = GST_VIDEO_MULTIVIEW_FLAGS_NONE;
    }
    if (out_state->caps == nullptr) {
        out_state->caps = gst_video_info_to_caps(&out_state->info);
    }
    if (out_state->allocation_caps == nullptr) {
        out_state->allocation_caps = gst_caps_ref(out_state->caps);
    }
    g_return_val_if_fail(gst_pad_set_caps(decoder->srcpad, out_state->caps), FALSE);
    return TRUE;
}

static void gst_vdec_base_remove_reconfig_flag(GstVideoDecoder *decoder)
{
    (void)gst_pad_check_reconfigure(decoder->srcpad);
}

static gboolean gst_vdec_base_negotiate(GstVdecBase *self)
{
    MediaTrace trace("VdecBase::Negotiate");
    GST_DEBUG_OBJECT(self, "Negotiate");
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(GST_VIDEO_DECODER_SRC_PAD(self) != nullptr, FALSE);
    g_return_val_if_fail(gst_vdec_base_negotiate_format(self), FALSE);
    g_return_val_if_fail(gst_vdec_base_set_outstate(self), FALSE);
    g_return_val_if_fail(gst_video_decoder_negotiate(GST_VIDEO_DECODER(self)), FALSE);
    gst_vdec_base_remove_reconfig_flag(GST_VIDEO_DECODER(self));
    GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: negotiate end");
    return TRUE;
}

static gboolean gst_vdec_base_allocate_in_buffers(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Allocate input buffers");
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    g_return_val_if_fail(self->inpool != nullptr, FALSE);
    std::vector<GstBuffer*> buffers;
    GstBufferPool *pool = reinterpret_cast<GstBufferPool*>(gst_object_ref(self->inpool));
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    for (guint i = 0; i < self->input.buffer_cnt; ++i) {
        GST_DEBUG_OBJECT(self, "Allocate Buffer %u", i);
        GstBuffer *buffer = nullptr;
        GstFlowReturn flow_ret = gst_buffer_pool_acquire_buffer(pool, &buffer, nullptr);
        if (flow_ret != GST_FLOW_OK || buffer == nullptr) {
            GST_WARNING_OBJECT(self, "Acquire buffer is nullptr");
            gst_buffer_unref(buffer);
            continue;
        }
        buffers.push_back(buffer);
    }
    GST_DEBUG_OBJECT(self, "Use input buffers start");
    // no give ref to decoder
    gint ret = self->decoder->UseInputBuffers(buffers);
    // Return buffers to pool
    for (auto buffer : buffers) {
        gst_buffer_unref(buffer);
    }
    buffers.clear();
    GST_DEBUG_OBJECT(self, "Use input buffers end");
    return gst_codec_return_is_ok(self, ret, "usebuffer", TRUE);
}

static gboolean gst_vdec_base_update_out_port_def(GstVdecBase *self, guint *size)
{
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    g_return_val_if_fail(size != nullptr, FALSE);
    gint ret = self->decoder->GetParameter(GST_VIDEO_OUTPUT_COMMON, GST_ELEMENT(self));
    g_return_val_if_fail(ret == GST_CODEC_OK, FALSE);
    g_return_val_if_fail(self->output.min_buffer_cnt <= self->out_buffer_max_cnt, FALSE);
    self->output.buffer_cnt =
        self->out_buffer_max_cnt > self->output.buffer_cnt ? self->output.buffer_cnt : self->out_buffer_max_cnt;
    ret = self->decoder->SetParameter(GST_VIDEO_OUTPUT_COMMON, GST_ELEMENT(self));
    g_return_val_if_fail(ret == GST_CODEC_OK, FALSE);
    if (self->metadata_mode) {
        GST_DEBUG_OBJECT(self, "Set metadata mode");
        ret = self->decoder->SetParameter(GST_METADATA_MODE, GST_ELEMENT(self));
        g_return_val_if_fail(ret == GST_CODEC_OK, FALSE);
    }
    ret = self->decoder->GetParameter(GST_VIDEO_OUTPUT_COMMON, GST_ELEMENT(self));
    g_return_val_if_fail(ret == GST_CODEC_OK, FALSE);
    GST_INFO_OBJECT(self, "output params is min buffer count %u, buffer count %u, buffer size is %u",
        self->output.min_buffer_cnt, self->output.buffer_cnt, self->output.buffer_size);
    *size = self->output.buffer_size > *size ? self->output.buffer_size : *size;
    self->out_buffer_cnt = self->output.buffer_cnt;
    return TRUE;
}

static gboolean gst_vdec_base_allocate_out_buffers(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Allocate output buffers");
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    std::vector<GstBuffer*> buffers;
    self->coding_outbuf_cnt = self->out_buffer_cnt;
    if (self->memtype != GST_MEMTYPE_SURFACE) {
        MediaTrace trace("VdecBase::AllocateOutPutBuffer");
        for (guint i = 0; i < self->out_buffer_cnt; ++i) {
            GST_DEBUG_OBJECT(self, "Allocate output buffer %u", i);
            GstBuffer *buffer = gst_video_decoder_allocate_output_buffer(GST_VIDEO_DECODER(self));
            if (buffer != nullptr) {
                buffers.push_back(buffer);
            }
            g_warn_if_fail(buffer != nullptr);
        }
    }

    gint ret;
    {
        MediaTrace trace("VdecBase::UseOutputBuffers");
        // give buffer ref to decoder
        ret = self->decoder->UseOutputBuffers(buffers);
    }
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "usebuffer", TRUE), FALSE);
    return TRUE;
}

static gboolean gst_vdec_base_prepare(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Prepare");
    g_return_val_if_fail(self != nullptr, FALSE);

    // Negotiate with downstream and get format
    g_return_val_if_fail(gst_vdec_base_negotiate(self), FALSE);

    if (!gst_vdec_base_is_flushing(self)) {
        // Check allocate input buffer already
        gst_vdec_base_check_allocate_input(self);

        // To allocate output memory, we need to give the size
        g_return_val_if_fail(gst_vdec_base_allocate_out_buffers(self), FALSE);
    }

    return TRUE;
}

static void gst_vdec_base_clean_all_frames(GstVideoDecoder *decoder)
{
    GST_DEBUG_OBJECT(decoder, "Clean all frames");
    g_return_if_fail(decoder != nullptr);
    GList *frame_head = gst_video_decoder_get_frames(decoder);
    g_return_if_fail(frame_head != nullptr);
    for (GList *frame = frame_head; frame != nullptr; frame = frame->next) {
        GstVideoCodecFrame *tmp = reinterpret_cast<GstVideoCodecFrame*>(frame->data);
        if (tmp != nullptr) {
            gst_video_decoder_release_frame(decoder, tmp);
        }
    }
    g_list_free(frame_head);
}

static void gst_vdec_debug_input_time(GstVdecBase *self)
{
    if (self->input.first_frame_time == 0) {
        self->input.first_frame_time = g_get_monotonic_time();
    } else {
        self->input.last_frame_time = g_get_monotonic_time();
    }
    self->input.frame_cnt++;
    gint64 time_interval = self->input.last_frame_time - self->input.first_frame_time;
    gint64 frame_cnt = self->input.frame_cnt - 1;
    if (frame_cnt > 0) {
        gint64 time_every_frame = time_interval / frame_cnt;
        GST_DEBUG_OBJECT(self, "Decoder Input Time interval %" G_GINT64_FORMAT " us, frame count %" G_GINT64_FORMAT
        " ,every frame time %" G_GINT64_FORMAT " us, frame rate %.9f", time_interval, self->input.frame_cnt,
        time_every_frame, static_cast<double>(G_TIME_SPAN_SECOND) / static_cast<double>(time_every_frame));
    }
}

static void gst_vdec_debug_output_time(GstVdecBase *self)
{
    if (self->output.first_frame_time == 0) {
        self->output.first_frame_time = g_get_monotonic_time();
    } else {
        self->output.last_frame_time = g_get_monotonic_time();
    }
    self->output.frame_cnt++;
    gint64 time_interval = self->output.last_frame_time - self->output.first_frame_time;
    gint64 frame_cnt = self->output.frame_cnt - 1;
    if (frame_cnt > 0) {
        gint64 time_every_frame = time_interval / frame_cnt;
        GST_DEBUG_OBJECT(self, "Decoder Output Time interval %" G_GINT64_FORMAT " us, frame count %" G_GINT64_FORMAT
        " ,every frame time %" G_GINT64_FORMAT " us, frame rate %.9f", time_interval, self->output.frame_cnt,
        time_every_frame, static_cast<double>(G_TIME_SPAN_SECOND) / static_cast<double>(time_every_frame));
    }
}

static void gst_vdec_base_dump_input_buffer(GstVdecBase *self, GstBuffer *buffer)
{
    g_return_if_fail(self != nullptr);
    g_return_if_fail(buffer != nullptr);
    if (self->input.enable_dump == FALSE) {
        return;
    }
    GST_DEBUG_OBJECT(self, "Dump input buffer");
    std::string input_dump_file = "/data/media/vdecbase-in-" +
        std::to_string(static_cast<int32_t>(FAKE_POINTER(self))) + ".es";
    if (self->input.dump_file == nullptr) {
        self->input.dump_file = fopen(input_dump_file.c_str(), "wb+");
    }
    if (self->input.dump_file == nullptr) {
        GST_ERROR_OBJECT(self, "open file failed");
        return;
    }
    GstMapInfo info = GST_MAP_INFO_INIT;
    g_return_if_fail(gst_buffer_map(buffer, &info, GST_MAP_READ));
    (void)fwrite(info.data, info.size, 1, self->input.dump_file);
    (void)fflush(self->input.dump_file);
    gst_buffer_unmap(buffer, &info);
}

static void gst_vdec_base_dump_output_buffer(GstVdecBase *self, GstBuffer *buffer)
{
    g_return_if_fail(self != nullptr);
    g_return_if_fail(buffer != nullptr);
    if (self->output.enable_dump == FALSE) {
        return;
    }
    GST_DEBUG_OBJECT(self, "Dump output buffer");
    std::string output_dump_file = "/data/media/vdecbase-out-" +
        std::to_string(static_cast<int32_t>(FAKE_POINTER(self))) + ".yuv";
    if (self->output.dump_file == nullptr) {
        self->output.dump_file = fopen(output_dump_file.c_str(), "wb+");
    }
    if (self->output.dump_file == nullptr) {
        GST_DEBUG_OBJECT(self, "open file failed");
        return;
    }
    GstMapInfo info = GST_MAP_INFO_INIT;
    g_return_if_fail(gst_buffer_map(buffer, &info, GST_MAP_READ));
    guint stride = (guint)(self->real_stride == 0 ? self->stride : self->real_stride);
    guint stride_height = (guint)(self->real_stride_height == 0 ? self->stride_height : self->real_stride_height);
    stride = stride == 0 ? self->width : stride;
    stride_height = stride_height == 0 ? self->height : stride_height;
    // yuv size
    guint size = stride * stride_height * 3 / 2;
    size = size > info.size ? info.size : size;
    (void)fwrite(info.data, size, 1, self->output.dump_file);
    (void)fflush(self->output.dump_file);
    gst_buffer_unmap(buffer, &info);
}

static void gst_vdec_base_input_frame_pts_to_list(GstVdecBase *self, GstVideoCodecFrame *frame)
{
    GST_DEBUG_OBJECT(self, "Input frame pts %" G_GUINT64_FORMAT, frame->pts);
    g_mutex_lock(&self->lock);
    if (frame->pts == GST_CLOCK_TIME_NONE) {
        g_mutex_unlock(&self->lock);
        return;
    }
    if (self->pts_list.empty() || frame->pts > self->pts_list.back()) {
        self->pts_list.push_back(frame->pts);
        self->last_pts = frame->pts;
        g_mutex_unlock(&self->lock);
        return;
    }
    for (auto iter = self->pts_list.begin(); iter != self->pts_list.end(); ++iter) {
        if ((*iter) == frame->pts) {
            break;
        }
        if ((*iter) > frame->pts) {
            self->pts_list.insert(iter, frame->pts);
            break;
        }
    }
    g_mutex_unlock(&self->lock);
}

static int32_t gst_vdec_base_push_input_buffer_with_copy(GstVdecBase *self, GstBuffer *src_buffer,
    gboolean is_codec_data)
{
    MediaTrace trace("VdecBase::InputCopy");
    GstBuffer* dts_buffer;
    GstBufferPool *pool = reinterpret_cast<GstBufferPool *>(gst_object_ref(self->inpool));
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    GstFlowReturn flow_ret = gst_buffer_pool_acquire_buffer(pool, &dts_buffer, nullptr);
    ON_SCOPE_EXIT(1) { gst_buffer_unref(dts_buffer); };
    g_return_val_if_fail(flow_ret == GST_FLOW_OK && dts_buffer != nullptr, GST_CODEC_FLUSH);
    GstMapInfo dts_map = GST_MAP_INFO_INIT;
    g_return_val_if_fail(gst_buffer_map(dts_buffer, &dts_map, GST_MAP_WRITE) == TRUE, GST_CODEC_ERROR);
    ON_SCOPE_EXIT(2) { gst_buffer_unmap(dts_buffer, &dts_map); };
    GstMapInfo src_map = GST_MAP_INFO_INIT;
    g_return_val_if_fail(gst_buffer_map(src_buffer, &src_map, GST_MAP_READ) == TRUE, GST_CODEC_ERROR);
    ON_SCOPE_EXIT(3) { gst_buffer_unmap(src_buffer, &src_map); };

    GstBufferTypeMeta *meta = gst_buffer_get_buffer_type_meta(dts_buffer);
    g_return_val_if_fail(meta != nullptr, GST_CODEC_ERROR);
    GstVdecBaseClass *kclass = GST_VDEC_BASE_GET_CLASS(self);
    if (kclass->parser) {
        ParseMeta parse_meta = {src_map, dts_map, is_codec_data, meta->length};
        g_return_val_if_fail(kclass->parser(self, parse_meta) == TRUE, GST_CODEC_ERROR);
    } else {
        auto ret = memcpy_s(dts_map.data, dts_map.size, src_map.data, src_map.size);
        g_return_val_if_fail(ret == EOK, GST_CODEC_ERROR);
        meta->length = src_map.size;
    }
    CANCEL_SCOPE_EXIT_GUARD(2);
    CANCEL_SCOPE_EXIT_GUARD(3);
    gst_buffer_unmap(dts_buffer, &dts_map);
    gst_buffer_unmap(src_buffer, &src_map);
    gst_buffer_set_size(dts_buffer, meta->length);
    GST_BUFFER_PTS(dts_buffer) = GST_BUFFER_PTS(src_buffer);
    GST_BUFFER_DTS(dts_buffer) = GST_BUFFER_DTS(src_buffer);
    GST_BUFFER_DURATION(dts_buffer) = GST_BUFFER_DURATION(src_buffer);
    GST_DEBUG_OBJECT(self, "Input frame pts %" G_GUINT64_FORMAT, GST_BUFFER_PTS(dts_buffer));
    gst_vdec_base_dump_input_buffer(self, dts_buffer);
    gint codec_ret = self->decoder->PushInputBuffer(dts_buffer);
    return codec_ret;
}

static gboolean gst_vdec_check_ashmem_buffer(GstBuffer *buffer)
{
    g_return_val_if_fail(buffer != nullptr, FALSE);
    GstMemory *memory = gst_buffer_peek_memory(buffer, 0);
    return gst_is_shmem_memory(memory);
}

static void gst_vdec_base_drm_get_block(GstVdecBase *self, guint *crypt_byte_block, guint *skip_byte_block)
{
    if (!gst_structure_get_uint(self->crypto_info, "crypt_byte_block", crypt_byte_block)) {
        GST_ERROR_OBJECT(self, "failed to get crypt_byte_block");
        return;
    }
    if (!gst_structure_get_uint(self->crypto_info, "skip_byte_block", skip_byte_block)) {
        GST_ERROR_OBJECT(self, "failed to get skip_byte_block");
        return;
    }
    return;
}

static void gst_vdec_base_drm_get_algo(GstVdecBase *self, guint *algo)
{
    const gchar *mode = nullptr;
    gboolean encrypted;
    mode = gst_structure_get_string(self->crypto_info, "cipher-mode");
    if (mode == nullptr) {
        *algo = DRM_ALG_CENC_UNENCRYPTED;
    } else {
        gst_drm_get_algo(mode, algo);
    }
    if (!gst_structure_get_boolean(self->crypto_info, "encrypted", &encrypted)) {
        if (encrypted == FALSE) {
            *algo = DRM_ALG_CENC_UNENCRYPTED;
        }
    }
    return;
}

static GstFlowReturn gst_vdec_base_drm_get_iv(GstVdecBase *self, guint8 *iv_data, guint *iv_data_size)
{
    GstFlowReturn ret = GST_FLOW_OK;
    const GValue *value = nullptr;
    GstBuffer *iv_buf = nullptr;
    GstMapInfo iv_map;
    guint iv_size;
    if (!gst_structure_get_uint(self->crypto_info, "iv_size", &iv_size)) {
        GST_ERROR_OBJECT(self, "failed to get iv_size");
        return GST_FLOW_NOT_SUPPORTED;
    }
    if (iv_size > *iv_data_size) {
        return GST_FLOW_NOT_SUPPORTED;
    }
    value = gst_structure_get_value(self->crypto_info, "iv");
    if (value == nullptr) {
        GST_ERROR_OBJECT(self, "Failed to get iv for sample");
        return GST_FLOW_NOT_SUPPORTED;
    }
    iv_buf = gst_value_get_buffer(value);
    if (!gst_buffer_map(iv_buf, &iv_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map iv");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    if (iv_map.size > *iv_data_size) {
        GST_ERROR_OBJECT(self, "iv size err\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    *iv_data_size = iv_map.size;
    (void)memset_s(iv_data, *iv_data_size, 0, *iv_data_size);
    (void)memcpy_s(iv_data, *iv_data_size, iv_map.data, iv_map.size);
beach:
    if (iv_buf) {
        gst_buffer_unmap(iv_buf, &iv_map);
    }
    return ret;
}

static GstFlowReturn gst_vdec_base_drm_get_keyid(GstVdecBase *self, guint8 *keyid_data, guint *keyid_data_size)
{
    GstFlowReturn ret = GST_FLOW_OK;
    const GValue *value = nullptr;
    GstBuffer *keyid_buf = nullptr;
    GstMapInfo keyid_map;

    value = gst_structure_get_value(self->crypto_info, "kid");
    if (value == nullptr) {
        GST_ERROR_OBJECT(self, "Failed to get kid for sample");
        return GST_FLOW_NOT_SUPPORTED;
    }
    keyid_buf = gst_value_get_buffer(value);
    if (!gst_buffer_map(keyid_buf, &keyid_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map key id");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    if (keyid_map.size > *keyid_data_size) {
        GST_ERROR_OBJECT(self, "iv size err\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    *keyid_data_size = keyid_map.size;
    (void)memset_s(keyid_data, *keyid_data_size, 0, *keyid_data_size);
    (void)memcpy_s(keyid_data, *keyid_data_size, keyid_map.data, keyid_map.size);
beach:
    if (keyid_buf) {
        gst_buffer_unmap(keyid_buf, &keyid_map);
    }
    return ret;
}

static GstFlowReturn gst_vdec_base_drm_get_subsample_count(GstVdecBase *self, guint *subsample_count)
{
    if (!gst_structure_get_uint(self->crypto_info, "subsample_count", subsample_count)) {
        GST_ERROR_OBJECT(self, "failed to get subsample_count");
        return GST_FLOW_NOT_SUPPORTED;
    }

    if (*subsample_count > DRM_MAX_SUB_SAMPLE_NUM) {
        GST_ERROR_OBJECT(self, "subsample_count err");
        return GST_FLOW_NOT_SUPPORTED;
    }
    return GST_FLOW_OK;
}

static GstFlowReturn gst_vdec_base_drm_get_subsample(GstVdecBase *self, DRM_SubSample *subsample_info,
    guint subsample_count)
{
    GstFlowReturn ret = GST_FLOW_OK;
    const GValue *value = nullptr;
    GstBuffer *subsamples_buf = nullptr;
    GstMapInfo subsamples_map;
    GstByteReader *reader = nullptr;
    if (subsample_count) {
        value = gst_structure_get_value(self->crypto_info, "subsamples");
        if (value == nullptr) {
            GST_ERROR_OBJECT(self, "Failed to get subsamples");
            return GST_FLOW_NOT_SUPPORTED;
        }
        subsamples_buf = gst_value_get_buffer(value);
        if (!gst_buffer_map(subsamples_buf, &subsamples_map, GST_MAP_READ)) {
            GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
            ret = GST_FLOW_NOT_SUPPORTED;
            goto beach;
        }
        reader = gst_byte_reader_new(subsamples_map.data, subsamples_map.size);
        if (reader == nullptr) {
            GST_ERROR_OBJECT(self, "Failed to new subsample reader");
            ret = GST_FLOW_NOT_SUPPORTED;
            goto beach;
        }
    }
    for (guint i = 0; i < subsample_count; i++) {
        guint16 clear_header_len = 0;
        guint32 pay_load_len = 0;
        if (!gst_byte_reader_get_uint16_be(reader, &clear_header_len)
            || !gst_byte_reader_get_uint32_be(reader, &pay_load_len)) {
            ret = GST_FLOW_NOT_SUPPORTED;
            goto beach;
        }
        subsample_info[i].clear_header_len = (guint)clear_header_len;
        subsample_info[i].pay_load_len = (guint)pay_load_len;
    }
beach:
    if (reader) {
        gst_byte_reader_free(reader);
    }
    if (subsamples_buf) {
        gst_buffer_unmap(subsamples_buf, &subsamples_map);
    }
    return ret;
}

static GstFlowReturn gst_vdec_base_gst_buffer_copy_to_ashmem(GstVdecBase *in_self, GstBuffer *buf)
{
    GstFlowReturn ret = GST_FLOW_OK;
    guint8 *in_ashmem_buf = nullptr;
    GstMapInfo map;
    errno_t rek;
    buf = gst_buffer_make_writable(buf);
    if (!gst_buffer_map(buf, &map, GST_MAP_READWRITE)) {
        GST_ERROR_OBJECT(in_self, "Failed to map buffer");
        return GST_FLOW_NOT_SUPPORTED;
    }
    in_ashmem_buf = (guint8 *)mmap(nullptr, map.size, PROT_READ | PROT_WRITE, MAP_SHARED,
        in_self->drm_ashmem_infd, 0);
    if (in_ashmem_buf == nullptr) {
        GST_ERROR_OBJECT(in_self, "ashmem_inbuf mmap failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    rek = memcpy_s(in_ashmem_buf, map.size, map.data, map.size);
    if (rek != EOK) {
        GST_ERROR_OBJECT(in_self, "ashmem_inbuf memcpy failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
beach:
    if (in_ashmem_buf) {
        (void)munmap(in_ashmem_buf, map.size);
        in_ashmem_buf = nullptr;
    }
    gst_buffer_unmap(buf, &map);
    return ret;
}

static GstFlowReturn gst_vdec_base_ashmem_copy_to_gst_buffer(GstVdecBase *out_self, GstBuffer *buf)
{
    GstFlowReturn ret = GST_FLOW_OK;
    guint8 *out_ashmem_buf = nullptr;
    GstMapInfo map;
    errno_t rek;
    buf = gst_buffer_make_writable(buf);
    if (!gst_buffer_map(buf, &map, GST_MAP_READWRITE)) {
        GST_ERROR_OBJECT(out_self, "Failed to map buffer");
        return GST_FLOW_NOT_SUPPORTED;
    }
    out_ashmem_buf = (guint8 *)mmap(nullptr, map.size, PROT_READ | PROT_WRITE, MAP_SHARED,
        out_self->drm_ashmem_outfd, 0);
    if (out_ashmem_buf == nullptr) {
        GST_ERROR_OBJECT(out_self, "ashmem_outbuf mmap failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    rek = memcpy_s(map.data, map.size, out_ashmem_buf, map.size);
    if (rek != EOK) {
        GST_ERROR_OBJECT(out_self, "data memcpy failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
beach:
    if (out_ashmem_buf) {
        (void)munmap(out_ashmem_buf, map.size);
        out_ashmem_buf = nullptr;
    }
    gst_buffer_unmap(buf, &map);
    return ret;
}

static GstFlowReturn gst_vdec_base_drm_set_ts_subsample(GstMapInfo *map, DRM_SubSample *subsample_info,
    guint *subsample_count, guint skip_byte_block, guint *crypt_byte_block)
{
    if ((*subsample_count == 0) && (*crypt_byte_block + skip_byte_block == DRM_TS_FLAG_CRYPT_BYTE_BLOCK ||
        *crypt_byte_block + skip_byte_block == (DRM_CRYPT_BYTE_BLOCK + DRM_SKIP_BYTE_BLOCK))) {
        guint skip_bytes = 0;
        *subsample_count = (guint)DRM_TS_SUB_SAMPLE_NUM;
        subsample_info[0].clear_header_len = map->size;
        subsample_info[0].pay_load_len = 0;
        subsample_info[1].clear_header_len = 0;
        subsample_info[1].pay_load_len = 0;
        gst_vdec_base_drm_get_skip_clear_bytes(0, &skip_bytes);
        if (map->size > (skip_bytes + (guint)DRM_AES_BLOCK_SIZE)) {
            gst_vdec_base_drm_modify_cenc_info(0, map->data, map->size, subsample_info);
        }
        if (*crypt_byte_block >= DRM_TS_FLAG_CRYPT_BYTE_BLOCK) {
            *crypt_byte_block -= DRM_TS_FLAG_CRYPT_BYTE_BLOCK;
        }
    }
    return GST_FLOW_OK;
}

static GstFlowReturn gst_vdec_base_drm_cenc_decrypt(GstVideoDecoder *decoder, GstBuffer *buf)
{
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    GstFlowReturn ret = GST_FLOW_OK;
    GstMapInfo map;
    guint algo = 0;
    guint crypt_byte_block = 0;
    guint skip_byte_block = 0;
    guint subsample_count = 0;
    DRM_SubSample subsample_info[DRM_MAX_SUB_SAMPLE_NUM];
    guint iv_size = DRM_IV_SIZE;
    guint8 iv_data[DRM_IV_SIZE];
    guint keyid_size = DRM_KEY_ID_SIZE;
    guint8 keyid_data[DRM_KEY_ID_SIZE];
    gint res;

    GST_DEBUG_OBJECT(self, "drm cenc decrypt\n");
    if (buf == nullptr) {
        GST_ERROR_OBJECT(self, "Failed to get writable buffer\n");
        return GST_FLOW_NOT_SUPPORTED;
    }
    buf = gst_buffer_make_writable(buf);
    if (!gst_buffer_map(buf, &map, GST_MAP_READWRITE)) {
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return GST_FLOW_NOT_SUPPORTED;
    }
    if (self->crypto_info == nullptr) {
        gst_buffer_unmap(buf, &map);
        return ret;
    }

    gst_vdec_base_drm_get_block(self, &crypt_byte_block, &skip_byte_block);
    gst_vdec_base_drm_get_algo(self, &algo);
    gst_vdec_base_drm_get_iv(self, iv_data, &iv_size);
    gst_vdec_base_drm_get_keyid(self, keyid_data, &keyid_size);
    gst_vdec_base_drm_get_subsample_count(self, &subsample_count);
    gst_vdec_base_drm_get_subsample(self, subsample_info, subsample_count);

    gst_vdec_base_gst_buffer_copy_to_ashmem(self, buf);

    gst_vdec_base_drm_set_ts_subsample(&map, subsample_info, &subsample_count,
        skip_byte_block, &crypt_byte_block);

    g_signal_emit_by_name ((GstElement *)self, "media-decrypt", (gint64)(self->drm_ashmem_infd),
        (gint64)(self->drm_ashmem_outfd), map.size, keyid_data, keyid_size, iv_data, iv_size,
        subsample_count, subsample_info, algo, 0, crypt_byte_block, skip_byte_block, &res);

    gst_vdec_base_ashmem_copy_to_gst_buffer(self, buf);

    gst_buffer_unmap(buf, &map);
    return ret;
}

static GstBuffer *gst_vdec_base_get_input_buffer(GstVdecBase *self, GstVdecBaseClass *kclass, GstVideoCodecFrame *frame)
{
    GstBuffer *buf = nullptr;
    const GstProtectionMeta *prot_meta = nullptr;
    prot_meta = (GstProtectionMeta*)gst_buffer_get_protection_meta(frame->input_buffer);
    if (prot_meta != nullptr) {
        self->crypto_info = gst_structure_copy(prot_meta->info);
    }
    if (kclass->handle_slice_buffer != nullptr && self->player_scene == true) {
        bool ready_push_slice_buffer = false;
        GstBuffer *cat_buffer = kclass->handle_slice_buffer(self, frame->input_buffer, ready_push_slice_buffer, false);
        if (cat_buffer != nullptr && ready_push_slice_buffer == true) {
            buf = cat_buffer;
            GST_BUFFER_PTS(buf) = GST_BUFFER_PTS(frame->input_buffer);
            GST_BUFFER_DTS(buf) = GST_BUFFER_DTS(frame->input_buffer);
            GST_BUFFER_DURATION(buf) = GST_BUFFER_DURATION(frame->input_buffer);
        }
    } else {
        buf = frame->input_buffer;
        gst_buffer_ref(buf);
    }
    return buf;
}

static GstFlowReturn gst_vdec_base_push_input_buffer(GstVideoDecoder *decoder, GstVideoCodecFrame *frame)
{
    MediaTrace trace("VdecBase::PushInputBuffer");
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    gst_vdec_debug_input_time(self);
    gst_vdec_base_input_frame_pts_to_list(self, frame);
    GstVdecBaseClass *kclass = GST_VDEC_BASE_GET_CLASS(self);
    GstBuffer *buf = gst_vdec_base_get_input_buffer(self, kclass, frame);
    if (buf == nullptr) {
        GST_DEBUG_OBJECT(self, "buffer is null");
        return GST_FLOW_OK;
    }

    gst_vdec_base_drm_cenc_decrypt(decoder, buf);

    GST_VIDEO_DECODER_STREAM_UNLOCK(self);

    gint codec_ret = GST_CODEC_OK;
    if (!gst_vdec_check_ashmem_buffer(buf) && self->input_need_ashmem) {
        // codec data just for mkv/mp4, and this scene is no ashmem
        if (self->codec_data_update) {
            self->codec_data_update = FALSE;
            (void)gst_vdec_base_push_input_buffer_with_copy(self, self->codec_data, TRUE);
        }
        codec_ret = gst_vdec_base_push_input_buffer_with_copy(self, buf, FALSE);
    } else {
        gst_vdec_base_dump_input_buffer(self, buf);
        codec_ret = self->decoder->PushInputBuffer(buf);
    }
    gst_buffer_unref(buf);
    GST_VIDEO_DECODER_STREAM_LOCK(self);
    GstFlowReturn ret = GST_FLOW_OK;
    switch (codec_ret) {
        case GST_CODEC_OK:
            break;
        case GST_CODEC_FLUSH:
            ret = GST_FLOW_FLUSHING;
            GST_DEBUG_OBJECT(self, "Flushing");
            break;
        default:
            ret = GST_FLOW_ERROR;
            GST_ELEMENT_WARNING(self, STREAM, ENCODE, ("Hardware encoder error!"), ("pull"));
    }
    return ret;
}

static GstFlowReturn gst_vdec_base_handle_frame(GstVideoDecoder *decoder, GstVideoCodecFrame *frame)
{
    MediaTrace trace("VdecBase::HandleFrame");
    GST_DEBUG_OBJECT(decoder, "Handle frame");
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    ON_SCOPE_EXIT(0) { gst_video_codec_frame_unref(frame); };
    g_return_val_if_fail(GST_IS_VDEC_BASE(self), GST_FLOW_ERROR);
    g_return_val_if_fail(self != nullptr && frame != nullptr && self->decoder != nullptr, GST_FLOW_ERROR);
    if (self->input.first_frame) {
        GST_WARNING_OBJECT(decoder, "KPI-TRACE-VDEC: first in frame");
        self->input.first_frame = FALSE;
    }

    if (gst_vdec_base_is_flushing(self)) {
        return GST_FLOW_FLUSHING;
    }
    gst_vdec_base_clean_all_frames(decoder);

    GstVdecBaseClass *kclass = GST_VDEC_BASE_GET_CLASS(self);
    if (kclass->bypass_frame != nullptr && self->player_scene == true) {
        g_return_val_if_fail(!kclass->bypass_frame(self, frame), GST_FLOW_OK); // no fail
    }

    if (!self->prepared) {
        g_return_val_if_fail(gst_vdec_base_prepare(self), GST_FLOW_ERROR);

        /* When the resolution of HLS changes, the new one will block on the pad port and wait for connection.
            At this time, if the seek operation is issued, the pad port will be activated. At this time,
            the negotiation is unreliable and the frame data needs to be refreshed */
        g_return_val_if_fail(!gst_vdec_base_is_flushing(self), GST_FLOW_FLUSHING);
        self->prepared = TRUE;
    }
    GstPad *pad = GST_VIDEO_DECODER_SRC_PAD(self);
    if (!self->decoder_start) {
        gint ret = self->decoder->Start();
        g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "start", TRUE), GST_FLOW_ERROR);
        self->decoder_start = TRUE;
        self->is_eos_state = FALSE;
        GST_WARNING_OBJECT(decoder, "KPI-TRACE-VDEC: start end");
    }
    g_return_val_if_fail(gst_pad_get_task_state(pad) == GST_TASK_STARTED ||
        gst_pad_start_task(pad, (GstTaskFunction)gst_vdec_base_loop, decoder, nullptr) == TRUE, GST_FLOW_ERROR);

    GstFlowReturn ret = gst_vdec_base_push_input_buffer(decoder, frame);
    return ret;
}

static void update_video_meta(const GstVdecBase *self, GstBuffer *buffer)
{
    GST_DEBUG_OBJECT(self, "Update buffer video meta");
    g_return_if_fail(self != nullptr);
    g_return_if_fail(buffer != nullptr);
    GstVideoMeta *video_meta = gst_buffer_get_video_meta(buffer);
    if (video_meta == nullptr) {
        gst_buffer_add_video_meta(buffer, GST_VIDEO_FRAME_FLAG_NONE, self->format, self->width, self->height);
    } else {
        GST_DEBUG_OBJECT(self, "stride is %d, stride_height is %d", self->real_stride, self->real_stride_height);
        video_meta->width = self->width;
        video_meta->height = self->height;
        video_meta->offset[0] = 0;
        video_meta->stride[0] = self->real_stride;
        video_meta->stride[1] = self->real_stride;
        video_meta->offset[1] = self->real_stride * self->real_stride_height;
    }
}

static GstVideoCodecFrame *gst_vdec_base_new_frame(GstVdecBase *self, GstBuffer *buffer)
{
    GST_DEBUG_OBJECT(self, "New frame");
    GstVideoCodecFrame *frame = g_slice_new0(GstVideoCodecFrame);
    g_return_val_if_fail(frame != nullptr, nullptr);
    frame->ref_count = 1;
    frame->system_frame_number = 0;
    frame->decode_frame_number = 0;
    frame->dts = GST_CLOCK_TIME_NONE;
    g_mutex_lock(&self->lock);
    if (self->player_mode == FALSE) {
        frame->pts = GST_BUFFER_PTS(buffer);
    } else {
        frame->pts = self->last_pts;
        if (!self->pts_list.empty()) {
            frame->pts = self->pts_list.front();
            GST_DEBUG_OBJECT(self, "Pts %" G_GUINT64_FORMAT, frame->pts);
            self->pts_list.pop_front();
        }
    }
    g_mutex_unlock(&self->lock);
    frame->duration = GST_CLOCK_TIME_NONE;
    frame->events = nullptr;

    return frame;
}

static void gst_vdec_base_post_resolution_changed_message(GstVdecBase *self, gboolean need_post)
{
    if (!need_post) {
        return;
    }
    GstMessage *msg_resolution_changed = nullptr;
    msg_resolution_changed = gst_message_new_resolution_changed(GST_OBJECT(self),
        self->width, self->height);
    if (msg_resolution_changed) {
        gst_element_post_message(GST_ELEMENT(self), msg_resolution_changed);
    }
    self->resolution_changed = TRUE;
    GST_DEBUG_OBJECT(self, "post resolution changed message");
}

static GstFlowReturn push_output_buffer(GstVdecBase *self, GstBuffer *buffer)
{
    MediaTrace trace("VdecBase::PushBufferToSurfaceSink");
    GST_DEBUG_OBJECT(self, "Push output buffer");
    if (!gst_video_decoder_need_decode(GST_VIDEO_DECODER(self))) {
        GST_INFO_OBJECT(self, "only need one frame!");
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }
    g_return_val_if_fail(self != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);
    update_video_meta(self, buffer);
    GstVideoCodecFrame *frame = gst_vdec_base_new_frame(self, buffer);
    g_return_val_if_fail(frame != nullptr, GST_FLOW_ERROR);
    gst_vdec_debug_output_time(self);

    gst_vdec_base_post_resolution_changed_message(self, self->resolution_changed == FALSE);

    frame->output_buffer = buffer;
    gst_vdec_base_dump_output_buffer(self, buffer);
    GstFlowReturn flow_ret = gst_video_decoder_finish_frame(GST_VIDEO_DECODER(self), frame);
    return flow_ret;
}

static gboolean gst_vdec_check_out_format_change(GstVdecBase *self)
{
    gboolean is_format_change = (self->width != self->rect.width) || (self->height != self->rect.height);

    GST_INFO_OBJECT(self, "Format change width %d to %d, height %d to %d",
        self->width, self->output.width, self->height, self->output.height);
    self->width = self->rect.width;
    self->height = self->rect.height;
    return is_format_change;
}

static gboolean gst_vdec_check_out_buffer_cnt(GstVdecBase *self)
{
    g_return_val_if_fail(self->output.min_buffer_cnt <= self->out_buffer_max_cnt, FALSE);
    self->output.buffer_cnt =
        self->output.buffer_cnt > self->out_buffer_max_cnt ? self->out_buffer_max_cnt : self->output.buffer_cnt;
    gboolean is_buffer_cnt_change = self->out_buffer_cnt != self->output.buffer_cnt;
    GST_INFO_OBJECT(self, "Format change buffer %u to %u", self->out_buffer_cnt, self->output.buffer_cnt);
    self->out_buffer_cnt = self->output.buffer_cnt;
    return is_buffer_cnt_change;
}

static gboolean gst_vdec_check_out_buffer_usage(GstVdecBase *self)
{
    guint64 old_usage = self->usage;
    (void)self->decoder->GetParameter(GST_BUFFER_USAGE, GST_ELEMENT(self));
    GST_INFO_OBJECT(self, "buffer usage %" G_GUINT64_FORMAT " change to %" G_GUINT64_FORMAT,
        old_usage, self->usage);
    gboolean is_usage_change = old_usage != self->usage;
    return is_usage_change;
}

static void gst_vdec_base_get_real_stride(GstVdecBase *self)
{
    self->real_stride = self->stride;
    self->real_stride_height = self->stride_height;
}

static GstFlowReturn gst_vdec_base_format_change(GstVdecBase *self)
{
    g_return_val_if_fail(self != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(self->decoder != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&self->format_changed_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&self->format_changed_lock); };
    if (self->unsupport_format_changed || !self->decoder->IsFormatChanged()) {
        return GST_FLOW_OK;
    }

    MediaTrace trace("VdecBase::FormatChange");
    GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: format change start");
    gint ret = self->decoder->ActiveBufferMgr(GST_CODEC_OUTPUT, false);
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "ActiveBufferMgr", TRUE), GST_FLOW_ERROR);
    ret = self->decoder->GetParameter(GST_VIDEO_OUTPUT_COMMON, GST_ELEMENT(self));
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "GetParameter", TRUE), GST_FLOW_ERROR);
    gst_vdec_base_get_real_stride(self);
    gboolean format_change = gst_vdec_check_out_format_change(self);
    gboolean buffer_cnt_change = gst_vdec_check_out_buffer_cnt(self);
    gboolean buffer_usage_change = gst_vdec_check_out_buffer_usage(self);
    gst_vdec_base_post_resolution_changed_message(self, format_change == TRUE);
    if (format_change || buffer_usage_change || (buffer_cnt_change && self->memtype != GST_MEMTYPE_SURFACE)) {
        // wait all sufacebuffer already render
        GstQuery *query = gst_query_new_drain();
        if (!gst_pad_peer_query(GST_VIDEO_DECODER_SRC_PAD(self), query)) {
            GST_WARNING_OBJECT(self, "drain query failed");
        }
        gst_query_unref(query);
        g_return_val_if_fail(gst_buffer_pool_set_active(GST_BUFFER_POOL(self->outpool), FALSE), GST_FLOW_ERROR);
        ret = self->decoder->FreeOutputBuffers();
        g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "freebuffer", TRUE), GST_FLOW_ERROR);
        g_return_val_if_fail(gst_vdec_base_set_outstate(self), GST_FLOW_ERROR);
        g_return_val_if_fail(gst_video_decoder_negotiate(GST_VIDEO_DECODER(self)), GST_FLOW_ERROR);
    } else if (buffer_cnt_change) {
        ret = self->decoder->FreeOutputBuffers();
        g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "freebuffer", TRUE), GST_FLOW_ERROR);
        g_object_set(self->outpool, "dynamic-buffer-num", self->out_buffer_cnt, nullptr);
    } else {
        ret = self->decoder->FreeOutputBuffers();
        g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "freebuffer", TRUE), GST_FLOW_ERROR);
    }

    ret = self->decoder->ActiveBufferMgr(GST_CODEC_OUTPUT, true);
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "ActiveBufferMgr", TRUE), GST_FLOW_ERROR);
    g_return_val_if_fail(gst_vdec_base_allocate_out_buffers(self), GST_FLOW_ERROR);
    ret = self->decoder->Start();
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "Start", TRUE), GST_FLOW_ERROR);
    GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: format change end");
    return GST_FLOW_OK;
}

static GstFlowReturn gst_vdec_base_codec_eos(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Eos");
    g_return_val_if_fail(self != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(GST_VIDEO_DECODER_SRC_PAD(self) != nullptr, GST_FLOW_ERROR);
    g_mutex_lock(&self->drain_lock);
    if (self->draining) {
        GstQuery *query = gst_query_new_drain();

        if (!gst_pad_peer_query(GST_VIDEO_DECODER_SRC_PAD(self), query)) {
            GST_WARNING_OBJECT(self, "drain query failed");
        }
        gst_query_unref(query);

        GST_DEBUG_OBJECT(self, "Drained");
        self->draining = FALSE;
        g_cond_broadcast(&self->drain_cond);
    }
    g_mutex_unlock(&self->drain_lock);
    gst_vdec_base_pause_loop(self);
    return GST_FLOW_OK;
}

static void gst_vdec_base_pause_loop(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Pause loop start");
    g_return_if_fail(self != nullptr);
    g_mutex_lock(&self->drain_lock);
    if (self->draining) {
        self->draining = FALSE;
        g_cond_broadcast(&self->drain_cond);
    }
    g_mutex_unlock(&self->drain_lock);
    GST_DEBUG_OBJECT(self, "pause loop end");
    gst_pad_pause_task(GST_VIDEO_DECODER_SRC_PAD(self));
}

static gboolean gst_vdec_base_push_out_buffers(GstVdecBase *self)
{
    MediaTrace trace("VdecBase::PushOutputBuffer");
    GST_DEBUG_OBJECT(self, "Push out buffers");
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    GstBuffer *buffer = nullptr;
    GstBufferPool *pool = gst_video_decoder_get_buffer_pool(GST_VIDEO_DECODER(self));
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    GstFlowReturn flow = GST_FLOW_OK;
    GstBufferPoolAcquireParams params;
    g_return_val_if_fail(memset_s(&params, sizeof(params), 0, sizeof(params)) == EOK, FALSE);
    if (self->coding_outbuf_cnt > BLOCKING_ACQUIRE_BUFFER_THRESHOLD) {
        params.flags = GST_BUFFER_POOL_ACQUIRE_FLAG_DONTWAIT;
    }
    while (flow == GST_FLOW_OK) {
        flow = gst_buffer_pool_acquire_buffer(pool, &buffer, &params);
        if (flow == GST_FLOW_OK) {
            g_return_val_if_fail(buffer != nullptr, FALSE);
            ON_SCOPE_EXIT(1) {
                gst_buffer_unref(buffer);
            };
            gint codec_ret = self->decoder->PushOutputBuffer(buffer);
            g_return_val_if_fail(codec_ret != GST_CODEC_FLUSH, FALSE);
            g_return_val_if_fail(gst_codec_return_is_ok(self, codec_ret, "push buffer", TRUE), FALSE);
            self->coding_outbuf_cnt++;
            params.flags = GST_BUFFER_POOL_ACQUIRE_FLAG_DONTWAIT;
        }
    }
    if (flow != GST_FLOW_EOS) {
        GST_ELEMENT_ERROR(self, STREAM, DECODE, ("hardware decoder error!"), ("%s", "acquire buffer"));
        return FALSE;
    }
    return TRUE;
}

inline static void gst_vdec_reduce_coding_buffer(GstVdecBase *self)
{
    if (self->memtype != GST_MEMTYPE_SURFACE) {
        self->coding_outbuf_cnt--;
    }
}

static gint gst_vdec_base_loop_process(gint codec_ret, GstVdecBase *self, GstBuffer *gst_buffer)
{
    gint flow_ret = GST_FLOW_OK;
    switch (codec_ret) {
        case GST_CODEC_OK:
            gst_vdec_reduce_coding_buffer(self);
            flow_ret = push_output_buffer(self, gst_buffer);
            break;
        case GST_CODEC_FORMAT_CHANGE:
            (void)gst_vdec_base_format_change(self);
            flow_ret = GST_FLOW_OK;
            break;
        case GST_CODEC_EOS:
            gst_vdec_reduce_coding_buffer(self);
            flow_ret = gst_vdec_base_codec_eos(self);
            break;
        case GST_CODEC_FLUSH:
            flow_ret = GST_FLOW_FLUSHING;
            break;
        case GST_CODEC_ERROR:
            GST_ELEMENT_WARNING(self, STREAM, DECODE, ("Hardware decoder error!"), ("pull"));
            flow_ret = GST_FLOW_ERROR;
            break;
        default:
            flow_ret = GST_FLOW_ERROR;
            GST_ERROR_OBJECT(self, "Unknown error");
    }
    return flow_ret;
}

static void gst_vdec_base_loop(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "Loop in");
    g_return_if_fail(self != nullptr);
    g_return_if_fail(self->decoder != nullptr);

    pthread_setname_np(pthread_self(), "VDecOutPut");
    GstBuffer *gst_buffer = nullptr;
    if (self->memtype != GST_MEMTYPE_SURFACE) {
        ON_SCOPE_EXIT(0) { gst_vdec_base_pause_loop(self); };
        g_return_if_fail(gst_vdec_base_push_out_buffers(self) == TRUE);
        CANCEL_SCOPE_EXIT_GUARD(0);
        GST_DEBUG_OBJECT(self, "coding buffers %u", self->coding_outbuf_cnt);
    }
    gint codec_ret = GST_CODEC_OK;
    {
        MediaTrace trace("VdecBase::PullOutputBuffer");
        codec_ret = self->decoder->PullOutputBuffer(&gst_buffer);
    }

    GST_DEBUG_OBJECT(self, "Pull ret %d", codec_ret);
    gint flow_ret = gst_vdec_base_loop_process(codec_ret, self, gst_buffer);
    switch (flow_ret) {
        case GST_FLOW_OK:
            return;
        case GST_FLOW_FLUSHING:
            GST_DEBUG_OBJECT(self, "Flushing");
            break;
        case GST_FLOW_EOS:
            GST_DEBUG_OBJECT(self, "Eos");
            gst_pad_push_event(GST_VIDEO_DECODER_SRC_PAD(self), gst_event_new_eos());
            break;
        default:
            gst_pad_push_event(GST_VIDEO_DECODER_SRC_PAD(self), gst_event_new_eos());
    }
    gst_vdec_base_pause_loop(self);
}

static GstCaps* gst_vdec_swap_width_height(GstCaps *caps)
{
    caps = gst_caps_make_writable(caps);
    g_return_val_if_fail(caps != nullptr, nullptr);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    g_return_val_if_fail(structure != nullptr, caps);

    const GValue *width = gst_structure_get_value(structure, "width");
    const GValue *height = gst_structure_get_value(structure, "height");
    GValue temp_width = G_VALUE_INIT;
    gst_value_init_and_copy (&temp_width, width);
    width = &temp_width;
    gst_structure_set_value(structure, "width", height);
    gst_structure_set_value(structure, "height", width);
    return caps;
}

static gboolean gst_vdec_caps_fix_sink_caps(GstVdecBase *self)
{
    GstCaps *templ_caps = gst_pad_get_pad_template_caps(GST_VIDEO_DECODER_SRC_PAD(self));
    g_return_val_if_fail(templ_caps != nullptr, FALSE);
    ON_SCOPE_EXIT(0) { gst_caps_unref(templ_caps); };
    GstCaps *pool_caps = gst_caps_intersect(self->sink_caps, templ_caps);
    gboolean is_caps_valid = TRUE;
    if (gst_caps_is_empty(pool_caps)) {
        is_caps_valid = FALSE;
        if (self->is_support_swap_width_height) {
            templ_caps = gst_vdec_swap_width_height(templ_caps);
            gst_caps_unref(pool_caps);
            pool_caps = gst_caps_intersect(self->sink_caps, templ_caps);
            if (!gst_caps_is_empty(pool_caps)) {
                is_caps_valid = TRUE;
            }
        }
    }
    if (!is_caps_valid) {
        gst_caps_unref(pool_caps);
        return FALSE;
    }
    pool_caps = gst_caps_fixate(pool_caps);
    g_return_val_if_fail(pool_caps != nullptr, FALSE);
    gst_caps_unref(self->sink_caps);
    self->sink_caps = pool_caps;

    GstStructure *structure = gst_caps_get_structure(pool_caps, 0);
    const gchar *format_str = gst_structure_get_string(structure, "format");
    g_return_val_if_fail(format_str != nullptr, FALSE);

    GstVideoFormat format = gst_video_format_from_string(format_str);
    g_return_val_if_fail(format != GST_VIDEO_FORMAT_UNKNOWN, FALSE);

    self->format = format;
    gint ret = self->decoder->SetParameter(GST_VIDEO_FORMAT, GST_ELEMENT(self));
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "setparameter", TRUE), FALSE);
    return TRUE;
}

static gboolean gst_vdec_base_pre_init_surface(GstVdecBase *self)
{
    GST_DEBUG_OBJECT(self, "surface pre init");
    g_return_val_if_fail(self != nullptr, FALSE);
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    g_return_val_if_fail(self->sink_caps != nullptr, FALSE);
    g_return_val_if_fail(self->outpool != nullptr, FALSE);

    guint size = 0;
    GstStructure *structure = gst_caps_get_structure(self->sink_caps, 0);
    g_return_val_if_fail(structure != nullptr, FALSE);
    gst_structure_set(structure, "width", G_TYPE_INT, self->width, nullptr);
    gst_structure_set(structure, "height", G_TYPE_INT, self->height, nullptr);

    self->memtype = GST_MEMTYPE_SURFACE;
    gint ret = self->decoder->SetParameter(GST_VIDEO_SURFACE_INIT, GST_ELEMENT(self));
    g_return_val_if_fail(gst_codec_return_is_ok(self, ret, "GST_VIDEO_SURFACE_INIT", TRUE), FALSE);

    if (!gst_vdec_caps_fix_sink_caps(self)) {
        g_signal_emit(self, signals[SIGNAL_CAPS_FIX_ERROR], 0, nullptr);
        return FALSE;
    }
    gst_vdec_base_update_out_port_def(self, &size);
    gst_vdec_base_update_out_pool(self, &self->outpool, self->sink_caps, size);
    gst_buffer_pool_set_active(self->outpool, TRUE);
    self->pre_init_pool = TRUE;
    return TRUE;
}

static gboolean gst_vdec_base_set_format(GstVideoDecoder *decoder, GstVideoCodecState *state)
{
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    g_return_val_if_fail(self != nullptr, FALSE);
    if (self->prepared || self->has_set_format) {
        return TRUE;
    }
    g_return_val_if_fail(self->decoder != nullptr, FALSE);
    g_return_val_if_fail(state != nullptr, FALSE);
    GstVideoInfo *info = &state->info;
    g_return_val_if_fail(info != nullptr, FALSE);
    gboolean is_format_change = FALSE;
    gint ret = self->decoder->GetParameter(GST_VIDEO_INPUT_COMMON, GST_ELEMENT(self));
    g_return_val_if_fail(ret == GST_CODEC_OK, FALSE);
    GST_INFO_OBJECT(self, "Input params is min buffer count %u, buffer count %u, buffer size is %u",
        self->input.min_buffer_cnt, self->input.buffer_cnt, self->input.buffer_size);

    GST_DEBUG_OBJECT(self, "Setting new caps");

    constexpr gfloat epsilon = 1e-6;
    is_format_change = is_format_change || self->width != info->width;
    is_format_change = is_format_change || self->height != GST_VIDEO_INFO_FIELD_HEIGHT(info);
    is_format_change = is_format_change || (info->fps_n != 0 && self->frame_rate < epsilon &&
        self->frame_rate > -epsilon);

    if (is_format_change && info->width != 0 && GST_VIDEO_INFO_FIELD_HEIGHT(info) != 0) {
        self->width = info->width;
        self->height = GST_VIDEO_INFO_FIELD_HEIGHT(info);
        self->real_stride = self->width;
        self->real_stride_height = self->height;
        self->frame_rate  = static_cast<gfloat>(info->fps_n) / static_cast<gfloat>(info->fps_d);
        gst_vdec_base_post_resolution_changed_message(self, TRUE);
        GST_DEBUG_OBJECT(self, "width: %d, height: %d, frame_rate: %.3f", self->width, self->height, self->frame_rate);
    }

    GST_DEBUG_OBJECT(self, "Setting input port definition");
    ret = self->decoder->SetParameter(GST_VIDEO_INPUT_COMMON, GST_ELEMENT(self));
    g_return_val_if_fail(ret == GST_CODEC_OK, FALSE);
    ret = self->decoder->GetParameter(GST_VIDEO_INPUT_COMMON, GST_ELEMENT(self));
    g_return_val_if_fail(ret == GST_CODEC_OK, FALSE);
    self->input_state = gst_video_codec_state_ref(state);
    if (self->performance_mode) {
        g_return_val_if_fail(gst_vdec_base_pre_init_surface(self) != FALSE, FALSE);
    }
    self->has_set_format = TRUE;
    if (state->codec_data != nullptr && state->codec_data != self->codec_data) {
        gst_buffer_unref(self->codec_data);
        self->codec_data = gst_buffer_ref(state->codec_data);
        self->codec_data_update = TRUE;
    }

    return TRUE;
}

static GstFlowReturn gst_vdec_base_finish(GstVideoDecoder *decoder)
{
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    g_return_val_if_fail(self != nullptr && self->decoder != nullptr, GST_FLOW_ERROR);
    GST_DEBUG_OBJECT(self, "Finish codec start");
    GstPad *pad = GST_VIDEO_DECODER_SRC_PAD(self);
    if (gst_pad_get_task_state(pad) != GST_TASK_STARTED) {
        GST_DEBUG_OBJECT(self, "vdec not start yet");
        return GST_FLOW_OK;
    }
    GstVdecBaseClass *kclass = GST_VDEC_BASE_GET_CLASS(self);
    if (kclass->handle_slice_buffer != nullptr && self->player_scene == true) {
        bool ready_push_slice_buffer = false;
        GstBuffer *cat_buffer = kclass->handle_slice_buffer(self, nullptr, ready_push_slice_buffer, true);
        if (cat_buffer != nullptr && ready_push_slice_buffer == true) {
            GST_VIDEO_DECODER_STREAM_UNLOCK(self);
            gint codec_ret = GST_CODEC_OK;
            if (!gst_vdec_check_ashmem_buffer(cat_buffer) && self->input_need_ashmem) {
                codec_ret = gst_vdec_base_push_input_buffer_with_copy(self, cat_buffer, FALSE);
            } else {
                gst_vdec_base_dump_input_buffer(self, cat_buffer);
                codec_ret = self->decoder->PushInputBuffer(cat_buffer);
            }
            gst_codec_return_is_ok(self, codec_ret, "Finish push buffer failed", FALSE);
            gst_buffer_unref(cat_buffer);
        }
    } else {
        GST_VIDEO_DECODER_STREAM_UNLOCK(self);
    }
    
    g_mutex_lock(&self->drain_lock);
    ON_SCOPE_EXIT(0) {
        g_mutex_unlock(&self->drain_lock);
        GST_VIDEO_DECODER_STREAM_LOCK(self);
    };
    self->draining = TRUE;
    gint ret = self->decoder->PushInputBuffer(nullptr);
    g_return_val_if_fail(ret == GST_CODEC_OK, GST_FLOW_ERROR);
    gint64 wait_time = std::min(DRAIN_TIME_OUT_MAX,
        std::max(DRAIN_TIME_OUT_MIN, DRAIN_TIME_OUT_BY_FRAMERATE(self->frame_rate)));
    gint64 wait_until = g_get_monotonic_time() + wait_time;
    GST_DEBUG_OBJECT(self, "Waiting until codec is drained, wait %" G_GINT64_FORMAT " us", wait_time);
    if (!g_cond_wait_until(&self->drain_cond, &self->drain_lock, wait_until)) {
        GST_ERROR_OBJECT(self, "Drain timed out");
    } else {
        GST_DEBUG_OBJECT(self, "finish hdi end");
    }

    return GST_FLOW_OK;
}

static GstFlowReturn gst_vdec_base_drain(GstVideoDecoder *decoder)
{
    GST_DEBUG_OBJECT(decoder, "Draining codec start");
    g_return_val_if_fail(decoder != nullptr, GST_FLOW_ERROR);
    GstFlowReturn ret = gst_vdec_base_finish(decoder);
    gst_vdec_base_flush(decoder);
    GST_DEBUG_OBJECT(decoder, "Draining codec end");
    return ret;
}

static void gst_vdec_base_event_flush_start(GstVideoDecoder *decoder)
{
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: flush start");
    if (!self->prepared) {
        gst_vdec_base_set_flushing(self, TRUE);
        return;
    }
    g_mutex_lock(&self->format_changed_lock);
    GST_VIDEO_DECODER_STREAM_LOCK(self);
    GstVdecBaseClass *kclass = GST_VDEC_BASE_GET_CLASS(self);
    if (kclass->flush_cache_slice_buffer != nullptr) {
        (void)kclass->flush_cache_slice_buffer(self);
    }
    gst_vdec_base_set_flushing(self, TRUE);
    self->decoder_start = FALSE;
    self->idrframe = FALSE;
    if (self->decoder != nullptr) {
        (void)self->decoder->Flush(GST_CODEC_ALL);
    }
    GST_VIDEO_DECODER_STREAM_UNLOCK(self);
    g_mutex_unlock(&self->format_changed_lock);
}

static gboolean gst_vdec_base_event_flush_stop(GstVideoDecoder *decoder, GstEvent *event)
{
    gboolean ret = TRUE;
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    self->flushing_stoping = TRUE;
    ret = GST_VIDEO_DECODER_CLASS(parent_class)->sink_event(decoder, event);
    {
        g_mutex_lock(&self->lock);
        std::list<GstClockTime> empty;
        self->pts_list.swap(empty);
        self->last_pts = GST_CLOCK_TIME_NONE;
        g_mutex_unlock(&self->lock);
    }
    gst_vdec_base_set_flushing(self, FALSE);
    self->flushing_stoping = FALSE;
    GST_WARNING_OBJECT(self, "KPI-TRACE-VDEC: flush stop");
    return ret;
}

static gboolean gst_vdec_base_event(GstVideoDecoder *decoder, GstEvent *event)
{
    g_return_val_if_fail(decoder != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);
    g_return_val_if_fail(GST_VIDEO_DECODER_CLASS(parent_class) != nullptr, FALSE);
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    GST_DEBUG_OBJECT(self, "Gst_vdec_base_sink_event, type=%s", GST_EVENT_TYPE_NAME(event));

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_FLUSH_START:
            gst_vdec_base_event_flush_start(decoder);
            break;
        case GST_EVENT_FLUSH_STOP:
            return gst_vdec_base_event_flush_stop(decoder, event);
        case GST_EVENT_EOS:
            self->is_eos_state = TRUE;
            break;
        default:
            break;
    }

    return GST_VIDEO_DECODER_CLASS(parent_class)->sink_event(decoder, event);
}

static GstBufferPool *gst_vdec_base_new_out_shmem_pool(GstVdecBase *self, GstCaps *outcaps, gint size)
{
    g_return_val_if_fail(outcaps != nullptr, nullptr);
    GstShMemPool *pool = gst_shmem_pool_new();
    g_return_val_if_fail(pool != nullptr, nullptr);
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    g_return_val_if_fail(self->output.allocator != nullptr, nullptr);

    self->output.av_shmem_pool = std::make_shared<OHOS::Media::AVSharedMemoryPool>("vdec_out");
    (void)gst_shmem_pool_set_avshmempool(pool, self->output.av_shmem_pool);
    (void)gst_shmem_allocator_set_pool(self->output.allocator, self->output.av_shmem_pool);
    GstStructure *config = gst_buffer_pool_get_config(GST_BUFFER_POOL_CAST(pool));
    g_return_val_if_fail(config != nullptr, nullptr);
    gst_buffer_pool_config_set_allocator(config, GST_ALLOCATOR_CAST(self->output.allocator),
            &self->output.allocParams);
    gst_buffer_pool_config_set_params(config, outcaps, size, self->out_buffer_cnt, self->out_buffer_cnt);
    g_return_val_if_fail(gst_buffer_pool_set_config(GST_BUFFER_POOL_CAST(pool), config), nullptr);
    CANCEL_SCOPE_EXIT_GUARD(0);
    return GST_BUFFER_POOL(pool);
}

static GstBufferPool *gst_vdec_base_new_in_shmem_pool(GstVdecBase *self, GstCaps *outcaps, gint size,
    guint min_buffer_cnt, guint max_buffer_cnt)
{
    g_return_val_if_fail(outcaps != nullptr, nullptr);
    GstShMemPool *pool = gst_shmem_pool_new();
    g_return_val_if_fail(pool != nullptr, nullptr);
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    g_return_val_if_fail(self->input.allocator != nullptr, nullptr);

    self->input.av_shmem_pool = std::make_shared<OHOS::Media::AVSharedMemoryPool>("vdec_in");
    (void)gst_shmem_pool_set_avshmempool(pool, self->input.av_shmem_pool);
    (void)gst_shmem_allocator_set_pool(self->input.allocator, self->input.av_shmem_pool);
    GstStructure *config = gst_buffer_pool_get_config(GST_BUFFER_POOL_CAST(pool));
    g_return_val_if_fail(config != nullptr, nullptr);
    gst_buffer_pool_config_set_allocator(config, GST_ALLOCATOR_CAST(self->input.allocator), &self->input.allocParams);
    gst_buffer_pool_config_set_params(config, outcaps, size, min_buffer_cnt, max_buffer_cnt);
    g_return_val_if_fail(gst_buffer_pool_set_config(GST_BUFFER_POOL_CAST(pool), config), nullptr);
    CANCEL_SCOPE_EXIT_GUARD(0);
    return GST_BUFFER_POOL(pool);
}

static void gst_vdec_base_update_out_pool(GstVdecBase *self, GstBufferPool **pool, GstCaps *outcaps,
    gint size)
{
    g_return_if_fail(self != nullptr);
    g_return_if_fail(self->decoder != nullptr);
    g_return_if_fail(*pool != nullptr);
    GST_INFO_OBJECT(self, "vdec memType %d", self->memtype);
    ON_SCOPE_EXIT(0) { gst_object_unref(*pool); *pool = nullptr; };
    GstStructure *config = gst_buffer_pool_get_config(*pool);
    g_return_if_fail(config != nullptr);
    gst_buffer_pool_config_set_params(config, outcaps, size, self->out_buffer_cnt, self->out_buffer_cnt);
    if (self->memtype == GST_MEMTYPE_SURFACE) {
        gst_structure_set(config, "usage", G_TYPE_UINT64, self->usage, nullptr);
        self->decoder->SetOutputPool(*pool);
    }
    g_return_if_fail(gst_buffer_pool_set_config(*pool, config));
    CANCEL_SCOPE_EXIT_GUARD(0);
}

static gboolean gst_vdec_base_check_mem_type(GstVdecBase *self, GstQuery *query)
{
    guint index = 0;
    const GstStructure *buffer_type_struct = nullptr;
    if (gst_query_find_allocation_meta(query, GST_BUFFER_TYPE_META_API_TYPE, &index)) {
        (void)gst_query_parse_nth_allocation_meta(query, index, &buffer_type_struct);
        (void)get_memtype(self, buffer_type_struct);
    } else {
        GST_INFO_OBJECT(self, "No meta api");
        return FALSE;
    }
    return TRUE;
}

static gboolean gst_vdec_base_decide_allocation_with_pre_init_pool(GstVdecBase *self, GstQuery *query)
{
    g_return_val_if_fail(self != nullptr && query != nullptr, FALSE);
    GST_DEBUG_OBJECT(self, "Performance mode");
    guint pool_num = gst_query_get_n_allocation_pools(query);
    GstBufferPool *pool = self->outpool;
    guint size = self->output.buffer_size;
    if (pool_num > 0) {
        gst_query_set_nth_allocation_pool(query, 0, pool, size, self->output.buffer_cnt, self->output.buffer_cnt);
    } else {
        gst_query_add_allocation_pool(query, pool, size, self->output.buffer_cnt, self->output.buffer_cnt);
    }
    return TRUE;
}

static gboolean gst_vdec_base_decide_allocation(GstVideoDecoder *decoder, GstQuery *query)
{
    g_return_val_if_fail(decoder != nullptr && query != nullptr, FALSE);
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    if (self->performance_mode && self->pre_init_pool) {
        self->pre_init_pool = FALSE;
        return gst_vdec_base_decide_allocation_with_pre_init_pool(self, query);
    }
    GstCaps *outcaps = nullptr;
    GstVideoInfo vinfo;
    GstBufferPool *pool = nullptr;
    guint size = 0;
    guint min_buf = 0;
    guint max_buf = 0;
    gst_query_parse_allocation(query, &outcaps, nullptr);
    gst_video_info_init(&vinfo);
    if (outcaps != nullptr) {
        gst_video_info_from_caps(&vinfo, outcaps);
    }
    gboolean update_pool = FALSE;
    guint pool_num = gst_query_get_n_allocation_pools(query);
    if (pool_num > 0) {
        gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &min_buf, &max_buf);
        if (gst_vdec_base_check_mem_type(self, query) != TRUE) {
            gst_object_unref(pool);
            pool = nullptr;
        }
        update_pool = TRUE;
    }
    size = vinfo.size;
    self->out_buffer_max_cnt = max_buf == 0 ? DEFAULT_MAX_QUEUE_SIZE : max_buf;
    g_return_val_if_fail(gst_vdec_base_update_out_port_def(self, &size), FALSE);
    if (pool == nullptr) {
        pool = gst_vdec_base_new_out_shmem_pool(self, outcaps, size);
    } else {
        gst_vdec_base_update_out_pool(self, &pool, outcaps, size);
    }
    g_return_val_if_fail(pool != nullptr, FALSE);
    if (update_pool) {
        gst_query_set_nth_allocation_pool(query, 0, pool, size, self->output.buffer_cnt, self->output.buffer_cnt);
    } else {
        gst_query_add_allocation_pool(query, pool, size, self->output.buffer_cnt, self->output.buffer_cnt);
    }
    GST_DEBUG_OBJECT(decoder, "Pool ref %u", (reinterpret_cast<GObject*>(pool)->ref_count));
    gst_object_unref(self->outpool);
    self->outpool = pool;
    return TRUE;
}

static gboolean gst_vdec_base_check_allocate_input(GstVdecBase *self)
{
    MediaTrace trace("VdecBase::AllocateInputBuffer");
    g_return_val_if_fail(self != nullptr, FALSE);
    if (self->inpool == nullptr) {
        mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
        mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
        self->inpool = gst_vdec_base_new_in_shmem_pool(self, self->input_state->caps, self->input.buffer_size,
                self->input.buffer_cnt, self->input.buffer_cnt);
        gst_buffer_pool_set_active(self->inpool, TRUE);
        g_return_val_if_fail(self->inpool != nullptr, FALSE);
        g_return_val_if_fail(gst_vdec_base_allocate_in_buffers(self), FALSE);
    }
    if (self->drm_ashmem_infd == (gint)DRM_ASHMEM_INVALID_FD) {
        self->drm_ashmem_infd = AshmemCreate(g_drm_ashmem_inbuf_name, self->input.buffer_size);
    }
    if (self->drm_ashmem_outfd == (gint)DRM_ASHMEM_INVALID_FD) {
        self->drm_ashmem_outfd = AshmemCreate(g_drm_ashmem_outbuf_name, self->input.buffer_size);
    }
    return TRUE;
}

static gboolean gst_vdec_base_propose_allocation(GstVideoDecoder *decoder, GstQuery *query)
{
    g_return_val_if_fail(decoder != nullptr, FALSE);
    g_return_val_if_fail(query != nullptr, FALSE);
    GstVdecBase *self = GST_VDEC_BASE(decoder);
    GstCaps *incaps = nullptr;
    GstBufferPool *pool = nullptr;
    guint size = 0;
    gst_query_parse_allocation(query, &incaps, nullptr);
    guint pool_num = gst_query_get_n_allocation_pools(query);
    size = self->input.buffer_size;
    pool = gst_vdec_base_new_in_shmem_pool(self, incaps, size, self->input.buffer_cnt, self->input.buffer_cnt);
    g_return_val_if_fail(pool != nullptr, FALSE);
    if (pool_num > 0) {
        gst_query_set_nth_allocation_pool(query, 0, pool, size, self->input.buffer_cnt, self->input.buffer_cnt);
    } else {
        gst_query_add_allocation_pool(query, pool, size, self->input.buffer_cnt, self->input.buffer_cnt);
    }
    gst_vdec_reset_inpool(self);
    self->inpool = pool;
    GST_DEBUG_OBJECT(decoder, "pool ref_count %u", ((GObject *)pool)->ref_count);
    gst_buffer_pool_set_active(pool, TRUE);
    g_return_val_if_fail(gst_vdec_base_allocate_in_buffers(self), FALSE);
    gst_query_add_allocation_meta(query, GST_BUFFER_TYPE_META_API_TYPE, nullptr);
    return TRUE;
}
