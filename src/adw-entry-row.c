/*
 * Copyright (C) 2021 Maximiliano Sandoval <msandova@protonmail.com>
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"
#include "adw-entry-row-private.h"

#include "adw-animation-private.h"
#include "adw-animation-util.h"
#include "adw-gizmo-private.h"
#include "adw-macros-private.h"
#include "adw-timed-animation.h"
#include "adw-widget-utils-private.h"

#define EMPTY_ANIMATION_DURATION 150
#define TITLE_SPACING 3

/**
 * AdwEntryRow:
 *
 * A [class@Gtk.ListBoxRow] used to present an entry inside lists.
 *
 * <picture>
 *   <source srcset="entry-row-dark.png" media="(prefers-color-scheme: dark)">
 *   <img src="entry-row.png" alt="entry-row">
 * </picture>
 *
 * An edit icon is shown to indicate whether the underlying [class@Gtk.Text] has
 * focus and the widget is [property@Gtk.Editable:editable].
 *
 * The `AdwEntryRow` widget can have a [property@Adw.PreferencesRow:title] and
 * [property@Gtk.Editable:text]. The row can receive additional widgets at its
 * end, or prefix widgets at its start.
 *
 * `AdwEntryRow` text does not have API of its own, but it implements the
 * [iface@Gtk.Editable] interface and partially exposes the internal
 * [class@Gtk.Text] API.
 *
 * ## AdwEntryRow as GtkBuildable
 *
 * The `AdwEntryRow` implementation of the [iface@Gtk.Buildable] interface
 * supports adding children at its end by specifying “suffix” or omitting the
 * “type” attribute of a <child> element.
 *
 * It also supports adding a children as a prefix widget by specifying “prefix”
 * as the “type” attribute of a <child> element.
 *
 * # CSS nodes
 *
 * `AdwEntryRow` has a main CSS node with name `row` and the `.entry` style
 * class.
 *
 * Since: 1.2
 */

typedef struct
{
  GtkWidget *header;
  GtkWidget *text;
  GtkWidget *title;
  GtkWidget *empty_title;
  GtkWidget *editable_area;
  GtkWidget *edit_icon;
  GtkWidget *apply_button;
  GtkWidget *indicator;
  GtkBox *suffixes;
  GtkBox *prefixes;

  gboolean empty;
  double empty_progress;
  AdwAnimation *empty_animation;

  gboolean editing;
  gboolean show_apply_button;
  gboolean text_changed;
  gboolean show_indicator;
} AdwEntryRowPrivate;

static void adw_entry_row_editable_init (GtkEditableInterface *iface);
static void adw_entry_row_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (AdwEntryRow, adw_entry_row, ADW_TYPE_PREFERENCES_ROW,
                         G_ADD_PRIVATE (AdwEntryRow)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, adw_entry_row_buildable_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_EDITABLE, adw_entry_row_editable_init))

static GtkBuildableIface *parent_buildable_iface;

enum {
  PROP_0,
  PROP_SHOW_APPLY_BUTTON,
  PROP_LAST_PROP,
};

static GParamSpec *props[PROP_LAST_PROP];

enum {
  SIGNAL_APPLY,
  SIGNAL_LAST_SIGNAL,
};

static guint signals[SIGNAL_LAST_SIGNAL];

static void
empty_animation_value_cb (double       value,
                          AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  priv->empty_progress = value;

  gtk_widget_queue_allocate (priv->editable_area);

  gtk_widget_set_opacity (priv->text, value);
  gtk_widget_set_opacity (priv->title, value);
  gtk_widget_set_opacity (priv->empty_title, 1 - value);
}

static gboolean
is_text_focused (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);
  GtkStateFlags flags = gtk_widget_get_state_flags (priv->text);

  return !!(flags & GTK_STATE_FLAG_FOCUS_WITHIN);
}

static void
update_empty (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);
  GtkEntryBuffer *buffer = gtk_text_get_buffer (GTK_TEXT (priv->text));
  gboolean focused = is_text_focused (self);
  gboolean editable = gtk_editable_get_editable (GTK_EDITABLE (priv->text));
  gboolean empty = gtk_entry_buffer_get_length (buffer) == 0;

  gtk_widget_set_visible (priv->edit_icon, !priv->text_changed && (!priv->editing || !editable));
  gtk_widget_set_sensitive (priv->edit_icon, editable);
  gtk_widget_set_visible (priv->indicator, priv->editing && priv->show_indicator);
  gtk_widget_set_visible (priv->apply_button, priv->text_changed);

  priv->empty = empty && !(focused && editable) && !priv->text_changed;

  gtk_widget_queue_allocate (priv->editable_area);

  adw_timed_animation_set_value_from (ADW_TIMED_ANIMATION (priv->empty_animation),
                                      priv->empty_progress);
  adw_timed_animation_set_value_to (ADW_TIMED_ANIMATION (priv->empty_animation),
                                    priv->empty ? 0 : 1);
  adw_animation_play (priv->empty_animation);
}

static void
text_changed_cb (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  if (priv->show_apply_button && priv->editing)
    priv->text_changed = TRUE;

  update_empty (self);
}

static void
text_state_flags_changed_cb (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  priv->editing = is_text_focused (self);

  if (priv->editing)
    gtk_widget_add_css_class (GTK_WIDGET (self), "focused");
  else
    gtk_widget_remove_css_class (GTK_WIDGET (self), "focused");

  update_empty (self);
}

static gboolean
text_keynav_failed_cb (AdwEntryRow      *self,
                       GtkDirectionType  direction)
{
  if (direction == GTK_DIR_LEFT || direction == GTK_DIR_RIGHT)
    return gtk_widget_child_focus (GTK_WIDGET (self), direction);

  return GDK_EVENT_PROPAGATE;
}

static void
pressed_cb (GtkGesture  *gesture,
            int          n_press,
            double       x,
            double       y,
            AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);
  GtkWidget *picked;

  picked = gtk_widget_pick (GTK_WIDGET (self), x, y, GTK_PICK_DEFAULT);

  if (picked != GTK_WIDGET (self) &&
      picked != priv->header &&
      picked != priv->indicator &&
      picked != GTK_WIDGET (priv->prefixes) &&
      picked != GTK_WIDGET (priv->suffixes)) {
    gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_DENIED);

    return;
  }

  gtk_widget_grab_focus (GTK_WIDGET (priv->text));

  gtk_gesture_set_state (gesture, GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
apply_button_clicked_cb (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  if (gtk_widget_has_focus (priv->apply_button))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  priv->text_changed = FALSE;
  update_empty (self);

  g_signal_emit (self, signals[SIGNAL_APPLY], 0);
}

static void
text_activated_cb (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  if (gtk_widget_get_visible (priv->apply_button))
    apply_button_clicked_cb (self);
}

static void
measure_editable_area (GtkWidget      *widget,
                       GtkOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  AdwEntryRow *self = g_object_get_data (G_OBJECT (widget), "row");
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);
  int text_min = 0, text_nat = 0;
  int title_min = 0, title_nat = 0;
  int empty_min = 0, empty_nat = 0;

  gtk_widget_measure (priv->text, orientation, for_size,
                      &text_min, &text_nat, NULL, NULL);
  gtk_widget_measure (priv->title, orientation, for_size,
                      &title_min, &title_nat, NULL, NULL);
  gtk_widget_measure (priv->empty_title, orientation, for_size,
                      &empty_min, &empty_nat, NULL, NULL);

  if (minimum)
    *minimum = MAX (text_min + TITLE_SPACING + title_min, empty_min);

  if (natural)
    *natural = MAX (text_nat + TITLE_SPACING + title_nat, empty_nat);

  if (minimum_baseline)
    *minimum_baseline = -1;

  if (natural_baseline)
    *natural_baseline = -1;
}

static void
allocate_editable_area (GtkWidget *widget,
                        int        width,
                        int        height,
                        int        baseline)
{
  AdwEntryRow *self = g_object_get_data (G_OBJECT (widget), "row");
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);
  gboolean is_rtl = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
  GskTransform *transform;
  int empty_height = 0, title_height = 0, text_height = 0, text_baseline = -1;
  float empty_scale, title_scale, title_offset;

  gtk_widget_measure (priv->title, GTK_ORIENTATION_VERTICAL, width,
                      NULL, &title_height, NULL, NULL);
  gtk_widget_measure (priv->empty_title, GTK_ORIENTATION_VERTICAL, width,
                      NULL, &empty_height, NULL, NULL);
  gtk_widget_measure (priv->text, GTK_ORIENTATION_VERTICAL, width,
                      NULL, &text_height, NULL, &text_baseline);

  empty_scale = (float) adw_lerp (1.0, (double) title_height / empty_height, priv->empty_progress);
  title_scale = (float) adw_lerp ((double) empty_height / title_height, 1.0, priv->empty_progress);
  title_offset = (float) adw_lerp ((double) (height - empty_height) / 2.0,
                                   (double) (height - title_height - text_height - TITLE_SPACING) / 2.0,
                                   priv->empty_progress);

  transform = gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (0, title_offset));
  if (is_rtl)
    transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (width, 0));
  transform = gsk_transform_scale (transform, empty_scale, empty_scale);
  if (is_rtl)
    transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (-width, 0));
  gtk_widget_allocate (priv->empty_title, width, empty_height, -1, transform);

  transform = gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (0, title_offset));
  if (is_rtl)
    transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (width, 0));
  transform = gsk_transform_scale (transform, title_scale, title_scale);
  if (is_rtl)
    transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (-width, 0));
  gtk_widget_allocate (priv->title, width, title_height, -1, transform);

  text_baseline += (int) ((double) (height + title_height - text_height + TITLE_SPACING) / 2.0);
  gtk_widget_allocate (priv->text, width, height, text_baseline, NULL);
}

static gboolean
adw_entry_row_grab_focus (GtkWidget *widget)
{
  AdwEntryRow *self = ADW_ENTRY_ROW (widget);
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  return gtk_widget_grab_focus (priv->text);
}

static void
adw_entry_row_get_property (GObject     *object,
                            guint        prop_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
  AdwEntryRow *self = ADW_ENTRY_ROW (object);

  if (gtk_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  switch (prop_id) {
  case PROP_SHOW_APPLY_BUTTON:
    g_value_set_boolean (value, adw_entry_row_get_show_apply_button (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_entry_row_set_property (GObject       *object,
                            guint          prop_id,
                            const GValue  *value,
                            GParamSpec    *pspec)
{
  AdwEntryRow *self = ADW_ENTRY_ROW (object);

  if (gtk_editable_delegate_set_property (object, prop_id, value, pspec))
  {
    switch (prop_id) {
    case PROP_LAST_PROP + GTK_EDITABLE_PROP_EDITABLE:
      update_empty (self);
      break;
    default:;
    }
    return;
  }

  switch (prop_id) {
  case PROP_SHOW_APPLY_BUTTON:
    adw_entry_row_set_show_apply_button (self, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_entry_row_dispose (GObject *object)
{
  AdwEntryRow *self = ADW_ENTRY_ROW (object);
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  g_clear_object (&priv->empty_animation);

  if (priv->text)
    gtk_editable_finish_delegate (GTK_EDITABLE (self));

  G_OBJECT_CLASS (adw_entry_row_parent_class)->dispose (object);
}

static void
adw_entry_row_class_init (AdwEntryRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = adw_entry_row_get_property;
  object_class->set_property = adw_entry_row_set_property;
  object_class->dispose = adw_entry_row_dispose;

  widget_class->focus = adw_widget_focus_child;
  widget_class->grab_focus = adw_entry_row_grab_focus;

  /**
   * AdwEntryRow:show-apply-button: (attributes org.gtk.Property.get=adw_entry_row_get_show_apply_button org.gtk.Property.set=adw_entry_row_set_show_apply_button)
   *
   * Whether to show the apply button.
   *
   * See [signal@EntryRow::apply].
   *
   * Since: 1.2
   */
  props[PROP_SHOW_APPLY_BUTTON] =
    g_param_spec_boolean ("show-apply-button",
                          "Show Apply Button",
                          "Whether to show the apply button",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  gtk_editable_install_properties (object_class, PROP_LAST_PROP);

  /**
   * AdwEntryRow::apply:
   *
   * Emitted when the apply button is activated.
   *
   * See [property@EntryRow:show-apply-button].
   *
   * Since: 1.2
   */
  signals[SIGNAL_APPLY] =
    g_signal_new ("apply",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Adwaita/ui/adw-entry-row.ui");
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, header);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, prefixes);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, suffixes);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, editable_area);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, text);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, empty_title);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, title);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, edit_icon);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, apply_button);
  gtk_widget_class_bind_template_child_private (widget_class, AdwEntryRow, indicator);

  gtk_widget_class_bind_template_callback (widget_class, pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, text_state_flags_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, text_keynav_failed_cb);
  gtk_widget_class_bind_template_callback (widget_class, text_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, update_empty);
  gtk_widget_class_bind_template_callback (widget_class, text_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, apply_button_clicked_cb);

  g_type_ensure (ADW_TYPE_GIZMO);
}

static void
adw_entry_row_init (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);
  AdwAnimationTarget *target;

  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_editable_init_delegate (GTK_EDITABLE (self));

  adw_gizmo_set_measure_func (ADW_GIZMO (priv->editable_area), (AdwGizmoMeasureFunc) measure_editable_area);
  adw_gizmo_set_allocate_func (ADW_GIZMO (priv->editable_area), (AdwGizmoAllocateFunc) allocate_editable_area);
  adw_gizmo_set_focus_func (ADW_GIZMO (priv->editable_area), (AdwGizmoFocusFunc) adw_widget_focus_child);

  g_object_set_data (G_OBJECT (priv->editable_area), "row", self);

  priv->empty_progress = 0.0;

  target = adw_callback_animation_target_new ((AdwAnimationTargetFunc)
                                              empty_animation_value_cb,
                                              self, NULL);

  priv->empty_animation =
    adw_timed_animation_new (GTK_WIDGET (self), 0, 0,
                             EMPTY_ANIMATION_DURATION, target);

  update_empty (self);
}

static void
adw_entry_row_buildable_add_child (GtkBuildable *buildable,
                                   GtkBuilder   *builder,
                                   GObject      *child,
                                   const char   *type)
{
  AdwEntryRow *self = ADW_ENTRY_ROW (buildable);
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  if (!priv->header)
    parent_buildable_iface->add_child (buildable, builder, child, type);
  else if (g_strcmp0 (type, "prefix") == 0)
    adw_entry_row_add_prefix (self, GTK_WIDGET (child));
  else if (g_strcmp0 (type, "suffix") == 0)
    adw_entry_row_add_suffix (self, GTK_WIDGET (child));
  else if (!type && GTK_IS_WIDGET (child))
    adw_entry_row_add_suffix (self, GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
adw_entry_row_buildable_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = adw_entry_row_buildable_add_child;
}

static GtkEditable *
adw_entry_row_get_delegate (GtkEditable *editable)
{
  AdwEntryRow *self = ADW_ENTRY_ROW (editable);
  AdwEntryRowPrivate *priv = adw_entry_row_get_instance_private (self);

  return GTK_EDITABLE (priv->text);
}

void
adw_entry_row_editable_init (GtkEditableInterface *iface)
{
  iface->get_delegate = adw_entry_row_get_delegate;
}

/**
 * adw_entry_row_new:
 *
 * Creates a new `AdwEntryRow`.
 *
 * Returns: the newly created `AdwEntryRow`
 *
 * Since: 1.2
 */
GtkWidget *
adw_entry_row_new (void)
{
  return g_object_new (ADW_TYPE_ENTRY_ROW, NULL);
}

/**
 * adw_entry_row_add_prefix:
 * @self: an entry row
 * @widget: a widget
 *
 * Adds a prefix widget to @self.
 *
 * Since: 1.2
 */
void
adw_entry_row_add_prefix (AdwEntryRow *self,
                          GtkWidget   *widget)
{
  AdwEntryRowPrivate *priv;

  g_return_if_fail (ADW_IS_ENTRY_ROW (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  priv = adw_entry_row_get_instance_private (self);

  gtk_box_prepend (priv->prefixes, widget);
  gtk_widget_show (GTK_WIDGET (priv->prefixes));
}

/**
 * adw_entry_row_add_suffix:
 * @self: an entry row
 * @widget: a widget
 *
 * Adds a suffix widget to @self.
 *
 * Since: 1.2
 */
void
adw_entry_row_add_suffix (AdwEntryRow *self,
                          GtkWidget    *widget)
{
  AdwEntryRowPrivate *priv;

  g_return_if_fail (ADW_IS_ENTRY_ROW (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  priv = adw_entry_row_get_instance_private (self);

  gtk_box_append (priv->suffixes, widget);
  gtk_widget_show (GTK_WIDGET (priv->suffixes));
}

/**
 * adw_entry_row_remove:
 * @self: an entry row
 * @widget: the child to be removed
 *
 * Removes a child from @self.
 *
 * Since: 1.2
 */
void
adw_entry_row_remove (AdwEntryRow *self,
                      GtkWidget   *child)
{
  AdwEntryRowPrivate *priv;
  GtkWidget *parent;

  g_return_if_fail (ADW_IS_ENTRY_ROW (self));
  g_return_if_fail (GTK_IS_WIDGET (child));

  priv = adw_entry_row_get_instance_private (self);

  parent = gtk_widget_get_parent (child);

  if (parent == GTK_WIDGET (priv->prefixes))
    gtk_box_remove (priv->prefixes, child);
  else if (parent == GTK_WIDGET (priv->suffixes))
    gtk_box_remove (priv->suffixes, child);
  else
    ADW_CRITICAL_CANNOT_REMOVE_CHILD (self, child);
}

/**
 * adw_entry_row_get_show_apply_button: (attributes org.gtk.Method.get_property=show-apply-button)
 * @self: an entry row
 *
 * Gets whether the apply button is shown.
 *
 * Returns: whether apply button is shown
 *
 * Since: 1.2
 */
gboolean
adw_entry_row_get_show_apply_button (AdwEntryRow *self)
{
  AdwEntryRowPrivate *priv;

  g_return_val_if_fail (ADW_IS_ENTRY_ROW (self), FALSE);

  priv = adw_entry_row_get_instance_private (self);

  return priv->show_apply_button;
}

/**
 * adw_entry_row_set_show_apply_button: (attributes org.gtk.Method.set_property=show-apply-button)
 * @self: an entry row
 * @show_apply_button: whether apply button is shown
 *
 * Sets whether the apply button is shown.
 *
 * Since: 1.2
 */
void
adw_entry_row_set_show_apply_button (AdwEntryRow *self,
                                     gboolean     show_apply_button)
{
  AdwEntryRowPrivate *priv;

  g_return_if_fail (ADW_IS_ENTRY_ROW (self));

  priv = adw_entry_row_get_instance_private (self);

  show_apply_button = !!show_apply_button;

  if (priv->show_apply_button == show_apply_button)
    return;

  priv->show_apply_button = show_apply_button;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SHOW_APPLY_BUTTON]);
}

void
adw_entry_row_set_indicator_icon_name (AdwEntryRow *self,
                                       const char  *icon_name)
{
  AdwEntryRowPrivate *priv;

  g_return_if_fail (ADW_IS_ENTRY_ROW (self));

  priv = adw_entry_row_get_instance_private (self);

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->indicator), icon_name);
}

void
adw_entry_row_set_indicator_tooltip (AdwEntryRow *self,
                                     const char  *tooltip)
{
  AdwEntryRowPrivate *priv;

  g_return_if_fail (ADW_IS_ENTRY_ROW (self));

  priv = adw_entry_row_get_instance_private (self);

  gtk_widget_set_tooltip_text (priv->indicator, tooltip);
}

void
adw_entry_row_set_show_indicator (AdwEntryRow *self,
                                  gboolean     show_indicator)
{
  AdwEntryRowPrivate *priv;

  g_return_if_fail (ADW_IS_ENTRY_ROW (self));

  priv = adw_entry_row_get_instance_private (self);

  show_indicator = !!show_indicator;

  priv->show_indicator = show_indicator;

  update_empty (self);
}
