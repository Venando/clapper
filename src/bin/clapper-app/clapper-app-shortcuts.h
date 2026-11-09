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

#pragma once

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CLAPPER_APP_TYPE_SHORTCUTS (clapper_app_shortcuts_get_type ())
G_DECLARE_FINAL_TYPE (ClapperAppShortcuts, clapper_app_shortcuts, CLAPPER_APP, SHORTCUTS, GObject)

#define CLAPPER_APP_SHORTCUTS_MAX_ACCELS 4

typedef enum
{
  CLAPPER_APP_SHORTCUT_GROUP_GENERAL,
  CLAPPER_APP_SHORTCUT_GROUP_MEDIA,
  CLAPPER_APP_SHORTCUT_GROUP_QUEUE,
  CLAPPER_APP_SHORTCUT_GROUP_PLAYBACK,
  CLAPPER_APP_SHORTCUT_GROUP_CHAPTERS,
  CLAPPER_APP_SHORTCUT_N_GROUPS
} ClapperAppShortcutGroup;

/* Shortcuts that are not backed by a GAction are handled by the
 * window itself, this identifies what such shortcut should do */
typedef enum
{
  CLAPPER_APP_SHORTCUT_KEY_NONE = 0,
  CLAPPER_APP_SHORTCUT_KEY_TOGGLE_PLAY,
  CLAPPER_APP_SHORTCUT_KEY_SEEK_FORWARD,
  CLAPPER_APP_SHORTCUT_KEY_SEEK_BACKWARD,
  CLAPPER_APP_SHORTCUT_KEY_VOLUME_UP,
  CLAPPER_APP_SHORTCUT_KEY_VOLUME_DOWN,
  CLAPPER_APP_SHORTCUT_KEY_TOGGLE_MUTE,
  CLAPPER_APP_SHORTCUT_KEY_NEXT_ITEM,
  CLAPPER_APP_SHORTCUT_KEY_PREVIOUS_ITEM,
  CLAPPER_APP_SHORTCUT_KEY_PROGRESSION_MODE,
  CLAPPER_APP_SHORTCUT_KEY_SPEED_UP,
  CLAPPER_APP_SHORTCUT_KEY_SPEED_DOWN,
  CLAPPER_APP_SHORTCUT_KEY_ADVANCE_FRAME,
  CLAPPER_APP_SHORTCUT_KEY_NEXT_CHAPTER,
  CLAPPER_APP_SHORTCUT_KEY_PREVIOUS_CHAPTER
} ClapperAppShortcutKey;

typedef struct
{
  const gchar *id;
  const gchar *action; /* GAction to activate, NULL when handled by window */
  ClapperAppShortcutKey key;
  ClapperAppShortcutGroup group;
  const gchar *title; /* untranslated */
  gboolean repeatable; /* whether holding the key repeats the shortcut */
  const gchar *defaults[CLAPPER_APP_SHORTCUTS_MAX_ACCELS + 1];
  const gchar *defaults_rtl[CLAPPER_APP_SHORTCUTS_MAX_ACCELS + 1]; /* NULL = same as LTR */
  const gchar *subtitle; /* untranslated, hint about other ways to do the same */
  const gchar *subtitle_rtl; /* NULL = same as LTR */
} ClapperAppShortcutInfo;

G_GNUC_INTERNAL
ClapperAppShortcuts * clapper_app_shortcuts_get_default (void);

G_GNUC_INTERNAL
const gchar * clapper_app_shortcuts_group_get_title (ClapperAppShortcutGroup group);

G_GNUC_INTERNAL
guint clapper_app_shortcuts_get_n_infos (void);

G_GNUC_INTERNAL
const ClapperAppShortcutInfo * clapper_app_shortcuts_get_info (guint index);

G_GNUC_INTERNAL
const ClapperAppShortcutInfo * clapper_app_shortcuts_lookup (const gchar *id);

/* Currently effective accelerators (NULL terminated, free with g_strfreev) */
G_GNUC_INTERNAL
gchar ** clapper_app_shortcuts_get_accels (ClapperAppShortcuts *shortcuts, const ClapperAppShortcutInfo *info);

G_GNUC_INTERNAL
gboolean clapper_app_shortcuts_is_customized (ClapperAppShortcuts *shortcuts, const ClapperAppShortcutInfo *info);

/* Replaces all accelerators of a shortcut, empty list disables it */
G_GNUC_INTERNAL
void clapper_app_shortcuts_set_accels (ClapperAppShortcuts *shortcuts, const ClapperAppShortcutInfo *info, const gchar * const *accels);

/* Assigns accelerator, taking it away from any other shortcut that uses it.
 * Returns that other shortcut (or NULL) so the UI can tell the user. */
G_GNUC_INTERNAL
const ClapperAppShortcutInfo * clapper_app_shortcuts_assign (ClapperAppShortcuts *shortcuts, const ClapperAppShortcutInfo *info, const gchar *accel, gboolean append);

G_GNUC_INTERNAL
void clapper_app_shortcuts_reset (ClapperAppShortcuts *shortcuts, const ClapperAppShortcutInfo *info);

G_GNUC_INTERNAL
void clapper_app_shortcuts_reset_all (ClapperAppShortcuts *shortcuts);

/* Sets accelerators of all action based shortcuts on the application */
G_GNUC_INTERNAL
void clapper_app_shortcuts_apply (ClapperAppShortcuts *shortcuts, GtkApplication *application);

/* Finds window handled shortcut triggered by a key event. Matching ignores
 * active keyboard layout, so it works when e.g. a non-latin one is in use. */
G_GNUC_INTERNAL
const ClapperAppShortcutInfo * clapper_app_shortcuts_match_event (ClapperAppShortcuts *shortcuts, GdkEvent *event);

/* Converts key press to accelerator string that is the same regardless of
 * layout used (when possible). Returns NULL for events not usable as shortcut. */
G_GNUC_INTERNAL
gchar * clapper_app_shortcuts_accel_from_event (GdkEvent *event);

G_END_DECLS
