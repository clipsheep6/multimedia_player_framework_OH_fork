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

#include "gst_avspliter_utils.h"
#include "gst_avspliter_pad.h"

GstCaps *get_pad_caps(GstPad *pad)
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

gchar *error_message_to_string(GstMessage *msg)
{
    GError *err = nullptr;
    gchar *debug = nullptr;
    gchar *message = nullptr;
    gchar *fullMsg = nullptr;

    gst_message_parse_error(msg, &err, &debug);
    if (err == nullptr) {
        if (debug != nullptr) {
            g_free(debug);
        }
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

gboolean check_is_demuxer_element(GstElement *element)
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

gboolean check_is_parser_converter(GstElement *element)
{
    GstElementFactory *factory = gst_element_get_factory(element);
    const char *classification = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);
    if (classification == nullptr) {
        return FALSE;
    }

    return (strstr(classification, "Parser") != nullptr) && (strstr(classification, "Converter") != nullptr);
}

gboolean check_is_parser(GstElement *element)
{
    GstElementFactory *factory = gst_element_get_factory(element);
    const char *classification = gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS);
    if (classification == nullptr) {
        return FALSE;
    }

    return (strstr(classification, "Parser") != nullptr);
}

gboolean check_is_simple_demuxer_factory(GstElementFactory *factory)
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

struct SortPadsHelper {
    const gchar *name;
    gint score;
};

gint sort_pads(GstPad *pada, GstPad *padb)
{
    GstCaps *capsa = get_pad_caps(pada);
    GstCaps *capsb = get_pad_caps(padb);
    GstStructure *sa = gst_caps_get_structure(capsa, 0);
    GstStructure *sb = gst_caps_get_structure(capsb, 0);
    const gchar *namea = gst_structure_get_name(sa);
    const gchar *nameb = gst_structure_get_name(sb);
    gint scorea = -1;
    gint scoreb = -1;

    static const gint MIN_SCORE = 5;
    static const SortPadsHelper helper[] = {
        { "video/x-raw", 0}, { "video/", 1 }, { "image/", 2 }, { "audio/x-raw", 3 }, { "audio/", 4 }
    };

    for (guint i = 0; i < (sizeof(helper) / sizeof(SortPadsHelper)); i++) {
        if (g_strrstr(namea, helper[i].name) != nullptr) {
            scorea = helper[i].score;
            break;
        }
    }
    scorea = (scorea == -1) ? MIN_SCORE : scorea;

    for (guint i = 0; i < (sizeof(helper) / sizeof(SortPadsHelper)); i++) {
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

gboolean print_sticky_event(GstPad *pad, GstEvent **event, gpointer userdata)
{
    if (pad == nullptr || event == nullptr) {
        return TRUE;
    }

    (void)userdata;
    GST_DEBUG_OBJECT(pad, "sticky event %s %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(*event), *event);
    return TRUE;
}

GstStreamType guess_stream_type_from_caps(GstCaps *caps)
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

gboolean check_caps_is_factory_subset(GstElement *elem, GstCaps *caps, GstElementFactory *factory)
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

gint compare_factories_func(gconstpointer p1, gconstpointer p2)
{
    GstPluginFeature *f1, *f2;
    gboolean is_parser1, is_parser2;

    f1 = (GstPluginFeature *)p1;
    f2 = (GstPluginFeature *)p2;

    is_parser1 = gst_element_factory_list_is_type(GST_ELEMENT_FACTORY_CAST(f1),
        GST_ELEMENT_FACTORY_TYPE_PARSER);
    is_parser2 = gst_element_factory_list_is_type(GST_ELEMENT_FACTORY_CAST(f2),
        GST_ELEMENT_FACTORY_TYPE_PARSER);

    /* We want all parsers first as we always want to plug parsers
    * before decoders */
    if (is_parser1 && !is_parser2) {
        return -1;
    } else if (!is_parser1 && is_parser2) {
        return 1;
    }

    /* And if it's a both a parser we first sort by rank
    * and then by factory name */
    return gst_plugin_feature_rank_compare_func(p1, p2);
}