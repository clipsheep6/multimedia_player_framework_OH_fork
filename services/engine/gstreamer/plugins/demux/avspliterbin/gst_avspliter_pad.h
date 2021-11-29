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

#ifndef GST_AVSPLITER_PAD_H
#define GST_AVSPLITER_PAD_H

#include <gst/gst.h>

#define GST_TYPE_AVSPLITER_PAD (gst_avspliter_pad_get_type())
#define GST_AVSPLITER_PAD(obj) (G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_AVSPLITER_PAD, GstAVSpliterPad))
#define GST_AVSPLITER_PAD_CAST(obj) (GstAVSpliterPad *)(obj)

typedef struct _GstAVSpliterPad GstAVSpliterPad;
typedef GstGhostPadClass GstAVSpliterPadClass;
typedef struct _GstAVSpliterBin GstAVSpliterBin;
typedef struct _GstAVSpliterChain GstAVSpliterChain;

struct _GstAVSpliterPad {
    GstGhostPad ghostPad;   // derived to GstGhostPad
    GstAVSpliterBin *avspliter;
    GstAVSpliterChain *chain;
    gboolean blocked;
    gboolean exposed;
    gulong blockId;
    GstStream *activeStream;
};

G_GNUC_INTERNAL GType gst_avspliter_pad_get_type(void);

G_GNUC_INTERNAL GstAVSpliterPad *gst_avspliter_pad_new(GstAVSpliterChain *chain, GstStaticPadTemplate *tmpl);
G_GNUC_INTERNAL void gst_avspliter_pad_active(GstAVSpliterPad *avspliterPad, GstAVSpliterChain *chain);
G_GNUC_INTERNAL void gst_avspliter_pad_set_block(GstAVSpliterPad *avspliterPad, gboolean blocked);
G_GNUC_INTERNAL void gst_avspliter_pad_do_set_block(GstAVSpliterPad *avspliterPad, gboolean needBlock);
G_GNUC_INTERNAL void gst_avspliter_pad_set_target(GstAVSpliterPad *avspliterPad, GstPad *target);

#endif