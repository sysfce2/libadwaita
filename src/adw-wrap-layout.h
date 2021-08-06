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

#define ADW_TYPE_WRAP_LAYOUT (adw_wrap_layout_get_type())

ADW_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (AdwWrapLayout, adw_wrap_layout, ADW, WRAP_LAYOUT, GtkLayoutManager)

ADW_AVAILABLE_IN_ALL
GtkLayoutManager *adw_wrap_layout_new (void) G_GNUC_WARN_UNUSED_RESULT;

ADW_AVAILABLE_IN_ALL
int  adw_wrap_layout_get_spacing (AdwWrapLayout *self);
ADW_AVAILABLE_IN_ALL
void adw_wrap_layout_set_spacing (AdwWrapLayout *self,
                                  int            spacing);

ADW_AVAILABLE_IN_ALL
int  adw_wrap_layout_get_line_spacing (AdwWrapLayout *self);
ADW_AVAILABLE_IN_ALL
void adw_wrap_layout_set_line_spacing (AdwWrapLayout *self,
                                       int            line_spacing);

G_END_DECLS
