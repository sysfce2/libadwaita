/*
 * Copyright (C) 2019 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_ADWAITA_INSIDE) && !defined(ADWAITA_COMPILATION)
#error "Only <adwaita.h> can be included directly."
#endif

#include "adw-version.h"

#include <gtk/gtk.h>
#include "adw-navigation-view.h"
#include "adw-preferences-page.h"
#include "adw-toast.h"
#include "adw-window.h"

G_BEGIN_DECLS

#define ADW_TYPE_PREFERENCES_WINDOW (adw_preferences_window_get_type())

ADW_DEPRECATED_IN_1_6_FOR(AdwPreferencesDialog)
G_DECLARE_DERIVABLE_TYPE (AdwPreferencesWindow, adw_preferences_window, ADW, PREFERENCES_WINDOW, AdwWindow)

/**
 * AdwPreferencesWindowClass
 * @parent_class: The parent class
 */
struct _AdwPreferencesWindowClass
{
  AdwWindowClass parent_class;

  /*< private >*/
  gpointer padding[4];
};

ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_new)
GtkWidget *adw_preferences_window_new (void) G_GNUC_WARN_UNUSED_RESULT;

ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_add)
void adw_preferences_window_add    (AdwPreferencesWindow *self,
                                    AdwPreferencesPage   *page);
ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_remove)
void adw_preferences_window_remove (AdwPreferencesWindow *self,
                                    AdwPreferencesPage   *page);

ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_get_visible_page)
AdwPreferencesPage *adw_preferences_window_get_visible_page (AdwPreferencesWindow *self);
ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_set_visible_page)
void                adw_preferences_window_set_visible_page (AdwPreferencesWindow *self,
                                                             AdwPreferencesPage   *page);

ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_get_visible_page_name)
const char *adw_preferences_window_get_visible_page_name (AdwPreferencesWindow *self);
ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_set_visible_page_name)
void        adw_preferences_window_set_visible_page_name (AdwPreferencesWindow *self,
                                                          const char           *name);

ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_get_search_enabled)
gboolean adw_preferences_window_get_search_enabled (AdwPreferencesWindow *self);
ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_set_search_enabled)
void     adw_preferences_window_set_search_enabled (AdwPreferencesWindow *self,
                                                    gboolean              search_enabled);

ADW_DEPRECATED_IN_1_4_FOR(adw_navigation_page_get_can_pop)
gboolean adw_preferences_window_get_can_navigate_back (AdwPreferencesWindow *self);
ADW_DEPRECATED_IN_1_4_FOR(adw_navigation_page_set_can_pop)
void     adw_preferences_window_set_can_navigate_back (AdwPreferencesWindow *self,
                                                       gboolean              can_navigate_back);

ADW_DEPRECATED_IN_1_4_FOR (adw_preferences_window_push_subpage)
void adw_preferences_window_present_subpage (AdwPreferencesWindow *self,
                                             GtkWidget            *subpage);
ADW_DEPRECATED_IN_1_4_FOR (adw_preferences_window_pop_subpage)
void adw_preferences_window_close_subpage   (AdwPreferencesWindow *self);

ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_push_subpage)
void     adw_preferences_window_push_subpage (AdwPreferencesWindow *self,
                                              AdwNavigationPage    *page);
ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_pop_subpage)
gboolean adw_preferences_window_pop_subpage  (AdwPreferencesWindow *self);

ADW_DEPRECATED_IN_1_6_FOR(adw_preferences_dialog_add_toast)
void adw_preferences_window_add_toast (AdwPreferencesWindow *self,
                                       AdwToast             *toast);

G_END_DECLS
