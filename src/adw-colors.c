/*
 * Copyright (C) 2023 Christopher Davis <christopherdavis@gnome.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "adw-colors-private.h"

typedef struct {
  float hue;
  float saturation;
  float lightness;
  float alpha;
} AdwHSLA;

static void
adw_hsla_from_rgba (GdkRGBA *rgba,
                    AdwHSLA *out)
{
  float cmin = MIN (MIN (rgba->red, rgba->green), rgba->blue);
  float cmax = MAX (MAX (rgba->red, rgba->green), rgba->blue);
  float delta = cmax - cmin;

  if (delta == 0.0f)
    out->hue = 0.0f;
  else if (cmax == rgba->red)
    out->hue = fmodf ((rgba->green - rgba->blue) / delta, 6.0f);
  else if (cmax == rgba->green)
    out->hue = (rgba->blue - rgba->red) / delta + 2.0f;
  else
    out->hue = (rgba->red - rgba->green) / delta + 4.0f;

  out->hue = roundf (out->hue * 60.0f);
  if (out->hue < 0.0f)
    out->hue += 360.0f;

  out->lightness = (cmax + cmin) / 2.0f;

  out->saturation = delta == 0.0f ? 0.0f : delta / (1.0f - fabsf (2.0f * out->lightness - 1.0f));

  out->saturation *= 100.0f;
  out->lightness *= 100.0f;

  out->alpha = rgba->alpha;
}

static void
adw_hsla_to_rgba (AdwHSLA *hsla,
                  GdkRGBA *out)
{
  float hue = hsla->hue;
  float saturation = hsla->saturation / 100.0f;
  float lightness = hsla->lightness / 100.0f;
  float chroma = (1.0f - fabsf (2.0f * lightness - 1.0f)) * saturation;
  float secondary_component = chroma * (1.0f - fabsf (fmodf (hue / 60.0f, 2.0f) - 1.0f));
  float match = lightness - chroma / 2.0f;

  out->red = 0.0f;
  out->green = 0.0f;
  out->blue = 0.0f;

  if (0.0f <= hue && hue < 60.0f) {
    out->red = chroma;
    out->green = secondary_component;
    out->blue = 0.0f;
  } else if (60.0f <= hue && hue < 120.0f) {
    out->red = secondary_component;
    out->green = chroma;
    out->blue = 0.0f;
  } else if (120.0f <= hue && hue < 180.0f) {
    out->red = 0.0f;
    out->green = chroma;
    out->blue = secondary_component;
  } else if  (180.0f <= hue && hue < 240.0f) {
    out->red = 0.0f;
    out->green = secondary_component;
    out->blue = chroma;
  } else if (240.0f <= hue && hue < 300.0f) {
    out->red = secondary_component;
    out->green = 0.0f;
    out->blue = chroma;
  } else if (300.0f <= hue && hue < 360.0f) {
    out->red = chroma;
    out->green = 0.0f;
    out->blue = secondary_component;
  }

  out->red = roundf ((out->red + match) * 255.0f) / 255.0f;
  out->green = roundf ((out->green + match) * 255.0f) / 255.0f;
  out->blue = roundf ((out->blue + match) * 255.0f) / 255.0f;
  out->alpha = hsla->alpha;
}

void
adw_transparent_black (float    alpha,
                       GdkRGBA *out)
{
  out->red = 0.0f;
  out->green = 0.0f;
  out->blue = 0.0f;
  out->alpha = 1.0f - alpha;
}

void
adw_accent_color_to_rgba (AdwAccentColor color,
                          GdkRGBA       *light_rgba,
                          GdkRGBA       *dark_rgba)
{
  char *light_hex = NULL, *dark_hex = NULL;

  g_return_if_fail (color <= ADW_ACCENT_COLOR_SLATE);

  switch (color) {
  case ADW_ACCENT_COLOR_DEFAULT:
  case ADW_ACCENT_COLOR_BLUE:
    light_hex =  "#3584e4";
    dark_hex = "#3584e4";
    break;
  case ADW_ACCENT_COLOR_TEAL:
    light_hex = "#277779";
    dark_hex = "#6CB3B8";
    break;
  case ADW_ACCENT_COLOR_GREEN:
    light_hex = "#26a269";
    dark_hex = "#2ec27e";
    break;
  case ADW_ACCENT_COLOR_YELLOW:
    light_hex = "#cd9309";
    dark_hex = "#e5a50a";
    break;
  case ADW_ACCENT_COLOR_ORANGE:
    light_hex = "#e66100";
    dark_hex = "#ffa348";
    break;
  case ADW_ACCENT_COLOR_RED:
    light_hex = "#e01b24";
    dark_hex = "#c01c28";
    break;
  case ADW_ACCENT_COLOR_PINK:
    light_hex = "#D34166";
    dark_hex = "#D66FA6";
    break;
  case ADW_ACCENT_COLOR_PURPLE:
    light_hex = "#9141ac";
    dark_hex = "#c061cb";
    break;
  case ADW_ACCENT_COLOR_BROWN:
    light_hex = "#865e3c";
    dark_hex = "#b5835a";
    break;
  case ADW_ACCENT_COLOR_SLATE:
    light_hex = "#485a6c";
    dark_hex = "#758a99";
    break;
  default:
    g_assert_not_reached ();
  }

  gdk_rgba_parse (light_rgba, light_hex);
  gdk_rgba_parse (dark_rgba, dark_hex);
}

AdwAccentColor
adw_accent_color_nearest_from_rgba (GdkRGBA *original_color)
{

  return ADW_ACCENT_COLOR_DEFAULT;
}

void
adw_calculate_fg_for_rgba (GdkRGBA *rgba,
                           GdkRGBA *out)
{
  if (USE_DARK (rgba->red, rgba->green, rgba->blue)) {
    adw_transparent_black (0.2, out);
  } else {
    out->red = 1.0f;
    out->green = 1.0f;
    out->blue = 1.0f;
    out->alpha = 1.0f;
  }
}

/* TODO: This needs to be handled better. This function should take a
 * specific background color and adjust for the contrast against it.
 */
void
adw_adjust_rgba_for_text (GdkRGBA *rgba,
                          gboolean dark,
                          GdkRGBA *out)
{
  AdwHSLA hsla;

  adw_hsla_from_rgba (rgba, &hsla);

  if (dark) {
    hsla.lightness += 22.0f;
  } else {
    hsla.saturation += 1.0f;
    hsla.lightness -= 7.0f;
  }

  adw_hsla_to_rgba (&hsla, out);
}



