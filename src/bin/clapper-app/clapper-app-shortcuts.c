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

#include "clapper-app-shortcuts.h"

#define SETTINGS_KEY "shortcuts"

/* Number of keyboard layouts (groups) to look through for a latin one */
#define MAX_LAYOUT_GROUPS 4

/* Order here is the order in which shortcuts are listed in UI.
 * Left/Right (and < >) are mirrored in RTL languages. */
static const ClapperAppShortcutInfo shortcut_infos[] = {
  /* General */
  { "new-window", "app.new-window", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("New window"), FALSE, { "<Control>n", NULL }, { NULL }},
  { "preferences", "app.preferences", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Open preferences"), FALSE, { "<Control>comma", NULL }, { NULL }},
  { "show-help-overlay", "win.show-help-overlay", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Show shortcuts"), FALSE, { "<Control>question", NULL }, { NULL }},
  { "info", "app.info", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Show information"), FALSE, { "<Control>i", NULL }, { NULL }},
  { "pipeline", "app.pipeline", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Show pipeline"), FALSE, { "<Control><Shift>p", NULL }, { NULL }},
  { "about", "app.about", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("About"), FALSE, { "F1", NULL }, { NULL }},
  { "toggle-fullscreen", "win.toggle-fullscreen", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Toggle fullscreen"), FALSE, { "F11", "f", NULL }, { NULL }, N_("Double tap | Double click")},
  { "unfullscreen", "win.unfullscreen", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Unfullscreen"), FALSE, { "Escape", NULL }, { NULL }},
  { "auto-resize", "win.auto-resize", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Auto window resize"), FALSE, { "<Super>r", NULL }, { NULL }, N_("Middle click")},
  { "quit", "window.close", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
      N_("Quit"), FALSE, { "<Control>q", "q", NULL }, { NULL }},

  /* Media */
  { "add-files", "app.add-files", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_MEDIA,
      N_("Add Files…"), FALSE, { "<Control>o", NULL }, { NULL }},
  { "add-folder", "app.add-folder", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_MEDIA,
      N_("Add Folder…"), FALSE, { "<Control><Shift>o", NULL }, { NULL }},
  { "add-uri", "app.add-uri", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_MEDIA,
      N_("Add URI…"), FALSE, { "<Control>u", NULL }, { NULL }},

  /* Queue */
  { "next-item", NULL, CLAPPER_APP_SHORTCUT_KEY_NEXT_ITEM, CLAPPER_APP_SHORTCUT_GROUP_QUEUE,
      N_("Next item"), TRUE, { "<Control>Right", NULL }, { "<Control>Left", NULL }},
  { "previous-item", NULL, CLAPPER_APP_SHORTCUT_KEY_PREVIOUS_ITEM, CLAPPER_APP_SHORTCUT_GROUP_QUEUE,
      N_("Previous item"), TRUE, { "<Control>Left", NULL }, { "<Control>Right", NULL }},
  { "progression-mode", NULL, CLAPPER_APP_SHORTCUT_KEY_PROGRESSION_MODE, CLAPPER_APP_SHORTCUT_GROUP_QUEUE,
      N_("Change progression mode"), FALSE, { "p", NULL }, { NULL }},

  /* Playback */
  { "toggle-play", NULL, CLAPPER_APP_SHORTCUT_KEY_TOGGLE_PLAY, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Toggle play"), FALSE, { "space", "k", NULL }, { NULL }, N_("Tap | Left click")},
  { "seek-forward", NULL, CLAPPER_APP_SHORTCUT_KEY_SEEK_FORWARD, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Seek forward"), TRUE, { "Right", "l", NULL }, { "Left", "j", NULL }, N_("Double tap (right side) | Scroll right"), N_("Double tap (left side) | Scroll left")},
  { "seek-backward", NULL, CLAPPER_APP_SHORTCUT_KEY_SEEK_BACKWARD, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Seek backward"), TRUE, { "Left", "j", NULL }, { "Right", "l", NULL }, N_("Double tap (left side) | Scroll left"), N_("Double tap (right side) | Scroll right")},
  { "volume-up", NULL, CLAPPER_APP_SHORTCUT_KEY_VOLUME_UP, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Volume up"), TRUE, { "Up", NULL }, { NULL }, N_("Scroll up")},
  { "volume-down", NULL, CLAPPER_APP_SHORTCUT_KEY_VOLUME_DOWN, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Volume down"), TRUE, { "Down", NULL }, { NULL }, N_("Scroll down")},
  { "toggle-mute", NULL, CLAPPER_APP_SHORTCUT_KEY_TOGGLE_MUTE, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Toggle mute"), FALSE, { "m", NULL }, { NULL }},
  { "toggle-subtitles", "win.toggle-subtitles", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Toggle subtitles"), FALSE, { "<Alt>h", NULL }, { NULL }},
  { "cycle-subtitle-stream", "win.cycle-subtitle-stream", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Cycle subtitle track"), FALSE, { "<Alt>l", NULL }, { NULL }},
  { "cycle-audio-stream", "win.cycle-audio-stream", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Cycle audio track"), FALSE, { "<Alt>a", NULL }, { NULL }},
  { "next-subtitle", "win.next-subtitle", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Next subtitle"), FALSE, { "End", NULL }, { NULL }},
  { "previous-subtitle", "win.previous-subtitle", CLAPPER_APP_SHORTCUT_KEY_NONE, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Previous subtitle"), FALSE, { "Home", NULL }, { NULL }},
  { "speed-up", NULL, CLAPPER_APP_SHORTCUT_KEY_SPEED_UP, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Speed up"), FALSE, { "greater", NULL }, { "less", NULL }},
  { "speed-down", NULL, CLAPPER_APP_SHORTCUT_KEY_SPEED_DOWN, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Speed down"), FALSE, { "less", NULL }, { "greater", NULL }},
  { "advance-frame", NULL, CLAPPER_APP_SHORTCUT_KEY_ADVANCE_FRAME, CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
      N_("Advance frame"), TRUE, { "e", NULL }, { NULL }},

  /* Chapters */
  { "next-chapter", NULL, CLAPPER_APP_SHORTCUT_KEY_NEXT_CHAPTER, CLAPPER_APP_SHORTCUT_GROUP_CHAPTERS,
      N_("Next chapter"), FALSE, { "<Shift>Right", NULL }, { "<Shift>Left", NULL }},
  { "previous-chapter", NULL, CLAPPER_APP_SHORTCUT_KEY_PREVIOUS_CHAPTER, CLAPPER_APP_SHORTCUT_GROUP_CHAPTERS,
      N_("Previous chapter"), FALSE, { "<Shift>Left", NULL }, { "<Shift>Right", NULL }},
};

typedef struct
{
  const ClapperAppShortcutInfo *info;
  guint keyval;
  GdkModifierType mods;
} ClapperAppParsedAccel;

struct _ClapperAppShortcuts
{
  GObject parent;

  GSettings *settings;
  gboolean has_key; // FALSE when schema is older than this code
  GPtrArray *parsed; // ClapperAppParsedAccel of all shortcuts
};

enum
{
  SIGNAL_CHANGED,
  SIGNAL_LAST
};

#define parent_class clapper_app_shortcuts_parent_class
G_DEFINE_FINAL_TYPE (ClapperAppShortcuts, clapper_app_shortcuts, G_TYPE_OBJECT)

static guint signals[SIGNAL_LAST] = { 0, };

static ClapperAppShortcuts *default_shortcuts = NULL;

static inline gboolean
_is_rtl (void)
{
  return (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
}

static inline gboolean
_parse_accel (const gchar *accel, guint *keyval, GdkModifierType *mods)
{
  return (accel != NULL
      && gtk_accelerator_parse (accel, keyval, mods)
      && *keyval != 0);
}

/* Compare accelerators by meaning instead of text */
static gboolean
_accels_equal (const gchar *a, const gchar *b)
{
  guint a_keyval, b_keyval;
  GdkModifierType a_mods, b_mods;

  if (!_parse_accel (a, &a_keyval, &a_mods) || !_parse_accel (b, &b_keyval, &b_mods))
    return FALSE;

  return (gdk_keyval_to_lower (a_keyval) == gdk_keyval_to_lower (b_keyval)
      && a_mods == b_mods);
}

static gboolean
_accels_strv_contains (gchar **accels, const gchar *accel)
{
  guint i;

  for (i = 0; accels && accels[i]; ++i) {
    if (_accels_equal (accels[i], accel))
      return TRUE;
  }

  return FALSE;
}

static gchar **
_dup_defaults (const ClapperAppShortcutInfo *info)
{
  const gchar * const *src = (_is_rtl () && info->defaults_rtl[0])
      ? info->defaults_rtl
      : info->defaults;

  return g_strdupv ((gchar **) src);
}

static gboolean
_strv_equal (gchar **a, gchar **b)
{
  guint i;

  if (g_strv_length (a) != g_strv_length (b))
    return FALSE;

  for (i = 0; a[i]; ++i) {
    if (!_accels_equal (a[i], b[i]))
      return FALSE;
  }

  return TRUE;
}

/* Returns NULL when shortcut is not customized */
static gchar **
_dup_override (ClapperAppShortcuts *self, const ClapperAppShortcutInfo *info)
{
  GVariant *all, *value;
  gchar **strv = NULL;

  if (!self->has_key)
    return NULL;

  all = g_settings_get_value (self->settings, SETTINGS_KEY);
  value = g_variant_lookup_value (all, info->id, G_VARIANT_TYPE_STRING_ARRAY);

  if (value) {
    gchar **raw = g_variant_dup_strv (value, NULL);
    GStrvBuilder *builder = g_strv_builder_new ();
    guint i;

    /* Drop anything that cannot be understood (e.g. hand-edited values) */
    for (i = 0; raw[i]; ++i) {
      guint keyval;
      GdkModifierType mods;

      if (_parse_accel (raw[i], &keyval, &mods))
        g_strv_builder_add (builder, raw[i]);
    }

    strv = g_strv_builder_end (builder);

    /* Only garbage stored (e.g. edited by hand), use defaults instead of
     * silently disabling the shortcut. Empty list stored on purpose is fine. */
    if (g_strv_length (raw) > 0 && g_strv_length (strv) == 0)
      g_clear_pointer (&strv, g_strfreev);

    g_strv_builder_unref (builder);
    g_strfreev (raw);
    g_variant_unref (value);
  }

  g_variant_unref (all);

  return strv;
}

static void
_rebuild_parsed (ClapperAppShortcuts *self)
{
  guint i;

  g_ptr_array_set_size (self->parsed, 0);

  for (i = 0; i < G_N_ELEMENTS (shortcut_infos); ++i) {
    const ClapperAppShortcutInfo *info = &shortcut_infos[i];
    gchar **accels;
    guint j;

    accels = clapper_app_shortcuts_get_accels (self, info);

    for (j = 0; accels[j]; ++j) {
      ClapperAppParsedAccel *parsed = g_new (ClapperAppParsedAccel, 1);

      if (_parse_accel (accels[j], &parsed->keyval, &parsed->mods)) {
        parsed->info = info;
        g_ptr_array_add (self->parsed, parsed);
      } else {
        g_free (parsed);
      }
    }

    g_strfreev (accels);
  }
}

static void
_settings_changed_cb (GSettings *settings, const gchar *key, ClapperAppShortcuts *self)
{
  _rebuild_parsed (self);
  g_signal_emit (self, signals[SIGNAL_CHANGED], 0);
}

ClapperAppShortcuts *
clapper_app_shortcuts_get_default (void)
{
  if (!default_shortcuts)
    default_shortcuts = g_object_new (CLAPPER_APP_TYPE_SHORTCUTS, NULL);

  return default_shortcuts;
}

const gchar *
clapper_app_shortcuts_group_get_title (ClapperAppShortcutGroup group)
{
  switch (group) {
    case CLAPPER_APP_SHORTCUT_GROUP_GENERAL:
      return _("General");
    case CLAPPER_APP_SHORTCUT_GROUP_MEDIA:
      return _("Media");
    case CLAPPER_APP_SHORTCUT_GROUP_QUEUE:
      return _("Queue");
    case CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK:
      return _("Playback");
    case CLAPPER_APP_SHORTCUT_GROUP_CHAPTERS:
      return _("Chapters");
    default:
      break;
  }

  return NULL;
}

guint
clapper_app_shortcuts_get_n_infos (void)
{
  return G_N_ELEMENTS (shortcut_infos);
}

const ClapperAppShortcutInfo *
clapper_app_shortcuts_get_info (guint index)
{
  return (index < G_N_ELEMENTS (shortcut_infos))
      ? &shortcut_infos[index]
      : NULL;
}

const ClapperAppShortcutInfo *
clapper_app_shortcuts_lookup (const gchar *id)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (shortcut_infos); ++i) {
    if (strcmp (shortcut_infos[i].id, id) == 0)
      return &shortcut_infos[i];
  }

  return NULL;
}

gchar **
clapper_app_shortcuts_get_accels (ClapperAppShortcuts *self, const ClapperAppShortcutInfo *info)
{
  gchar **accels = _dup_override (self, info);

  return (accels) ? accels : _dup_defaults (info);
}

gboolean
clapper_app_shortcuts_is_customized (ClapperAppShortcuts *self, const ClapperAppShortcutInfo *info)
{
  gchar **override = _dup_override (self, info);
  gboolean customized = (override != NULL);

  g_strfreev (override);

  return customized;
}

static void
_store_override (ClapperAppShortcuts *self, const ClapperAppShortcutInfo *info,
    const gchar * const *accels)
{
  GVariant *all;
  GVariantBuilder builder;
  GVariantIter iter;
  gchar *key;
  GVariant *value;

  if (!self->has_key)
    return;

  all = g_settings_get_value (self->settings, SETTINGS_KEY);
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sas}"));

  g_variant_iter_init (&iter, all);
  while (g_variant_iter_next (&iter, "{s@as}", &key, &value)) {
    if (strcmp (key, info->id) != 0)
      g_variant_builder_add (&builder, "{s@as}", key, value);

    g_free (key);
    g_variant_unref (value);
  }

  if (accels) // NULL = remove customization
    g_variant_builder_add (&builder, "{s@as}", info->id, g_variant_new_strv (accels, -1));

  g_settings_set_value (self->settings, SETTINGS_KEY, g_variant_builder_end (&builder));

  g_variant_unref (all);
}

void
clapper_app_shortcuts_set_accels (ClapperAppShortcuts *self,
    const ClapperAppShortcutInfo *info, const gchar * const *accels)
{
  gchar **defaults = _dup_defaults (info);

  /* No need to keep it stored when same as default */
  if (_strv_equal (defaults, (gchar **) accels))
    _store_override (self, info, NULL);
  else
    _store_override (self, info, accels);

  g_strfreev (defaults);
}

const ClapperAppShortcutInfo *
clapper_app_shortcuts_assign (ClapperAppShortcuts *self,
    const ClapperAppShortcutInfo *info, const gchar *accel, gboolean append)
{
  const ClapperAppShortcutInfo *taken_from = NULL;
  GStrvBuilder *builder;
  gchar **current, **result;
  guint i;

  /* Every accelerator can only trigger one thing */
  for (i = 0; i < G_N_ELEMENTS (shortcut_infos); ++i) {
    const ClapperAppShortcutInfo *other = &shortcut_infos[i];
    gchar **other_accels;

    if (other == info)
      continue;

    other_accels = clapper_app_shortcuts_get_accels (self, other);

    if (_accels_strv_contains (other_accels, accel)) {
      GStrvBuilder *other_builder = g_strv_builder_new ();
      gchar **remaining;
      guint j;

      for (j = 0; other_accels[j]; ++j) {
        if (!_accels_equal (other_accels[j], accel))
          g_strv_builder_add (other_builder, other_accels[j]);
      }

      remaining = g_strv_builder_end (other_builder);
      clapper_app_shortcuts_set_accels (self, other, (const gchar * const *) remaining);

      if (!taken_from)
        taken_from = other;

      g_strfreev (remaining);
      g_strv_builder_unref (other_builder);
    }

    g_strfreev (other_accels);
  }

  builder = g_strv_builder_new ();
  current = (append) ? clapper_app_shortcuts_get_accels (self, info) : NULL;

  for (i = 0; current && current[i]; ++i) {
    if (!_accels_equal (current[i], accel))
      g_strv_builder_add (builder, current[i]);
  }
  g_strv_builder_add (builder, accel);

  result = g_strv_builder_end (builder);

  /* Make room by dropping the oldest ones */
  if (g_strv_length (result) > CLAPPER_APP_SHORTCUTS_MAX_ACCELS) {
    guint offset = g_strv_length (result) - CLAPPER_APP_SHORTCUTS_MAX_ACCELS;
    gchar **trimmed = g_strdupv (result + offset);

    g_strfreev (result);
    result = trimmed;
  }

  clapper_app_shortcuts_set_accels (self, info, (const gchar * const *) result);

  g_strfreev (result);
  g_strfreev (current);
  g_strv_builder_unref (builder);

  return taken_from;
}

void
clapper_app_shortcuts_reset (ClapperAppShortcuts *self, const ClapperAppShortcutInfo *info)
{
  _store_override (self, info, NULL);
}

void
clapper_app_shortcuts_reset_all (ClapperAppShortcuts *self)
{
  if (self->has_key)
    g_settings_reset (self->settings, SETTINGS_KEY);
}

void
clapper_app_shortcuts_apply (ClapperAppShortcuts *self, GtkApplication *application)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (shortcut_infos); ++i) {
    const ClapperAppShortcutInfo *info = &shortcut_infos[i];
    gchar **accels;

    if (!info->action)
      continue;

    accels = clapper_app_shortcuts_get_accels (self, info);
    gtk_application_set_accels_for_action (application,
        info->action, (const gchar * const *) accels);
    g_strfreev (accels);
  }
}

static inline void
_canonical_from_keyval (guint keyval, GdkModifierType state, GdkModifierType consumed,
    guint *out_keyval, GdkModifierType *out_mods)
{
  GdkModifierType mask = gtk_accelerator_get_default_mod_mask ();
  guint lower = gdk_keyval_to_lower (keyval);
  GdkModifierType mods;

  if (lower != keyval) {
    /* Letters: Shift is an explicit part of the shortcut */
    keyval = lower;
    mods = state & mask;
  } else {
    /* Symbols: Shift is already part of the symbol (e.g. "<" is Shift+","
     * on some layouts) so it should not be counted twice */
    mods = state & mask & ~consumed;
  }

  if (keyval == GDK_KEY_ISO_Left_Tab) {
    keyval = GDK_KEY_Tab;
    mods |= GDK_SHIFT_MASK;
  }

  *out_keyval = keyval;
  *out_mods = mods;
}

/* What the same physical key types in keyboard layout (group) number @group.
 * Returns FALSE if there is no such layout or it types no printable character. */
static gboolean
_translate_in_group (GdkEvent *event, gint group, guint *keyval, GdkModifierType *consumed)
{
  GdkModifierType state = gdk_event_get_modifier_state (event);
  gint effective_group, level;

  *keyval = 0;
  *consumed = 0;

  /* Only Shift picks level of a key, other modifiers are not part of it */
  if (!gdk_display_translate_key (gdk_event_get_display (event),
      gdk_key_event_get_keycode (event), state & GDK_SHIFT_MASK, group,
      keyval, &effective_group, &level, consumed))
    return FALSE;

  return (effective_group == group && *keyval >= 0x20 && *keyval < 0x7f);
}

const ClapperAppShortcutInfo *
clapper_app_shortcuts_match_event (ClapperAppShortcuts *self, GdkEvent *event)
{
  GdkModifierType state;
  guint i;
  gint group;

  if (!event || gdk_event_get_event_type (event) != GDK_KEY_PRESS)
    return NULL;

  /* Typed character in currently active layout always wins */
  for (i = 0; i < self->parsed->len; ++i) {
    ClapperAppParsedAccel *parsed = g_ptr_array_index (self->parsed, i);

    if (gdk_key_event_matches (event, parsed->keyval, parsed->mods) == GDK_KEY_MATCH_EXACT)
      return parsed->info;
  }

  /* Then look at what this physical key types in every other layout. We do
   * it ourselves instead of relying on GDK "partial" matches, as those need
   * same shift level and are skipped when active layout has the wanted
   * character anywhere else, which breaks e.g. "<" and ">" in Ukrainian. */
  state = gdk_event_get_modifier_state (event);

  for (group = 0; group < MAX_LAYOUT_GROUPS; ++group) {
    guint keyval;
    GdkModifierType consumed, mods;

    if (!_translate_in_group (event, group, &keyval, &consumed))
      continue;

    _canonical_from_keyval (keyval, state, consumed, &keyval, &mods);

    for (i = 0; i < self->parsed->len; ++i) {
      ClapperAppParsedAccel *parsed = g_ptr_array_index (self->parsed, i);

      if (parsed->keyval == keyval && parsed->mods == mods)
        return parsed->info;
    }
  }

  /* Non-printable keys (arrows, F-keys) are the same in all layouts */
  {
    guint keyval;
    GdkModifierType mods;

    _canonical_from_keyval (gdk_key_event_get_keyval (event), state,
        gdk_key_event_get_consumed_modifiers (event), &keyval, &mods);

    for (i = 0; i < self->parsed->len; ++i) {
      ClapperAppParsedAccel *parsed = g_ptr_array_index (self->parsed, i);

      if (parsed->keyval == keyval && parsed->mods == mods)
        return parsed->info;
    }
  }

  return NULL;
}

gchar *
clapper_app_shortcuts_accel_from_event (GdkEvent *event)
{
  guint keyval;
  GdkModifierType state, consumed, mods;
  gint group;

  if (!event || gdk_event_get_event_type (event) != GDK_KEY_PRESS)
    return NULL;

  if (gdk_key_event_is_modifier (event))
    return NULL;

  keyval = gdk_key_event_get_keyval (event);
  state = gdk_event_get_modifier_state (event);
  consumed = gdk_key_event_get_consumed_modifiers (event);

  /* Keys that type a character: store the one from first layout that has
   * a latin character on this key, so the shortcut is the same no matter
   * which layout was active when it was recorded (and matches the same
   * physical key later). Function keys etc. are left as they are. */
  if (gdk_keyval_to_unicode (keyval) != 0) {
    for (group = 0; group < MAX_LAYOUT_GROUPS; ++group) {
      guint latin_keyval;
      GdkModifierType latin_consumed;

      if (_translate_in_group (event, group, &latin_keyval, &latin_consumed)) {
        keyval = latin_keyval;
        consumed = latin_consumed;
        break;
      }
    }
  }

  if (keyval == 0)
    return NULL;

  _canonical_from_keyval (keyval, state, consumed, &keyval, &mods);

  return gtk_accelerator_name (keyval, mods);
}

static void
clapper_app_shortcuts_init (ClapperAppShortcuts *self)
{
  GSettingsSchema *schema = NULL;

  self->settings = g_settings_new (CLAPPER_APP_ID);
  self->parsed = g_ptr_array_new_with_free_func (g_free);

  /* Do not crash (GSettings aborts on unknown keys) if installed schema
   * is older than this code, only run with default shortcuts then */
  g_object_get (self->settings, "settings-schema", &schema, NULL);
  self->has_key = g_settings_schema_has_key (schema, SETTINGS_KEY);
  g_settings_schema_unref (schema);

  if (!self->has_key)
    g_warning ("Settings schema has no \"" SETTINGS_KEY "\" key, shortcuts cannot be customized");

  _rebuild_parsed (self);

  g_signal_connect (self->settings, "changed::" SETTINGS_KEY,
      G_CALLBACK (_settings_changed_cb), self);
}

static void
clapper_app_shortcuts_finalize (GObject *object)
{
  ClapperAppShortcuts *self = CLAPPER_APP_SHORTCUTS (object);

  g_signal_handlers_disconnect_by_data (self->settings, self);
  g_object_unref (self->settings);
  g_ptr_array_unref (self->parsed);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clapper_app_shortcuts_class_init (ClapperAppShortcutsClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->finalize = clapper_app_shortcuts_finalize;

  signals[SIGNAL_CHANGED] = g_signal_new ("changed",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
      0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}
