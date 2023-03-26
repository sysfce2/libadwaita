/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alexander Mikhaylenko <alexander.mikhaylenko@puri.sm>
 */

#pragma once

#if !defined(_ADWAITA_INSIDE) && !defined(ADWAITA_COMPILATION)
#error "Only <adwaita.h> can be included directly."
#endif

#include "adw-version.h"

#include <gtk/gtk.h>

#include "adw-colors.h"
#include "adw-enums.h"

G_BEGIN_DECLS

typedef enum {
  ADW_COLOR_SCHEME_DEFAULT,
  ADW_COLOR_SCHEME_FORCE_LIGHT,
  ADW_COLOR_SCHEME_PREFER_LIGHT,
  ADW_COLOR_SCHEME_PREFER_DARK,
  ADW_COLOR_SCHEME_FORCE_DARK,
} AdwColorScheme;

#define ADW_TYPE_STYLE_MANAGER (adw_style_manager_get_type())

ADW_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (AdwStyleManager, adw_style_manager, ADW, STYLE_MANAGER, GObject)

ADW_AVAILABLE_IN_ALL
AdwStyleManager *adw_style_manager_get_default (void);
ADW_AVAILABLE_IN_ALL
AdwStyleManager *adw_style_manager_get_for_display (GdkDisplay *display);

ADW_AVAILABLE_IN_ALL
GdkDisplay *adw_style_manager_get_display (AdwStyleManager *self);

ADW_AVAILABLE_IN_ALL
AdwColorScheme adw_style_manager_get_color_scheme (AdwStyleManager *self);
ADW_AVAILABLE_IN_ALL
void           adw_style_manager_set_color_scheme (AdwStyleManager *self,
                                                   AdwColorScheme   color_scheme);

ADW_AVAILABLE_IN_ALL
gboolean adw_style_manager_get_system_supports_color_schemes (AdwStyleManager *self);

ADW_AVAILABLE_IN_ALL
gboolean adw_style_manager_get_dark          (AdwStyleManager *self);
ADW_AVAILABLE_IN_ALL
gboolean adw_style_manager_get_high_contrast (AdwStyleManager *self);

ADW_AVAILABLE_IN_1_4
void adw_style_manager_get_current_accent_colors  (AdwStyleManager *self,
                                                   GdkRGBA         *accent,
                                                   GdkRGBA         *fg);
ADW_AVAILABLE_IN_1_4
void adw_style_manager_get_light_accent_colors    (AdwStyleManager *self,
                                                   GdkRGBA         *accent,
                                                   GdkRGBA         *fg);
ADW_AVAILABLE_IN_1_4
void adw_style_manager_get_dark_accent_colors     (AdwStyleManager *self,
                                                   GdkRGBA         *accent,
                                                   GdkRGBA         *fg);
ADW_AVAILABLE_IN_1_4
void adw_style_manager_set_accent_color           (AdwStyleManager *self,
                                                   AdwAccentColor   color);
ADW_AVAILABLE_IN_1_4
void adw_style_manager_set_accent_color_from_rgba (AdwStyleManager *self,
                                                   const GdkRGBA   *light_color,
                                                   const GdkRGBA   *dark_color);

ADW_AVAILABLE_IN_1_4
void     adw_style_manager_set_follow_system_accent_color (AdwStyleManager *self,
                                                           gboolean         follow_system);
ADW_AVAILABLE_IN_1_4
gboolean adw_style_manager_get_follow_system_accent_color (AdwStyleManager *self);

ADW_AVAILABLE_IN_1_4
gboolean adw_style_manager_get_system_supports_accent_colors (AdwStyleManager *self);

G_END_DECLS

