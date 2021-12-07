#include "adw-tab-view-demo-window.h"

#include <glib/gi18n.h>

struct _AdwTabViewDemoWindow
{
  AdwWindow parent_instance;
  AdwTabView *view;
  AdwTabBar *tab_bar;

  GActionMap *tab_action_group;

  AdwTabPage *menu_page;
};

G_DEFINE_TYPE (AdwTabViewDemoWindow, adw_tab_view_demo_window, ADW_TYPE_WINDOW)

char **icon_names = NULL;
gsize n_icon_names = 0;

static void
init_icon_names (GtkIconTheme *theme)
{
  if (icon_names)
    return;

  icon_names = gtk_icon_theme_get_icon_names (theme);
  n_icon_names = g_strv_length (icon_names);
}

static GIcon *
get_random_icon (AdwTabViewDemoWindow *self)
{
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (self));
  GtkIconTheme *theme = gtk_icon_theme_get_for_display (display);
  int index;

  init_icon_names (theme);

  index = g_random_int_range (0, n_icon_names);

  return g_themed_icon_new (icon_names[index]);
}

static void
window_new (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  AdwTabViewDemoWindow *window = adw_tab_view_demo_window_new ();

  adw_tab_view_demo_window_prepopulate (window);

  gtk_window_present (GTK_WINDOW (window));
}

static gboolean
text_to_tooltip (GBinding     *binding,
                 const GValue *input,
                 GValue       *output,
                 gpointer      user_data)
{
  const char *title = g_value_get_string (input);
  char *tooltip = g_markup_printf_escaped (_("Elaborate tooltip for <b>%s</b>"), title);

  g_value_take_string (output, tooltip);

  return TRUE;
}

static AdwTabPage *
add_page (AdwTabViewDemoWindow *self,
          AdwTabPage           *parent,
          const char           *title,
          GIcon                *icon)
{
  GtkWidget *content;
  AdwTabPage *page;

  content = g_object_new (GTK_TYPE_ENTRY,
                          "text", title,
                          "halign", GTK_ALIGN_CENTER,
                          "valign", GTK_ALIGN_CENTER,
                          NULL);

  page = adw_tab_view_add_page (self->view, GTK_WIDGET (content), parent);

  g_object_bind_property (content, "text",
                          page, "title",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_object_bind_property_full (content, "text",
                               page, "tooltip",
                               G_BINDING_SYNC_CREATE,
                               text_to_tooltip, NULL,
                               NULL, NULL);

  adw_tab_page_set_icon (page, icon);
  adw_tab_page_set_indicator_activatable (page, TRUE);

  return page;
}

static void
tab_new (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);
  g_autofree char *title = NULL;
  AdwTabPage *page;
  GtkWidget *content;
  GIcon *icon;
  static int next_page = 1;

  title = g_strdup_printf (_("Tab %d"), next_page);
  icon = get_random_icon (self);

  page = add_page (self, NULL, title, icon);
  content = adw_tab_page_get_child (page);

  adw_tab_view_set_selected_page (self->view, page);

  gtk_widget_grab_focus (content);

  next_page++;
}

static AdwTabPage *
get_current_page (AdwTabViewDemoWindow *self)
{
  if (self->menu_page)
    return self->menu_page;

  return adw_tab_view_get_selected_page (self->view);
}

static void
tab_pin (GSimpleAction *action,
         GVariant      *parameter,
         gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);

  adw_tab_view_set_page_pinned (self->view, get_current_page (self), TRUE);
}

static void
tab_unpin (GSimpleAction *action,
           GVariant      *parameter,
           gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);

  adw_tab_view_set_page_pinned (self->view, get_current_page (self), FALSE);
}

static void
tab_close (GSimpleAction *action,
           GVariant      *parameter,
           gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);

  adw_tab_view_close_page (self->view, get_current_page (self));
}

static void
tab_close_other (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);

  adw_tab_view_close_other_pages (self->view, get_current_page (self));
}

static void
tab_close_before (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);

  adw_tab_view_close_pages_before (self->view, get_current_page (self));
}

static void
tab_close_after (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);

  adw_tab_view_close_pages_after (self->view, get_current_page (self));
}

static void
tab_move_to_new_window (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);

  AdwTabViewDemoWindow *window = adw_tab_view_demo_window_new ();

  adw_tab_view_transfer_page (self->view,
                              self->menu_page,
                              window->view,
                              0);

  gtk_window_present (GTK_WINDOW (window));
}

static void
tab_change_needs_attention (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);
  gboolean need_attention = g_variant_get_boolean (parameter);

  adw_tab_page_set_needs_attention (get_current_page (self), need_attention);
  g_simple_action_set_state (action, g_variant_new_boolean (need_attention));
}

static void
tab_change_loading (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);
  gboolean loading = g_variant_get_boolean (parameter);

  adw_tab_page_set_loading (get_current_page (self), loading);
  g_simple_action_set_state (action, g_variant_new_boolean (loading));
}

static GIcon *
get_indicator_icon (AdwTabPage *page)
{
  gboolean muted;

  muted = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (page),
                                              "adw-tab-view-demo-muted"));

  if (muted)
    return g_themed_icon_new ("tab-audio-muted-symbolic");
  else
    return g_themed_icon_new ("tab-audio-playing-symbolic");
}

static void
tab_change_indicator (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);
  gboolean indicator = g_variant_get_boolean (parameter);
  g_autoptr (GIcon) icon = NULL;

  if (indicator)
    icon = get_indicator_icon (get_current_page (self));

  adw_tab_page_set_indicator_icon (get_current_page (self), icon);
  g_simple_action_set_state (action, g_variant_new_boolean (indicator));
}

static void
tab_change_icon (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);
  gboolean enable_icon = g_variant_get_boolean (parameter);

  if (enable_icon) {
    g_autoptr (GIcon) icon = get_random_icon (self);

    adw_tab_page_set_icon (get_current_page (self), icon);
  } else {
    adw_tab_page_set_icon (get_current_page (self), NULL);
  }

  g_simple_action_set_state (action, g_variant_new_boolean (enable_icon));
}

static void
tab_refresh_icon (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);
  g_autoptr (GIcon) icon = get_random_icon (self);

  adw_tab_page_set_icon (get_current_page (self), icon);
}

static void
tab_duplicate (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (user_data);
  AdwTabPage *parent = get_current_page (self);
  AdwTabPage *page;

  page = add_page (self, parent,
                   adw_tab_page_get_title (parent),
                   adw_tab_page_get_icon (parent));

  adw_tab_page_set_indicator_icon (page, adw_tab_page_get_indicator_icon (parent));
  adw_tab_page_set_loading (page, adw_tab_page_get_loading (parent));
  adw_tab_page_set_needs_attention (page, adw_tab_page_get_needs_attention (parent));

  g_object_set_data (G_OBJECT (page),
                     "adw-tab-view-demo-muted",
                     g_object_get_data (G_OBJECT (parent),
                                        "adw-tab-view-demo-muted"));

  adw_tab_view_set_selected_page (self->view, page);
}

static GActionEntry action_entries[] = {
  { "window-new", window_new },
  { "tab-new", tab_new },
};

static GActionEntry tab_action_entries[] = {
  { "pin", tab_pin },
  { "unpin", tab_unpin },
  { "close", tab_close },
  { "close-other", tab_close_other },
  { "close-before", tab_close_before },
  { "close-after", tab_close_after },
  { "move-to-new-window", tab_move_to_new_window },
  { "needs-attention", NULL, NULL, "false", tab_change_needs_attention },
  { "loading", NULL, NULL, "false", tab_change_loading },
  { "indicator", NULL, NULL, "false", tab_change_indicator },
  { "icon", NULL, NULL, "false", tab_change_icon },
  { "refresh-icon", tab_refresh_icon },
  { "duplicate", tab_duplicate },
};

static inline void
set_tab_action_enabled (AdwTabViewDemoWindow *self,
                        const char           *name,
                        gboolean              enabled)
{
  GAction *action = g_action_map_lookup_action (self->tab_action_group, name);

  g_assert (G_IS_SIMPLE_ACTION (action));

  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               enabled);
}

static inline void
set_tab_action_state (AdwTabViewDemoWindow *self,
                      const char           *name,
                      gboolean              state)
{
  GAction *action = g_action_map_lookup_action (self->tab_action_group, name);

  g_assert (G_IS_SIMPLE_ACTION (action));

  g_simple_action_set_state (G_SIMPLE_ACTION (action),
                             g_variant_new_boolean (state));
}

static void
page_detached_cb (AdwTabViewDemoWindow *self,
                  AdwTabPage           *page)
{
  if (!adw_tab_view_get_n_pages (self->view))
    gtk_window_close (GTK_WINDOW (self));
}

static void
setup_menu_cb (AdwTabViewDemoWindow *self,
               AdwTabPage           *page,
               AdwTabView           *view)
{
  AdwTabPage *prev = NULL;
  gboolean can_close_before = TRUE, can_close_after = TRUE;
  gboolean pinned = FALSE, prev_pinned;
  gboolean has_icon = FALSE;
  guint n_pages, pos;

  self->menu_page = page;

  n_pages = adw_tab_view_get_n_pages (self->view);

  if (page) {
    pos = adw_tab_view_get_page_position (self->view, page);

    if (pos > 0)
      prev = adw_tab_view_get_nth_page (self->view, pos - 1);

    pinned = adw_tab_page_get_pinned (page);
    prev_pinned = prev && adw_tab_page_get_pinned (prev);

    can_close_before = !pinned && prev && !prev_pinned;
    can_close_after = pos < n_pages - 1;

    has_icon = adw_tab_page_get_icon (page) != NULL;
  }

  set_tab_action_enabled (self, "pin", !page || !pinned);
  set_tab_action_enabled (self, "unpin", !page || pinned);
  set_tab_action_enabled (self, "close", !page || !pinned);
  set_tab_action_enabled (self, "close-before", can_close_before);
  set_tab_action_enabled (self, "close-after", can_close_after);
  set_tab_action_enabled (self, "close-other", can_close_before || can_close_after);
  set_tab_action_enabled (self, "move-to-new-window", !page || (!pinned && n_pages > 1));
  set_tab_action_enabled (self, "refresh-icon", has_icon);

  if (page) {
    set_tab_action_state (self, "icon", has_icon);
    set_tab_action_state (self, "loading", adw_tab_page_get_loading (page));
    set_tab_action_state (self, "needs-attention", adw_tab_page_get_needs_attention (page));
    set_tab_action_state (self, "indicator", adw_tab_page_get_indicator_icon (page) != NULL);
  }
}

static AdwTabView *
create_window_cb (AdwTabViewDemoWindow *self)
{
  AdwTabViewDemoWindow *window = adw_tab_view_demo_window_new ();

  gtk_window_present (GTK_WINDOW (window));

  return window->view;
}

static void
indicator_activated_cb (AdwTabViewDemoWindow *self,
                        AdwTabPage           *page)
{
  g_autoptr (GIcon) icon = NULL;
  gboolean muted;

  muted = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (page),
                                              "adw-tab-view-demo-muted"));

  g_object_set_data (G_OBJECT (page),
                     "adw-tab-view-demo-muted",
                     GINT_TO_POINTER (!muted));

  icon = get_indicator_icon (page);

  adw_tab_page_set_indicator_icon (page, icon);
}

static gboolean
extra_drag_drop_cb (AdwTabViewDemoWindow *self,
                    AdwTabPage           *page,
                    GValue               *value)
{
  adw_tab_page_set_title (page,  g_value_get_string (value));

  return GDK_EVENT_STOP;
}

static void
adw_tab_view_demo_window_dispose (GObject *object)
{
  AdwTabViewDemoWindow *self = ADW_TAB_VIEW_DEMO_WINDOW (object);

  g_clear_object (&self->tab_action_group);

  G_OBJECT_CLASS (adw_tab_view_demo_window_parent_class)->dispose (object);
}

static void
adw_tab_view_demo_window_class_init (AdwTabViewDemoWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = adw_tab_view_demo_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Adwaita1/Demo/ui/adw-tab-view-demo-window.ui");
  gtk_widget_class_bind_template_child (widget_class, AdwTabViewDemoWindow, view);
  gtk_widget_class_bind_template_child (widget_class, AdwTabViewDemoWindow, tab_bar);
  gtk_widget_class_bind_template_callback (widget_class, page_detached_cb);
  gtk_widget_class_bind_template_callback (widget_class, setup_menu_cb);
  gtk_widget_class_bind_template_callback (widget_class, create_window_cb);
  gtk_widget_class_bind_template_callback (widget_class, indicator_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, extra_drag_drop_cb);

  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_t, GDK_CONTROL_MASK, "win.tab-new", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_n, GDK_CONTROL_MASK, "win.window-new", NULL);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_w, GDK_CONTROL_MASK, "tab.close", NULL);
}

static void
adw_tab_view_demo_window_init (AdwTabViewDemoWindow *self)
{
  GActionMap *action_map;

  gtk_widget_init_template (GTK_WIDGET (self));

  action_map = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (action_map,
                                   action_entries,
                                   G_N_ELEMENTS (action_entries),
                                   self);
  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "win",
                                  G_ACTION_GROUP (action_map));

  self->tab_action_group = G_ACTION_MAP (g_simple_action_group_new ());
  g_action_map_add_action_entries (self->tab_action_group,
                                   tab_action_entries,
                                   G_N_ELEMENTS (tab_action_entries),
                                   self);

  gtk_widget_insert_action_group (GTK_WIDGET (self),
                                  "tab",
                                  G_ACTION_GROUP (self->tab_action_group));

  adw_tab_bar_setup_extra_drop_target (self->tab_bar,
                                       GDK_ACTION_COPY,
                                       (GType[1]) { G_TYPE_STRING }, 1);
}

AdwTabViewDemoWindow *
adw_tab_view_demo_window_new (void)
{
  return g_object_new (ADW_TYPE_TAB_VIEW_DEMO_WINDOW, NULL);
}

void
adw_tab_view_demo_window_prepopulate (AdwTabViewDemoWindow *self)
{
  tab_new (NULL, NULL, self);
  tab_new (NULL, NULL, self);
  tab_new (NULL, NULL, self);
}
