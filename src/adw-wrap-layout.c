/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "adw-wrap-layout.h"

#include <math.h>

/**
 * AdwWrapLayout:
 *
 * `AdwWrapLayout` is something I couldn't describe because I'm bad at docs.
 *
 * `AdwWrapLayout` is similar to `GtkBoxLayout` but can wrap lines when the
 * widgets cannot fit otherwise. Unlike `GtkFlowBox`, the children aren't
 * arranged into a grid and behave more like words in a wrapped label.
 *
 * Since: 1.0
 */

struct _AdwWrapLayout
{
  GtkLayoutManager parent_instance;

  int spacing;
  int line_spacing;
  GtkOrientation orientation;
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

G_DEFINE_TYPE_WITH_CODE (AdwWrapLayout, adw_wrap_layout, GTK_TYPE_LAYOUT_MANAGER,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

static void
set_orientation (AdwWrapLayout  *self,
                 GtkOrientation  orientation)
{
  if (self->orientation == orientation)
    return;

  self->orientation = orientation;

  gtk_layout_manager_layout_changed (GTK_LAYOUT_MANAGER (self));

  g_object_notify (G_OBJECT (self), "orientation");
}

typedef struct _AllocationData AllocationData;

struct _AllocationData {
  // Provided values
  int minimum_size;
  int natural_size;
  gboolean expand;

  // Computed values
  int allocated_size;

  // Context
  union {
    GtkWidget *widget;
    struct {
      AllocationData *children;
      int n_children;
    } line;
  } data;
};

static int
count_line_children (AdwWrapLayout  *self,
                     int             for_size,
                     AllocationData *child_data,
                     int             n_children,
                     int            *unused_space)
{
  int remaining_space = for_size + self->spacing;
  int n_line_children = 0;

  /* Count how many widgets can fit into this line */
  while (true) {
    int delta;

    if (n_line_children >= n_children)
      break;

    delta = child_data[n_line_children].natural_size + self->spacing; // TODO policy

    // FIXME should set explicit overflow flag, this is dirty
    if (remaining_space - delta < 0)
      break;

    remaining_space -= delta;
    n_line_children++;
  }

  if (unused_space)
    *unused_space = remaining_space;

  return n_line_children;
}

static int
count_lines (AdwWrapLayout  *self,
             int             for_size,
             AllocationData *child_data,
             int             n_children)
{
  int n_lines = 0;

  while (n_children > 0) {
    int unused_space;
    int n_line_children = count_line_children (self, for_size, child_data,
                                               n_children, &unused_space);

    if (n_line_children == 0)
      n_line_children++;

    n_children -= n_line_children;
    child_data = &child_data[n_line_children];
    n_lines++;
  }

  return n_lines;
}

static void
box_allocate (AllocationData *child_data,
              int             n_children,
              int             for_size,
              int             spacing)
{
  int extra_space;
  int n_expand = 0;
  int size_given_to_child = 0;
  int n_extra_widgets = 0;
  int children_minimum_size = 0;
  int i;
  GtkRequestedSize *sizes = g_newa (GtkRequestedSize, n_children);

  for (i = 0; i < n_children; i++) {
    if (child_data[i].expand)
      n_expand++;

    children_minimum_size += child_data[i].minimum_size;
  }

  extra_space = for_size - (n_children - 1) * spacing;
  g_assert (extra_space >= 0);

  for (i = 0; i < n_children; i++) {
    sizes[i].minimum_size = child_data[i].minimum_size;
    sizes[i].natural_size = child_data[i].natural_size;
  }

  /* Bring children up to size first */
  extra_space -= children_minimum_size;
  extra_space = MAX (0, extra_space);
  extra_space = gtk_distribute_natural_allocation (extra_space, n_children, sizes);

  /* Calculate space which hasn't been distributed yet,
   * and is available for expanding children.
   */
  if (n_expand > 0) {
    size_given_to_child = extra_space / n_expand;
    n_extra_widgets = extra_space % n_expand;
  }

  /* Allocate sizes */
  for (i = 0; i < n_children; i++) {
    int allocated_size = sizes[i].minimum_size;

    if (child_data[i].expand) {
      allocated_size += size_given_to_child;

      if (n_extra_widgets > 0) {
        allocated_size++;
        n_extra_widgets--;
      }
    }

    child_data[i].allocated_size = allocated_size;
  }
}

static int
compute_line (AdwWrapLayout  *self,
              int             for_size,
              AllocationData *child_data,
              int             n_children)
{
  int n_line_children, remaining_space;

  g_assert (n_children > 0);

  /* Count how many widgets can fit into this line */
  n_line_children = count_line_children (self, for_size, child_data,
                                         n_children, &remaining_space);

  /* Even one widget doesn't fit. Since we can't have a line with 0 widgets,
   * we take the first one and allocate it out of bounds. Since this only
   * happens with for_size == -1 or when allocating less than minimum width,
   * it's acceptable. */
  if (n_line_children == 0) {
    child_data[0].allocated_size = MAX (for_size, child_data[0].minimum_size);

    return 1;
  }

  /* All widgets fit, we can calculate their exact sizes within the line. */
  box_allocate (child_data, n_line_children, for_size, self->spacing);

  return n_line_children;
}

static void
compute_sizes (AdwWrapLayout   *self,
               GtkWidget       *widget,
               int              for_size,
               AllocationData **child_allocation,
               AllocationData **line_allocation,
               int             *n_lines)
{
  AllocationData *child_data, *line_data, *line_start;
  GtkWidget *child;
  int n_visible_children = 0;
  int i = 0, j;
  GtkOrientation opposite_orientation;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    opposite_orientation = GTK_ORIENTATION_VERTICAL;
  else
    opposite_orientation = GTK_ORIENTATION_HORIZONTAL;

  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child)) {
    if (!gtk_widget_should_layout (child))
      continue;

    n_visible_children++;
  }

  child_data = g_new0 (AllocationData, n_visible_children);

  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child)) {
    if (!gtk_widget_should_layout (child))
      continue;

    gtk_widget_measure (child, self->orientation, -1,
                        &child_data[i].minimum_size,
                        &child_data[i].natural_size,
                        NULL, NULL);

    child_data[i].expand = gtk_widget_compute_expand (child, self->orientation);
    child_data[i].data.widget = child;
    i++;
  }

  *n_lines = count_lines (self, for_size, child_data, n_visible_children);
  line_data = g_new0 (AllocationData, *n_lines);
  line_start = child_data;

  for (i = 0; i < *n_lines; i++) {
    int line_min = 0, line_nat = 0;
    int n_line_children;
    gboolean expand = FALSE;

    n_line_children = compute_line (self, for_size, line_start, n_visible_children);

    g_assert (n_line_children > 0);

    for (j = 0; j < n_line_children; j++) {
      int child_min = 0, child_nat = 0;

      gtk_widget_measure (line_start[j].data.widget,
                          opposite_orientation,
                          line_start[j].allocated_size,
                          &child_min, &child_nat, NULL, NULL);

      expand = expand || gtk_widget_compute_expand (line_start[j].data.widget,
                                                    opposite_orientation);

      line_min = MAX (line_min, child_min);
      line_nat = MAX (line_nat, child_nat);
    }

    line_data[i].minimum_size = line_min;
    line_data[i].natural_size = line_nat;
    line_data[i].expand = expand;
    line_data[i].data.line.children = line_start;
    line_data[i].data.line.n_children = n_line_children;

    n_visible_children -= n_line_children;
    line_start = &line_start[n_line_children];
  }

  *child_allocation = child_data;
  *line_allocation = line_data;
}

static void
adw_wrap_layout_measure (GtkLayoutManager *manager,
                         GtkWidget        *widget,
                         GtkOrientation    orientation,
                         int               for_size,
                         int              *minimum,
                         int              *natural,
                         int              *minimum_baseline,
                         int              *natural_baseline)
{
  AdwWrapLayout *self = ADW_WRAP_LAYOUT (manager);
  GtkWidget *child;
  int min = 0, nat = 0;

  if (self->orientation == orientation) {
    min -= self->spacing;
    nat -= self->spacing;

    for (child = gtk_widget_get_first_child (widget);
         child != NULL;
         child = gtk_widget_get_next_sibling (child)) {
      int child_min, child_nat;

      if (!gtk_widget_should_layout (child))
        continue;

      gtk_widget_measure (child, orientation, -1,
                          &child_min, &child_nat, NULL, NULL);

      min = MAX (min, child_min);
      nat += child_nat + self->spacing;
    }
  } else {
    g_autofree AllocationData *child_data = NULL;
    g_autofree AllocationData *line_data = NULL;
    int i, n_lines;

    compute_sizes (self, widget, for_size, &child_data, &line_data, &n_lines);

    for (i = 0; i < n_lines; i++) {
      min += line_data[i].minimum_size;
      nat += line_data[i].natural_size;
    }

    min += self->line_spacing * (n_lines - 1);
    nat += self->line_spacing * (n_lines - 1);
  }

  if (minimum)
    *minimum = min;
  if (natural)
    *natural = nat;
  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
}

static void
allocate_line (AdwWrapLayout  *self,
               int             width,
               gboolean        is_rtl,
               gboolean        horiz,
               AllocationData *line_child_data,
               int             n_children,
               int             line_size,
               int             line_offset)
{
  int i, widget_offset = 0;

  if (is_rtl && horiz)
    widget_offset = width + self->spacing;

  for (i = 0; i < n_children; i++) {
    GtkWidget *widget = line_child_data[i].data.widget;
    int widget_size = line_child_data[i].allocated_size;
    GskTransform *transform;
    int x, y, w, h;

    if (is_rtl && horiz)
      widget_offset -= widget_size + self->spacing;

    if (horiz) {
      x = widget_offset;
      y = line_offset;
      w = widget_size;
      h = line_size;
    } else {
      x = line_offset;
      y = widget_offset;
      w = line_size;
      h = widget_size;
    }

    transform = gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (x, y));
    gtk_widget_allocate (widget, w, h, -1, transform);

    if (!is_rtl || !horiz)
      widget_offset += widget_size + self->spacing;
  }
}

static void
adw_wrap_layout_allocate (GtkLayoutManager *manager,
                          GtkWidget        *widget,
                          int               width,
                          int               height,
                          int               baseline)
{
  AdwWrapLayout *self = ADW_WRAP_LAYOUT (manager);
  g_autofree AllocationData *child_data = NULL;
  g_autofree AllocationData *line_data = NULL;
  int n_lines;
  gboolean horiz = self->orientation == GTK_ORIENTATION_HORIZONTAL;
  gboolean is_rtl = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
  int i, line_pos = 0;

  if (is_rtl && !horiz)
    line_pos = width + self->line_spacing;

  compute_sizes (self, widget, horiz ? width : height,
                 &child_data, &line_data, &n_lines);
  box_allocate (line_data, n_lines,
                horiz ? height : width, self->line_spacing);

  for (i = 0; i < n_lines; i++) {
    if (is_rtl && !horiz)
      line_pos -= line_data[i].allocated_size + self->line_spacing;

    allocate_line (self, width, is_rtl, horiz,
                   line_data[i].data.line.children,
                   line_data[i].data.line.n_children,
                   line_data[i].allocated_size, line_pos);

    if (!is_rtl || horiz)
      line_pos += line_data[i].allocated_size + self->line_spacing;
  }
}

static GtkSizeRequestMode
adw_wrap_layout_get_request_mode (GtkLayoutManager *manager,
                                  GtkWidget        *widget)
{
  AdwWrapLayout *self = ADW_WRAP_LAYOUT (manager);

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;

  return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

static void
adw_wrap_layout_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  AdwWrapLayout *self = ADW_WRAP_LAYOUT (object);

  switch (prop_id) {
  case PROP_SPACING:
    g_value_set_int (value, adw_wrap_layout_get_spacing (self));
    break;
  case PROP_LINE_SPACING:
    g_value_set_int (value, adw_wrap_layout_get_line_spacing (self));
    break;
  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_wrap_layout_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  AdwWrapLayout *self = ADW_WRAP_LAYOUT (object);

  switch (prop_id) {
  case PROP_SPACING:
    adw_wrap_layout_set_spacing (self, g_value_get_int (value));
    break;
  case PROP_LINE_SPACING:
    adw_wrap_layout_set_line_spacing (self, g_value_get_int (value));
    break;
  case PROP_ORIENTATION:
    set_orientation (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
adw_wrap_layout_class_init (AdwWrapLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkLayoutManagerClass *layout_manager_class = GTK_LAYOUT_MANAGER_CLASS (klass);

  object_class->get_property = adw_wrap_layout_get_property;
  object_class->set_property = adw_wrap_layout_set_property;

  layout_manager_class->measure = adw_wrap_layout_measure;
  layout_manager_class->allocate = adw_wrap_layout_allocate;
  layout_manager_class->get_request_mode = adw_wrap_layout_get_request_mode;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  /**
   * AdwWrapLayout:spacing: (attributes org.gtk.Property.get=adw_wrap_layout_get_spacing org.gtk.Property.set=adw_wrap_layout_set_spacing)
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
   * AdwWrapLayout:line-spacing: (attributes org.gtk.Property.get=adw_wrap_layout_get_line_spacing org.gtk.Property.set=adw_wrap_layout_set_line_spacing)
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
}

static void
adw_wrap_layout_init (AdwWrapLayout *self)
{
}

/**
 * adw_wrap_layout_new:
 *
 * Creates a new `AdwWrapLayout`.
 *
 * Returns: the newly created `AdwWrapLayout`
 *
 * Since: 1.0
 */
GtkLayoutManager *
adw_wrap_layout_new (void)
{
  return g_object_new (ADW_TYPE_WRAP_LAYOUT, NULL);
}

/**
 * adw_wrap_layout_get_spacing: (attributes org.gtk.Method.get_property=spacing)
 * @self: a `AdwWrapLayout`
 *
 * Gets spacing between widgets on the same line.
 *
 * Returns: spacing between widgets on the same line
 *
 * Since: 1.0
 */
int
adw_wrap_layout_get_spacing (AdwWrapLayout *self)
{
  g_return_val_if_fail (ADW_IS_WRAP_LAYOUT (self), 0);

  return self->spacing;
}

/**
 * adw_wrap_layout_set_spacing: (attributes org.gtk.Method.set_property=spacing)
 * @self: a `AdwWrapLayout`
 * @spacing: the spacing
 *
 * Sets the spacing between widgets on the same line.
 *
 * Since: 1.0
 */
void
adw_wrap_layout_set_spacing (AdwWrapLayout *self,
                             int            spacing)
{
  g_return_if_fail (ADW_IS_WRAP_LAYOUT (self));

  if (spacing < 0)
    spacing = 0;

  if (spacing == self->spacing)
    return;

  self->spacing = spacing;

  gtk_layout_manager_layout_changed (GTK_LAYOUT_MANAGER (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SPACING]);
}

/**
 * adw_wrap_layout_get_line_spacing: (attributes org.gtk.Method.get_property=line-spacing)
 * @self: a `AdwWrapLayout`
 *
 * Gets the spacing between lines.
 *
 * Returns: the line spacing
 *
 * Since: 1.0
 */
int
adw_wrap_layout_get_line_spacing (AdwWrapLayout *self)
{
  g_return_val_if_fail (ADW_IS_WRAP_LAYOUT (self), 0);

  return self->line_spacing;
}

/**
 * adw_wrap_layout_set_line_spacing: (attributes org.gtk.Method.set_property=line-spacing)
 * @self: a `AdwWrapLayout`
 * @line_spacing: the line spacing
 *
 * Sets the spacing between lines.
 *
 * Since: 1.0
 */
void
adw_wrap_layout_set_line_spacing (AdwWrapLayout *self,
                                  int            line_spacing)
{
  g_return_if_fail (ADW_IS_WRAP_LAYOUT (self));

  if (line_spacing < 0)
    line_spacing = 0;

  if (line_spacing == self->line_spacing)
    return;

  self->line_spacing = line_spacing;

  gtk_layout_manager_layout_changed (GTK_LAYOUT_MANAGER (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LINE_SPACING]);
}
