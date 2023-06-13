/*
 * Copyright (C) 2023 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alice Mikhaylenko <alice.mikhaylenko@puri.sm>
 */

#pragma once

#if !defined(_ADWAITA_INSIDE) && !defined(ADWAITA_COMPILATION)
#error "Only <adwaita.h> can be included directly."
#endif

#include "adw-version.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ADW_TYPE_ADAPTIVE_SLOT (adw_adaptive_slot_get_type())

ADW_AVAILABLE_IN_1_4
G_DECLARE_FINAL_TYPE (AdwAdaptiveSlot, adw_adaptive_slot, ADW, ADAPTIVE_SLOT, GtkWidget)

ADW_AVAILABLE_IN_1_4
GtkWidget *adw_adaptive_slot_new (void) G_GNUC_WARN_UNUSED_RESULT;

#define ADW_TYPE_ADAPTIVE_LAYOUT (adw_adaptive_layout_get_type())

ADW_AVAILABLE_IN_1_4
G_DECLARE_FINAL_TYPE (AdwAdaptiveLayout, adw_adaptive_layout, ADW, ADAPTIVE_LAYOUT, GObject)

ADW_AVAILABLE_IN_1_4
AdwAdaptiveLayout *adw_adaptive_layout_new_from_bytes    (GtkBuilderScope *scope,
                                                          GBytes          *bytes) G_GNUC_WARN_UNUSED_RESULT;
ADW_AVAILABLE_IN_1_4
AdwAdaptiveLayout *adw_adaptive_layout_new_from_resource (GtkBuilderScope *scope,
                                                          const char      *resource_path) G_GNUC_WARN_UNUSED_RESULT;

ADW_AVAILABLE_IN_1_4
GBytes *adw_adaptive_layout_get_bytes (AdwAdaptiveLayout *self);

ADW_AVAILABLE_IN_1_4
const char *adw_adaptive_layout_get_resource (AdwAdaptiveLayout *self);

ADW_AVAILABLE_IN_1_4
GtkBuilderScope *adw_adaptive_layout_get_scope (AdwAdaptiveLayout *self);

#define ADW_TYPE_ADAPTIVE_HOST (adw_adaptive_host_get_type())

ADW_AVAILABLE_IN_1_4
G_DECLARE_FINAL_TYPE (AdwAdaptiveHost, adw_adaptive_host, ADW, ADAPTIVE_HOST, GtkWidget)

ADW_AVAILABLE_IN_1_4
GtkWidget *adw_adaptive_host_new (void) G_GNUC_WARN_UNUSED_RESULT;

ADW_AVAILABLE_IN_1_4
AdwAdaptiveLayout *adw_adaptive_host_get_current_layout (AdwAdaptiveHost   *self);
ADW_AVAILABLE_IN_1_4
void               adw_adaptive_host_set_current_layout (AdwAdaptiveHost   *self,
                                                         AdwAdaptiveLayout *layout);

ADW_AVAILABLE_IN_1_4
void adw_adaptive_host_add_layout (AdwAdaptiveHost   *self,
                                   AdwAdaptiveLayout *layout);

ADW_AVAILABLE_IN_1_4
void adw_adaptive_host_add_child_widget (AdwAdaptiveHost *self,
                                         const char      *id,
                                         GtkWidget       *child);

G_END_DECLS
