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
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * ClapperAppShortcutDialogFunc:
 * @accel: pressed accelerator, or %NULL when shortcut should be disabled
 * @user_data: user data
 *
 * Not called when dialog was cancelled.
 */
typedef void (* ClapperAppShortcutDialogFunc) (const gchar *accel, gpointer user_data);

/* Asks user to press a new key combination for a shortcut with given title */
G_GNUC_INTERNAL
void clapper_app_shortcut_dialog_present (GtkWindow *parent, const gchar *shortcut_title,
    ClapperAppShortcutDialogFunc func, gpointer user_data, GDestroyNotify destroy_notify);

G_END_DECLS
