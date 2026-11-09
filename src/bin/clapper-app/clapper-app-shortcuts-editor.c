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

#include <glib/gi18n.h>

#include "clapper-app-shortcuts-editor.h"
#include "clapper-app-shortcuts.h"
#include "clapper-app-shortcut-dialog.h"

typedef struct
{
  AdwPreferencesWindow *window;
  const ClapperAppShortcutInfo *info;
  gboolean append;
} ClapperAppShortcutEditData;

static void
_refresh_row (AdwActionRow *row)
{
  ClapperAppShortcuts *shortcuts = clapper_app_shortcuts_get_default ();
  const ClapperAppShortcutInfo *info = g_object_get_data (G_OBJECT (row), "info");
  GtkWidget *label = g_object_get_data (G_OBJECT (row), "label");
  GtkWidget *add_button = g_object_get_data (G_OBJECT (row), "add-button");
  GtkWidget *reset_button = g_object_get_data (G_OBJECT (row), "reset-button");
  gchar **accels = clapper_app_shortcuts_get_accels (shortcuts, info);
  gchar *joined = g_strjoinv (" ", accels);

  gtk_shortcut_label_set_accelerator (GTK_SHORTCUT_LABEL (label), joined);
  gtk_widget_set_sensitive (add_button,
      g_strv_length (accels) < CLAPPER_APP_SHORTCUTS_MAX_ACCELS);
  gtk_widget_set_visible (reset_button,
      clapper_app_shortcuts_is_customized (shortcuts, info));

  g_free (joined);
  g_strfreev (accels);
}

static void
_changed_cb (ClapperAppShortcuts *shortcuts, AdwPreferencesPage *page)
{
  GPtrArray *rows = g_object_get_data (G_OBJECT (page), "shortcut-rows");
  guint i;

  for (i = 0; rows && i < rows->len; ++i)
    _refresh_row (g_ptr_array_index (rows, i));
}

static void
_captured_cb (const gchar *accel, ClapperAppShortcutEditData *edit)
{
  ClapperAppShortcuts *shortcuts = clapper_app_shortcuts_get_default ();

  if (accel) {
    const ClapperAppShortcutInfo *taken_from;

    taken_from = clapper_app_shortcuts_assign (shortcuts, edit->info, accel, edit->append);

    if (taken_from) {
      gchar *text = g_strdup_printf (
          _("Key combination was used by “%s”, it was removed from there"),
          _(taken_from->title));

      adw_preferences_window_add_toast (edit->window, adw_toast_new (text));
      g_free (text);
    }
  } else {
    static const gchar * const none[] = { NULL };

    clapper_app_shortcuts_set_accels (shortcuts, edit->info, none);
  }
}

static void
_present_dialog (AdwActionRow *row, gboolean append)
{
  ClapperAppShortcutEditData *edit = g_new (ClapperAppShortcutEditData, 1);

  edit->window = g_object_get_data (G_OBJECT (row), "window");
  edit->info = g_object_get_data (G_OBJECT (row), "info");
  edit->append = append;

  clapper_app_shortcut_dialog_present (GTK_WINDOW (edit->window), _(edit->info->title),
      (ClapperAppShortcutDialogFunc) _captured_cb, edit, g_free);
}

static void
_row_activated_cb (AdwActionRow *row, gpointer user_data)
{
  _present_dialog (row, FALSE);
}

static void
_add_clicked_cb (GtkButton *button, AdwActionRow *row)
{
  _present_dialog (row, TRUE);
}

static void
_reset_clicked_cb (GtkButton *button, AdwActionRow *row)
{
  clapper_app_shortcuts_reset (clapper_app_shortcuts_get_default (),
      g_object_get_data (G_OBJECT (row), "info"));
}

static void
_reset_all_clicked_cb (GtkButton *button, gpointer user_data)
{
  clapper_app_shortcuts_reset_all (clapper_app_shortcuts_get_default ());
}

void
clapper_app_shortcuts_editor_populate (AdwPreferencesPage *page, AdwPreferencesWindow *window)
{
  ClapperAppShortcuts *shortcuts = clapper_app_shortcuts_get_default ();
  AdwPreferencesGroup *groups[CLAPPER_APP_SHORTCUT_N_GROUPS] = { NULL, };
  GPtrArray *rows;
  GtkWidget *intro, *reset_all_button;
  gboolean is_rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
  guint i, n_infos = clapper_app_shortcuts_get_n_infos ();

  /* Rows are owned by the page, we just need a list to refresh them */
  rows = g_ptr_array_new ();
  g_object_set_data_full (G_OBJECT (page), "shortcut-rows", rows,
      (GDestroyNotify) g_ptr_array_unref);

  intro = adw_preferences_group_new ();
  adw_preferences_group_set_title (ADW_PREFERENCES_GROUP (intro), _("Keyboard Shortcuts"));
  adw_preferences_group_set_description (ADW_PREFERENCES_GROUP (intro),
      _("Select a shortcut to change it. Shortcuts work regardless of the active keyboard layout."));

  reset_all_button = gtk_button_new_with_label (_("Reset All"));
  gtk_widget_set_valign (reset_all_button, GTK_ALIGN_CENTER);
  g_signal_connect (reset_all_button, "clicked", G_CALLBACK (_reset_all_clicked_cb), NULL);
  adw_preferences_group_set_header_suffix (ADW_PREFERENCES_GROUP (intro), reset_all_button);
  adw_preferences_page_add (page, ADW_PREFERENCES_GROUP (intro));

  for (i = 0; i < n_infos; ++i) {
    const ClapperAppShortcutInfo *info = clapper_app_shortcuts_get_info (i);
    const gchar *subtitle;
    GtkWidget *row, *label, *add_button, *reset_button;

    if (!groups[info->group]) {
      groups[info->group] = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
      adw_preferences_group_set_title (groups[info->group],
          clapper_app_shortcuts_group_get_title (info->group));
      adw_preferences_page_add (page, groups[info->group]);
    }

    row = adw_action_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), _(info->title));
    gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), TRUE);

    subtitle = (is_rtl && info->subtitle_rtl) ? info->subtitle_rtl : info->subtitle;
    if (subtitle)
      adw_action_row_set_subtitle (ADW_ACTION_ROW (row), _(subtitle));

    label = gtk_shortcut_label_new ("");
    gtk_shortcut_label_set_disabled_text (GTK_SHORTCUT_LABEL (label), _("Disabled"));
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);

    add_button = gtk_button_new_from_icon_name ("list-add-symbolic");
    gtk_widget_set_valign (add_button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (add_button, "flat");
    gtk_widget_set_tooltip_text (add_button, _("Add Alternative Shortcut"));

    reset_button = gtk_button_new_from_icon_name ("edit-undo-symbolic");
    gtk_widget_set_valign (reset_button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class (reset_button, "flat");
    gtk_widget_set_tooltip_text (reset_button, _("Reset to Default"));

    adw_action_row_add_suffix (ADW_ACTION_ROW (row), label);
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), add_button);
    adw_action_row_add_suffix (ADW_ACTION_ROW (row), reset_button);

    g_object_set_data (G_OBJECT (row), "window", window);
    g_object_set_data (G_OBJECT (row), "info", (gpointer) info);
    g_object_set_data (G_OBJECT (row), "label", label);
    g_object_set_data (G_OBJECT (row), "add-button", add_button);
    g_object_set_data (G_OBJECT (row), "reset-button", reset_button);

    g_signal_connect (row, "activated", G_CALLBACK (_row_activated_cb), NULL);
    g_signal_connect (add_button, "clicked", G_CALLBACK (_add_clicked_cb), row);
    g_signal_connect (reset_button, "clicked", G_CALLBACK (_reset_clicked_cb), row);

    adw_preferences_group_add (groups[info->group], row);
    g_ptr_array_add (rows, row);

    _refresh_row (ADW_ACTION_ROW (row));
  }

  /* Handler is disconnected automatically when page is gone */
  g_signal_connect_object (shortcuts, "changed", G_CALLBACK (_changed_cb), page, 0);
}
