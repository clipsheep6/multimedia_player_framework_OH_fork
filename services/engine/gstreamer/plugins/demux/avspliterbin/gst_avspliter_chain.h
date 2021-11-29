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

#ifndef GST_AVSPLITER_CHAIN_H
#define GST_AVSPLITER_CHAIN_H

#include <gst/gst.h>
#include "gst_avspliter_pad.h"

typedef struct _GstAVSpliterBin GstAVSpliterBin;
typedef struct _GstAVSpliterChain GstAVSpliterChain;

struct _GstAVSpliterChain {
    GstAVSpliterChain *parentChain;
    GstAVSpliterBin *avspliter;
    GList *childChains;
    GMutex lock;
    GstPad *startPad;
    GstCaps *startCaps;
    gboolean isDemuxer;
    gboolean hasParser;
    GList *elements;
    GList *pendingPads;
    GstAVSpliterPad *currentPad;
    GstAVSpliterPad *endPad;
    GstCaps *endCaps;
    gboolean noMorePads;
    gboolean isDead;
    gchar *deadDetails;
};

G_GNUC_INTERNAL GstAVSpliterChain *gst_avspliter_chain_new(
    GstAVSpliterBin *avspliter, GstAVSpliterChain *parent, GstPad *startPad, GstCaps *startCaps);
G_GNUC_INTERNAL void gst_avspliter_chain_free(GstAVSpliterChain *chain);
G_GNUC_INTERNAL void gst_avspliter_chain_analyze_new_pad(
    GstElement *src, GstPad *pad, GstCaps *caps, GstAVSpliterChain *chain);
G_GNUC_INTERNAL gboolean gst_avspliter_chain_is_complete(GstAVSpliterChain *chain);
G_GNUC_INTERNAL gboolean gst_avspliter_chain_expose(GstAVSpliterChain *chain, GList **endpads,
    gboolean *missingPlugin, GString *missingPluginDetails);

#define CHAIN_LOCK(chain)                                                                             \
    do {                                                                                              \
        GST_LOG_OBJECT(chain->avspliter, "locking chain 0x%06" PRIxPTR " from thread 0x%06" PRIxPTR,  \
                       FAKE_POINTER(chain), FAKE_POINTER(g_thread_self()));                           \
        g_mutex_lock(&chain->lock);                                                                   \
        GST_LOG_OBJECT(chain->avspliter, "locked chain 0x%06" PRIXPTR " from thread 0x%06" PRIxPTR,   \
                       FAKE_POINTER(chain), FAKE_POINTER(g_thread_self()));                           \
    } while(0)

#define CHAIN_UNLOCK(chain)                                                                           \
    do {                                                                                              \
        GST_LOG_OBJECT(chain->avspliter, "unlocking chain 0x%06" PRIXPTR "from thread 0x%06" PRIxPTR, \
                       FAKE_POINTER(chain), FAKE_POINTER(g_thread_self()));                           \
        g_mutex_lock(&chain->lock);                                                                   \
    } while (0)

#endif
