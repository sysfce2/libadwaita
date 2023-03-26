/*
 * Copyright (C) 2023 Christopher Davis <christopherdavis@gnome.org>
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

typedef enum {
  ADW_ACCENT_COLOR_DEFAULT,
  ADW_ACCENT_COLOR_BLUE,
  ADW_ACCENT_COLOR_TEAL,
  ADW_ACCENT_COLOR_GREEN,
  ADW_ACCENT_COLOR_YELLOW,
  ADW_ACCENT_COLOR_ORANGE,
  ADW_ACCENT_COLOR_RED,
  ADW_ACCENT_COLOR_PINK,
  ADW_ACCENT_COLOR_PURPLE,
  ADW_ACCENT_COLOR_BROWN,
  ADW_ACCENT_COLOR_SLATE,
} AdwAccentColor;

ADW_AVAILABLE_IN_1_4
void adw_accent_color_to_rgba (AdwAccentColor   color,
                               GdkRGBA         *light_rgba,
                               GdkRGBA         *dark_rgba);

ADW_AVAILABLE_IN_1_4
void adw_calculate_fg_for_rgba (GdkRGBA *rgba,
                                GdkRGBA *out);

ADW_AVAILABLE_IN_1_4
void adw_adjust_rgba_for_text (GdkRGBA *rgba,
                               gboolean dark,
                               GdkRGBA *out);
G_END_DECLS
