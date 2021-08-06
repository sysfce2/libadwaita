/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_ADWAITA_INSIDE) && !defined(ADWAITA_COMPILATION)
#error "Only <adwaita.h> can be included directly."
#endif

#include "adw-version.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ADW_TYPE_WRAP_BOX (adw_wrap_box_get_type())

ADW_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (AdwWrapBox, adw_wrap_box, ADW, WRAP_BOX, GtkWidget)

ADW_AVAILABLE_IN_ALL
GtkWidget *adw_wrap_box_new (void) G_GNUC_WARN_UNUSED_RESULT;

ADW_AVAILABLE_IN_ALL
int  adw_wrap_box_get_spacing (AdwWrapBox *self);
ADW_AVAILABLE_IN_ALL
void adw_wrap_box_set_spacing (AdwWrapBox *self,
                               int         spacing);

ADW_AVAILABLE_IN_ALL
int  adw_wrap_box_get_line_spacing (AdwWrapBox *self);
ADW_AVAILABLE_IN_ALL
void adw_wrap_box_set_line_spacing (AdwWrapBox *self,
                                    int         line_spacing);

ADW_AVAILABLE_IN_ALL
void adw_wrap_box_insert_child_after  (AdwWrapBox *self,
                                       GtkWidget  *child,
                                       GtkWidget  *sibling);
ADW_AVAILABLE_IN_ALL
void adw_wrap_box_reorder_child_after (AdwWrapBox *self,
                                       GtkWidget  *child,
                                       GtkWidget  *sibling);

ADW_AVAILABLE_IN_ALL
void adw_wrap_box_append  (AdwWrapBox *self,
                           GtkWidget  *child);
ADW_AVAILABLE_IN_ALL
void adw_wrap_box_prepend (AdwWrapBox *self,
                           GtkWidget  *child);
ADW_AVAILABLE_IN_ALL
void adw_wrap_box_remove  (AdwWrapBox *self,
                           GtkWidget  *child);

G_END_DECLS
