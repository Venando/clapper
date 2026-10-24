/* Clapper Application
 * Copyright (C) 2024 Rafał Dzięgiel <rafostar.github@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "clapper-app-utils.h"
#include "clapper-app-media-item-box.h"

#ifdef HAVE_GRAPHVIZ
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#ifdef HAVE_WIN_PROCESS_THREADS_API
#include <processthreadsapi.h>
#endif
#ifdef HAVE_WIN_TIME_API
#include <timeapi.h>
#endif
#endif

#define GST_CAT_DEFAULT clapper_app_utils_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

void
clapper_app_utils_debug_init (void)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "clapperapputils", 0,
      "Clapper App Utils");
}

/* Windows specific functions */
#ifdef G_OS_WIN32

/*
 * clapper_app_utils_win_enforce_hi_res_clock:
 *
 * Enforce high resolution clock by explicitly disabling Windows
 * timer resolution power throttling. When disabled, system remembers
 * and honors any previous timer resolution request by the process.
 *
 * By default, Windows 11 may automatically ignore the timer
 * resolution requests in certain scenarios.
 */
void
clapper_app_utils_win_enforce_hi_res_clock (void)
{
#ifdef HAVE_WIN_PROCESS_THREADS_API
  PROCESS_POWER_THROTTLING_STATE PowerThrottling;
  gboolean success;

  RtlZeroMemory (&PowerThrottling, sizeof (PowerThrottling));

  PowerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
  PowerThrottling.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
  PowerThrottling.StateMask = 0; // Always honor timer resolution requests

  success = (gboolean) SetProcessInformation(
      GetCurrentProcess (),
      ProcessPowerThrottling,
      &PowerThrottling,
      sizeof (PowerThrottling));

  /* Not an error. Older Windows does not have this functionality, but
   * also honor hi-res clock by default anyway, so do not print then. */
  GST_INFO ("Windows hi-res clock support is %senforced",
      (success) ? "" : "NOT ");
#endif
}

/*
 * clapper_app_utils_win_hi_res_clock_start:
 *
 * Start Windows high resolution clock which will improve
 * accuracy of various Windows timer APIs and precision
 * of #GstSystemClock during playback.
 *
 * On Windows 10 version 2004 (and older), this function affects
 * a global Windows setting. On any other (newer) version this
 * will only affect a single process.
 *
 * Returns: Timer resolution period value.
 */
guint
clapper_app_utils_win_hi_res_clock_start (void)
{
  guint resolution = 0;

#ifdef HAVE_WIN_TIME_API
  TIMECAPS time_caps;
  MMRESULT res;

  if ((res = timeGetDevCaps (&time_caps, sizeof (TIMECAPS))) != TIMERR_NOERROR) {
    GST_WARNING ("Could not query timer resolution, code: %u", res);
    return 0;
  }

  resolution = MIN (MAX (time_caps.wPeriodMin, 1), time_caps.wPeriodMax);

  if ((res = timeBeginPeriod (resolution)) != TIMERR_NOERROR) {
    GST_WARNING ("Could not request timer resolution, code: %u", res);
    return 0;
  }

  GST_INFO ("Started Windows hi-res clock, precision: %ums", resolution);
#endif

  return resolution;
}

/*
 * clapper_app_utils_win_hi_res_clock_stop:
 * @resolution: started resolution value (non-zero)
 *
 * Stop previously started Microsoft Windows high resolution clock.
 */
void
clapper_app_utils_win_hi_res_clock_stop (guint resolution)
{
#ifdef HAVE_WIN_TIME_API
  MMRESULT res;

  if ((res = timeEndPeriod (resolution)) == TIMERR_NOERROR)
    GST_INFO ("Stopped Windows hi-res clock");
  else
    GST_ERROR ("Could not stop hi-res clock, code: %u", res);
#endif
}

/* Extensions are used only on Windows */
const gchar *const *
clapper_app_utils_get_extensions (void)
{
  static const gchar *const all_extensions[] = {
    "avi", "claps", "m2ts", "mkv", "mov",
    "mp4", "webm", "wmv", NULL
  };

  return all_extensions;
}

const gchar *const *
clapper_app_utils_get_subtitles_extensions (void)
{
  static const gchar *const subs_extensions[] = {
    "srt", "vtt", NULL
  };

  return subs_extensions;
}
#endif // G_OS_WIN32

const gchar *const *
clapper_app_utils_get_mime_types (void)
{
  static const gchar *const all_mime_types[] = {
    "video/*",
    "audio/*",
    "application/claps",
    "application/x-subrip",
    "text/x-ssa",
    NULL
  };

  return all_mime_types;
}

const gchar *const *
clapper_app_utils_get_subtitles_mime_types (void)
{
  static const gchar *const subs_mime_types[] = {
    "application/x-subrip",
    "text/x-ssa",
    NULL
  };

  return subs_mime_types;
}

void
clapper_app_utils_parse_progression (ClapperQueueProgressionMode mode,
    const gchar **icon, const gchar **label)
{
  const gchar *const icon_names[] = {
    "action-unavailable-symbolic",
    "media-playlist-consecutive-symbolic",
    "media-playlist-repeat-song-symbolic",
    "media-playlist-repeat-symbolic",
    "media-playlist-shuffle-symbolic",
    NULL
  };
  const gchar *const labels[] = {
    _("No progression"),
    _("Consecutive"),
    _("Repeat item"),
    _("Carousel"),
    _("Shuffle"),
    NULL
  };

  *icon = icon_names[mode];
  *label = labels[mode];
}

gboolean
clapper_app_utils_is_subtitles_file (GFile *file)
{
  GFileInfo *info;
  gboolean is_subs = FALSE;

  if ((info = g_file_query_info (file,
      G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
      G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,
      G_FILE_QUERY_INFO_NONE,
      NULL, NULL))) {
    const gchar *content_type = NULL;

    if (g_file_info_has_attribute (info,
        G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)) {
      content_type = g_file_info_get_content_type (info);
    } else if (g_file_info_has_attribute (info,
        G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE)) {
      content_type = g_file_info_get_attribute_string (info,
          G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
    }

    is_subs = (content_type && g_strv_contains (
        clapper_app_utils_get_subtitles_mime_types (),
        content_type));

    g_object_unref (info);
  }

  return is_subs;
}

static gboolean
_parse_timestamp (const gchar *str, gdouble *seconds)
{
  gchar *copy, **parts;
  gdouble total = 0;
  guint i, n_parts;
  gboolean success = TRUE;

  copy = g_strstrip (g_strdup (str));
  g_strdelimit (copy, ",", '.');

  parts = g_strsplit (copy, ":", 0);
  n_parts = g_strv_length (parts);

  if (n_parts == 0 || n_parts > 3)
    success = FALSE;

  for (i = 0; success && i < n_parts; ++i) {
    gchar *end = NULL;
    gdouble value = g_ascii_strtod (parts[i], &end);

    if (end == parts[i] || *end != '\0' || value < 0) {
      success = FALSE;
      break;
    }
    total = total * 60 + value;
  }

  g_strfreev (parts);
  g_free (copy);

  if (success)
    *seconds = total;

  return success;
}

static gint
_compare_times (gconstpointer a, gconstpointer b)
{
  gdouble da = *(const gdouble *) a, db = *(const gdouble *) b;

  return (da > db) - (da < db);
}

/*
 * clapper_app_utils_load_subtitle_times:
 * @uri: URI of an external subtitles file
 *
 * Reads cue start times (in seconds) from a local SRT, WebVTT
 * or ASS/SSA file. Embedded subtitles are not supported.
 *
 * Returns: (transfer full) (nullable): sorted #GArray of #gdouble
 *   start times, or %NULL if file could not be read or has no cues.
 */
GArray *
clapper_app_utils_load_subtitle_times (const gchar *uri)
{
  GFile *file;
  GArray *times = NULL;
  gchar *contents = NULL;
  gchar **lines;
  guint i;

  file = g_file_new_for_uri (uri);

  if (!g_file_is_native (file)
      || !g_file_load_contents (file, NULL, &contents, NULL, NULL, NULL)) {
    g_object_unref (file);
    return NULL;
  }
  g_object_unref (file);

  times = g_array_new (FALSE, FALSE, sizeof (gdouble));
  lines = g_strsplit (contents, "\n", 0);
  g_free (contents);

  for (i = 0; lines[i]; ++i) {
    const gchar *line = lines[i];
    gchar *start_str = NULL;
    gdouble start;

    if (g_str_has_prefix (line, "\xEF\xBB\xBF"))
      line += 3;

    if (g_str_has_prefix (line, "Dialogue:")) {
      /* ASS/SSA: Dialogue: Layer,Start,End,... */
      gchar **fields = g_strsplit (line + 9, ",", 3);

      if (g_strv_length (fields) == 3)
        start_str = g_strdup (fields[1]);
      g_strfreev (fields);
    } else {
      /* SRT/WebVTT: [H:]MM:SS.mmm --> [H:]MM:SS.mmm */
      const gchar *arrow = strstr (line, "-->");

      if (arrow)
        start_str = g_strndup (line, arrow - line);
    }

    if (start_str && _parse_timestamp (start_str, &start))
      g_array_append_val (times, start);

    g_free (start_str);
  }
  g_strfreev (lines);

  if (times->len == 0) {
    g_array_unref (times);
    return NULL;
  }

  g_array_sort (times, _compare_times);

  return times;
}

typedef struct
{
  GMutex lock;
  GPtrArray *tracks;
  GstElement *pipeline;
} SubtitleScan;

typedef struct
{
  SubtitleScan *scan;
  gchar *stream_id;
  GArray *times;
} SubtitleTrack;

typedef struct
{
  gchar *uri;
  guint stream_index;
} SubtitleScanRequest;

static void
_subtitle_track_free (SubtitleTrack *track)
{
  g_free (track->stream_id);
  g_array_unref (track->times);
  g_free (track);
}

static void
_subtitle_scan_request_free (SubtitleScanRequest *req)
{
  g_free (req->uri);
  g_free (req);
}

static gboolean
_pad_is_text (GstPad *pad, gchar **stream_id)
{
  GstStream *stream;
  GstStreamType type = GST_STREAM_TYPE_UNKNOWN;
  GstCaps *caps;
  const gchar *name;
  gboolean is_text = FALSE;

  if ((stream = gst_pad_get_stream (pad))) {
    type = gst_stream_get_stream_type (stream);
    *stream_id = g_strdup (gst_stream_get_stream_id (stream));
    gst_object_unref (stream);
  }
  if (type != GST_STREAM_TYPE_UNKNOWN)
    return (type & GST_STREAM_TYPE_TEXT) != 0;

  caps = gst_pad_query_caps (pad, NULL);
  if (!gst_caps_is_empty (caps) && !gst_caps_is_any (caps)) {
    name = gst_structure_get_name (gst_caps_get_structure (caps, 0));

    is_text = (g_str_has_prefix (name, "text/")
        || g_str_has_prefix (name, "subpicture/")
        || g_str_has_prefix (name, "application/x-ass")
        || g_str_has_prefix (name, "application/x-ssa")
        || g_str_has_prefix (name, "application/x-subtitle"));
  }
  gst_caps_unref (caps);

  return is_text;
}

static GstPadProbeReturn
_text_buffer_probe_cb (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
  SubtitleTrack *track = user_data;
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);

  if (buffer && GST_BUFFER_PTS_IS_VALID (buffer)
      && gst_buffer_get_size (buffer) > 0) {
    gdouble time = (gdouble) GST_BUFFER_PTS (buffer) / GST_SECOND;

    g_mutex_lock (&track->scan->lock);
    g_array_append_val (track->times, time);
    g_mutex_unlock (&track->scan->lock);
  }

  return GST_PAD_PROBE_OK;
}

static void
_scan_pad_added_cb (GstElement *parsebin, GstPad *pad, SubtitleScan *scan)
{
  GstElement *sink = gst_element_factory_make ("fakesink", NULL);
  GstPad *sinkpad;
  gchar *stream_id = NULL;

  /* Every stream must go somewhere, but only text ones are inspected */
  g_object_set (sink, "sync", FALSE, "async", FALSE, NULL);
  gst_bin_add (GST_BIN_CAST (scan->pipeline), sink);

  sinkpad = gst_element_get_static_pad (sink, "sink");

  if (_pad_is_text (pad, &stream_id)) {
    SubtitleTrack *track = g_new0 (SubtitleTrack, 1);

    track->scan = scan;
    track->stream_id = stream_id;
    track->times = g_array_new (FALSE, FALSE, sizeof (gdouble));

    g_mutex_lock (&scan->lock);
    g_ptr_array_add (scan->tracks, track);
    g_mutex_unlock (&scan->lock);

    gst_pad_add_probe (sinkpad, GST_PAD_PROBE_TYPE_BUFFER,
        _text_buffer_probe_cb, track, NULL);
  } else {
    g_free (stream_id);
  }

  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);

  gst_element_sync_state_with_parent (sink);
}

static void
_scan_subtitle_times_thread (GTask *task, gpointer source_object,
    gpointer task_data, GCancellable *cancellable)
{
  SubtitleScanRequest *req = task_data;
  SubtitleScan scan = { 0, };
  SubtitleTrack *track = NULL;
  GstElement *src, *parsebin;
  GstStreamCollection *collection = NULL;
  GstBus *bus;
  GArray *times = NULL;
  gboolean done = FALSE;
  guint i;

  src = gst_element_make_from_uri (GST_URI_SRC, req->uri, NULL, NULL);
  parsebin = gst_element_factory_make ("parsebin", NULL);

  if (!src || !parsebin) {
    gst_clear_object (&src);
    gst_clear_object (&parsebin);
    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
        "Could not create subtitle scanning elements");
    return;
  }

  g_mutex_init (&scan.lock);
  scan.tracks = g_ptr_array_new_with_free_func ((GDestroyNotify) _subtitle_track_free);
  scan.pipeline = gst_pipeline_new ("subtitle-scan");

  gst_bin_add_many (GST_BIN_CAST (scan.pipeline), src, parsebin, NULL);
  gst_element_link (src, parsebin);
  g_signal_connect (parsebin, "pad-added", G_CALLBACK (_scan_pad_added_cb), &scan);

  bus = gst_element_get_bus (scan.pipeline);
  gst_element_set_state (scan.pipeline, GST_STATE_PLAYING);

  while (!done && !g_cancellable_is_cancelled (cancellable)) {
    GstMessage *msg = gst_bus_timed_pop_filtered (bus, 100 * GST_MSECOND,
        GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_STREAM_COLLECTION);

    if (!msg)
      continue;

    switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_STREAM_COLLECTION:
        gst_clear_object (&collection);
        gst_message_parse_stream_collection (msg, &collection);
        break;
      default:
        done = TRUE;
        break;
    }
    gst_message_unref (msg);
  }

  gst_element_set_state (scan.pipeline, GST_STATE_NULL);
  gst_object_unref (bus);
  gst_object_unref (scan.pipeline);

  if (g_cancellable_is_cancelled (cancellable)) {
    g_task_return_error_if_cancelled (task);
    goto finish;
  }

  /* Pick the n-th text stream, ordered like the player lists them */
  if (collection) {
    guint n_streams = gst_stream_collection_get_size (collection);
    guint text_index = 0;

    for (i = 0; i < n_streams && !track; ++i) {
      GstStream *stream = gst_stream_collection_get_stream (collection, i);
      guint j;

      if (!(gst_stream_get_stream_type (stream) & GST_STREAM_TYPE_TEXT))
        continue;

      if (text_index++ != req->stream_index)
        continue;

      for (j = 0; j < scan.tracks->len; ++j) {
        SubtitleTrack *t = g_ptr_array_index (scan.tracks, j);

        if (g_strcmp0 (t->stream_id, gst_stream_get_stream_id (stream)) == 0) {
          track = t;
          break;
        }
      }
    }
  }
  if (!track && req->stream_index < scan.tracks->len)
    track = g_ptr_array_index (scan.tracks, req->stream_index);

  if (track && track->times->len > 0) {
    times = g_array_ref (track->times);
    g_array_sort (times, _compare_times);
  }

  if (times)
    g_task_return_pointer (task, times, (GDestroyNotify) g_array_unref);
  else
    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
        "No subtitle cues found");

finish:
  gst_clear_object (&collection);
  g_ptr_array_unref (scan.tracks);
  g_mutex_clear (&scan.lock);
}

/*
 * clapper_app_utils_load_media_subtitle_times_async:
 * @uri: URI of a local media file with embedded subtitles
 * @stream_index: index among the media text streams
 *
 * Demuxes (without decoding) the file in a background thread to
 * collect start times of the cues of an embedded subtitle stream.
 */
void
clapper_app_utils_load_media_subtitle_times_async (const gchar *uri,
    guint stream_index, GCancellable *cancellable,
    GAsyncReadyCallback callback, gpointer user_data)
{
  GTask *task;
  SubtitleScanRequest *req;

  req = g_new0 (SubtitleScanRequest, 1);
  req->uri = g_strdup (uri);
  req->stream_index = stream_index;

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_task_data (task, req, (GDestroyNotify) _subtitle_scan_request_free);
  g_task_run_in_thread (task, _scan_subtitle_times_thread);
  g_object_unref (task);
}

GArray *
clapper_app_utils_load_media_subtitle_times_finish (GAsyncResult *result, GError **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

gboolean
clapper_app_utils_value_for_item_is_valid (const GValue *value)
{
  if (G_VALUE_HOLDS (value, GTK_TYPE_WIDGET))
    return CLAPPER_APP_IS_MEDIA_ITEM_BOX (g_value_get_object (value));

  if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
      || G_VALUE_HOLDS (value, G_TYPE_FILE))
    return TRUE;

  if (G_VALUE_HOLDS (value, G_TYPE_STRING))
    return gst_uri_is_valid (g_value_get_string (value));

  return FALSE;
}

gboolean
clapper_app_utils_files_from_list_model (GListModel *files_model, GFile ***files, gint *n_files)
{
  guint i, len = g_list_model_get_n_items (files_model);

  if (G_UNLIKELY (len == 0 || len > G_MAXINT))
    return FALSE;

  *files = g_new (GFile *, len + 1);

  if (n_files)
    *n_files = (gint) len;

  for (i = 0; i < len; ++i) {
    (*files)[i] = g_list_model_get_item (files_model, i);
  }
  (*files)[i] = NULL;

  return TRUE;
}

gboolean
clapper_app_utils_files_from_slist (GSList *file_list, GFile ***files, gint *n_files)
{
  GSList *fl;
  guint len, i = 0;

  len = g_slist_length (file_list);

  if (G_UNLIKELY (len == 0 || len > G_MAXINT))
    return FALSE;

  *files = g_new (GFile *, len + 1);

  if (n_files)
    *n_files = (gint) len;

  for (fl = file_list; fl != NULL; fl = fl->next) {
    (*files)[i] = (GFile *) g_object_ref (fl->data);
    i++;
  }
  (*files)[i] = NULL;

  return TRUE;
}

gboolean
clapper_app_utils_files_from_string (const gchar *string, GFile ***files, gint *n_files)
{
  GSList *slist = NULL;
  gchar **uris = g_strsplit (string, "\n", 0);
  guint i;
  gboolean success;

  for (i = 0; uris[i]; ++i) {
    const gchar *uri = uris[i];

    if (!gst_uri_is_valid (uri))
      continue;

    slist = g_slist_append (slist, g_file_new_for_uri (uri));
  }

  g_strfreev (uris);

  if (!slist)
    return FALSE;

  success = clapper_app_utils_files_from_slist (slist, files, n_files);
  g_slist_free_full (slist, g_object_unref);

  return success;
}

gboolean
clapper_app_utils_files_from_command_line (GApplicationCommandLine *cmd_line, GFile ***files, gint *n_files)
{
  GSList *slist = NULL;
  gchar **argv;
  gint i, argc = 0;
  gboolean success;

  argv = g_application_command_line_get_arguments (cmd_line, &argc);

  for (i = 1; i < argc; ++i)
    slist = g_slist_append (slist, g_application_command_line_create_file_for_arg (cmd_line, argv[i]));

  g_strfreev (argv);

  if (!slist)
    return FALSE;

  success = clapper_app_utils_files_from_slist (slist, files, n_files);
  g_slist_free_full (slist, g_object_unref);

  return success;
}

static inline gboolean
_files_from_file (GFile *file, GFile ***files, gint *n_files)
{
  *files = g_new (GFile *, 2);

  (*files)[0] = g_object_ref (file);
  (*files)[1] = NULL;

  if (n_files)
    *n_files = 1;

  return TRUE;
}

gboolean
clapper_app_utils_files_from_value (const GValue *value, GFile ***files, gint *n_files)
{
  if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)) {
    return clapper_app_utils_files_from_slist (
        (GSList *) g_value_get_boxed (value), files, n_files);
  } else if (G_VALUE_HOLDS (value, G_TYPE_FILE)) {
    return _files_from_file (
        (GFile *) g_value_get_object (value), files, n_files);
  } else if (G_VALUE_HOLDS (value, G_TYPE_STRING)) {
    return clapper_app_utils_files_from_string (
        g_value_get_string (value), files, n_files);
  }

  return FALSE;
}

void
clapper_app_utils_files_free (GFile **files)
{
  gint i;

  for (i = 0; files[i]; ++i)
    g_object_unref (files[i]);

  g_free (files);
}

static inline gboolean
_parse_feature_name (gchar *str, const gchar **feature_name)
{
  if (!str)
    return FALSE;

  g_strstrip (str);

  if (str[0] == '\0')
    return FALSE;

  *feature_name = str;
  return TRUE;
}

static inline gboolean
_parse_feature_rank (gchar *str, GstRank *rank)
{
  if (!str)
    return FALSE;

  g_strstrip (str);

  if (str[0] == '\0')
    return FALSE;

  if (g_ascii_isdigit (str[0])) {
    gulong l;
    gchar *endptr;

    l = strtoul (str, &endptr, 10);
    if (endptr > str && endptr[0] == 0) {
      *rank = (GstRank) l;
    } else {
      return FALSE;
    }
  } else if (g_ascii_strcasecmp (str, "NONE") == 0) {
    *rank = GST_RANK_NONE;
  } else if (g_ascii_strcasecmp (str, "MARGINAL") == 0) {
    *rank = GST_RANK_MARGINAL;
  } else if (g_ascii_strcasecmp (str, "SECONDARY") == 0) {
    *rank = GST_RANK_SECONDARY;
  } else if (g_ascii_strcasecmp (str, "PRIMARY") == 0) {
    *rank = GST_RANK_PRIMARY;
  } else if (g_ascii_strcasecmp (str, "MAX") == 0) {
    *rank = (GstRank) G_MAXINT;
  } else {
    return FALSE;
  }

  return TRUE;
}

void
clapper_app_utils_iterate_plugin_feature_ranks (GSettings *settings,
    ClapperAppUtilsIterRanks callback, gpointer user_data)
{
  gchar **split, **walk, *stored_overrides;
  const gchar *env_overrides;
  gboolean from_env = FALSE;

  stored_overrides = g_settings_get_string (settings, "plugin-feature-ranks");
  env_overrides = g_getenv ("GST_PLUGIN_FEATURE_RANK");

  /* Iterate from GSettings, then from ENV */
parse_overrides:
  split = g_strsplit ((from_env) ? env_overrides : stored_overrides, ",", 0);

  for (walk = split; *walk; walk++) {
    gchar **values;

    if (!strchr (*walk, ':'))
      continue;

    values = g_strsplit (*walk, ":", 2);

    if (g_strv_length (values) == 2) {
      GstRank rank;
      const gchar *feature_name;

      if (_parse_feature_name (values[0], &feature_name)
          && _parse_feature_rank (values[1], &rank))
        callback (feature_name, rank, from_env, user_data);
    }

    g_strfreev (values);
  }

  g_strfreev (split);

  if (!from_env && env_overrides) {
    from_env = TRUE;
    goto parse_overrides;
  }

  g_free (stored_overrides);
}

GstElement *
clapper_app_utils_make_element (const gchar *string)
{
  gchar *char_loc;

  if (strcmp (string, "none") == 0)
    return NULL;

  char_loc = strchr (string, ' ');

  if (char_loc) {
    GstElement *element;
    GError *error = NULL;

    element = gst_parse_bin_from_description (string, TRUE, &error);
    if (error) {
      GST_ERROR ("Bin parse error: \"%s\", reason: %s", string, error->message);
      g_error_free (error);
    }

    return element;
  }

  return gst_element_factory_make (string, NULL);
}

/*
 * _get_tmp_dir:
 * @subdir: (nullable): an optional subdirectory
 *
 * Returns: (transfer full): a newly constructed #GFile
 */
static inline GFile *
_get_tmp_dir (const gchar *subdir)
{
  /* XXX: System tmp directory does not work within containers such as Flatpak
   * for our usage with file launcher, so make our own temp in app data dir */
  return g_file_new_build_filename (
      g_get_user_data_dir (), CLAPPER_APP_ID, "tmp", subdir, NULL);
}

#ifdef HAVE_GRAPHVIZ
static GFile *
_create_tmp_subdir (const gchar *subdir, GCancellable *cancellable, GError **error)
{
  GFile *tmp_dir;
  GError *my_error = NULL;

  tmp_dir = _get_tmp_dir (subdir);

  if (!g_file_make_directory_with_parents (tmp_dir, cancellable, &my_error)) {
    if (my_error->domain != G_IO_ERROR || my_error->code != G_IO_ERROR_EXISTS) {
      *error = g_error_copy (my_error);
      g_clear_object (&tmp_dir); // return NULL
    }
    g_error_free (my_error);
  }

  return tmp_dir;
}
#endif

static void
_create_pipeline_svg_file_in_thread (GTask *task, GObject *source G_GNUC_UNUSED,
    ClapperPlayer *player, GCancellable *cancellable)
{
  GFile *tmp_file = NULL;
  GError *error = NULL;

#ifdef HAVE_GRAPHVIZ
  GFile *tmp_subdir;
  Agraph_t *graph;
  GVC_t *gvc;
  gchar *path, *template = NULL, *dot_data = NULL, *img_data = NULL;
  gint fd;
  gsize size = 0;

  if (!(tmp_subdir = _create_tmp_subdir ("pipelines", cancellable, &error)))
    goto finish;

  path = g_file_get_path (tmp_subdir);
  g_object_unref (tmp_subdir);

  template = g_build_filename (path, "pipeline-XXXXXX.svg", NULL);
  g_free (path);

  fd = g_mkstemp (template); // Modifies template to actual filename

  if (G_UNLIKELY (fd == -1)) {
    g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
        "Could not open temp file for writing");
    goto finish;
  }

  dot_data = clapper_player_make_pipeline_graph (player, GST_DEBUG_GRAPH_SHOW_ALL);

  if (g_cancellable_is_cancelled (cancellable))
    goto close_and_finish;

  graph = agmemread (dot_data);

  gvc = gvContext ();
  gvLayout (gvc, graph, "dot");

#ifdef HAVE_GVC_13
  gvRenderData (gvc, graph, "svg", &img_data, &size);
#else
  {
    guint tmp_size = 0; // Temporary uint to satisfy older API
    gvRenderData (gvc, graph, "svg", &img_data, &tmp_size);
    size = tmp_size;
  }
#endif

  agclose (graph);
  gvFreeContext (gvc);

  if (g_cancellable_is_cancelled (cancellable))
    goto close_and_finish;

  if (write (fd, img_data, size) != -1) {
    tmp_file = g_file_new_for_path (template);
  } else {
    g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
        "Could not write data to temp file");
  }

close_and_finish:
  /* Always close the file IO */
  if (G_UNLIKELY (close (fd) == -1))
    GST_ERROR ("Could not close temp file!");

finish:
  g_free (template);
  g_free (dot_data);
  g_free (img_data);
#else
  g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
      "Cannot create graph file when compiled without Graphviz");
#endif

  if (tmp_file)
    g_task_return_pointer (task, tmp_file, (GDestroyNotify) g_object_unref);
  else
    g_task_return_error (task, error);
}

void
clapper_app_utils_create_pipeline_svg_file_async (ClapperPlayer *player,
    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GTask *task;

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_task_data (task, gst_object_ref (player), (GDestroyNotify) gst_object_unref);
  g_task_run_in_thread (task, (GTaskThreadFunc) _create_pipeline_svg_file_in_thread);

  g_object_unref (task);
}

static gboolean
_delete_dir_recursive (GFile *dir, GError **error)
{
  GFileEnumerator *dir_enum;

  if ((dir_enum = g_file_enumerate_children (dir,
      G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, error))) {
    while (TRUE) {
      GFileInfo *info = NULL;
      GFile *child = NULL;

      if (!g_file_enumerator_iterate (dir_enum, &info,
          &child, NULL, error) || !info)
        break;

      if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
        if (!_delete_dir_recursive (child, error))
          break;
      } else if (!g_file_delete (child, NULL, error)) {
        break;
      }
    }

    g_object_unref (dir_enum);
  }

  if (*error != NULL)
    return FALSE;

  return g_file_delete (dir, NULL, error);
}

void
clapper_app_utils_delete_tmp_dir (void)
{
  GFile *tmp_dir = _get_tmp_dir (NULL);
  GError *error = NULL;

  if (!_delete_dir_recursive (tmp_dir, &error)) {
    if (error->domain != G_IO_ERROR || error->code != G_IO_ERROR_NOT_FOUND) {
      GST_ERROR ("Could not remove temp dir, reason: %s",
          GST_STR_NULL (error->message));
    }
    g_error_free (error);
  }

  g_object_unref (tmp_dir);
}
