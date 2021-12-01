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

#ifndef GST_AVSPLITER_BIN_PRIV_H
#define GST_AVSPLITER_BIN_PRIV_H

#include <gst/gst.h>
#include "gst_avspliter_pad.h"
#include "gst_avspliter_chain.h"

#define GST_TYPE_AVSPLITER_BIN (gst_avspliter_bin_get_type())
#define GST_AVSPLITER_BIN_CAST(obj) ((GstAVSpliterBin *)(obj))
#define GST_AVSPLITER_BIN(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AVSPLITER_BIN, GstAVSpliterBin))
#define GST_AVSPLITER_BIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_AVSPLITER_BIN, GstAVSpliterBinClass))
#define GST_IS_AVSPLITER_BIN(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AVSPLITER_BIN))
#define GST_IS_AVSPLITER_BIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_AVSPLITER_BIN))

typedef struct _GstAVSpliterBin GstAVSpliterBin;
typedef struct _GstAVSpliterBinClass GstAVSpliterBinClass;

struct _GstAVSpliterBin {
    GstBin bin;
    GMutex factoriesLock;
    guint32 factoriesCookie;
    GList *factories;
    GstElement *typefind;
    GMutex exposeLock;
    GstAVSpliterChain *chain;
    gboolean haveType;
    guint haveTypeSignalId;
    GMutex dynlock;
    gboolean shutDown;
    GList *blockedPads;
    GList *filtered;
    GList *filteredErrors;
    guint npads;
    GstStaticPadTemplate *srcTmpl;
};

struct _GstAVSpliterBinClass {
    GstBinClass parent_class;

    gboolean (*autoplug_continue)(GstElement *element, GstPad *pad, GstCaps *caps);
    void (*Drained)(GstElement *element);
};

#define DYN_LOCK(avspliter)                                                             \
    do {                                                                                \
        GST_LOG_OBJECT(avspliter, "dynlock locking from thread %06" PRIxPTR,            \
                       FAKE_POINTER(g_thread_self()));                                  \
        g_mutex_lock(&GST_AVSPLITER_BIN_CAST(avspliter)->dynlock);                      \
        GST_LOG_OBJECT(avspliter, "dynlock locked from thread 0x%06" PRIxPTR,           \
                       FAKE_POINTER(g_thread_self()));                                  \
    } while(0)

#define DYN_UNLOCK(avspliter)                                                           \
    do {                                                                                \
        GST_LOG_OBJECT(avspliter, "dynlock unlocking from thread 0x%06" PRIxPTR,        \
                       FAKE_POINTER(g_thread_self()));                                  \
        g_mutex_unlock(&GST_AVSPLITER_BIN_CAST(avspliter)->dynlock);                    \
    } while (0)

#define EXPOSE_LOCK(avspliter)                                                          \
    do {                                                                                \
        GST_LOG_OBJECT(avspliter, "expose locking from thread 0x%06" PRIxPTR,           \
                       FAKE_POINTER(g_thread_self()));                                  \
        g_mutex_lock(&GST_AVSPLITER_BIN_CAST(avspliter)->exposeLock);                   \
        GST_LOG_OBJECT(avspliter, "expose locked from thread 0x%06" PRIxPTR,            \
                       FAKE_POINTER(g_thread_self()));                                  \
    } while(0)

#define EXPOSE_UNLOCK(avspliter)                                                        \
    do {                                                                                \
        GST_LOG_OBJECT(avspliter, "expose unlocking from thread 0x%06" PRIxPTR,         \
                       FAKE_POINTER(g_thread_self()));                                  \
        g_mutex_unlock(&GST_AVSPLITER_BIN_CAST(avspliter)->exposeLock);                 \
    } while (0)

G_GNUC_INTERNAL GType gst_avspliter_bin_get_type(void);

G_GNUC_INTERNAL void gst_avspliter_bin_add_errfilter(GstAVSpliterBin *avspliter, GstElement *element);

G_GNUC_INTERNAL void gst_avspliter_bin_remove_errfilter(
    GstAVSpliterBin *avspliter, GstElement *element, GstMessage **error);

G_GNUC_INTERNAL void gst_avspliter_bin_collect_errmsg(GstAVSpliterBin *avspliter, GstElement *element,
    GString *errorDetails, const gchar *format, ...);

G_GNUC_INTERNAL gboolean gst_avspliter_bin_expose(GstAVSpliterBin *avspliter);

G_GNUC_INTERNAL gboolean gst_avspliter_bin_emit_appcontinue(GstAVSpliterBin *avspliter, GstPad *pad, GstCaps *caps);

G_GNUC_INTERNAL GList *gst_avspliter_bin_find_elem_factories(GstAVSpliterBin *avspliter, GstCaps *caps);

#endif