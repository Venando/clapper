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
#include <adwaita.h>

#include "clapper-app-shortcut-dialog.h"
#include "clapper-app-shortcuts.h"

typedef struct
{
  ClapperAppShortcutDialogFunc func;
  gpointer user_data;
  GDestroyNotify destroy_notify;
} ClapperAppShortcutDialogData;

static void
_dialog_data_free (ClapperAppShortcutDialogData *data)
{
  if (data->destroy_notify)
    data->destroy_notify (data->user_data);

  g_free (data);
}

static gboolean
key_pressed_cb (GtkEventControllerKey *controller, guint keyval,
    guint keycode, GdkModifierType state, GtkWindow *window)
{
  ClapperAppShortcutDialogData *data;
  GdkEvent *event;
  gchar *accel;

  /* Escape always cancels. Shortcut that uses it by default
   * can be restored with reset button in preferences. */
  if (keyval == GDK_KEY_Escape && (state & gtk_accelerator_get_default_mod_mask ()) == 0) {
    gtk_window_destroy (window);
    return TRUE;
  }

  event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (controller));

  /* Only modifiers pressed so far, wait for the rest of combination */
  if (!(accel = clapper_app_shortcuts_accel_from_event (event)))
    return TRUE;

  data = g_object_get_data (G_OBJECT (window), "dialog-data");
  data->func (accel, data->user_data);

  g_free (accel);
  gtk_window_destroy (window);

  return TRUE;
}

static void
disable_clicked_cb (GtkButton *button, GtkWindow *window)
{
  ClapperAppShortcutDialogData *data = g_object_get_data (G_OBJECT (window), "dialog-data");

  data->func (NULL, data->user_data);
  gtk_window_destroy (window);
}

static void
cancel_clicked_cb (GtkButton *button, GtkWindow *window)
{
  gtk_window_destroy (window);
}

void
clapper_app_shortcut_dialog_present (GtkWindow *parent, const gchar *shortcut_title,
    ClapperAppShortcutDialogFunc func, gpointer user_data, GDestroyNotify destroy_notify)
{
  ClapperAppShortcutDialogData *data;
  GtkWidget *window, *box, *page, *buttons, *cancel_button, *disable_button;
  GtkEventController *controller;
  gchar *description;

  data = g_new (ClapperAppShortcutDialogData, 1);
  data->func = func;
  data->user_data = user_data;
  data->destroy_notify = destroy_notify;

  window = gtk_window_new ();
  gtk_window_set_title (GTK_WINDOW (window), _("Set Shortcut"));
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window), parent);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (window), 380, -1);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  g_object_set_data_full (G_OBJECT (window), "dialog-data", data, (GDestroyNotify) _dialog_data_free);

  description = g_strdup_printf (_("Press the new key combination for “%s”.\nPress Esc to cancel."),
      shortcut_title);

  page = adw_status_page_new ();
  adw_status_page_set_icon_name (ADW_STATUS_PAGE (page), "input-keyboard-symbolic");
  adw_status_page_set_title (ADW_STATUS_PAGE (page), _("Press a Key Combination"));
  adw_status_page_set_description (ADW_STATUS_PAGE (page), description);
  gtk_widget_set_vexpand (page, TRUE);

  g_free (description);

  cancel_button = gtk_button_new_with_label (_("Cancel"));
  disable_button = gtk_button_new_with_label (_("Disable Shortcut"));
  gtk_widget_add_css_class (disable_button, "destructive-action");

  /* Keys are grabbed here to be assigned, so keep focus off the buttons */
  gtk_widget_set_focusable (cancel_button, FALSE);
  gtk_widget_set_focusable (disable_button, FALSE);

  g_signal_connect (cancel_button, "clicked", G_CALLBACK (cancel_clicked_cb), window);
  g_signal_connect (disable_button, "clicked", G_CALLBACK (disable_clicked_cb), window);

  buttons = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_halign (buttons, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_bottom (buttons, 24);
  gtk_box_append (GTK_BOX (buttons), cancel_button);
  gtk_box_append (GTK_BOX (buttons), disable_button);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (box), page);
  gtk_box_append (GTK_BOX (box), buttons);
  gtk_window_set_child (GTK_WINDOW (window), box);

  controller = gtk_event_controller_key_new ();
  gtk_event_controller_set_propagation_phase (controller, GTK_PHASE_CAPTURE);
  g_signal_connect (controller, "key-pressed", G_CALLBACK (key_pressed_cb), window);
  gtk_widget_add_controller (window, controller);

  gtk_window_present (GTK_WINDOW (window));
}
