/*
 * Copyright (C) 2023 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Alice Mikhaylenko <alice.mikhaylenko@puri.sm>
 */

#include "config.h"

#include "adw-adaptive-host.h"

#include "adw-widget-utils-private.h"

struct _AdwAdaptiveSlot
{
  GtkWidget parent_instance;

  GtkWidget *child;
  char *id;
};

static void adw_adaptive_slot_buildable_init (GtkBuildableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (AdwAdaptiveSlot, adw_adaptive_slot, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, adw_adaptive_slot_buildable_init))

static GtkBuildableIface *slot_parent_buildable_iface;

static void register_slot (AdwAdaptiveHost *self,
                           const char      *id,
                           AdwAdaptiveSlot *slot);

static void
adw_adaptive_slot_root (GtkWidget *widget)
{
  AdwAdaptiveSlot *self = ADW_ADAPTIVE_SLOT (widget);
  GtkWidget *host;

  GTK_WIDGET_CLASS (adw_adaptive_slot_parent_class)->root (widget);

  host = gtk_widget_get_ancestor (widget, ADW_TYPE_ADAPTIVE_HOST);

  if (self->id)
    register_slot (ADW_ADAPTIVE_HOST (host), self->id, self);
}

static void
adw_adaptive_slot_dispose (GObject *object)
{
  AdwAdaptiveSlot *self = ADW_ADAPTIVE_SLOT (object);

  g_clear_pointer (&self->child, gtk_widget_unparent);

  G_OBJECT_CLASS (adw_adaptive_slot_parent_class)->dispose (object);
}

static void
adw_adaptive_slot_class_init (AdwAdaptiveSlotClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = adw_adaptive_slot_dispose;

  widget_class->root = adw_adaptive_slot_root;
  widget_class->compute_expand = adw_widget_compute_expand;

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_GROUP);
}

static void
adw_adaptive_slot_buildable_set_id (GtkBuildable *buildable,
                                    const char   *id)
{
  AdwAdaptiveSlot *self = ADW_ADAPTIVE_SLOT (buildable);

  slot_parent_buildable_iface->set_id (buildable, id);

  g_clear_pointer (&self->id, g_free);
  self->id = g_strdup (id);
}

static void
adw_adaptive_slot_buildable_init (GtkBuildableIface *iface)
{
  slot_parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->set_id = adw_adaptive_slot_buildable_set_id;
}

static void
adw_adaptive_slot_init (AdwAdaptiveSlot *self)
{
}

GtkWidget *
adw_adaptive_slot_new (void)
{
  return g_object_new (ADW_TYPE_ADAPTIVE_SLOT, NULL);
}

struct _AdwAdaptiveLayout
{
  GObject parent_instance;

  GtkBuilderScope *scope;
  GBytes *bytes;
  char *resource;

  GObject *current_object;
};

static void adw_adaptive_layout_buildable_init (GtkBuildableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (AdwAdaptiveLayout, adw_adaptive_layout, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, adw_adaptive_layout_buildable_init))

enum {
  LAYOUT_PROP_0,
  LAYOUT_PROP_BYTES,
  LAYOUT_PROP_RESOURCE,
  LAYOUT_PROP_SCOPE,
  LAST_LAYOUT_PROP
};

static GParamSpec *layout_props[LAST_LAYOUT_PROP];

static gboolean
set_bytes (AdwAdaptiveLayout *self,
           GBytes            *bytes)
{
  if (bytes == NULL)
    return FALSE;

  if (self->bytes) {
    g_critical ("Data for AdwAdaptiveLayout has already been set.");
    return FALSE;
  }

  self->bytes = g_bytes_ref (bytes);

  return TRUE;
}

static void
set_resource (AdwAdaptiveLayout *self,
              const char        *resource)
{
  GError *error = NULL;
  GBytes *bytes;

  if (resource == NULL)
    return;

  bytes = g_resources_lookup_data (resource, 0, &error);
  if (bytes) {
    if (set_bytes (self, bytes))
      self->resource = g_strdup (resource);

    g_bytes_unref (bytes);
  } else {
    g_critical ("Unable to load resource for layout template: %s", error->message);
    g_error_free (error);
  }
}

static GtkBuilder *
create_builder (AdwAdaptiveLayout *self,
                AdwAdaptiveHost   *host)
{
  GtkBuilder *builder = gtk_builder_new ();
  const char *data;
  gsize data_size;
  GError *error = NULL;

  if (self->current_object)
    gtk_builder_set_current_object (builder, self->current_object);

  if (self->scope)
    gtk_builder_set_scope (builder, self->scope);

  data = g_bytes_get_data (self->bytes, &data_size);

  gtk_builder_add_from_string (builder, data, data_size, &error);
  if (error) {
    g_critical ("Error building layout: %s", error->message);
    g_error_free (error);
    g_object_unref (builder);

    return NULL;
  }

  return builder;
}

static void
adw_adaptive_layout_finalize (GObject *object)
{
  AdwAdaptiveLayout *self = ADW_ADAPTIVE_LAYOUT (object);

  g_clear_object (&self->scope);
  g_bytes_unref (self->bytes);
  g_free (self->resource);

  g_clear_object (&self->current_object);

  G_OBJECT_CLASS (adw_adaptive_layout_parent_class)->finalize (object);
}

static void
adw_adaptive_layout_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  AdwAdaptiveLayout *self = ADW_ADAPTIVE_LAYOUT (object);

  switch (prop_id) {
  case LAYOUT_PROP_BYTES:
    g_value_set_boxed (value, adw_adaptive_layout_get_bytes (self));
    break;
  case LAYOUT_PROP_RESOURCE:
    g_value_set_string (value, adw_adaptive_layout_get_resource (self));
    break;
  case LAYOUT_PROP_SCOPE:
    g_value_set_object (value, adw_adaptive_layout_get_scope (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_adaptive_layout_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  AdwAdaptiveLayout *self = ADW_ADAPTIVE_LAYOUT (object);

  switch (prop_id) {
  case LAYOUT_PROP_BYTES:
    set_bytes (self, g_value_get_boxed (value));
    break;
  case LAYOUT_PROP_RESOURCE:
    set_resource (self, g_value_get_string (value));
    break;
  case LAYOUT_PROP_SCOPE:
    self->scope = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_adaptive_layout_class_init (AdwAdaptiveLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = adw_adaptive_layout_finalize;
  object_class->get_property = adw_adaptive_layout_get_property;
  object_class->set_property = adw_adaptive_layout_set_property;

  layout_props[LAYOUT_PROP_BYTES] =
    g_param_spec_boxed ("bytes", NULL, NULL,
                        G_TYPE_BYTES,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  layout_props[LAYOUT_PROP_RESOURCE] =
    g_param_spec_string ("resource", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  layout_props[LAYOUT_PROP_SCOPE] =
    g_param_spec_object ("scope", NULL, NULL,
                         GTK_TYPE_BUILDER_SCOPE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_LAYOUT_PROP, layout_props);

}

static void
adw_adaptive_layout_init (AdwAdaptiveLayout *self)
{
}

static void
adw_adaptive_layout_parser_finished (GtkBuildable *buildable,
                                     GtkBuilder   *builder)
{
  AdwAdaptiveLayout *self = ADW_ADAPTIVE_LAYOUT (buildable);

  g_set_object (&self->scope, gtk_builder_get_scope (builder));
  g_set_object (&self->current_object, gtk_builder_get_current_object (builder));
}

static void
adw_adaptive_layout_buildable_init (GtkBuildableIface *iface)
{
  iface->parser_finished = adw_adaptive_layout_parser_finished;
}

AdwAdaptiveLayout *
adw_adaptive_layout_new_from_bytes (GtkBuilderScope *scope,
                                    GBytes          *bytes)
{
  g_return_val_if_fail (scope == NULL || GTK_IS_BUILDER_SCOPE (scope), NULL);
  g_return_val_if_fail (bytes != NULL, NULL);

  return g_object_new (ADW_TYPE_ADAPTIVE_LAYOUT,
                       "bytes", bytes,
                       "scope", scope,
                       NULL);
}

AdwAdaptiveLayout *
adw_adaptive_layout_new_from_resource (GtkBuilderScope *scope,
                                       const char      *resource_path)
{
  g_return_val_if_fail (scope == NULL || GTK_IS_BUILDER_SCOPE (scope), NULL);
  g_return_val_if_fail (resource_path != NULL, NULL);

  return g_object_new (ADW_TYPE_ADAPTIVE_LAYOUT,
                       "resource", resource_path,
                       "scope", scope,
                       NULL);
}

GBytes *
adw_adaptive_layout_get_bytes (AdwAdaptiveLayout *self)
{
  g_return_val_if_fail (ADW_IS_ADAPTIVE_LAYOUT (self), NULL);

  return self->bytes;
}

const char *
adw_adaptive_layout_get_resource (AdwAdaptiveLayout *self)
{
  g_return_val_if_fail (ADW_IS_ADAPTIVE_LAYOUT (self), NULL);

  return self->resource;
}

GtkBuilderScope *
adw_adaptive_layout_get_scope (AdwAdaptiveLayout *self)
{
  g_return_val_if_fail (ADW_IS_ADAPTIVE_LAYOUT (self), NULL);

  return self->scope;
}

























































struct _AdwAdaptiveHost
{
  GtkWidget parent_instance;

  GList *layouts;
  GHashTable *children;

  AdwAdaptiveLayout *current_layout;
  GtkWidget *current_layout_child;
  GHashTable *slots;

  gboolean accepting_slots;
};

static void adw_adaptive_host_buildable_init (GtkBuildableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (AdwAdaptiveHost, adw_adaptive_host, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, adw_adaptive_host_buildable_init))

static GtkBuildableIface *parent_buildable_iface;

enum {
  PROP_0,
  PROP_CURRENT_LAYOUT,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP];

static void
parent_child (AdwAdaptiveHost *self,
              const char      *id)
{
  GtkWidget *slot = g_hash_table_lookup (self->slots, id);
  GtkWidget *child;

  if (!slot)
    return;

  child = g_hash_table_lookup (self->children, id);

  gtk_widget_set_parent (child, GTK_WIDGET (slot));
}

static void
parent_child_func (const char      *id,
                   GtkWidget       *child,
                   AdwAdaptiveHost *self)
{
  parent_child (self, id);
}

static void
unparent_child (const char      *id,
                GtkWidget       *child,
                AdwAdaptiveHost *self)
{
  if (!g_hash_table_contains (self->slots, id))
    return;

  gtk_widget_unparent (child);
}

static void
register_slot (AdwAdaptiveHost *self,
               const char      *id,
               AdwAdaptiveSlot *slot)
{
  if (!self->accepting_slots)
    return;

  g_hash_table_insert (self->slots,
                       g_strdup (id),
                       slot);
}

static void
destroy_current_layout (AdwAdaptiveHost *self)
{
  g_hash_table_foreach (self->children, (GHFunc) unparent_child, self);
  g_hash_table_remove_all (self->slots);
  g_clear_pointer (&self->current_layout_child, gtk_widget_unparent);
}

static void
rebuild_current_layout (AdwAdaptiveHost *self)
{
  GtkBuilder *builder;
  GObject *root;

  builder = create_builder (self->current_layout, self);

  if (!builder) {
    // FIXME: didn't even notify
    return;
  }

  root = gtk_builder_get_object (builder, "root");
  if (!root)
    g_critical ("Missing root object");

  if (!GTK_IS_WIDGET (root))
    g_critical ("Root is not a widget");

  if (self->current_layout_child)
    destroy_current_layout (self);

  self->current_layout_child = GTK_WIDGET (root);

  self->accepting_slots = TRUE;
  gtk_widget_set_parent (self->current_layout_child, GTK_WIDGET (self));
  self->accepting_slots = FALSE;

  g_hash_table_foreach (self->children, (GHFunc) parent_child_func, self);

  g_object_unref (builder);
}

static void
adw_adaptive_host_realize (GtkWidget *widget)
{
  AdwAdaptiveHost *self = ADW_ADAPTIVE_HOST (widget);

  GTK_WIDGET_CLASS (adw_adaptive_host_parent_class)->realize (widget);

  rebuild_current_layout (self);
}

static void
adw_adaptive_host_unrealize (GtkWidget *widget)
{
  AdwAdaptiveHost *self = ADW_ADAPTIVE_HOST (widget);

  destroy_current_layout (self);

  GTK_WIDGET_CLASS (adw_adaptive_host_parent_class)->unrealize (widget);
}

static void
adw_adaptive_host_dispose (GObject *object)
{
  AdwAdaptiveHost *self = ADW_ADAPTIVE_HOST (object);

  g_clear_pointer (&self->children, g_hash_table_unref);
  g_clear_pointer (&self->layouts, g_object_unref);

  g_clear_pointer (&self->current_layout_child, gtk_widget_unparent);
  g_clear_pointer (&self->slots, g_hash_table_unref);

  G_OBJECT_CLASS (adw_adaptive_host_parent_class)->dispose (object);
}

static void
adw_adaptive_host_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  AdwAdaptiveHost *self = ADW_ADAPTIVE_HOST (object);

  switch (prop_id) {
  case PROP_CURRENT_LAYOUT:
    g_value_set_object (value, adw_adaptive_host_get_current_layout (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_adaptive_host_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  AdwAdaptiveHost *self = ADW_ADAPTIVE_HOST (object);

  switch (prop_id) {
  case PROP_CURRENT_LAYOUT:
    adw_adaptive_host_set_current_layout (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_adaptive_host_class_init (AdwAdaptiveHostClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = adw_adaptive_host_dispose;
  object_class->get_property = adw_adaptive_host_get_property;
  object_class->set_property = adw_adaptive_host_set_property;

  widget_class->realize = adw_adaptive_host_realize;
  widget_class->unrealize = adw_adaptive_host_unrealize;
  widget_class->compute_expand = adw_widget_compute_expand;

  props[PROP_CURRENT_LAYOUT] =
    g_param_spec_object ("current-layout", NULL, NULL,
                         ADW_TYPE_ADAPTIVE_LAYOUT,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "adaptive-host");
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_GROUP);
}

static void
adw_adaptive_host_init (AdwAdaptiveHost *self)
{
  self->children = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  self->slots = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
adw_adaptive_host_buildable_add_child (GtkBuildable *buildable,
                                       GtkBuilder   *builder,
                                       GObject      *child,
                                       const char   *type)
{
  AdwAdaptiveHost *self = ADW_ADAPTIVE_HOST (buildable);

  if (ADW_IS_ADAPTIVE_LAYOUT (child))
    adw_adaptive_host_add_layout (self, ADW_ADAPTIVE_LAYOUT (child));
  else if (type)
    adw_adaptive_host_add_child_widget (self, type, GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
adw_adaptive_host_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = adw_adaptive_host_buildable_add_child;
}

GtkWidget *
adw_adaptive_host_new (void)
{
  return g_object_new (ADW_TYPE_ADAPTIVE_HOST, NULL);
}

AdwAdaptiveLayout *
adw_adaptive_host_get_current_layout (AdwAdaptiveHost *self)
{
  g_return_val_if_fail (ADW_IS_ADAPTIVE_HOST (self), NULL);

  return self->current_layout;
}

void
adw_adaptive_host_set_current_layout (AdwAdaptiveHost   *self,
                                      AdwAdaptiveLayout *layout)
{
  g_return_if_fail (ADW_IS_ADAPTIVE_HOST (self));
  g_return_if_fail (ADW_IS_ADAPTIVE_LAYOUT (layout));

  if (layout == self->current_layout)
    return;

  g_set_object (&self->current_layout, layout);

  if (gtk_widget_get_realized (GTK_WIDGET (self)))
    rebuild_current_layout (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CURRENT_LAYOUT]);
}

void
adw_adaptive_host_add_layout (AdwAdaptiveHost   *self,
                              AdwAdaptiveLayout *layout)
{
  g_return_if_fail (ADW_IS_ADAPTIVE_HOST (self));
  g_return_if_fail (ADW_IS_ADAPTIVE_LAYOUT (layout));

  if (!self->layouts)
    adw_adaptive_host_set_current_layout (self, layout);

  self->layouts = g_list_prepend (self->layouts, layout);
}

void
adw_adaptive_host_add_child_widget (AdwAdaptiveHost *self,
                                    const char      *id,
                                    GtkWidget       *child)
{
  g_return_if_fail (ADW_IS_ADAPTIVE_HOST (self));
  g_return_if_fail (id != NULL);
  g_return_if_fail (GTK_IS_WIDGET (child));

  if (g_hash_table_contains (self->children, id)) {
    g_critical ("A child with the type '%s' already exists", id);
    return;
  }

  g_hash_table_insert (self->children, g_strdup (id), g_object_ref_sink (child));

  if (self->current_layout)
    parent_child (self, id);
}
