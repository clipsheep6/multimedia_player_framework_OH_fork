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

#ifndef GST_AVSPLITER_PRIV_H
#define GST_AVSPLITER_PRIV_H

#include <gst/gst.h>
#include "gst/base/gstqueuearray.h"


#define GST_AVSPLITER_STREAM(obj) ((GstAVSpliterStream *)(obj))

typedef struct _GstAVSpliterStream GstAVSpliterStream;

struct _GstAVSpliterStream {
    guint id;
    GstPad *avsBinPad; // not ref
    GstElement *shmemSink; // not ref
    gboolean inBin;
    GstPad *sinkpad;
    gulong sinkPadProbeId;
    GstStream *info;

    // cache sample info
    guint cacheLimit;
    GstQueueArray *sampleQueue;
    guint cacheSize;
    gboolean eos;
    GstClockTime lastPos;
    gboolean needSeekBack;
    gboolean isFlushing;
    gboolean selected;
};

enum TrackPullReturn {
    TRACK_PULL_FAILED,
    TRACK_PULL_UNSELECTED,
    TRACK_PULL_ENDTIME,
    TRACK_PULL_EOS,
    TRACK_PULL_SUCCESS,
};

enum AVSpliterPullReturn {
    AVS_PULL_SUCCESS,
    AVS_PULL_REACH_ENDTIME,
    AVS_PULL_REACH_EOS,
    AVS_PULL_FAILED
};

static const guint INVALID_TRACK_ID = UINT_MAX;

#define DYN_LOCK(avspliter)                                                             \
    do {                                                                                \
        GST_LOG_OBJECT(avspliter, "dynlock locking from thread %06" PRIxPTR,            \
                       FAKE_POINTER(g_thread_self()));                                  \
        g_mutex_lock(&GST_AVSPLITER_CAST(avspliter)->dynLock);                          \
        GST_LOG_OBJECT(avspliter, "dynlock locked from thread 0x%06" PRIxPTR,           \
                       FAKE_POINTER(g_thread_self()));                                  \
    } while(0)

#define DYN_UNLOCK(avspliter)                                                           \
    do {                                                                                \
        GST_LOG_OBJECT(avspliter, "dynlock unlocking from thread 0x%06" PRIxPTR,        \
                       FAKE_POINTER(g_thread_self()));                                  \
        g_mutex_unlock(&GST_AVSPLITER_CAST(avspliter)->dynLock);                        \
    } while (0)


#define SAMPLE_LOCK(avspliter) g_mutex_lock(&avspliter->sampleLock)
#define SAMPLE_UNLOCK(avspliter) g_mutex_unlock(&avspliter->sampleLock);
#define WAIT_SAMPLE_COND(avspliter) g_cond_wait(&avspliter->sampleCond, &avspliter->sampleLock);
#define SIGNAL_SAMPLE_COND(avspliter) g_cond_signal(&avspliter->sampleCond)

#define CHECK_AND_BREAK_LOG(obj, cond, fmt, ...)       \
    if (1) {                                           \
        if (!(cond)) {                                 \
            GST_ERROR_OBJECT(obj, fmt, ##__VA_ARGS__); \
            break;                                     \
        }                                              \
    } else void (0)

#endif