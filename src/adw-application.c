/*
 * Copyright (C) 2021 Nahuel Gomez Castro
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"
#include "adw-application.h"
#include "adw-dialog.h"
#include "adw-main-private.h"

/**
 * AdwApplication:
 *
 * A base class for Adwaita applications.
 *
 * `AdwApplication` handles library initialization by calling [func@init] in the
 * default [signal@Gio.Application::startup] signal handler, in turn chaining up
 * as required by [class@Gtk.Application]. Therefore, any subclass of
 * `AdwApplication` should always chain up its `startup` handler before using
 * any Adwaita or GTK API.
 *
 * ## Automatic Resources
 *
 * ### Shortcuts Dialog
 *
 * If there's a resource located at `shortcuts-dialog.ui` which defines an
 * [class@ShortcutsDialog] with the ID `shortcuts_dialog`, `AdwApplication`
 * will set up an `app.shortcuts` action that creates and presents this dialog,
 * as well as a <kbd>Ctrl</kbd><kbd>?</kbd> accelerator for it.
 *
 * ### Stylesheet
 *
 * `AdwApplication` will automatically load stylesheets located in the
 * application's resource base path (see
 * [method@Gio.Application.set_resource_base_path], if they're present.
 *
 * They can be used to add custom styles to the application, as follows:
 *
 * - `style.css` contains styles that are always present.
 *
 * - `style-dark.css` contains styles only used when
 * [property@StyleManager:dark] is `TRUE`.
 *
 * - `style-hc.css` contains styles used when the system high contrast
 *   preference is enabled.
 *
 * - `style-hc-dark.css` contains styles used when the system high contrast
 *   preference is enabled and [property@StyleManager:dark] is `TRUE`.
 */

typedef struct
{
  GtkStyleProvider *base_style_provider;
  GtkStyleProvider *dark_style_provider;
  GtkStyleProvider *hc_style_provider;
  GtkStyleProvider *hc_dark_style_provider;
  char *shortcuts_dialog_path;
} AdwApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AdwApplication, adw_application, GTK_TYPE_APPLICATION)

enum {
  PROP_0,
  PROP_STYLE_MANAGER,
  LAST_PROP,
};

static GParamSpec *props[LAST_PROP];

static GFile *
get_resource_base_path_file (AdwApplication *self)
{
  const char *base_path;
  char *base_uri;
  GFile *base_file;

  base_path = g_application_get_resource_base_path (G_APPLICATION (self));

  if (base_path == NULL)
    return NULL;

  base_uri = g_strconcat ("resource://", base_path, NULL);
  base_file = g_file_new_for_uri (base_uri);

  g_free (base_uri);

  return base_file;
}

static void
disable_shortcuts_action (AdwApplication *self)
{
  GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self), "shortcuts");

  g_assert (G_IS_SIMPLE_ACTION (action));

  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
}

static void
shortcuts_action_cb (AdwApplication *self)
{
  AdwApplicationPrivate *priv = adw_application_get_instance_private (self);
  GtkBuilder *builder;
  GError *error = NULL;
  GObject *dialog;
  GtkWindow *window;

  builder = gtk_builder_new ();

  gtk_builder_add_from_resource (builder, priv->shortcuts_dialog_path, &error);

  if (error) {
    g_critical ("Failed to create shortcuts window: %s", error->message);
    disable_shortcuts_action (self);
    g_error_free (error);
    g_object_unref (builder);
    return;
  }

  dialog = gtk_builder_get_object (builder, "shortcuts_dialog");
  if (!ADW_IS_DIALOG (dialog)) {
    disable_shortcuts_action (self);
    g_object_unref (builder);
    return;
  }

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  adw_dialog_present (ADW_DIALOG (dialog), GTK_WIDGET (window));

  g_object_unref (builder);
}

static inline void
style_provider_set_enabled (GtkStyleProvider *provider,
                            gboolean          enabled)
{
  if (enabled)
    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                                provider,
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  else
    gtk_style_context_remove_provider_for_display (gdk_display_get_default (),
                                                   provider);
}

static void
update_stylesheet (AdwApplication *self)
{
  AdwApplicationPrivate *priv = adw_application_get_instance_private (self);
  AdwStyleManager *manager = adw_style_manager_get_default ();
  gboolean is_dark, is_hc;

  is_dark = adw_style_manager_get_dark (manager);
  is_hc = adw_style_manager_get_high_contrast (manager);

  if (priv->dark_style_provider)
    style_provider_set_enabled (priv->dark_style_provider, is_dark);

  if (priv->hc_style_provider)
    style_provider_set_enabled (priv->hc_style_provider, is_hc);

  if (priv->hc_dark_style_provider)
    style_provider_set_enabled (priv->hc_dark_style_provider, is_hc && is_dark);
}

static void
init_provider_from_file (GtkStyleProvider **provider,
                         GFile             *file)
{
  if (!g_file_query_exists (file, NULL)) {
    g_clear_object (&file);
    return;
  }

  *provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_file (GTK_CSS_PROVIDER (*provider), file);

  g_clear_object (&file);
}

static void
init_providers (AdwApplication *self)
{
  AdwApplicationPrivate *priv = adw_application_get_instance_private (self);
  GFile *base_file = get_resource_base_path_file (self);

  if (base_file == NULL)
    return;

  if (!adw_is_granite_present ()) {
    init_provider_from_file (&priv->base_style_provider,
                             g_file_get_child (base_file, "style.css"));
    init_provider_from_file (&priv->dark_style_provider,
                             g_file_get_child (base_file, "style-dark.css"));
    init_provider_from_file (&priv->hc_style_provider,
                             g_file_get_child (base_file, "style-hc.css"));
    init_provider_from_file (&priv->hc_dark_style_provider,
                             g_file_get_child (base_file, "style-hc-dark.css"));
  }

  g_object_unref (base_file);
}

static void
init_styling (AdwApplication *self)
{
  AdwApplicationPrivate *priv = adw_application_get_instance_private (self);

  GdkDisplay *display = gdk_display_get_default ();

  if (display == NULL)
    return;

  if (priv->base_style_provider != NULL)
    gtk_style_context_add_provider_for_display (display,
                                                priv->base_style_provider,
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  /* If gdk_display_get_default() worked, it means that
   * gtk_settings adw_style_manager_get_default() won't return NULL, so we don't
   * need to check it separately */
  g_signal_connect_object (adw_style_manager_get_default (),
                           "notify::dark",
                           G_CALLBACK (update_stylesheet),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (adw_style_manager_get_default (),
                           "notify::high-contrast",
                           G_CALLBACK (update_stylesheet),
                           self,
                           G_CONNECT_SWAPPED);

  update_stylesheet (self);
}

static void
init_shortcuts_dialog (AdwApplication *self)
{
  AdwApplicationPrivate *priv = adw_application_get_instance_private (self);
  GFile *base_file, *ui_file;
  GSimpleAction *action;
  char *uri;

#ifdef __APPLE__
  const char * const accels[] = { "<Meta>question", NULL };
#else
  const char * const accels[] = { "<Control>question", NULL };
#endif

  /* An app.shortcuts action already exists, so we don't add ours */
  if (g_action_map_lookup_action (G_ACTION_MAP (self), "shortcuts") != NULL)
    return;

  base_file = get_resource_base_path_file (self);
  if (base_file == NULL)
    return;

  ui_file = g_file_get_child (base_file, "shortcuts-dialog.ui");

  if (!g_file_query_exists (ui_file, NULL)) {
    g_object_unref (base_file);
    g_object_unref (ui_file);
    return;
  }

  uri = g_file_get_uri (ui_file);

  /* Trim "resource://" to get a resource path */
  priv->shortcuts_dialog_path = g_strdup (&uri[11]);

  action = g_simple_action_new ("shortcuts", NULL);
  g_signal_connect_swapped (action, "activate", G_CALLBACK (shortcuts_action_cb), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (action));

  gtk_application_set_accels_for_action (GTK_APPLICATION (self), "app.shortcuts", accels);

  g_free (uri);
  g_object_unref (base_file);
  g_object_unref (ui_file);
  g_object_unref (action);
}

static void
adw_application_startup (GApplication *application)
{
  AdwApplication *self = ADW_APPLICATION (application);

  G_APPLICATION_CLASS (adw_application_parent_class)->startup (application);

  adw_init ();

  init_providers (self);
  init_styling (self);
  init_shortcuts_dialog (self);
}

static void
adw_application_dispose (GObject *object)
{
  AdwApplication *self = ADW_APPLICATION (object);
  AdwApplicationPrivate *priv = adw_application_get_instance_private (self);

  g_clear_object (&priv->base_style_provider);
  g_clear_object (&priv->dark_style_provider);
  g_clear_object (&priv->hc_style_provider);
  g_clear_object (&priv->hc_dark_style_provider);

  G_OBJECT_CLASS (adw_application_parent_class)->dispose (object);
}

static void
adw_application_finalize (GObject *object)
{
  AdwApplication *self = ADW_APPLICATION (object);
  AdwApplicationPrivate *priv = adw_application_get_instance_private (self);

  g_free (priv->shortcuts_dialog_path);

  G_OBJECT_CLASS (adw_application_parent_class)->finalize (object);
}

static void
adw_application_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  AdwApplication *self = ADW_APPLICATION (object);

  switch (prop_id) {
  case PROP_STYLE_MANAGER:
    g_value_set_object (value, adw_application_get_style_manager (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_application_class_init (AdwApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  object_class->dispose = adw_application_dispose;
  object_class->finalize = adw_application_finalize;
  object_class->get_property = adw_application_get_property;

  application_class->startup = adw_application_startup;

  /**
   * AdwApplication:style-manager:
   *
   * The style manager for this application.
   *
   * This is a convenience property allowing to access `AdwStyleManager` through
   * property bindings or expressions.
   */
  props[PROP_STYLE_MANAGER] =
    g_param_spec_object ("style-manager", NULL, NULL,
                         ADW_TYPE_STYLE_MANAGER,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

static void
adw_application_init (AdwApplication *self)
{
}

/**
 * adw_application_new:
 * @application_id: (nullable): The application ID
 * @flags: The application flags
 *
 * Creates a new `AdwApplication`.
 *
 * If `application_id` is not `NULL`, then it must be valid. See
 * [func@Gio.Application.id_is_valid].
 *
 * If no application ID is given then some features (most notably application
 * uniqueness) will be disabled.
 *
 * Returns: the newly created `AdwApplication`
 */
AdwApplication *
adw_application_new (const char        *application_id,
                     GApplicationFlags  flags)
{
  return g_object_new (ADW_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

/**
 * adw_application_get_style_manager:
 * @self: an application
 *
 * Gets the style manager for @self.
 *
 * This is a convenience property allowing to access `AdwStyleManager` through
 * property bindings or expressions.
 *
 * Returns: (transfer none): the style manager
 */
AdwStyleManager *
adw_application_get_style_manager (AdwApplication *self)
{
  g_return_val_if_fail (ADW_IS_APPLICATION (self), NULL);

  return adw_style_manager_get_default ();
}
