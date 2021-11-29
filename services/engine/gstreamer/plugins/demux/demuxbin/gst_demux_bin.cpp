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

#include "config.h"
#include "gst_demux_bin.h"
#include <cinttypes>
#include <stdarg.h>
#include "gst/pbutils/missing-plugins.h"
#include "gst/pbutils/descriptions.h"
#include "gst/playback/gstrawcaps.h"
#include "gst/playback/gstplaybackutils.h"
#include "scope_guard.h"

using namespace OHOS;

#define CHECK_AND_BREAK_LOG(obj, cond, fmt, ...)       \
    if (1) {                                           \
        if (!(cond)) {                                 \
            GST_ERROR_OBJECT(obj, fmt, ##__VA_ARGS__); \
            break;                                     \
        }                                              \
    } else void (0)

#define CHECK_AND_BREAK(cond)                          \
    if (!(cond)) {                                     \
        break;                                         \
    }

#define CHECK_AND_CONTINUE(cond)                       \
    if (!(cond)) {                                     \
        continue;                                      \
    }

#define FREE_GST_OBJECT(obj)                           \
    if ((obj) != nullptr) {                            \
        gst_object_unref(obj);                         \
        (obj) = nullptr;                               \
    }

#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))

#define FACTORY_NAME(factory) gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory))

/* generic templates */
static GstStaticPadTemplate g_demuxBinSinkTemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate g_demuxBinSrcTemplate = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

enum {
    SIGNAL_AUTOPLUG_CONTINUE,
    LAST_SIGNAL
};

enum {
    PROP_0,
};

GST_DEBUG_CATEGORY_STATIC (gst_demux_bin_debug);
#define GST_CAT_DEFAULT gst_demux_bin_debug

#define GST_TYPE_DEMUX_BIN             (gst_demux_bin_get_type())
#define GST_DEMUX_BIN_CAST(obj)        ((GstDemuxBin *)(obj))
#define GST_DEMUX_BIN(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DEMUX_BIN, GstDemuxBin))
#define GST_DEMUX_BIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_DEMUX_BIN, GstDemuxBinClass))
#define GST_IS_DEMUX_BIN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_DEMUX_BIN))
#define GST_IS_DEMUX_BIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_DEMUX_BIN))

typedef struct _GstDemuxBin GstDemuxBin;
typedef struct _GstDemuxBinClass GstDemuxBinClass;
typedef struct _GstDemuxChain GstDemuxChain;
typedef struct _GstDemuxPad GstDemuxPad;
typedef struct _GstDemuxElement GstDemuxElement;
typedef struct _GstPendingPad GstPendingPad;
typedef GstGhostPadClass GstDemuxPadClass;

struct _GstDemuxBin {
    GstBin bin;
    GMutex factoriesLock;
    guint32 factoriesCookie;
    GList *factories;
    GstElement *typefind;
    GMutex exposeLock;
    GstDemuxChain *demuxChain;
    gboolean haveType;
    guint haveTypeSignalId;
    GMutex dynlock;
    gboolean shutDown;
    GList *blockedPads;
    GList *filtered;
    GList *filteredErrors;
    guint npads;
};

struct _GstDemuxBinClass {
    GstBinClass parent_class;

    gboolean (*AutoPlugContinue)(GstElement *element, GstPad *pad, GstCaps *caps);
    void (*Drained)(GstElement *element);
};

static guint gst_demux_bin_signals[LAST_SIGNAL] = { 0 };

static void gst_demux_bin_dispose(GObject *object);
static void gst_demux_bin_finalize(GObject *object);
static void gst_demux_bin_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec);
static void gst_demux_bin_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_demux_bin_change_state(GstElement *element, GstStateChange transition);
static void gst_demux_bin_handle_message(GstBin *bin, GstMessage *message);
static gboolean gst_demux_bin_expose(GstDemuxBin *demuxBin);
static void found_type_callback(GstElement *typefind, guint probability, GstCaps *caps, GstDemuxBin *demuxBin);
static gboolean gst_demux_bin_autoplug_continue(GstElement *element, GstPad *pad, GstCaps *caps);

#define gst_demux_bin_parent_class parent_class
G_DEFINE_TYPE(GstDemuxBin, gst_demux_bin, GST_TYPE_BIN);

#define DYN_LOCK(demuxBin)                                                             \
    do {                                                                               \
        GST_LOG_OBJECT(demuxBin, "dynlock locking from thread %06" PRIxPTR,            \
                       FAKE_POINTER(g_thread_self()));                                 \
        g_mutex_lock(&GST_DEMUX_BIN_CAST(demuxBin)->dynlock);                          \
        GST_LOG_OBJECT(demuxBin, "dynlock locked from thread 0x%06" PRIxPTR,           \
                       FAKE_POINTER(g_thread_self()));                                 \
    } while(0)

#define DYN_UNLOCK(demuxBin)                                                           \
    do {                                                                               \
        GST_LOG_OBJECT(demuxBin, "dynlock unlocking from thread 0x%06" PRIxPTR,        \
                       FAKE_POINTER(g_thread_self()));                                 \
        g_mutex_unlock(&GST_DEMUX_BIN_CAST(demuxBin)->dynlock);                        \
    } while (0)

#define EXPOSE_LOCK(demuxBin)                                                          \
    do {                                                                               \
        GST_LOG_OBJECT(demuxBin, "expose locking from thread 0x%06" PRIxPTR,           \
                       FAKE_POINTER(g_thread_self()));                                 \
        g_mutex_lock(&GST_DEMUX_BIN_CAST(demuxBin)->exposeLock);                       \
        GST_LOG_OBJECT(demuxBin, "expose locked from thread 0x%06" PRIxPTR,            \
                       FAKE_POINTER(g_thread_self()));                                 \
    } while(0)

#define EXPOSE_UNLOCK(demuxBin)                                                        \
    do {                                                                               \
        GST_LOG_OBJECT(demuxBin, "expose unlocking from thread 0x%06" PRIxPTR,         \
                       FAKE_POINTER(g_thread_self()));                                 \
        g_mutex_unlock(&GST_DEMUX_BIN_CAST(demuxBin)->exposeLock);                     \
    } while (0)

struct _GstDemuxChain {
    GstDemuxChain *parentChain;
    GstDemuxBin *demuxBin;
    GList *childChains;
    GMutex lock;
    GstPad *startPad;
    GstCaps *startCaps;
    gboolean isDemuxer;
    gboolean hasParser;
    GList *elements;
    GList *pendingPads;
    GstDemuxPad *currentPad;
    GstDemuxPad *endPad;
    GstCaps *endCaps;
    gboolean noMorePads;
    gboolean isDead;
    gchar *deadDetails;
};

static void gst_demux_chain_free(GstDemuxChain *chain);
static GstDemuxChain *gst_demux_chain_new(
    GstDemuxBin *demuxBin, GstDemuxChain *parent, GstPad *startPad, GstCaps *startCaps);
static gboolean gst_demux_chain_is_complete(GstDemuxChain *chain);

#define CHAIN_LOCK(chain)                                                                            \
    do {                                                                                             \
        GST_LOG_OBJECT(chain->demuxBin, "locking chain 0x%06" PRIxPTR " from thread 0x%06" PRIxPTR,  \
                       FAKE_POINTER(chain), FAKE_POINTER(g_thread_self()));                          \
        g_mutex_lock(&chain->lock);                                                                  \
        GST_LOG_OBJECT(chain->demuxBin, "locked chain 0x%06" PRIXPTR " from thread 0x%06" PRIxPTR,   \
                       FAKE_POINTER(chain), FAKE_POINTER(g_thread_self()));                          \
    } while(0)

#define CHAIN_UNLOCK(chain)                                                                          \
    do {                                                                               \
        GST_LOG_OBJECT(chain->demuxBin, "unlocking chain 0x%06" PRIXPTR "from thread 0x%06" PRIxPTR, \
                       FAKE_POINTER(chain), FAKE_POINTER(g_thread_self()));                          \
        g_mutex_lock(&chain->lock);                                                                  \
    } while (0)

struct _GstDemuxElement {
    GstElement *element;
    GstElement *capsfilter;
    gulong padAddedSignalId;
    gulong padRemoveSignalId;
    gulong noMorePadsSignalId;
};

#define GST_DEMUX_ELEMENT(obj) (GstDemuxElement *)(obj)

struct _GstPendingPad {
    GstPad *pad;
    GstDemuxChain *chain;
    gulong eventProbeId;
    gulong notifyCapsSignalId;
};

struct _GstDemuxPad {
    GstGhostPad ghostPad;   // derived to GstGhostPad
    GstDemuxBin *demuxBin;
    GstDemuxChain *chain;
    gboolean blocked;
    gboolean exposed;
    gulong blockId;
    GstStream *activeStream;
};

GType gst_demux_pad_get_type(void);
G_DEFINE_TYPE(GstDemuxPad, gst_demux_pad, GST_TYPE_GHOST_PAD);
#define GST_TYPE_DEMUX_PAD (gst_demux_pad_get_type())
#define GST_DEMUX_PAD(obj) (G_TYPE_CHECK_INSTANCE_CAST(obj, GST_TYPE_DEMUX_PAD, GstDemuxPad))
#define GST_DEMUX_PAD_CAST(obj) (GstDemuxPad *)(obj)

static GstDemuxPad *gst_demux_pad_new(GstDemuxBin *demuxBin, GstDemuxChain *chain);
static void gst_demux_pad_active(GstDemuxPad *demuxPad, GstDemuxChain *chain);
static void gst_demux_pad_set_block(GstDemuxPad *demuxPad, gboolean blocked);
static GstPadProbeReturn gst_demux_pad_event_probe(GstPad *pad, GstPadProbeInfo *info, gpointer userdata);
static void gst_demux_pad_update_caps(GstDemuxPad *demuxPad, GstCaps *caps);
static void gst_demux_pad_update_tags(GstDemuxPad *demuxPad, GstTagList *tags);
static GstEvent *gst_demux_pad_stream_start_event(GstDemuxPad *demuxPad, GstEvent *event);
static void gst_demux_pad_do_set_block(GstDemuxPad *demuxPad, gboolean needBlock);

enum AnalyzeResult {
    ANALYZE_UNKNOWN_TYPE,
    ANALYZE_DEFER,
    ANALYZE_CONTINUE,
    ANALYZE_EXPOSE,
};

struct AnalyzeParams {
    GstPad *pad;   // new pad
    GstCaps *caps; // caps for new pad
    GstElement *src; // new pad's parent element
    GstDemuxPad *demuxPad; // the ghost pad for new pad
    GList *factories; // the factories for creating the next element
    gchar *deadDetails; // if the caps is unknow type, record the detail for current analyze process.
    gboolean isParserConverter; // the src is parser_converter
};

static gboolean gst_demux_bin_boolean_accumulator(
    GSignalInvocationHint *iHint, GValue *returnAccu, const GValue *handlerReturn, gpointer dummy)
{
    (void)iHint;
    (void)dummy;
    gboolean myboolean = g_value_get_boolean(handlerReturn);
    g_value_set_boolean(returnAccu, myboolean);
    return myboolean;
}

static void gst_demux_bin_class_init(GstDemuxBinClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);
    GstElementClass *elementClass = GST_ELEMENT_CLASS(klass);
    g_return_if_fail(elementClass != nullptr);
    GstBinClass *binClass = GST_BIN_CLASS(klass);
    g_return_if_fail(binClass != nullptr);

    gobjectClass->dispose = gst_demux_bin_dispose;
    gobjectClass->finalize = gst_demux_bin_finalize;
    gobjectClass->set_property = gst_demux_bin_set_property;
    gobjectClass->get_property = gst_demux_bin_get_property;

    /**
     * @brief This signal is emitted whenever DemuxBin finds a new stream. It is emitted before looking for any
     * elements that can handle that stream.
     *
     * Invocation of signal handlers stops after the first signal handler return FALSE. Signal handlers are
     * invoked in the order they were connected in. If no handler connected, defaultly TRUE.
     *
     * Returns: TRUE if you wish DemuxBin to look for elements that can handle the given caps. If FALSE, those
     * caps will be considered as final and the pad will be exposed as such (see 'pad-added' signal of GstElement).
     */
    gst_demux_bin_signals[SIGNAL_AUTOPLUG_CONTINUE] = g_signal_new("autoplug-continue", G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET(GstDemuxBinClass, AutoPlugContinue), gst_demux_bin_boolean_accumulator,
        nullptr, g_cclosure_marshal_generic, G_TYPE_BOOLEAN, 2, GST_TYPE_PAD, GST_TYPE_CAPS);

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&g_demuxBinSinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&g_demuxBinSrcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Demux Bin", "Generic/Bin/Parser/Demuxer",
        "Parse and de-multiplex to elementary stream",
        "OpenHarmony");

    elementClass->change_state = GST_DEBUG_FUNCPTR(gst_demux_bin_change_state);
    binClass->handle_message = GST_DEBUG_FUNCPTR(gst_demux_bin_handle_message);

    g_type_class_ref(GST_TYPE_DEMUX_PAD); // TODO: why?
}

static void gst_demux_bin_init(GstDemuxBin *demuxBin)
{
    g_return_if_fail(demuxBin != nullptr);

    g_mutex_init(&demuxBin->factoriesLock);
    g_mutex_init(&demuxBin->exposeLock);
    g_mutex_unlock(&demuxBin->dynlock);
    demuxBin->factories = nullptr;
    demuxBin->factoriesCookie = 0;
    demuxBin->shutDown = FALSE;
    demuxBin->blockedPads = nullptr;
    demuxBin->demuxChain = nullptr;
    demuxBin->filtered = nullptr;
    demuxBin->filteredErrors = nullptr;
    demuxBin->haveType = FALSE;
    demuxBin->haveTypeSignalId = 0;
    demuxBin->npads = 0;

    gboolean connectSrc = FALSE;
    GstPad *pad = nullptr;
    GstPadTemplate *padTmpl = nullptr;
    GstPad *ghostPad = nullptr;

    do {
        demuxBin->typefind = gst_element_factory_make("typefind", "typefind");
        CHECK_AND_BREAK_LOG(demuxBin, demuxBin->typefind != nullptr, "can't find typefind elemen");

        gboolean ret = gst_bin_add(GST_BIN(demuxBin), demuxBin->typefind);
        CHECK_AND_BREAK_LOG(demuxBin, ret, "can not add typefind element");

        pad = gst_element_get_static_pad(demuxBin->typefind, "sink");
        CHECK_AND_BREAK_LOG(demuxBin, pad != nullptr, "can not get sink pad from typefind");

        padTmpl = gst_static_pad_template_get(&g_demuxBinSinkTemplate);
        CHECK_AND_BREAK_LOG(demuxBin, padTmpl != nullptr, "can not get sink pad template");

        ghostPad = gst_ghost_pad_new_from_template("sink", pad, padTmpl);
        CHECK_AND_BREAK_LOG(demuxBin, ghostPad != nullptr, "create ghost pad failed");
        gst_pad_set_active(ghostPad, TRUE);
        gst_element_add_pad(GST_ELEMENT(demuxBin), ghostPad);

        connectSrc = TRUE;
    } while (0);

    if (!connectSrc) {
        if (demuxBin->typefind != nullptr) {
            gst_object_unref(demuxBin->typefind);
            demuxBin->typefind = nullptr;
        }
    }

    if (pad != nullptr) {
        gst_object_unref(pad);
    }

    if (padTmpl != nullptr) {
        gst_object_unref(padTmpl);
    }
}

static void gst_demux_bin_dispose(GObject *object)
{
    GstDemuxBin *demuxBin = GST_DEMUX_BIN(object);
    g_return_if_fail(demuxBin != nullptr);

    if (demuxBin->factories != nullptr) {
        gst_plugin_feature_list_free(demuxBin->factories);
        demuxBin->factories = nullptr;
    }

    if (demuxBin->demuxChain != nullptr) {
        gst_demux_chain_free(demuxBin->demuxChain);
        demuxBin->demuxChain = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_demux_bin_finalize(GObject *object)
{
    GstDemuxBin *demuxBin = GST_DEMUX_BIN(object);
    g_return_if_fail(demuxBin != nullptr);

    g_mutex_clear(&demuxBin->exposeLock);
    g_mutex_clear(&demuxBin->dynlock);
    g_mutex_clear(&demuxBin->factoriesLock);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_demux_bin_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    (void)value;

    switch(propId) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_demux_bin_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    (void)value;

    switch (propId) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static gboolean gst_demux_bin_autoplug_continue(GstElement *element, GstPad *pad, GstCaps *caps)
{
    static GstStaticCaps rawcaps = GST_STATIC_CAPS(DEFAULT_RAW_CAPS);

    GST_DEBUG_OBJECT(element, "caps %" GST_PTR_FORMAT, caps);

    /* If it matches our target caps, expose it */
    if (gst_caps_can_intersect(caps, gst_static_caps_get(&rawcaps))) {
        GST_DEBUG_OBJECT(element, "autoplug-continue returns FALSE");
        return FALSE;
    }

    GST_DEBUG_OBJECT(element, "autoplug-continue returns TRUE");
    return TRUE;
}

/* call with dyn_lock held */
static void unblock_pads(GstDemuxBin *demuxBin)
{
    GST_LOG_OBJECT(demuxBin, "unblocking pads");

    for (GList *tmp = demuxBin->blockedPads; tmp != nullptr; tmp = tmp->next) {
        if (tmp->data == nullptr) {
            continue;
        }
        GstDemuxPad *demuxPad = GST_DEMUX_PAD_CAST(tmp->data);
        gst_demux_pad_do_set_block(demuxPad, FALSE);
        gst_object_unref(demuxPad);
        GST_DEBUG_OBJECT(demuxPad, "unblocked");
    }

    g_list_free(demuxBin->blockedPads);
    demuxBin->blockedPads = nullptr;
}

static gboolean gst_demux_bin_ready_to_pause(GstDemuxBin *demuxBin)
{
    EXPOSE_LOCK(demuxBin);
    if (demuxBin->demuxChain != nullptr) {
        gst_demux_chain_free(demuxBin->demuxChain);
        demuxBin->demuxChain = nullptr;
    }
    EXPOSE_UNLOCK(demuxBin);

    DYN_LOCK(demuxBin);
    GST_LOG_OBJECT(demuxBin, "clearing shutdown flag");
    demuxBin->shutDown = FALSE;
    DYN_UNLOCK(demuxBin);
    demuxBin->haveType = FALSE;

    demuxBin->haveTypeSignalId =
        g_signal_connect(demuxBin->typefind, "have-type", G_CALLBACK(found_type_callback), demuxBin);
    if (demuxBin->haveTypeSignalId == 0) {
        GST_ELEMENT_ERROR(demuxBin, CORE, FAILED, (nullptr), ("have-type signal connect failed!"));
        return FALSE;
    }

    return TRUE;
}

static GstStateChangeReturn gst_demux_bin_change_state(GstElement *element, GstStateChange transition)
{
    GstDemuxBin *demuxBin = GST_DEMUX_BIN(element);
    g_return_val_if_fail(demuxBin != nullptr, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (demuxBin->typefind == nullptr) {
                gst_element_post_message(element, gst_missing_element_message_new(element, "typefind"));
                GST_ELEMENT_ERROR(demuxBin, CORE, MISSING_PLUGIN, (nullptr), ("no typefind!"));
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (!gst_demux_bin_ready_to_pause(demuxBin)) {
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            if (demuxBin->haveTypeSignalId != 0) {
                g_signal_handler_disconnect(demuxBin->typefind, demuxBin->haveTypeSignalId);
                demuxBin->haveTypeSignalId = 0;
            }
            DYN_LOCK(demuxBin);
            GST_LOG_OBJECT(demuxBin, "setting shutdown flag");
            demuxBin->shutDown = TRUE;
            unblock_pads(demuxBin);
            DYN_UNLOCK(demuxBin);
        default:
            break;
    }

    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(demuxBin, "element failed to change states");
        return GST_STATE_CHANGE_FAILURE;
    }

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            EXPOSE_LOCK(demuxBin);
            if (demuxBin->demuxChain != nullptr) {
                gst_demux_chain_free(demuxBin->demuxChain); // maybe time-consuming, considering start a thread
                demuxBin->demuxChain = nullptr;
            }
            EXPOSE_UNLOCK(demuxBin);
        default:
            break;
    }

    return ret;
}

static void gst_demux_bin_handle_message(GstBin *bin, GstMessage *msg)
{
    GstDemuxBin *demuxBin = GST_DEMUX_BIN(bin);
    g_return_if_fail(demuxBin != nullptr && msg != nullptr);

    gboolean drop = FALSE;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            DYN_LOCK(demuxBin);
            drop = demuxBin->shutDown;
            DYN_UNLOCK(demuxBin);

            if (!drop) {
                GST_OBJECT_LOCK(demuxBin);
                drop = (g_list_find(demuxBin->filtered, GST_MESSAGE_SRC(msg)) != nullptr);
                if (drop) {
                    demuxBin->filteredErrors = g_list_prepend(demuxBin->filteredErrors, gst_message_ref(msg));
                }
                GST_OBJECT_UNLOCK(demuxBin);
            }
            break;
        }
        default:
            break;
    }

    if (drop) {
        gst_message_unref(msg);
    } else {
        GST_BIN_CLASS(parent_class)->handle_message(bin, msg);
    }
}

/* muse be called with chain lock */
static gboolean gst_demux_chain_expose(GstDemuxChain *chain, GList **endpads,
    gboolean *missingPlugin, GString * missingPluginDetails)
{
    if (chain->isDead) {
        *missingPlugin = TRUE;
        if (chain->endCaps == nullptr) {
            return TRUE;
        }
        if (chain->deadDetails != nullptr) {
            g_string_append(missingPluginDetails, chain->deadDetails);
            g_string_append_c(missingPluginDetails, '\n');
        } else {
            gchar *desc = gst_pb_utils_get_codec_description(chain->endCaps);
            gchar *capsStr = gst_caps_to_string(chain->endCaps);
            g_string_append_printf(missingPluginDetails, "Missing parser: %s (%s)\n", desc, capsStr);
            g_free(capsStr);
            g_free(desc);
        }
        return TRUE;
    }

    /* The Chain has a pending pad from a parser, let's expose it as the endpad. */
    if (chain->endPad == nullptr && chain->hasParser && chain->pendingPads != nullptr) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(chain->pendingPads->data);
        GstPad *endpad = GST_PAD_CAST(gst_object_ref(ppad->pad));
        GstElement *elem = GST_ELEMENT_CAST(gst_object_get_parent(GST_OBJECT_CAST(endpad)));
        chain->pendingPads = g_list_remove(chain->pendingPads, ppad);
        gst_pending_pad_free(ppad);
        GST_DEBUG_OBJECT(chain->demuxBin, "Exposing pad %" GST_PTR_FORMAT " with incomplete caps"
            " because it's parsed", endpad);
        expose_pad(chain->demuxBin, chain, nullptr, endpad, chain->currentPad);
        gst_object_unref(endpad);
        gst_object_unref(elem);
    }

    if (chain->endPad != nullptr) {
        *endpads = g_list_prepend(*endpads, gst_object_ref(chain->endPad));
        return TRUE;
    }

    if (chain->childChains == nullptr) {
        return FALSE;
    }

    gboolean ret = FALSE;
    for (GList *node = chain->childChains; node != nullptr; node = node->next) {
        GstDemuxChain *child = reinterpret_cast<GstDemuxChain *>(node->data);
        CHAIN_LOCK(child);
        ret |= gst_demux_chain_expose(child, endpads, missingPlugin, missingPluginDetails);
        CHAIN_UNLOCK(child);
    }

    return ret;
}

struct SortEndPadsHelper {
    const gchar *name;
    gint score;
};

static gint sort_end_pads(GstDemuxPad *pada, GstDemuxPad *padb)
{
    GstCaps *capsa = get_pad_caps(GST_PAD_CAST(pada));
    GstCaps *capsb = get_pad_caps(GST_PAD_CAST(padb));
    GstStructure *sa = gst_caps_get_structure(capsa, 0);
    GstStructure *sb = gst_caps_get_structure(capsb, 0);
    const gchar *namea = gst_structure_get_name(sa);
    const gchar *nameb = gst_structure_get_name(sb);
    gint scorea = -1;
    gint scoreb = -1;

    static const gint MIN_SCORE = 5;
    static const SortEndPadsHelper helper[] = {
        { "video/x-raw", 0}, { "video/", 1 }, { "image/", 2 }, { "audio/x-raw", 3 }, { "audio/", 4 }
    };

    for (guint i = 0; i < (sizeof(helper) / sizeof(SortEndPadsHelper)); i++) {
        if (g_strrstr(namea, helper[i].name) != nullptr) {
            scorea = helper[i].score;
            break;
        }
    }
    scorea = (scorea == -1) ? MIN_SCORE : scorea;

    for (guint i = 0; i < (sizeof(helper) / sizeof(SortEndPadsHelper)); i++) {
        if (g_strrstr(nameb, helper[i].name) != nullptr) {
            scoreb = helper[i].score;
            break;
        }
    }
    scoreb = (scoreb == -1) ? MIN_SCORE : scoreb;

    gst_caps_unref(capsa);
    gst_caps_unref(capsb);

    if (scorea != scoreb) {
        return scorea - scoreb;
    }

    gchar *ida = gst_pad_get_stream_id(GST_PAD_CAST(pada));
    gchar *idb = gst_pad_get_stream_id(GST_PAD_CAST(padb));
    gint ret = (ida != nullptr) ? ((idb != nullptr) ? strcmp(ida, idb) : -1) : 1;
    g_free(ida);
    g_free(idb);

    return ret;
}

static gboolean debug_sticky_event(GstPad *pad, GstEvent **event, gpointer userdata)
{
    if (pad == nullptr || event == nullptr) {
        return TRUE;
    }

    (void)userdata;
    GST_DEBUG_OBJECT(pad, "sticky event %s %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(*event), *event);
    return TRUE;
}

/* Must be called with expose lock. */
static GList *try_collect_all_endpads(GstDemuxBin *demuxBin)
{
    GList *endpads = nullptr;
    gboolean missingPlugin = FALSE;
    GString *missingPluginDetails = g_string_new("");

    CHAIN_LOCK(demuxBin->demuxChain);
    if (!gst_demux_chain_expose(demuxBin->demuxChain, &endpads, &missingPlugin, missingPluginDetails)) {
        g_list_free_full(endpads, (GDestroyNotify)gst_object_unref);
        g_string_free(missingPluginDetails, TRUE);
        GST_ERROR_OBJECT(demuxBin, "Broken chain tree");
        CHAIN_UNLOCK(demuxBin->demuxChain);
        return nullptr;
    }
    CHAIN_UNLOCK(demuxBin->demuxChain);

    if (endpads != nullptr) {
        g_string_free(missingPluginDetails, TRUE);
        return endpads;
    }

    if (missingPlugin) {
        if (missingPluginDetails->len > 0) {
            gchar *details = g_string_free(missingPluginDetails, FALSE);
            GST_ELEMENT_ERROR(demuxBin, CORE, MISSING_PLUGIN, (nullptr),
                ("no suitable plugins found:\n%s", details));
            g_free(details);
        } else {
            g_string_free(missingPluginDetails, TRUE);
            GST_ELEMENT_ERROR(demuxBin, CORE, MISSING_PLUGIN, (nullptr), ("no suitable plugins found"));
        }
    } else {
        g_string_free(missingPluginDetails, TRUE);
        GST_WARNING_OBJECT(demuxBin, "All streams finished without buffers. ");
        GST_ELEMENT_ERROR(demuxBin, STREAM, FAILED, (nullptr), ("all streams without buffers."));
    }
    return nullptr;
}

static gboolean check_already_exposed_pads(GstDemuxBin *demuxBin, GList *endpads)
{
    gboolean allExposed = FALSE;
    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstDemuxPad *demuxPad = reinterpret_cast<GstDemuxPad *>(tmp->data);
        allExposed &= demuxPad->exposed;
    }

    if (allExposed) {
        GST_INFO_OBJECT(demuxBin, "Everything was exposed already !");
        g_list_free_full(endpads, (GDestroyNotify)gst_object_unref);
        return TRUE;
    }

    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstDemuxPad *demuxPad = reinterpret_cast<GstDemuxPad *>(tmp->data);
        if (demuxPad->exposed) {
            GST_DEBUG_OBJECT(demuxPad, "blocking exposed pad");
            gst_demux_pad_set_block(demuxPad, TRUE);
        }
    }

    return FALSE;
}

static void expose_end_pads_to_app(GstDemuxBin *demuxBin, GList *endpads)
{
    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstDemuxPad *demuxPad = reinterpret_cast<GstDemuxPad *>(tmp->data);
        gchar *padname = g_strdup_printf("src_%u", demuxBin->npads);
        demuxBin->npads += 1;
        GST_DEBUG_OBJECT(demuxBin, "About to expose demux pad %s as %s", GST_OBJECT_NAME(demuxPad), padname);
        gst_object_set_name(GST_OBJECT(demuxPad), padname);
        g_free(padname);

        gst_pad_sticky_events_foreach(GST_PAD_CAST(demuxPad), debug_sticky_event, demuxPad);

        if (!demuxPad->exposed) {
            demuxPad->exposed = TRUE;
            if (!gst_element_add_pad(GST_ELEMENT(demuxBin), GST_PAD_CAST(demuxPad))) {
                GST_WARNING_OBJECT(demuxBin, "error adding pad to bin");
                demuxPad->exposed = FALSE;
                continue;
            }
        }
        GST_INFO_OBJECT(demuxPad, "added new demux pad");
    }

    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstDemuxPad *demuxPad = reinterpret_cast<GstDemuxPad *>(tmp->data);
        if (demuxPad->exposed) {
            GST_DEBUG_OBJECT(demuxPad, "unblocking");
            gst_demux_pad_set_block(demuxPad, FALSE);
            GST_DEBUG_OBJECT(demuxPad, "unblocking");
        }

        gst_object_unref(demuxPad);
    }
}

/* Must only be called if the toplevel chain is complete and blocked, called with expose block */
static gboolean gst_demux_bin_expose(GstDemuxBin *demuxBin)
{
    GST_DEBUG_OBJECT(demuxBin, "Exposing demux bin");

    DYN_LOCK(demuxBin);
    if (demuxBin->shutDown) {
        GST_WARNING_OBJECT(demuxBin, "shutting down, aborting exposing.");
        DYN_UNLOCK(demuxBin);
        return FALSE;
    }
    DYN_UNLOCK(demuxBin);

    GList *endpads = try_collect_all_endpads(demuxBin);
    if (endpads == nullptr) {
        return FALSE;
    }

    if (check_already_exposed_pads(demuxBin, endpads)) {
        return TRUE;
    }

    endpads = g_list_sort(endpads, (GCompareFunc)sort_end_pads);

    DYN_LOCK(demuxBin);
    if (demuxBin->shutDown) {
        GST_WARNING_OBJECT(demuxBin, "shutting down, aborting exposing.");
        DYN_UNLOCK(demuxBin);
        return FALSE;
    }

    expose_end_pads_to_app(demuxBin, endpads);
    DYN_UNLOCK(demuxBin);

    g_list_free(endpads);
    endpads = nullptr;

    GST_DEBUG_OBJECT(demuxBin, "Exposed everything");
    return TRUE;
}

static void gst_demux_chain_free(GstDemuxChain *chain)
{
    CHAIN_LOCK(chain);
    GST_DEBUG_OBJECT(chain->demuxBin, "free chain 0x%06" PRIXPTR, chain);

    for (GList *node = chain->childChains; node != nullptr; node = node->next) {
        gst_demux_chain_free(reinterpret_cast<GstDemuxChain *>(node->data));
    }
    g_list_free(chain->childChains);
    chain->childChains = nullptr;

    gst_object_replace(reinterpret_cast<GstObject **>(&chain->currentPad), nullptr);

    for (GList *node = chain->pendingPads; node != nullptr; node = node->next) {
        gst_pending_pad_free(reinterpret_cast<GstPendingPad *>(node->data));
    }
    g_list_free(chain->pendingPads);
    chain->pendingPads = nullptr;

    for (GList *node = chain->elements; node != nullptr; node = node->next) {
        GstDemuxElement *delem = reinterpret_cast<GstDemuxElement *>(node->data);
        free_demux_element(chain->demuxBin, chain, delem);
    }
    g_list_free(chain->elements);
    chain->elements = nullptr;

    if (chain->endPad != nullptr) {
        if (chain->endPad->exposed) {
            GstPad *endpad = GST_PAD_CAST(chain->endPad);
            GST_DEBUG_OBJECT(chain->demuxBin, "Removing pad %s:%s", GST_DEBUG_PAD_NAME(endpad));
            gst_pad_push_event(endpad, gst_event_new_eos());
            gst_element_remove_pad(GST_ELEMENT_CAST(chain->demuxBin), endpad);
        }

        demux_pad_set_target(chain->endPad, nullptr);
        chain->endPad->exposed = FALSE;
        gst_object_unref(chain->endPad);
        chain->endPad = nullptr;
    }

    FREE_GST_OBJECT(chain->currentPad);
    FREE_GST_OBJECT(chain->startPad);
    FREE_GST_OBJECT(chain->startCaps);
    FREE_GST_OBJECT(chain->endCaps);

    if (chain->deadDetails != nullptr) {
        g_free(chain->deadDetails);
        chain->deadDetails = nullptr;
    }

    CHAIN_UNLOCK(chain);

    g_mutex_clear(&chain->lock);
    g_slice_free(GstDemuxChain, chain);
}

static GstDemuxChain *gst_demux_chain_new(
    GstDemuxBin *demuxBin, GstDemuxChain *parent, GstPad *startPad, GstCaps *startCaps)
{
    GstDemuxChain *chain = g_slice_new0(GstDemuxChain);
    if (chain == nullptr) {
        GST_ERROR_OBJECT(demuxBin, "create new chain failed");
        return nullptr;
    }

    GST_DEBUG_OBJECT(demuxBin, "Createing new chain 0x%06" PRIXPTR " with parent chain: 0x%06" PRIXPTR,
        FAKE_POINTER(chain), FAKE_POINTER(parent));

    chain->parentChain = parent;
    chain->demuxBin = demuxBin;
    g_mutex_init(&chain->lock);
    chain->startPad = GST_PAD_CAST(gst_object_ref(startPad));
    if (startCaps != nullptr) {
        chain->startCaps = gst_caps_ref(startCaps);
    }
    chain->hasParser = FALSE;
    chain->elements = nullptr;
    chain->endCaps = nullptr;
    chain->endPad = nullptr;
    chain->isDead = FALSE;
    chain->isDemuxer = FALSE;
    chain->noMorePads = FALSE;
    chain->pendingPads = nullptr;
    chain->childChains = nullptr;
    chain->currentPad = nullptr;
    chain->deadDetails = nullptr;

    return chain;
}

/* Must called with expose lock */
static gboolean gst_demux_chain_is_complete(GstDemuxChain *chain)
{
    if (chain == nullptr) {
        return FALSE;
    }

    gboolean complete = FALSE;

    CHAIN_LOCK(chain);
    do {
        if (chain->demuxBin->shutDown) {
            break;
        }

        if (chain->isDead) {
            complete = TRUE;
            break;
        }

        if (chain->endPad != nullptr && (chain->endPad->blocked || chain->endPad->exposed)) {
            complete = TRUE;
            break;
        }

        if (chain->isDemuxer && chain->noMorePads) {
            gboolean childComplete = TRUE;
            for (GList *node = chain->childChains; node != nullptr; node = node->next) {
                GstDemuxChain *child = reinterpret_cast<GstDemuxChain *>(node->data);
                /* if the child chain blocked, we must complete this whole demuxer, since
                 * everything is synchronous, we can't proceed otherwise. */
                if (child->endPad != nullptr && chain->endPad->blocked) {
                    break;
                }

                if (!gst_demux_chain_is_complete(child)) {
                    childComplete = FALSE;
                    break;
                }
            }
            GST_DEBUG_OBJECT(chain->demuxBin, "chain: 0x%06" PRIXPTR " is complete: %d",
                FAKE_POINTER(chain), childComplete);
            complete = childComplete;
        }

        if (chain->hasParser) {
            complete = TRUE;
            break;
        }
    } while (0);

    CHAIN_UNLOCK(chain);
    GST_DEBUG_OBJECT(chain->demuxBin, "Chain 0x%06" PRIXPTR " is complete: %d",
        FAKE_POINTER(chain), complete);
    return complete;
}

static GstPadProbeReturn pending_pad_event_callback(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(data);
    GstDemuxChain *chain = ppad->chain;
    GstDemuxBin *demuxBin = chain->demuxBin;

    if (event == nullptr || ppad == nullptr || chain == nullptr || demuxBin == nullptr) {
        return GST_PAD_PROBE_DROP;
    }

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_EOS: {
            GST_DEBUG_OBJECT(pad, "Received EOS on pad %s:%s without fixed caps, this stream ended too early",
                             GST_DEBUG_PAD_NAME(pad));
            chain->isDead = TRUE;
            gst_object_replace(reinterpret_cast<GstObject **>(&chain->currentPad), nullptr);
            EXPOSE_LOCK(demuxBin);
            if (gst_demux_chain_is_complete(demuxBin->demuxChain)) {
                gst_demux_bin_expose(demuxBin);
            }
            EXPOSE_UNLOCK(demuxBin);
            break;
        }
        default:
            break;
    }

    return GST_PAD_PROBE_OK;
}

static void gst_pending_pad_free(GstPendingPad *ppad)
{
    if (ppad->eventProbeId != 0) {
        gst_pad_remove_probe(ppad->pad, ppad->eventProbeId);
    }
    if (ppad->notifyCapsSignalId != 0) {
        g_signal_handler_disconnect(ppad->pad, ppad->notifyCapsSignalId);
    }
    gst_object_unref(ppad->pad);
    g_slice_free(GstPendingPad, ppad);
}

static void pending_pad_caps_notify_callback(GstPad *pad, GParamSpec *unused, GstDemuxChain *chain)
{
    GstElement *element = GST_ELEMENT_CAST(gst_pad_get_parent(pad));
    if (pad == nullptr || element == nullptr || chain == nullptr) {
        return;
    }

    GST_LOG_OBJECT(pad, "Notified caps for pad %s:%s", GST_DEBUG_PAD_NAME(pad));

    CHAIN_LOCK(chain);
    for (GList *node = chain->pendingPads; node != nullptr; node = node->next) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(node->data);
        if (ppad->pad == pad) {
            gst_pending_pad_free(ppad);
            chain->pendingPads = g_list_delete_link(chain->pendingPads, node);
            break;
        }
    }
    CHAIN_UNLOCK(chain);

    pad_added_callback(element, pad, chain);
    gst_object_unref(element);
}

static GList *find_compatible_element_fatories(GstDemuxBin *demuxBin, GstCaps *caps)
{
    GST_DEBUG_OBJECT(demuxBin, "Finding factories");
    g_mutex_lock(&demuxBin->factoriesLock);

    guint cookie = gst_registry_get_feature_list_cookie(gst_registry_get());
    if (demuxBin->factories == nullptr || demuxBin->factoriesCookie != cookie) {
        if (demuxBin->factories != nullptr) {
            gst_plugin_feature_list_free(demuxBin->factories);
        }
        demuxBin->factories = gst_element_factory_list_get_elements(
            GST_ELEMENT_FACTORY_TYPE_DECODABLE, GST_RANK_MARGINAL);
        demuxBin->factories = g_list_sort(demuxBin->factories, gst_playback_utils_compare_factories_func);
        demuxBin->factoriesCookie = cookie;
    }
    GList *list = gst_element_factory_list_filter(demuxBin->factories,
        caps, GST_PAD_SINK, gst_caps_is_fixed(caps));

    g_mutex_unlock(&demuxBin->factoriesLock);
    GST_DEBUG_OBJECT(demuxBin, "find factories finish");
    return list;
}

gboolean check_is_parser_converter(GstElement *element)
{
    GstElementFactory *factory = gst_element_get_factory(element);
    const char *classification = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);
    if (classification == nullptr) {
        return FALSE;
    }

    return (strstr(classification, "Parser") && strstr(classification, "Converter"));
}

static void process_unknown_type(GstDemuxBin *demuxBin, GstDemuxChain *chain, AnalyzeParams *params)
{
    GST_LOG_OBJECT(demuxBin, "Unknown type, posting message");

    if (chain == nullptr) {
        GST_ELEMENT_ERROR(GST_ELEMENT_CAST(demuxBin), CORE, FAILED, ("demuxbin's chain corrupted"), (""));
        return;
    }

    chain->deadDetails = params->deadDetails;
    chain->isDead = TRUE;
    chain->endCaps = (params->caps != nullptr) ? gst_caps_ref(params->caps) : nullptr;
    gst_object_replace(reinterpret_cast<GstObject **>(&chain->currentPad), nullptr);
    gst_element_post_message(GST_ELEMENT_CAST(demuxBin),
        gst_missing_decoder_message_new(GST_ELEMENT_CAST(demuxBin), params->caps));

    EXPOSE_LOCK(demuxBin);
    if (gst_demux_chain_is_complete(demuxBin->demuxChain)) {
        gst_demux_bin_expose(demuxBin);
    }
    EXPOSE_UNLOCK(demuxBin);

    if (params->src == demuxBin->typefind) {
        if (params->caps == nullptr || gst_caps_is_empty(params->caps)) {
            GST_ELEMENT_ERROR(demuxBin, STREAM, TYPE_NOT_FOUND,
                ("Could not determine type of stream"), (nullptr));
        }
    }
}

static gboolean defer_caps_setup(GstDemuxBin *demuxBin, GstDemuxChain *chain, AnalyzeParams *params)
{
    CHAIN_LOCK(chain);

    GST_LOG_OBJECT(demuxBin, "chain 0x%06" PRIxPTR " now has %d dynamic pads",
        FAKE_POINTER(chain), g_list_length(chain->pendingPads));

    GstPendingPad *pendingPad = g_slice_new0(GstPendingPad);
    if (pendingPad == nullptr) {
        CHAIN_UNLOCK(chain);
        GST_ERROR_OBJECT(demuxBin, "create pending pad failed");
        return FALSE;
    }

    pendingPad->pad = GST_PAD_CAST(gst_object_ref(params->pad));
    pendingPad->chain = chain;
    pendingPad->eventProbeId = gst_pad_add_probe(params->pad,
        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, pending_pad_event_callback, pendingPad, nullptr);

    pendingPad->notifyCapsSignalId = g_signal_connect(
        params->pad, "notify::caps", G_CALLBACK(pending_pad_caps_notify_callback), chain);
    if (pendingPad->notifyCapsSignalId == 0) {
        gst_pending_pad_free(pendingPad);
        CHAIN_UNLOCK(chain);
        GST_ERROR_OBJECT(demuxBin, "setup notify::caps failed");
        return FALSE;
    }

    chain->pendingPads = g_list_prepend(chain->pendingPads, pendingPad);
    CHAIN_UNLOCK(chain);
}

static void expose_pad(GstDemuxBin *demuxBin, GstDemuxChain *chain, GstCaps *caps, GstPad *pad, GstDemuxPad *dpad)
{
    GST_DEBUG_OBJECT(demuxBin, "pad %s:%s, chain:0x%06" PRIxPTR,
        GST_DEBUG_PAD_NAME(pad), FAKE_POINTER(chain));

    gst_demux_pad_active(dpad, chain);
    chain->endPad = GST_DEMUX_PAD_CAST(gst_object_ref(dpad));
    if (caps != nullptr) {
        chain->endCaps = gst_caps_ref(caps);
    } else {
        chain->endCaps = nullptr;
    }
}

static GstDemuxChain *add_children_chain_for_demuxer(GstDemuxBin *demuxBin,
    GstDemuxChain *chain, AnalyzeParams *params)
{
    if (chain->currentPad != nullptr) {
        gst_object_unref(chain->currentPad);
        chain->currentPad = nullptr;
    }

    if (chain->elements != nullptr) {
        GstDemuxElement *demux = GST_DEMUX_ELEMENT(chain->elements->data);
        /* if this is not a dynamic pad demuxer, directly set the no more pads flag */
        if (demux == nullptr || demux->noMorePadsSignalId == 0) { // TODO: 为什么在这里处理
            chain->noMorePads = TRUE;
        }
    }

    /* for demuxer's downstream, we add new children chain to current chain */
    GstDemuxChain *oldchain = chain;
    chain = gst_demux_chain_new(demuxBin, oldchain, params->pad, params->caps);
    g_return_val_if_fail(chain != nullptr, nullptr);

    CHAIN_LOCK(oldchain);
    oldchain->childChains = g_list_prepend(oldchain->childChains, chain);
    if (oldchain->childChains == nullptr) {
        g_mutex_clear(&chain->lock);
        g_slice_free(GstDemuxChain, chain);
        CHAIN_UNLOCK(oldchain);
        return nullptr;
    }
    CHAIN_UNLOCK(oldchain);

    return chain;
}

static gboolean analyze_caps_for_non_parser_converter(AnalyzeParams *params)
{
    if (!params->isParserConverter) {
        if (!gst_caps_is_fixed(params->caps)) {
            return FALSE;
        } else {
            // if we can get caps, it means caps event received at this pad.
            params->caps = gst_pad_get_current_caps(params->pad);
            if (params->caps == nullptr) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * check whether this caps has good type and whether we need to expose the pad directly.
 * - if the caps is invalid, or error happened, return ANALYZE_UNKNOWN_TYPE.
 * - if the caps is not fixed, return ANALYZE_DEFER
 * - if the caps is fixed, emit the autoplug-continue to app for ensureing whether we need to
 *   expose the pad directly.
 *
 * otherwise, we can continue to add new element to process this caps, then, return ANALYZE_CONTINUE
 * and a list of compatible element factories for this caps.
 */
static AnalyzeResult analyze_caps_before_continue(GstDemuxBin *demuxBin,
    GstDemuxChain *chain, AnalyzeParams *params)
{
    if (params->caps == nullptr || gst_caps_is_empty(params->caps)) {
        return ANALYZE_UNKNOWN_TYPE;
    }

    if (gst_caps_is_any(params->caps)) {
        return ANALYZE_DEFER;
    }

    GstDemuxPad *demuxPad = params->demuxPad;
    gboolean appcontinue = TRUE;

    if (gst_caps_is_fixed(params->caps)) {
        g_signal_emit(G_OBJECT(demuxBin), gst_demux_bin_signals[SIGNAL_AUTOPLUG_CONTINUE],
            0, demuxPad, params->caps, &appcontinue);
    }

    if (!appcontinue) {
        GST_LOG_OBJECT(demuxBin, "Pad is final. auto-plug-continue: %d", appcontinue) ;
        return ANALYZE_EXPOSE;
    }

    if (!analyze_caps_for_non_parser_converter(params)) {
        GST_DEBUG_OBJECT(demuxBin, "no final caps set yet, delayig autopluging");
        return ANALYZE_DEFER;
    }

    params->factories = find_compatible_element_fatories(demuxBin, params->caps);
    if (params->factories == nullptr) {
        return ANALYZE_UNKNOWN_TYPE;
    }

    return ANALYZE_CONTINUE;
}

static GstCaps *filter_caps_for_capsfilter(AnalyzeParams *params)
{
    GstCaps *filteredCaps = gst_caps_new_empty();
    g_return_val_if_fail(filteredCaps != nullptr, FALSE);

    for (GList *node = params->factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY_CAST(node->data);
        if (factory == nullptr) {
            continue;
        }

        GST_DEBUG("Trying factory %s", gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
        if (gst_element_get_factory(params->src) == factory ||
            gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_PARSER)) {
            GST_DEBUG("Skipping factory");
            continue;
        }

        const GList *tmps = gst_element_factory_get_static_pad_templates(factory);
        for (; tmps != nullptr; tmps = tmps->next) {
            GstStaticPadTemplate *st = reinterpret_cast<GstStaticPadTemplate *>(tmps->data);
            if (st == nullptr || st->direction != GST_PAD_SINK || st->presence != GST_PAD_ALWAYS) {
                continue;
            }
            GstCaps *tcaps = gst_static_pad_template_get_caps(st);
            if (tcaps == nullptr) {
                continue;
            }
            GstCaps *intersection = gst_caps_intersect_full(tcaps, params->caps, GST_CAPS_INTERSECT_FIRST);
            filteredCaps = gst_caps_merge(filteredCaps, intersection);
            gst_caps_unref(tcaps);
        }
    }

    // Append the parser caps to prevent any not-negotiated errors
    filteredCaps = gst_caps_merge(filteredCaps, gst_caps_ref(params->caps));
    return filteredCaps;
}

/**
 * for parser_converter, the output caps maybe unfixed, we try add a capfilter to downstream
 * to fixate the output caps of this chain. TRUE if success, or we need to setup a signal
 * at the capsfilter's srcpad for autoplug-continue.
 */
static AnalyzeResult add_capsfilter_for_parser_converter(GstDemuxBin *demuxBin,
    GstDemuxChain *chain, AnalyzeParams *params)
{
    if (!params->isParserConverter) {
        return ANALYZE_CONTINUE;
    }

    g_return_val_if_fail(chain->elements != nullptr, ANALYZE_UNKNOWN_TYPE);
    GstDemuxElement *demuxElem = GST_DEMUX_ELEMENT(chain->elements->data);

    GstCaps *filteredCaps = filter_caps_for_capsfilter(params);
    g_return_val_if_fail(filteredCaps != nullptr, ANALYZE_UNKNOWN_TYPE);

    demuxElem->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    if (demuxElem->capsfilter == nullptr) {
        gst_caps_unref(filteredCaps);
        GST_WARNING_OBJECT(demuxBin, "can not create capsfilter");
        return ANALYZE_UNKNOWN_TYPE;
    }

    g_object_set(G_OBJECT(demuxElem->capsfilter), "caps", filteredCaps, nullptr);
    gst_caps_unref(filteredCaps);
    filteredCaps = nullptr;
    (void)gst_element_set_state(demuxElem->capsfilter, GST_STATE_PAUSED);
    (void)gst_bin_add(GST_BIN_CAST(demuxBin), GST_ELEMENT_CAST(gst_object_ref(demuxElem->capsfilter)));

    demux_pad_set_target(params->demuxPad, nullptr);
    GstPad *newPad = gst_element_get_static_pad(demuxElem->capsfilter, "sink");
    if (newPad == nullptr) {
        GST_WARNING_OBJECT(demuxBin, "can not get capsfilter's sinkpad");
        return ANALYZE_UNKNOWN_TYPE;
    }

    (void)gst_pad_link_full(params->pad, newPad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref(newPad);
    newPad = gst_element_get_static_pad(demuxElem->capsfilter, "src");
    if (newPad == nullptr) {
        GST_WARNING_OBJECT(demuxBin, "can not get capsfilter's srcpad");
        return ANALYZE_UNKNOWN_TYPE;
    }

    demux_pad_set_target(params->demuxPad, newPad);
    params->pad = newPad;

    params->caps = gst_pad_get_current_caps(params->pad);
    if (params->caps == nullptr) {
        GST_DEBUG_OBJECT(demuxBin, "No final caps set yet, delaying autopluggging");
        return ANALYZE_DEFER;
    }

    return ANALYZE_CONTINUE;
}

struct ConnectPadParams {
    GstDemuxPad *demuxPad;
    GstDemuxElement *demuxElem;
    GstPad *srcpad;
    GstCaps *srccaps;
    GstElement *element;
    GstPad *sinkpad;
    GstElement *nextElem;
    GList *nextSrcPads;
    gboolean addToBin;
    gboolean isParserConverter;
    gboolean isSimpleDemuxer;
    GString *errorDetails;
};

static void init_connect_pad_param(AnalyzeParams *params, ConnectPadParams *connParams)
{
    connParams->element = params->src;
    connParams->srccaps = params->caps;
    connParams->srcpad = params->pad;
    connParams->demuxPad = params->demuxPad;
    connParams->demuxElem = nullptr;
    connParams->errorDetails = g_string_new("");
    connParams->isParserConverter = FALSE;
    connParams->isSimpleDemuxer = FALSE;
    connParams->nextElem = nullptr;
    connParams->sinkpad = nullptr;
    connParams->addToBin = FALSE;
    connParams->nextSrcPads = nullptr;
}

static gboolean check_caps_is_factory_subset(GstElement *elem, GstCaps *caps, GstElementFactory *factory)
{
    /* Check if the caps are really supported by the factory. Only do this for fixed caps here.
     * No-fixed caps can happen if a Parser/Converter was autoplugged before this. We then assume
     * that it will be able to convert to everything that the parser would want.
     */

    gboolean result = TRUE;

    if (!gst_caps_is_fixed(caps)) {
        GST_DEBUG_OBJECT(elem, "has unfixed caps %" GST_PTR_FORMAT, caps);
        return result;
    }

    const GList *templs = gst_element_factory_get_static_pad_templates(factory);
    while (templs) {
        GstStaticPadTemplate *templ = reinterpret_cast<GstStaticPadTemplate *>(templs->data);
        templs = g_list_next(templs);
        if (templ == nullptr || templ->direction != GST_PAD_SINK) {
            continue;
        }
        GstCaps *templCaps = gst_static_caps_get(&templ->static_caps);
        if (!gst_caps_is_subset(caps, templCaps)) {
            GST_DEBUG_OBJECT(elem, "caps %" GST_PTR_FORMAT " not subset of %" GST_PTR_FORMAT, caps, templCaps);
            gst_caps_unref(templCaps);
            result = FALSE;
            break;
        }
        gst_caps_unref(templCaps);
    }

    return result;
}

static gboolean check_is_simple_demuxer_factory(GstElementFactory *factory)
{
    /**
     * we consider elements as "simple demuxer" when they are a demuxer with one
     * and only one always source pad.
     */
    gboolean isDemuxer = (strstr(gst_element_factory_get_metadata(
        factory, GST_ELEMENT_METADATA_KLASS), "Demuxer") != nullptr);
    gint alwaySrcPadsNum = 0;

    if (!isDemuxer) {
        return FALSE;
    }

    const GList *node = gst_element_factory_get_static_pad_templates(factory);
    for(; node != nullptr; node = node->next) {
        GstStaticPadTemplate *temp = reinterpret_cast<GstStaticPadTemplate *>(node->data);
        if (temp == nullptr || temp->direction != GST_PAD_SRC) {
            continue;
        }
        if (temp->presence == GST_PAD_ALWAYS) {
            if (alwaySrcPadsNum > 0) {
                return FALSE;
            }
            alwaySrcPadsNum += 1;
        } else {
            return FALSE;
        }
    }

    if (alwaySrcPadsNum == 1) {
        return TRUE;
    }

    return FALSE;
}

static gboolean check_parser_repeat_in_chain(GstElementFactory *factory,
    GstDemuxChain *chain, ConnectPadParams *params)
{
    /**
     * If the parser was used already, we would create an infinite loop here because the
     * parser apparentlt accepts its own output as input.
     */

    params->isParserConverter = (strstr(gst_element_factory_get_metadata(
        factory, GST_ELEMENT_METADATA_KLASS), "Parser") != nullptr);
    params->isSimpleDemuxer = check_is_simple_demuxer_factory(factory);

    if (!params->isSimpleDemuxer) {
        return FALSE;
    }

    CHAIN_LOCK(chain);

    for (GList *node = chain->elements; node != nullptr; node = node->next) {
        GstDemuxElement *demuxElem = GST_DEMUX_ELEMENT(node->data);
        GstElement *upstreamElem = demuxElem->element;

        if (gst_element_get_factory(upstreamElem) == factory) {
            GST_DEBUG_OBJECT(chain->demuxBin, "Skipping factory '%s' because it was already used in this chain",
                gst_plugin_feature_get_name(GST_PLUGIN_FEATURE_CAST(factory)));
            CHAIN_UNLOCK(chain);
            return TRUE;
        }
    }

    if (chain->parentChain != nullptr) {
        if (chain->parentChain->elements != nullptr) {
            GstDemuxElement *demuxElem = GST_DEMUX_ELEMENT(chain->parentChain->elements->data);
            if (gst_element_get_factory(demuxElem->element) == factory) {
                GST_DEBUG_OBJECT(chain->demuxBin, "Skipping factory '%s' because it was already used in this chain",
                    gst_plugin_feature_get_name(GST_PLUGIN_FEATURE_CAST(factory)));
                CHAIN_UNLOCK(chain);
                return TRUE;
            }
        }
    }

    CHAIN_UNLOCK(chain);
    return FALSE;
}

static void add_element_error_filter(GstDemuxBin *demuxBin, GstElement *element)
{
    GST_OBJECT_LOCK(demuxBin);
    demuxBin->filtered = g_list_prepend(demuxBin->filtered, element);
    GST_OBJECT_UNLOCK(demuxBin);
}

static void remove_element_error_filter(GstDemuxBin *demuxBin, GstElement *element, GstMessage **error)
{
    GST_OBJECT_LOCK(demuxBin);
    demuxBin->filtered = g_list_remove(demuxBin->filtered, element);
    if (error != nullptr) {
        *error = nullptr;
    }
    GList *node = demuxBin->filteredErrors;
    while (node != nullptr) {
        GstMessage *msg = GST_MESSAGE_CAST(node->data);
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT_CAST(element)) {
            if (error != nullptr) {
                gst_message_replace(error, msg);
            }
            gst_message_unref(msg);
            demuxBin->filteredErrors = g_list_delete_link(demuxBin->filteredErrors, node);
            node = demuxBin->filteredErrors;
        } else {
            node = node->next;
        }
    }
    GST_OBJECT_UNLOCK(demuxBin);
}

static gchar *error_message_to_string(GstMessage *msg)
{
    GError *err = nullptr;
    gchar *debug = nullptr;
    gchar *message = nullptr;
    gchar *fullMsg = nullptr;

    gst_message_parse_error(msg, &err, &debug);
    if (err == nullptr) {
        return nullptr;
    }

    message = gst_error_get_message(err->domain, err->code);
    if (debug != nullptr) {
        fullMsg = g_strdup_printf("%s\n%s\n%s", message, err->message, debug);
    } else {
        fullMsg = g_strdup_printf("%s\n%s", message, err->message);
    }

    g_free(message);
    g_free(debug);
    g_clear_error(&err);

    return fullMsg;
}

static void collect_element_error_message(GstDemuxBin *demuxBin, GstElement *element,
    GString *errorDetails, const gchar *format, ...)
{
    va_list args;
    va_start (args, format);
    GST_WARNING_OBJECT(demuxBin, format, args);
    g_string_append_printf(errorDetails, format, args);
    va_end(args);

    if (element == nullptr) {
        return;
    }

    GstMessage *errorMsg = nullptr;
    remove_element_error_filter(demuxBin, element, &errorMsg);
    if (errorMsg != nullptr) {
        gchar *errorStr = error_message_to_string(errorMsg);
        if (errorStr != nullptr) {
            g_string_append_printf(errorDetails, "\n%s", errorStr);
            g_free(errorStr);
        }
        gst_message_unref(errorMsg);
    }
}

static gboolean cleanup_add_new_element(GstDemuxChain *chain, ConnectPadParams *params, gboolean ret)
{
    if (!ret) {
        if (params->nextElem != nullptr) {
            gst_element_set_state(params->nextElem, GST_STATE_NULL);
            if (params->addToBin) {
                gst_bin_remove(GST_BIN_CAST(chain->demuxBin), params->nextElem);
            } else {
                gst_object_unref(params->nextElem);
            }
            params->nextElem = nullptr;
        }
    } else {
        GST_LOG_OBJECT(chain->demuxBin, "linked on pad %s:%s", GST_DEBUG_PAD_NAME(params->srcpad));
    }

    if (params->sinkpad != nullptr) {
        gst_object_unref(params->sinkpad);
        params->sinkpad = nullptr;
    }
}

static gboolean check_is_demuxer_element(GstElement *element)
{
    GstElementFactory *factory = gst_element_get_factory(element);
    const gchar *klass = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);
    if (strstr(klass, "Demux") == nullptr) {
        return FALSE;
    }

    GstElementClass *elemClass = GST_ELEMENT_GET_CLASS(element);
    GList *padTempls = gst_element_class_get_pad_template_list(elemClass);
    gint potentialSrcPads = 0;
    static const gint multipleSrcPads = 2;

    while (padTempls != nullptr) {
        GstPadTemplate *templ = reinterpret_cast<GstPadTemplate *>(padTempls->data);
        padTempls = g_list_next(padTempls);

        if (templ == nullptr) {
            continue;
        }

        if (GST_PAD_TEMPLATE_DIRECTION(templ) != GST_PAD_SRC) {
            continue;
        }

        switch (GST_PAD_TEMPLATE_PRESENCE(templ)) {
            case GST_PAD_ALWAYS:
                [[clang::fallthrough]];
            case GST_PAD_SOMETIMES:
                if (strstr(GST_PAD_TEMPLATE_NAME_TEMPLATE(templ), "%") != nullptr) {
                    potentialSrcPads += multipleSrcPads;
                } else {
                    potentialSrcPads += 1;
                }
                break;
            case GST_PAD_REQUEST:
                potentialSrcPads += multipleSrcPads;
                break;
        }
    }

    if (potentialSrcPads < multipleSrcPads) {
        return FALSE;
    }

    return TRUE;
}

static gboolean add_new_demux_element(GstDemuxChain *chain, ConnectPadParams *params)
{
    GstDemuxElement *demuxElem = g_slice_new0(GstDemuxElement);
    if (demuxElem == nullptr) {
        GST_ERROR_OBJECT(chain->demuxBin, "Create GstDemuxElemen failed, no memory");
        return FALSE;
    }

    CHAIN_LOCK(chain);
    (void)gst_object_ref(params->nextElem);
    demuxElem->element = params->nextElem;
    demuxElem->capsfilter = nullptr;
    GList *list = g_list_prepend(chain->elements, demuxElem);
    if (list == nullptr) {
        GST_ERROR_OBJECT(chain->demuxBin, "append new demux element failed");
        g_slice_free(GstDemuxElement, demuxElem);
        CHAIN_UNLOCK(chain);
        return FALSE;
    }
    params->demuxElem = demuxElem;
    chain->elements = list;
    chain->isDemuxer = check_is_demuxer_element(params->nextElem);
    chain->hasParser |= params->isParserConverter;
    CHAIN_UNLOCK(chain);

    return TRUE;
}

/* need remove the demux element from chain's elements by the caller. */
static gboolean free_demux_element(GstDemuxBin *demuxBin, GstDemuxChain *chain, GstDemuxElement *element)
{
    if (element->padAddedSignalId != 0) {
        g_signal_handler_disconnect(element->element, element->padAddedSignalId);
    }
    if (element->padRemoveSignalId != 0) {
        g_signal_handler_disconnect(element->element, element->padRemoveSignalId);
    }
    if (element->noMorePadsSignalId != 0) {
        g_signal_handler_disconnect(element->element, element->noMorePadsSignalId);
    }

    for (GList *node = chain->pendingPads; node != nullptr;) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(node->data);
        if (GST_PAD_PARENT(ppad->pad) != element->element) {
            node = node->next;
            continue;
        }
        gst_pending_pad_free(ppad);
        GList *next = node->next;
        chain->pendingPads = g_list_delete_link(chain->pendingPads, node);
        node = next;
    }

    if (element->capsfilter != nullptr) {
        gst_bin_remove(GST_BIN(demuxBin), element->capsfilter);
        gst_element_set_state(element->capsfilter, GST_STATE_NULL);
        gst_object_unref(element->capsfilter);
    }

    gst_bin_remove(GST_BIN(demuxBin), element->element);
    gst_element_set_state(element->element, GST_STATE_NULL);
    gst_object_unref(element->element);

    g_slice_free(GstDemuxElement, element);
}

static gboolean try_add_new_element(GstDemuxBin *demuxBin, GstDemuxChain *chain,
    ConnectPadParams *params, GstElementFactory *factory)
{
    gboolean ret = FALSE;

    do {
        params->nextElem = gst_element_factory_create(factory, nullptr);
        if (params->nextElem == nullptr) {
            collect_element_error_message(demuxBin, nullptr, params->errorDetails,
                "Could not create an element from %s", FACTORY_NAME(factory));
            continue;
        }

        /* Filter errors, this will prevent the element from causing the pipeline to error while we
         * just only test this element. */
        add_element_error_filter(demuxBin, params->nextElem);

        /* we don't yet want the bin to control the element's state */
        gst_element_set_locked_state(params->nextElem, TRUE);

        if (!gst_bin_add(GST_BIN_CAST(demuxBin), params->nextElem)) {
            collect_element_error_message(demuxBin, params->nextElem, params->errorDetails,
                "Couldn't add %s to the bin", GST_ELEMENT_NAME(params->nextElem));
            break;
        }
        params->addToBin = TRUE;

        GST_OBJECT_LOCK(params->nextElem);
        if (params->nextElem->sinkpads != nullptr && params->nextElem->sinkpads->data != nullptr) {
            params->sinkpad = GST_PAD_CAST(gst_object_ref(params->nextElem->sinkpads->data));
        }
        GST_OBJECT_UNLOCK(params->nextElem);

        if (params->sinkpad == nullptr) {
            collect_element_error_message(demuxBin, params->nextElem, params->errorDetails,
                "Element %s doesn't have a sink pad", GST_ELEMENT_NAME(params->nextElem));
            break;
        }

        if (gst_pad_link_full(params->srcpad, params->sinkpad, GST_PAD_LINK_CHECK_NOTHING) != GST_PAD_LINK_OK) {
            collect_element_error_message(demuxBin, params->nextElem, params->errorDetails,
                "Link failed on pad %s:%s", GST_DEBUG_PAD_NAME(params->sinkpad));
            break;
        }

        if (gst_element_set_state(params->nextElem, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
            collect_element_error_message(demuxBin, params->nextElem, params->errorDetails,
                "Couldn't set %s to READY", GST_ELEMENT_NAME(params->nextElem));
            break;
        }

        if (!gst_pad_query_accept_caps(params->sinkpad, params->srccaps)) {
            collect_element_error_message(demuxBin, params->nextElem, params->errorDetails,
                "Element %s does not accept caps", GST_ELEMENT_NAME(params->nextElem));
            break;
        }

        ret = TRUE;
    } while (0);

    ret = ret && add_new_demux_element(chain, params);
    cleanup_add_new_element(chain, params, ret);
    return ret;
}

static void pad_added_callback(GstElement *element, GstPad *pad, GstDemuxChain *chain)
{
    if (element == nullptr || pad == nullptr || chain == nullptr) {
        return;
    }

    GST_DEBUG_OBJECT(pad, "pad added, chain: 0x%06" PRIXPTR, FAKE_POINTER(chain));

    // FIXME: for some streams, i.e. ogg, this maybe normal, output log to trace it.
    if (chain->noMorePads) {
        GST_WARNING_OBJECT(chain->demuxBin, "chain has already no more pads, but pad_added called");
        return;
    }

    GstCaps *caps = get_pad_caps(pad);
    analyze_new_pad(element, pad, caps, chain);
    if (caps != nullptr) {
        gst_caps_unref(caps);
        caps = nullptr;
    }

    EXPOSE_LOCK(chain->demuxBin);
    if (gst_demux_chain_is_complete(chain->demuxBin->demuxChain)) {
        GST_LOG_OBJECT(chain->demuxBin,
            "that was the last dynamic pad, now attempting to expose the bin");
        if (!gst_demux_bin_expose(chain->demuxBin)) {
            GST_WARNING_OBJECT(chain->demuxBin, "Couldn't expose bin");
        }
    }
    EXPOSE_UNLOCK(chain->demuxBin);
}

static void pad_removed_callback(GstElement *element, GstPad *pad, GstDemuxChain *chain)
{
    if (element == nullptr || pad == nullptr || chain == nullptr) {
        return;
    }

    GST_LOG_OBJECT(pad, "pad remove, chain:0x%06" PRIXPTR, FAKE_POINTER(chain));

    CHAIN_LOCK(chain);
    for (GList *node = chain->pendingPads; node != nullptr; node = node->next) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(node->data);
        if (ppad->pad == pad) {
            gst_pending_pad_free(ppad);
            ppad = nullptr;
            chain->pendingPads = g_list_delete_link(chain->pendingPads, node);
            break;
        }
    }
    CHAIN_UNLOCK(chain);
}

static void no_more_pads_callback(GstElement *element, GstDemuxChain *chain)
{
    if (element == nullptr || chain == nullptr) {
        return;
    }

    GST_LOG_OBJECT(element, "got no more pads");

    CHAIN_LOCK(chain);
    if (chain->elements != nullptr) {
        GstDemuxElement *demuxElem = reinterpret_cast<GstDemuxElement *>(chain->elements->data);
        if (demuxElem->element != element) {
            GST_LOG_OBJECT(chain->demuxBin,
                "no-more-pads from chain old element '%s'", GST_OBJECT_NAME(element));
            CHAIN_UNLOCK(chain);
            return;
        }
    } else if (!chain->isDemuxer) {
        GST_LOG_OBJECT(chain->demuxBin,
            "no-more-pads from a non-demuxer element '%s'", GST_OBJECT_NAME(element));
        CHAIN_UNLOCK(chain);
        return;
    }

    GST_DEBUG_OBJECT(chain->demuxBin, "setting bin to complete");

    chain->noMorePads = TRUE;
    CHAIN_UNLOCK(chain);

    EXPOSE_LOCK(chain->demuxBin);
    if (gst_demux_chain_is_complete(chain->demuxBin->demuxChain)) {
        gst_demux_bin_expose(chain->demuxBin);
    }
    EXPOSE_UNLOCK(chain->demuxBin);
}

static void collect_new_element_srcpads(GstDemuxBin *demuxBin, GstDemuxChain *chain, ConnectPadParams *params)
{
    GstElement *element = params->demuxElem->element;

    GST_DEBUG_OBJECT(demuxBin, "Attempting to connect element %s [chain: 0x%06" PRIXPTR "] further",
        GST_ELEMENT_NAME(element), FAKE_POINTER(chain));

    GstElementClass *klass = GST_ELEMENT_GET_CLASS(element);
    g_return_if_fail(klass != nullptr);
    gboolean dynamic = TRUE;

    for (GList *pads = klass->padtemplates; pads != nullptr; pads = g_list_next(pads)) {
        GstPadTemplate *templ = GST_PAD_TEMPLATE(pads->data);
        if (templ == nullptr || GST_PAD_TEMPLATE_DIRECTION(templ) != GST_PAD_SRC) {
            continue;
        }
        const gchar *templName = GST_PAD_TEMPLATE_NAME_TEMPLATE(templ);
        GST_DEBUG_OBJECT(demuxBin, "got a source pad template %s", templName);

        if (GST_PAD_TEMPLATE_PRESENCE(templ) == GST_PAD_REQUEST) {
            GST_DEBUG_OBJECT(params->demuxElem, "ignore request padtemplate %s", templName);
            continue;
        }

        GstPad *pad = gst_element_get_static_pad(element, templName);
        if (pad != nullptr) {
            GST_DEBUG_OBJECT(demuxBin, "got the pad for template %s", templName);
            params->nextSrcPads = g_list_prepend(params->nextSrcPads, pad);
            continue;
        }

        if (GST_PAD_TEMPLATE_PRESENCE(templ) == GST_PAD_SOMETIMES) {
            GST_DEBUG_OBJECT(demuxBin, "did not get the sometimes pad of template %s", templName);
            dynamic = TRUE;
        } else {
            GST_WARNING_OBJECT(demuxBin, "Could not get the pad for always template %s", templName);
        }
    }

    if (dynamic) {
        GST_LOG_OBJECT(demuxBin, "Adding signals to element %s in chain 0x%06" PRIXPTR,
            GST_ELEMENT_NAME(element), FAKE_POINTER(chain));
        params->demuxElem->padAddedSignalId = g_signal_connect(
            element, "pad-added", G_CALLBACK(pad_added_callback), chain);
        params->demuxElem->padRemoveSignalId = g_signal_connect(
            element, "pad-removed", G_CALLBACK(pad_removed_callback), chain);
        params->demuxElem->noMorePadsSignalId = g_signal_connect(
            element, "no-more-pads", G_CALLBACK(no_more_pads_callback), chain);
    }
}

static GstCaps *get_pad_caps(GstPad *pad)
{
    /* first check the pad caps, it this is set, we are definitely sure it is fixed and
     * exactly what the element will produce. */
    GstCaps *caps = gst_pad_get_current_caps(pad);

    /* then use the getcaps function if we don't have caps. These caps might not be fixed
     * in some cases, in which case analyze_new_pad will set up a notify::caps signal to
     * continue autoplugging. */
    if (caps == nullptr) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    return caps;
}

static void connect_new_element_srcpads(GstDemuxBin *demuxBin, GstDemuxChain *chain, ConnectPadParams *params)
{
    (void)demuxBin;

    for (GList *node = params->nextSrcPads; node != nullptr; node = g_list_next(node)) {
        GstPad *srcpad = GST_PAD_CAST(node->data);
        GstCaps *srccaps = get_pad_caps(srcpad);
        analyze_new_pad(params->demuxElem->element, srcpad, srccaps, chain);
        if (srccaps != nullptr) {
            gst_caps_unref(srccaps);
            srccaps = nullptr;
        }
        gst_object_unref(srcpad);
    }

    g_list_free(params->nextSrcPads);
    params->nextSrcPads = nullptr;
}

static gboolean send_sticky_event(GstPad *pad, GstEvent **event, gpointer userdata)
{
    if (pad == nullptr || userdata == nullptr || event == nullptr) {
        return FALSE;
    }

    GstPad *targetPad = GST_PAD_CAST(userdata);
    gboolean ret = gst_pad_send_event(targetPad, gst_event_ref(*event));
    if (!ret) {
        GST_WARNING_OBJECT(targetPad,
            "Send sticky event %" GST_PTR_FORMAT " failed, event from %" GST_PTR_FORMAT, *event, pad);
    }
    return ret;
}

static void send_sticky_events(GstDemuxBin *demuxBin, GstPad *pad)
{
    GstPad *peer = gst_pad_get_peer(pad);
    gst_pad_sticky_events_foreach(pad, send_sticky_event, peer);
    gst_object_unref(peer);
}

static gboolean change_new_element_to_pause(GstDemuxBin *demuxBin, GstDemuxChain *chain, ConnectPadParams *params)
{
    /* lock element's sinkpad stream lock so no data reaches the possible new element added when caps
       are sent by current element while we're still sending sticky events */

    GST_PAD_STREAM_LOCK(params->sinkpad);

    if (gst_element_set_state(params->nextElem, GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE) {
        send_sticky_events(demuxBin, params->srcpad);
        GST_PAD_STREAM_UNLOCK(params->sinkpad);

        remove_element_error_filter(demuxBin, params->nextElem, nullptr);
        gst_element_set_locked_state(params->nextElem, FALSE);
        return TRUE;
    }

    GST_PAD_STREAM_UNLOCK(params->sinkpad);

    collect_element_error_message(demuxBin, params->nextElem, params->errorDetails,
        "Couldn't set %s to PAUSED", GST_ELEMENT_NAME(params->nextElem));
    g_list_free_full(params->nextSrcPads, (GDestroyNotify)gst_object_unref);
    params->nextSrcPads = nullptr;

    // remove all elements in this chain until current new element.
    CHAIN_LOCK(chain);

    GstElement *tmpElem = nullptr;
    do {
        GstDemuxElement *tmpDemuxElem = GST_DEMUX_ELEMENT(chain->elements->data);
        tmpElem = tmpDemuxElem->element;
        free_demux_element(demuxBin, chain, tmpDemuxElem);
        chain->elements = g_list_delete_link(chain->elements, chain->elements);
    } while (tmpElem != params->nextElem);

    CHAIN_UNLOCK(chain);
    return FALSE;
}

/**
 * Try to connect the pad to an element created from one of the factories, and recursively.
 */
static gboolean connect_new_pad(GstDemuxBin *demuxBin, GstDemuxChain *chain, AnalyzeParams *params)
{
    GST_DEBUG_OBJECT(demuxBin, "pad %s:%s , chain: 0x%06" PRIxPTR ", %d factories, caps %" GST_PTR_FORMAT,
        GST_DEBUG_PAD_NAME(params->pad), FAKE_POINTER(chain), g_list_length(params->factories), params->caps);

    ConnectPadParams connParams = { 0 };
    init_connect_pad_param(params, &connParams);
    gboolean ret = FALSE;

    for (GList *node = params->factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY_CAST(node->data);
        CHECK_AND_CONTINUE(factory != nullptr);

        GST_LOG_OBJECT(connParams.element, "try factory %" GST_PTR_FORMAT, factory);

        /* Set demuxpad target to pad again, it might have been unset below but we came back
           here because something failed */
        demux_pad_set_target(connParams.demuxPad, connParams.srcpad);

        ret = check_caps_is_factory_subset(connParams.element, connParams.srccaps, factory);
        CHECK_AND_CONTINUE(ret);

        if (gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER)) {
            expose_pad(demuxBin, chain, connParams.srccaps, connParams.srcpad, connParams.demuxPad);
            ret = TRUE;
            break;
        }

        ret = check_parser_repeat_in_chain(factory, chain, &connParams);
        CHECK_AND_CONTINUE(!ret);

        demux_pad_set_target(connParams.demuxPad, nullptr);

        ret = try_add_new_element(demuxBin, chain, &connParams, factory);
        CHECK_AND_CONTINUE(ret);

        collect_new_element_srcpads(demuxBin, chain, &connParams);
        if (connParams.isParserConverter || connParams.isSimpleDemuxer) {
            connect_new_element_srcpads(demuxBin, chain, &connParams);
        }

        ret = change_new_element_to_pause(demuxBin, chain, &connParams);
        CHECK_AND_CONTINUE(ret);

        connect_new_element_srcpads(demuxBin, chain, &connParams);
        ret = TRUE;
        break;
    }

    if (connParams.errorDetails != nullptr) {
        gboolean freeSegment = (connParams.errorDetails->len == 0 || ret);
        params->deadDetails = g_string_free(connParams.errorDetails, freeSegment);
    } else {
        params->deadDetails = nullptr;
    }
    return ret;
}

static void process_analyze_result(GstDemuxBin *demuxBin, GstDemuxChain *chain,
    AnalyzeParams *params, AnalyzeResult result)
{
    if (result == ANALYZE_EXPOSE) {
        expose_pad(demuxBin, chain, params->caps, params->pad, params->demuxPad);
    }

    if (result == ANALYZE_DEFER) {
        if (!defer_caps_setup(demuxBin, chain, params)) {
            result = ANALYZE_UNKNOWN_TYPE;
        }
    }

    if (result == ANALYZE_UNKNOWN_TYPE) {
        process_unknown_type(demuxBin, chain, params);
    }

    if (params->demuxPad != nullptr) {
        gst_object_unref(params->demuxPad);
        params->demuxPad = nullptr;
    }

    // for parser converter, we will change the pad to capsfilter' srcpad.
    if (params->isParserConverter) {
        gst_object_unref(params->pad);
    }

    if (params->factories != nullptr) {
        gst_plugin_feature_list_free(params->factories);
        params->factories = nullptr;
    }

    if (result != ANALYZE_UNKNOWN_TYPE) {
        g_free(params->deadDetails);
        params->deadDetails = nullptr;
    }
}

/* called when a new pad is discovered. It will perform some basic actions
 * before trying to link something to it.
 *
 *  - Check the caps, don't do anything when there are no caps or when they have no good type.
 *  - signal AUTOPLUG_CONTINUE to check if we need to continue autoplugging this pad.
 *  - if the caps are non-fixed, setup a handler to continue autoplugging when the caps become
 *    fixed (connect to notify::caps).
 *  - get list of factories to autoplug.
 *  - continue autoplugging to one of the factories.
 */
static void analyze_new_pad(GstElement *src, GstPad *pad, GstCaps *caps, GstDemuxChain *chain)
{
    GstDemuxBin *demuxBin = chain->demuxBin;
    GST_DEBUG_OBJECT(demuxBin, "Pad %s: %s caps: %" GST_PTR_FORMAT, GST_DEBUG_PAD_NAME(pad), caps);

    AnalyzeResult ret = ANALYZE_UNKNOWN_TYPE;
    /*
     * On this call chain, we may enter this function again before finishing analyzing
     * this pad. But something maybe changed. Therefore, we need a stack local variable
     * to save the context of current analyze.
     */
    AnalyzeParams params = { pad, caps, src, nullptr, nullptr, nullptr, FALSE };
    params.isParserConverter = check_is_parser_converter(params.src);

    do {
        // check whether the pad is from the last element in this chain.
        if (chain->elements != nullptr) {
            GstDemuxElement *lastDemuxElem = GST_DEMUX_ELEMENT(chain->elements->data);
            gboolean check = (src != lastDemuxElem->element && src != lastDemuxElem->capsfilter);
            CHECK_AND_BREAK_LOG(demuxBin, check, "New pad not from the last element in this chain");
        }

        if (chain->isDemuxer) {
            chain = add_children_chain_for_demuxer(demuxBin, chain, &params);
            CHECK_AND_BREAK_LOG(demuxBin, chain != nullptr, "Add children for demuxer failed");
        }

        // we need update the current demux pad for this chain
        if (chain->currentPad == nullptr) {
            chain->currentPad = gst_demux_pad_new(demuxBin, chain);
            CHECK_AND_BREAK_LOG(demuxBin, chain->currentPad != nullptr,  "create current demuxPad failed");
        }
        gst_pad_set_active(GST_PAD_CAST(chain->currentPad), TRUE);
        demux_pad_set_target(chain->currentPad, pad);
        params.demuxPad = GST_DEMUX_PAD(gst_object_ref(chain->currentPad));

        // check whether this caps has good type and whether we need to expose the pad directly.
        ret = analyze_caps_before_continue(demuxBin, chain, &params);
        CHECK_AND_BREAK(ret == ANALYZE_CONTINUE);

        ret = add_capsfilter_for_parser_converter(demuxBin, chain, &params);
        CHECK_AND_BREAK(ret == ANALYZE_CONTINUE);

        if (!connect_new_pad(demuxBin, chain, &params)) {
            ret = ANALYZE_UNKNOWN_TYPE;
            break;
        }
    } while (0);

    return process_analyze_result(demuxBin, chain, &params, ret);
}

static void found_type_callback(GstElement *typefind, guint probability, GstCaps *caps, GstDemuxBin *demuxBin)
{
    GST_DEBUG_OBJECT(demuxBin, "typefind found caps %" GST_PTR_FORMAT, caps);
    g_return_if_fail(typefind != nullptr && caps != nullptr && demuxBin != nullptr);

    if (gst_structure_has_name(gst_caps_get_structure(caps, 0), "text/plain")) {
        GST_ELEMENT_ERROR(demuxBin, STREAM, WRONG_TYPE, ("This appears to be a text file"),
                          ("DemuxBin can not process plain text failes"));
        return;
    }
    // not yet support dynamically chaning caps from the typefind element.
    if (demuxBin->haveType || demuxBin->demuxChain != nullptr) {
        return;
    }
    demuxBin->haveType = TRUE;

    GstPad *srcpad = gst_element_get_static_pad(typefind, "src");
    GstPad *sinkpad = gst_element_get_static_pad(typefind, "sink");

    ON_SCOPE_EXIT(0) {
        gst_object_unref(srcpad);
        gst_object_unref(sinkpad);
    };

    g_return_if_fail(srcpad != nullptr && sinkpad != nullptr);

    /**
     * need stream lock here to prevent race with shutdown state change which might yank
     * away e.g. demux chain while building stuff here. In the GstElement, change state
     * from pause to ready will firstly deactive all pads, which need this stream lock.
     * In most cases, the stream lock has already held, but we grab it anyway.
     */
    GST_PAD_STREAM_LOCK(sinkpad);
    demuxBin->demuxChain = gst_demux_chain_new(demuxBin, nullptr, srcpad, caps);
    if (demuxBin->demuxChain == nullptr) {
        GST_ELEMENT_ERROR(demuxBin, RESOURCE, FAILED, ("create chain failed when have type"), (""));
        GST_PAD_STREAM_UNLOCK(sinkpad);
        return;
    }

    analyze_new_pad(typefind, srcpad, caps, demuxBin->demuxChain);
    GST_PAD_STREAM_UNLOCK(sinkpad);
}

static gboolean clear_sticky_events(GstPad *pad, GstEvent **event, gpointer userdata)
{
    GST_DEBUG_OBJECT(pad, "clearing sticky event %" GST_PTR_FORMAT, *event);
    gst_event_unref(*event);
    *event = nullptr;
    return TRUE;
}

static gboolean copy_sticky_events(GstPad *pad, GstEvent **eventptr, gpointer userdata)
{
    GstDemuxPad *demuxPad = GST_DEMUX_PAD_CAST(userdata);
    GstEvent *event = gst_event_ref(*eventptr);
    g_return_val_if_fail(event != nullptr, FALSE);

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_CAPS: {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            gst_demux_pad_update_caps(demuxPad, caps);
            break;
        }
        case GST_EVENT_STREAM_START: {
            event = gst_demux_pad_stream_start_event(demuxPad, event);
            break;
        }
        default:
            break;
    }

    GST_DEBUG_OBJECT(demuxPad, "store stick event %" GST_PTR_FORMAT, event);
    gst_pad_store_sticky_event(GST_PAD_CAST(demuxPad), event);
    gst_event_unref(event);

    return TRUE;
}

static void demux_pad_set_target(GstDemuxPad *demuxPad, GstPad *target)
{
    GstPad *oldTarget = gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(demuxPad));
    if (oldTarget != nullptr) {
        gst_object_unref(oldTarget);
    }

    if (oldTarget == target) {
        return;
    }

    gst_pad_sticky_events_foreach(GST_PAD_CAST(demuxPad), clear_sticky_events, nullptr);
    gst_ghost_pad_set_target(GST_GHOST_PAD_CAST(demuxPad), target);

    if (target == nullptr) {
        GST_LOG_OBJECT(demuxPad->demuxBin, "Setting pad %" GST_PTR_FORMAT " target to null", demuxPad);
    } else {
        GST_LOG_OBJECT(demuxPad->demuxBin, "Setting pad %" GST_PTR_FORMAT
            " target to %" GST_PTR_FORMAT, demuxPad, target);
        gst_pad_sticky_events_foreach(target, copy_sticky_events, demuxPad);
    }
}

static void gst_demux_pad_active(GstDemuxPad *demuxPad, GstDemuxChain *chain)
{
    g_return_if_fail(chain != nullptr);
    demuxPad->chain = chain; // TODO 是否多余
    gst_pad_set_active(GST_PAD_CAST(demuxPad), TRUE);
    gst_demux_pad_set_block(demuxPad, TRUE);
}

static GstPadProbeReturn src_pad_block_callback(GstPad *pad, GstPadProbeInfo *info, gpointer userdata)
{
    g_return_val_if_fail(pad != nullptr && info != nullptr && userdata != nullptr, GST_PAD_PROBE_DROP);
    GstDemuxPad *demuxPad = reinterpret_cast<GstDemuxPad *>(userdata);

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
        g_return_val_if_fail(event != nullptr, GST_PAD_PROBE_DROP);
        GST_LOG_OBJECT(pad, "Seeing event '%s'", GST_EVENT_TYPE_NAME(event));

        /**
         * do not block on sticky event or out-of-band events otherwise the allocation query from
         * demuxr might block the loop thread
         */
        if (!GST_EVENT_IS_SERIALIZED(event)) {
            GST_LOG_OBJECT(pad, "Letting OOB event through");
            return GST_PAD_PROBE_PASS;
        }

        if (GST_EVENT_IS_STICKY(event) && GST_EVENT_TYPE(event) != GST_EVENT_EOS) {
            GstPad *peer = gst_pad_get_peer(pad);  // the GstProxyPad's internel pad
            (void)gst_pad_send_event(peer, event);
            gst_object_unref(peer);
            GST_LOG_OBJECT(pad, "Manually pushed sticky event through");
            return GST_PAD_PROBE_HANDLED;
        }
    }

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM) {
        GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
        g_return_val_if_fail(query != nullptr, GST_PAD_PROBE_DROP);

        if (!GST_QUERY_IS_SERIALIZED(query)) {
            GST_LOG_OBJECT(pad, "Letting non-serialized query through");
            return GST_PAD_PROBE_PASS;
        }

        /**
         * do not block on allocation queries before we have caps, this would deadlock because we
         * are need the GST_EVENT_CAPS to continue everything, but the event is serialized.
         */
        if (!gst_pad_has_current_caps(pad)) {
            GST_LOG_OBJECT(pad, "letting serilized query before caps through");
            return GST_PAD_PROBE_PASS;
        }
    }

    GstDemuxChain *chain = demuxPad->chain;
    GstDemuxBin *demuxBin = demuxPad->demuxBin;
    g_return_val_if_fail(demuxBin != nullptr, GST_PAD_PROBE_DROP);
    GST_LOG_OBJECT(demuxPad, "blocked: demuxPad->chain: 0x%06" PRIxPTR, FAKE_POINTER(chain));
    demuxPad->blocked = TRUE;

    EXPOSE_LOCK(demuxBin);
    if (demuxBin->demuxChain != nullptr) {
        // got EOS event or Buffer, we try to expose the whole DemuxBin immediately!
        if (!gst_demux_bin_expose(demuxBin)) {
            GST_WARNING_OBJECT(demuxBin, "Couldn't expose group");
        }
    }
    EXPOSE_UNLOCK(demuxBin);

    return GST_PAD_PROBE_OK;
}

/* call with dyn lock held */
static void gst_demux_pad_do_set_block(GstDemuxPad *demuxPad, gboolean needBlock)
{
    GstDemuxBin *demuxBin = demuxPad->demuxBin;

    GstPad *targetPad = gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(demuxPad));
    if (targetPad == nullptr) {
        return;
    }

    if (needBlock) {
        if (!demuxBin->shutDown && (demuxPad->blockId == 0)) {
            demuxPad->blockId = gst_pad_add_probe(targetPad,
                static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM |
                GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM), src_pad_block_callback,
                gst_object_ref(demuxPad), (GDestroyNotify)gst_object_unref);
            gst_object_ref(demuxPad);
            demuxBin->blockedPads = g_list_prepend(demuxBin->blockedPads, demuxPad);
        }
    } else {
        if (demuxPad->blockId != 0) {
            gst_pad_remove_probe(targetPad, demuxPad->blockId);
            demuxPad->blockId = 0;
        }
        demuxPad->blocked = FALSE;

        GList *node = g_list_find(demuxBin->blockedPads, demuxPad);
        if (node != nullptr) {
            gst_object_unref(demuxBin);
            demuxBin->blockedPads = g_list_delete_link(demuxBin->blockedPads, node);
        }
    }

    if (demuxBin->shutDown) {
        /* deactivate to force flushing state to prevent NOT_LINKED errors */
        gst_pad_set_active(GST_PAD_CAST(demuxPad), FALSE);
    }

    gst_object_unref(targetPad);
}

static void gst_demux_pad_set_block(GstDemuxPad *demuxPad, gboolean needBlock)
{
    GstDemuxBin *demuxBin = demuxPad->demuxBin;

    DYN_LOCK(demuxBin);
    GST_DEBUG_OBJECT(demuxPad, "blocking pad: %d", needBlock);
    gst_demux_pad_do_set_block(demuxPad, needBlock);
    DYN_UNLOCK(demuxBin);
}

static GstStreamType guess_stream_type_from_caps(GstCaps *caps)
{
    if (gst_caps_get_size(caps) < 1) {
        return GST_STREAM_TYPE_UNKNOWN;
    }

    GstStructure *s = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(s);

    if (g_str_has_prefix(name, "video/") || g_str_has_prefix(name, "image/")) {
        return GST_STREAM_TYPE_VIDEO;
    }

    if (g_str_has_prefix(name, "audio/")) {
        return GST_STREAM_TYPE_AUDIO;
    }

    if (g_str_has_prefix(name, "text/") ||
        g_str_has_prefix(name, "subpicture/") ||
        g_str_has_prefix(name, "closedcaption/")) {
        return GST_STREAM_TYPE_TEXT;
    }

    return GST_STREAM_TYPE_UNKNOWN;
}

static void gst_demux_pad_update_caps(GstDemuxPad *demuxPad, GstCaps *caps)
{
    if (caps == nullptr || demuxPad->activeStream == nullptr)  {
        return;
    }

    GST_DEBUG_OBJECT(demuxPad, "Storing caps %" GST_PTR_FORMAT
        " on stream %" GST_PTR_FORMAT, caps, demuxPad->activeStream);

    if (gst_caps_is_fixed(caps)) {
        gst_stream_set_caps(demuxPad->activeStream, caps);
    }

    if (gst_stream_get_stream_type(demuxPad->activeStream) == GST_STREAM_TYPE_UNKNOWN)  {
        GstStreamType newType = guess_stream_type_from_caps(caps);
        if (newType != GST_STREAM_TYPE_UNKNOWN) {
            gst_stream_set_stream_type(demuxPad->activeStream, newType);
        }
    }
}

static void gst_demux_pad_update_tags(GstDemuxPad *demuxPad, GstTagList *tags)
{
    if (tags == nullptr || demuxPad->activeStream == nullptr) {
        return;
    }

    if (gst_tag_list_get_scope(tags) != GST_TAG_SCOPE_STREAM) {
        return;
    }

    GST_DEBUG_OBJECT(demuxPad, "Storing new tags %" GST_PTR_FORMAT
        " on stream %" GST_PTR_FORMAT, tags, demuxPad->activeStream);
    gst_stream_set_tags(demuxPad->activeStream, tags);
}

static GstEvent *gst_demux_pad_stream_start_event(GstDemuxPad *demuxPad, GstEvent *event)
{
    const gchar *streamId = nullptr;
    gst_event_parse_stream_start(event, &streamId);

    gboolean repeatEvent = FALSE;
    if (demuxPad->activeStream != nullptr && g_str_equal(demuxPad->activeStream->stream_id, streamId)) {
        repeatEvent = TRUE;
    }

    GstStream *stream = nullptr;
    gst_event_parse_stream(event, &stream);
    if (stream == nullptr) {
        GstCaps *caps = gst_pad_get_current_caps(GST_PAD_CAST(demuxPad));
        if (caps == nullptr) {
            GstPad *peer = gst_ghost_pad_get_target(GST_GHOST_PAD(demuxPad));
            caps = gst_pad_get_current_caps(peer);
            gst_object_unref(peer);
        }
        if (caps == nullptr && demuxPad->chain != nullptr && demuxPad->chain->startCaps != nullptr) {
            caps = gst_caps_ref(demuxPad->chain->startCaps);
        }

        GST_DEBUG_OBJECT(demuxPad, "Saw stream_start with no GstStream, Adding one. Caps %" GST_PTR_FORMAT, caps);

        if (repeatEvent) {
            stream = GST_STREAM_CAST(gst_object_ref(demuxPad->activeStream));
        } else {
            stream = gst_stream_new(streamId, nullptr, GST_STREAM_TYPE_UNKNOWN, GST_STREAM_FLAG_NONE);
            GstObject **obj = reinterpret_cast<GstObject **>(&demuxPad->activeStream);
            gst_object_replace(obj, GST_OBJECT_CAST(stream));
        }

        if (caps != nullptr) {
            gst_demux_pad_update_caps(demuxPad, caps);
            gst_caps_unref(caps);
        }

        event = gst_event_make_writable(event);
        gst_event_set_stream(event, stream);
    }

    gst_object_unref(stream);
    GST_LOG_OBJECT (demuxPad, "Saw stream %s (GstStream %" GST_PTR_FORMAT ")", stream->stream_id, stream);
    return event;
}

static GstPadProbeReturn gst_demux_pad_event_probe(GstPad *pad, GstPadProbeInfo *info, gpointer userdata)
{
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    g_return_val_if_fail(event != nullptr, GST_PAD_PROBE_DROP);
    GstObject *parent = gst_pad_get_parent(pad);
    GstDemuxPad *demuxPad = GST_DEMUX_PAD_CAST(parent);
    g_return_val_if_fail(demuxPad != nullptr, GST_PAD_PROBE_DROP);
    // TODO ??? 为什么默认drop掉，都drop掉了，下游岂不是收不到事件
    GstPadProbeReturn ret = GST_PAD_PROBE_DROP;

    GST_LOG_OBJECT(pad, "%s demuxpad: %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(event), demuxPad);

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_CAPS: {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            gst_demux_pad_update_caps(demuxPad, caps);
            break;
        }
        case GST_EVENT_TAG: {
            GstTagList *tags = nullptr;
            gst_event_parse_tag(event, &tags);
            gst_demux_pad_update_tags(demuxPad, tags);
            break;
        }
        case GST_EVENT_STREAM_START: {
            GST_PAD_PROBE_INFO_DATA(info) = gst_demux_pad_stream_start_event(demuxPad, event);
            break;
        }
        case GST_EVENT_EOS: {
            GST_INFO_OBJECT(pad, "EOS received");
            ret = GST_PAD_PROBE_OK;
            break;
        }
        default:
            break;
    }

    gst_object_unref(parent);
    return ret;
}

static void gst_demux_pad_dispose(GObject *object);

static void gst_demux_pad_class_init(GstDemuxPadClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobjectKlass = reinterpret_cast<GObjectClass *>(klass);
    gobjectKlass->dispose = gst_demux_pad_dispose;
}

static void gst_demux_pad_init(GstDemuxPad *pad)
{
    pad->chain = NULL;
    pad->blocked = FALSE;
    pad->exposed = FALSE;
    gst_object_ref_sink (pad);
}

static void gst_demux_pad_dispose(GObject *object)
{
    GstDemuxPad *demuxPad = GST_DEMUX_PAD_CAST(object);
    g_return_if_fail(demuxPad != nullptr);

    demux_pad_set_target(demuxPad, nullptr);

    GstObject **obj = reinterpret_cast<GstObject **>(&demuxPad->activeStream);
    gst_object_replace(obj, nullptr);

    G_OBJECT_CLASS(gst_demux_pad_parent_class)->dispose(object);
}

static GstDemuxPad *gst_demux_pad_new(GstDemuxBin *demuxBin, GstDemuxChain *chain)
{
    GST_DEBUG_OBJECT(demuxBin, "making new demuxpad");

    GstPadTemplate *padTmpl = gst_static_pad_template_get(&g_demuxBinSrcTemplate);
    g_return_val_if_fail(padTmpl != nullptr, nullptr);
    GstDemuxPad *demuxPad = GST_DEMUX_PAD_CAST(g_object_new(GST_TYPE_DEMUX_PAD, "direction", GST_PAD_SRC,
        "template", padTmpl, nullptr));
    if (demuxPad == nullptr) {
        GST_ERROR_OBJECT(demuxBin, "new demux pad failed");
        gst_object_unref(padTmpl);
        return nullptr;
    }
    (void)gst_ghost_pad_construct(GST_GHOST_PAD_CAST(demuxPad));

    demuxPad->chain = chain;
    demuxPad->demuxBin = demuxBin;
    // proxypad is the peer of ghostpad's target pad
    GstProxyPad *proxyPad = gst_proxy_pad_get_internal(GST_PROXY_PAD(demuxPad));
    gst_pad_add_probe(GST_PAD_CAST(proxyPad), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        gst_demux_pad_event_probe, demuxPad, nullptr);

    gst_object_unref(padTmpl);
    gst_object_unref(proxyPad);
    return demuxPad;
}

gboolean plugin_init(GstPlugin *plugin)
{
    GST_DEBUG_CATEGORY_INIT(gst_demux_bin_debug, "demuxbin", 0, "demux bin");
    return gst_element_register(plugin, "demuxbin", GST_RANK_NONE, GST_TYPE_DEMUX_BIN);
}