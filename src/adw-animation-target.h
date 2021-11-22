/*
 * Copyright (C) 2021 Manuel Genovés <manuel.genoves@gmail.com>
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

#define ADW_TYPE_ANIMATION_TARGET (adw_animation_target_get_type())

ADW_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (AdwAnimationTarget, adw_animation_target, ADW, ANIMATION_TARGET, GObject)


/**
 * AdwAnimationTargetFunc:
 * @user_data: (nullable): The user data provided when creating the target
 * @value: The animation value
 *
 * Prototype for animation targets based on user callbacks.
 *
 * Since: 1.0
 */
typedef void (*AdwAnimationTargetFunc) (gpointer user_data,
                                        double   value);

#define ADW_TYPE_CALLBACK_ANIMATION_TARGET (adw_callback_animation_target_get_type())

ADW_AVAILABLE_IN_ALL
GDK_DECLARE_INTERNAL_TYPE (AdwCallbackAnimationTarget, adw_callback_animation_target, ADW, CALLBACK_ANIMATION_TARGET, AdwAnimationTarget)

ADW_AVAILABLE_IN_ALL
AdwAnimationTarget *adw_callback_animation_target_new (AdwAnimationTargetFunc callback,
                                                       gpointer               user_data,
                                                       GDestroyNotify         destroy);

G_END_DECLS