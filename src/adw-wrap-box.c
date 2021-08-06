/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "adw-wrap-box.h"

#include "adw-wrap-layout.h"

/**
 * AdwWrapBox:
 *
 * TODO
 *
 * ## CSS nodes
 *
 * `AdwWrapBox` uses a single CSS node with name `wrapbox`.
 *
 * ## Accessibility
 *
 * `AdwWrapBox` uses the `GTK_ACCESSIBLE_ROLE_GROUP` role.
 *
 * Since: 1.0
 */

struct _AdwWrapBox
{
  GtkWidget parent_instance;
};

enum {
  PROP_0,
  PROP_SPACING,
  PROP_LINE_SPACING,

  /* Overridden properties */
  PROP_ORIENTATION,

  LAST_PROP = PROP_LINE_SPACING + 1,
};

static GParamSpec *props[LAST_PROP];

static void adw_wrap_box_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (AdwWrapBox, adw_wrap_box, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, adw_wrap_box_buildable_init))

static GtkBuildableIface *parent_buildable_iface;

static GtkOrientation
get_orientation (AdwWrapBox *self)
{
  GtkLayoutManager *layout = gtk_widget_get_layout_manager (GTK_WIDGET (self));

  return gtk_orientable_get_orientation (GTK_ORIENTABLE (layout));
}

static void
set_orientation (AdwWrapBox     *self,
                 GtkOrientation  orientation)
{
  GtkLayoutManager *layout = gtk_widget_get_layout_manager (GTK_WIDGET (self));

  if (orientation == get_orientation (self))
    return;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (layout), orientation);

  g_object_notify (G_OBJECT (self), "orientation");
}

static void
adw_wrap_box_compute_expand (GtkWidget *widget,
                             gboolean  *hexpand_p,
                             gboolean  *vexpand_p)
{
  GtkWidget *w;
  gboolean hexpand = FALSE;
  gboolean vexpand = FALSE;

  for (w = gtk_widget_get_first_child (widget);
       w != NULL;
       w = gtk_widget_get_next_sibling (w)) {
    hexpand = hexpand || gtk_widget_compute_expand (w, GTK_ORIENTATION_HORIZONTAL);
    vexpand = vexpand || gtk_widget_compute_expand (w, GTK_ORIENTATION_VERTICAL);
  }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static void
adw_wrap_box_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  AdwWrapBox *self = ADW_WRAP_BOX (object);

  switch (prop_id) {
  case PROP_SPACING:
    g_value_set_int (value, adw_wrap_box_get_spacing (self));
    break;
  case PROP_LINE_SPACING:
    g_value_set_int (value, adw_wrap_box_get_line_spacing (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, get_orientation (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_wrap_box_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  AdwWrapBox *self = ADW_WRAP_BOX (object);

  switch (prop_id) {
  case PROP_SPACING:
    adw_wrap_box_set_spacing (self, g_value_get_int (value));
    break;
  case PROP_LINE_SPACING:
    adw_wrap_box_set_line_spacing (self, g_value_get_int (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_wrap_box_dispose (GObject *object)
{
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (adw_wrap_box_parent_class)->dispose (object);
}

static void
adw_wrap_box_class_init (AdwWrapBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = adw_wrap_box_get_property;
  object_class->set_property = adw_wrap_box_set_property;
  object_class->dispose = adw_wrap_box_dispose;

  widget_class->compute_expand = adw_wrap_box_compute_expand;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * AdwWrapBox:spacing: (attributes org.gtk.Property.get=adw_wrap_box_get_spacing org.gtk.Property.set=adw_wrap_box_set_spacing)
   *
   * The spacing between widgets on the same line.
   *
   * Since: 1.0
   */
  props[PROP_SPACING] =
    g_param_spec_int ("spacing",
                      "Spacing",
                      "The spacing between widgets on the same line",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * AdwWrapBox:line-spacing: (attributes org.gtk.Property.get=adw_wrap_box_get_line_spacing org.gtk.Property.set=adw_wrap_box_set_line_spacing)
   *
   * The spacing between lines.
   *
   * Since: 1.0
   */
  props[PROP_LINE_SPACING] =
    g_param_spec_int ("line-spacing",
                      "Line spacing",
                      "The spacing between lines",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_layout_manager_type (widget_class, ADW_TYPE_WRAP_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "wrapbox");
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_GROUP);
}

static void
adw_wrap_box_init (AdwWrapBox *self)
{
}

static void
adw_wrap_box_buildable_add_child (GtkBuildable *buildable,
                                  GtkBuilder   *builder,
                                  GObject      *child,
                                  const char   *type)
{
  if (GTK_IS_WIDGET (child))
    adw_wrap_box_append (ADW_WRAP_BOX (buildable), GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
adw_wrap_box_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = adw_wrap_box_buildable_add_child;
}

/**
 * adw_wrap_box_new:
 *
 * Creates a new `AdwWrapBox`.
 *
 * Returns: the newly created `AdwWrapBox`
 *
 * Since: 1.0
 */
GtkWidget *
adw_wrap_box_new (void)
{
  return g_object_new (ADW_TYPE_WRAP_BOX, NULL);
}

/**
 * adw_wrap_box_get_spacing: (attributes org.gtk.Method.get_property=spacing)
 * @self: a `AdwWrapBox`
 *
 * Gets spacing between widgets on the same line.
 *
 * Returns: spacing between widgets on the same line
 *
 * Since: 1.0
 */
int
adw_wrap_box_get_spacing (AdwWrapBox *self)
{
  AdwWrapLayout *layout;

  g_return_val_if_fail (ADW_IS_WRAP_BOX (self), 0);

  layout = ADW_WRAP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  return adw_wrap_layout_get_spacing (layout);
}

/**
 * adw_wrap_box_set_spacing: (attributes org.gtk.Method.set_property=spacing)
 * @self: a `AdwWrapBox`
 * @spacing: the spacing
 *
 * Sets the spacing between widgets on the same line.
 *
 * Since: 1.0
 */
void
adw_wrap_box_set_spacing (AdwWrapBox *self,
                          int         spacing)
{
  AdwWrapLayout *layout;

  g_return_if_fail (ADW_IS_WRAP_BOX (self));

  if (spacing < 0)
    spacing = 0;

  layout = ADW_WRAP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  if (spacing == adw_wrap_layout_get_spacing (layout))
    return;

  adw_wrap_layout_set_spacing (layout, spacing);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SPACING]);
}

/**
 * adw_wrap_box_get_line_spacing: (attributes org.gtk.Method.get_property=line-spacing)
 * @self: a `AdwWrapBox`
 *
 * Gets the spacing between lines.
 *
 * Returns: the line spacing
 *
 * Since: 1.0
 */
int
adw_wrap_box_get_line_spacing (AdwWrapBox *self)
{
  AdwWrapLayout *layout;

  g_return_val_if_fail (ADW_IS_WRAP_BOX (self), 0);

  layout = ADW_WRAP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  return adw_wrap_layout_get_line_spacing (layout);
}

/**
 * adw_wrap_box_set_line_spacing: (attributes org.gtk.Method.set_property=line-spacing)
 * @self: a `AdwWrapBox`
 * @line_spacing: the line spacing
 *
 * Sets the spacing between lines.
 *
 * Since: 1.0
 */
void
adw_wrap_box_set_line_spacing (AdwWrapBox *self,
                               int         line_spacing)
{
  AdwWrapLayout *layout;

  g_return_if_fail (ADW_IS_WRAP_BOX (self));

  if (line_spacing < 0)
    line_spacing = 0;

  layout = ADW_WRAP_LAYOUT (gtk_widget_get_layout_manager (GTK_WIDGET (self)));

  if (line_spacing == adw_wrap_layout_get_line_spacing (layout))
    return;

  adw_wrap_layout_set_line_spacing (layout, line_spacing);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LINE_SPACING]);
}

/**
 * adw_wrap_box_insert_child_after:
 * @self: a `AdwWrapBox`
 * @child: the widget to insert
 * @sibling: (nullable): the sibling after which to insert @child
 *
 * Inserts @child in the position after @sibling in the list of @self children.
 *
 * If @sibling is `NULL`, inserts @child at the first position.
 */
void
adw_wrap_box_insert_child_after (AdwWrapBox *self,
                                 GtkWidget  *child,
                                 GtkWidget  *sibling)
{
  g_return_if_fail (ADW_IS_WRAP_BOX (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  if (sibling) {
    g_return_if_fail (GTK_IS_WIDGET (sibling));
    g_return_if_fail (gtk_widget_get_parent (sibling) == GTK_WIDGET (self));
  }

  if (child == sibling)
    return;

  gtk_widget_insert_after (child, GTK_WIDGET (self), sibling);
}

/**
 * adw_wrap_box_reorder_child_after:
 * @self: a `AdwWrapBox`
 * @child: the widget to move, must be a child of @self
 * @sibling: (nullable): the sibling to move @child after
 *
 * Moves @child to the position after @sibling in the list of @self children.
 *
 * If @sibling is `NULL`, moves @child to the first position.
 */
void
adw_wrap_box_reorder_child_after (AdwWrapBox *self,
                                  GtkWidget  *child,
                                  GtkWidget  *sibling)
{
  g_return_if_fail (ADW_IS_WRAP_BOX (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (self));

  if (sibling) {
    g_return_if_fail (GTK_IS_WIDGET (sibling));
    g_return_if_fail (gtk_widget_get_parent (sibling) == GTK_WIDGET (self));
  }

  if (child == sibling)
    return;

  gtk_widget_insert_after (child, GTK_WIDGET (self), sibling);
}

/**
 * adw_wrap_box_append:
 * @self: a `AdwWrapBox`
 * @child: the widget to append
 *
 * Adds @child as the last child to @self.
 */
void
adw_wrap_box_append (AdwWrapBox *self,
                     GtkWidget  *child)
{
  g_return_if_fail (ADW_IS_WRAP_BOX (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  gtk_widget_insert_before (child, GTK_WIDGET (self), NULL);
}

/**
 * adw_wrap_box_prepend:
 * @self: a `AdwWrapBox`
 * @child: the widget to prepend
 *
 * Adds @child as the first child to @self.
 */
void
adw_wrap_box_prepend (AdwWrapBox *self,
                      GtkWidget  *child)
{
  g_return_if_fail (ADW_IS_WRAP_BOX (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == NULL);

  gtk_widget_insert_after (child, GTK_WIDGET (self), NULL);
}

/**
 * adw_wrap_box_remove:
 * @self: a `AdwWrapBox`
 * @child: the child to remove
 *
 * Removes a child widget from @self.
 *
 * The child must have been added before with [method@Adw.WrapBox.append],
 * [method@Adw.WrapBox.prepend], or [method@Adw.WrapBox.insert_child_after].
 */
void
adw_wrap_box_remove (AdwWrapBox *self,
                     GtkWidget  *child)
{
  g_return_if_fail (ADW_IS_WRAP_BOX (self));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (gtk_widget_get_parent (child) == GTK_WIDGET (self));

  gtk_widget_unparent (child);
}

