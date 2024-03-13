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

#include "gst_vdec_base_cenc_dec.h"
#include <unistd.h>
#include <gst/gstprotection.h>
#include <gst/base/gstbytereader.h>
#include <sys/mman.h>
#include "securec.h"
#include "buffer_type_meta.h"
#include "media_log.h"

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

static const guint8 g_video_frame_arr[DRM_VIDEO_FRAME_ARR_LEN] = { 0x00, 0x00, 0x01 };

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
    errno_t rek;
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
    rek = memset_s(iv_data, *iv_data_size, 0, *iv_data_size);
    if (rek != EOK) {
        GST_ERROR_OBJECT(self, "iv memset failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    rek = memcpy_s(iv_data, *iv_data_size, iv_map.data, iv_map.size);
    if (rek != EOK) {
        GST_ERROR_OBJECT(self, "iv memcpy failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
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
    errno_t rek;

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
        GST_ERROR_OBJECT(self, "keyid size err\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    *keyid_data_size = keyid_map.size;
    rek = memset_s(keyid_data, *keyid_data_size, 0, *keyid_data_size);
    if (rek != EOK) {
        GST_ERROR_OBJECT(self, "keyid memset failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
    rek = memcpy_s(keyid_data, *keyid_data_size, keyid_map.data, keyid_map.size);
    if (rek != EOK) {
        GST_ERROR_OBJECT(self, "keyid memcpy failed\n");
        ret = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }
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
    in_ashmem_buf = static_cast<guint8 *>(mmap(nullptr, map.size, PROT_READ | PROT_WRITE, MAP_SHARED,
        in_self->drm_ashmem_infd, 0));
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
    out_ashmem_buf = static_cast<guint8 *>(mmap(nullptr, map.size, PROT_READ | PROT_WRITE, MAP_SHARED,
        out_self->drm_ashmem_outfd, 0));
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

GstFlowReturn gst_vdec_base_drm_cenc_decrypt(GstVideoDecoder *decoder, GstBuffer *buf)
{
    GstVdecBase *self = nullptr;
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

    if (decoder == nullptr) {
        GST_ERROR_OBJECT(nullptr, "decoder is null\n");
        return GST_FLOW_NOT_SUPPORTED;
    }
    self = GST_VDEC_BASE(decoder);
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

    g_signal_emit_by_name((GstElement *)self, "media-decrypt", static_cast<gint64>(self->drm_ashmem_infd),
        static_cast<gint64>(self->drm_ashmem_outfd), map.size, keyid_data, keyid_size, iv_data, iv_size,
        subsample_count, subsample_info, algo, 0, crypt_byte_block, skip_byte_block, &res);

    gst_vdec_base_ashmem_copy_to_gst_buffer(self, buf);

    gst_buffer_unmap(buf, &map);
    return ret;
}
