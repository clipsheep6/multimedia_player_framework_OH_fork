#include "gst_subtitle_meta.h"

GType gst_subtitle_meta_api_get_type(void)
{
    static volatile GType type;
    static const gchar *tags[] = { "memory", nullptr };
    if (g_once_init_enter(&type)) {
        GType get_type = gst_meta_api_type_register("GstSubtitleMeta", tags);
        g_once_init_leave(&type, get_type);
    }
    return type;
}