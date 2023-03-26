/*
 * Copyright (C) 2023 Christopher Davis <christopherdavis@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined(_ADWAITA_INSIDE) && !defined(ADWAITA_COMPILATION)
#error "Only <adwaita.h> can be included directly."
#endif

#include "adw-colors.h"

G_BEGIN_DECLS

/*
 * Copyright (C) 2018 Christian Hergert
 *
 * Copied from https://gist.github.com/chergert/acc7f41c4d5a45ba254188bb19764f72
 */
# define _GDK_RGBA_DECODE(c) \
  ((unsigned)((c >= 'A' && c <= 'F') ? (c-'A'+10) : \
              (c >= 'a' && c <= 'f') ? (c-'a'+10) : \
              (c >= '0' && c <= '9') ? (c-'0') : -1))
# define _GDK_RGBA_HIGH(c) (_GDK_RGBA_DECODE(c) << 4)
# define _GDK_RGBA_LOW(c) _GDK_RGBA_DECODE(c)
# define _GDK_RGBA(r,g,b,a) ((GdkRGBA){(r)/255.,(g)/255.,(b)/255.,(a)/255.})
# define _GDK_RGBA8(str) \
  _GDK_RGBA(_GDK_RGBA_HIGH(str[0]) | _GDK_RGBA_LOW(str[1]), \
            _GDK_RGBA_HIGH(str[2]) | _GDK_RGBA_LOW(str[3]), \
            _GDK_RGBA_HIGH(str[4]) | _GDK_RGBA_LOW(str[5]), \
            _GDK_RGBA_HIGH(str[6]) | _GDK_RGBA_LOW(str[7]))
# define _GDK_RGBA6(str) \
  _GDK_RGBA(_GDK_RGBA_HIGH(str[0]) | _GDK_RGBA_LOW(str[1]), \
            _GDK_RGBA_HIGH(str[2]) | _GDK_RGBA_LOW(str[3]), \
            _GDK_RGBA_HIGH(str[4]) | _GDK_RGBA_LOW(str[5]), \
            0xFF)
# define _GDK_RGBA4(str) \
  _GDK_RGBA(_GDK_RGBA_HIGH(str[0]) | _GDK_RGBA_LOW(str[0]), \
            _GDK_RGBA_HIGH(str[1]) | _GDK_RGBA_LOW(str[1]), \
            _GDK_RGBA_HIGH(str[2]) | _GDK_RGBA_LOW(str[2]), \
            _GDK_RGBA_HIGH(str[3]) | _GDK_RGBA_LOW(str[3]))
# define _GDK_RGBA3(str) \
  _GDK_RGBA(_GDK_RGBA_HIGH(str[0]) | _GDK_RGBA_LOW(str[0]), \
            _GDK_RGBA_HIGH(str[1]) | _GDK_RGBA_LOW(str[1]), \
            _GDK_RGBA_HIGH(str[2]) | _GDK_RGBA_LOW(str[2]), \
            0xFF)
# define GDK_RGBA(str) ((sizeof(str) == 9) ? _GDK_RGBA8(str) : \
                        (sizeof(str) == 7) ? _GDK_RGBA6(str) : \
                        (sizeof(str) == 5) ? _GDK_RGBA4(str) : \
                        (sizeof(str) == 4) ? _GDK_RGBA3(str) : \
                        ((GdkRGBA){0,0,0,0}))

/*
 * Adapted from https://gitlab.gnome.org/World/design/palette/-/blob/4aaa771584f3084f56502597d823c867500b90ef/src/palette.vala#L202
 * (C) Zander Brown, Tobias Bernard
 */
#define USE_DARK(r, g, b) \
(((r * 255.0) * 0.299) + ((g * 255.0) * 0.587) + ((b * 255.0) * 0.114)) > 160.0

void adw_transparent_black (float    alpha,
                            GdkRGBA *out);

AdwAccentColor adw_accent_color_nearest_from_rgba (GdkRGBA *original_color);

G_END_DECLS
