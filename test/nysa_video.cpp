/*****************************************************************************
 * nysa-video.c: memory video driver for vlc
 *****************************************************************************
 * Copyright (C) 2008 VLC authors and VideoLAN
 * Copyrgiht (C) 2010 Rémi Denis-Courmont
 *
 * Authors: Sam Hocevar <sam@zoy.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/



#include <assert.h>
#include <stdio.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>
#include "dionysus.hpp"
#include "nh_lcd_480_272.hpp"


#define MODULE_STRING "nysa-video"

#define T_CHROMA ("Chroma")
#define LT_CHROMA ("Output chroma for the memory image as a 4-character " \
                      "string, eg. \"RV24\".") 

#define T_WIDTH ("Width")
#define LT_WIDTH ("Video memory buffer width.")

#define T_HEIGHT ("Height")
#define LT_HEIGHT ("Video memory buffer height.")

#define T_PITCH ("Pitch")
#define LT_PITCH ("Video memory buffer pitch in bytes.")



static int Open(vlc_object_t *object);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_description     ("Nysa Video Output"                                       )
    set_shortname       ("Nysa Video"                                              )

    set_category        (CAT_VIDEO                                                 )
    set_subcategory     (SUBCAT_VIDEO_VOUT                                         )
    set_capability      ("vout display", 0                                         )

    add_integer         ("nysa-width", 480, T_WIDTH, LT_WIDTH, false               )
        change_private()
    add_integer         ("nysa-height", 272, T_HEIGHT, LT_HEIGHT, false            )
        change_private()
    add_integer         ("nysa-pitch", (480 * 4), T_PITCH, LT_PITCH, false               )
        change_private()
    add_string          ("nysa-chroma", "RV32", T_CHROMA, LT_CHROMA, true          )
    //add_string          ("nysa-chroma", "ARGB", T_CHROMA, LT_CHROMA, true          )
        change_private()

    set_callbacks       (Open, Close                                               )

vlc_module_end()

struct vout_display_sys_t {
  picture_pool_t  *pool;
  Dionysus        *d;
  NH_LCD_480_272  *lcd;

  unsigned        pitches[PICTURE_PLANE_MAX];
  unsigned        lines[PICTURE_PLANE_MAX];

  void (*display)(void *sys, void *id);
};

static picture_pool_t *Pool   (vout_display_t *, unsigned count);
static void            Display(vout_display_t *, picture_t *, subpicture_t *);
static int             Control (vout_display_t *, int, va_list);

static int Open(vlc_object_t *object){
  uint32_t device = 0;
  vout_display_t *vd = (vout_display_t *)object;
  vout_display_sys_t *sys;
  //Get a reference to the current format
  video_format_t fmt = vd->fmt;
  msg_Dbg(vd, "Openning");
  printf ("Openning\n");

  sys = (vout_display_sys_t *) calloc(1, sizeof(*sys));
  if (!sys){
      return VLC_EGENERIC;
  }
  vd->sys = sys;
  sys->pool = NULL;

  printf ("Openning Dionysus\n");
  sys->d = new Dionysus(true);
  printf ("New instance of dionysus, now open\n");
  sys->d->open();

  if (!sys->d->is_open()){
    printf ("Failed to open Dionysus\n");
  }
  else {
    sys->d->reset();
    sys->d->ping();
    sys->d->read_drt();
    device = sys->d->find_device(get_lcd_type(), get_nh_lcd_480_272_type());
    if (device > 0){
      printf ("\tFound an LCD Device!\n");
      sys->lcd = new NH_LCD_480_272(sys->d, device, false);
      sys->lcd->setup();
    }
  }
  //Debug Interface
  //End Debug Interface

  //Setup the format of the image
  char *chroma = var_InheritString(vd, "nysa-chroma");
  vlc_fourcc_t fcc = vlc_fourcc_GetCodecFromString(VIDEO_ES, chroma);
  const vlc_chroma_description_t *chroma_desc = NULL;
  if (fcc != 0) {
      chroma_desc = vlc_fourcc_GetChromaDescription(fcc);
      msg_Dbg(vd, "forcing chroma 0x%.8x (%4.4s)", fcc, (char*)&fcc);
      vd->fmt.i_chroma = fcc;
      printf ("\tChroma Description\n");
      printf ("\t\tPlane Count: %d\n", chroma_desc->plane_count);
      for (unsigned i = 0; i < 4; i++){
        printf ("\t\tP[%d]\n", i);
        printf ("\t\t\tWidth Ratio: %d / %d\n", chroma_desc->p[i].w.num, chroma_desc->p[i].w.den);
        printf ("\t\t\tHeight Ratio: %d / %d\n", chroma_desc->p[i].h.num, chroma_desc->p[i].h.den);
      }
      printf ("\t\tPixel Size: %d\n", chroma_desc->pixel_size);
      printf ("\t\tPixel Bits: %d\n", chroma_desc->pixel_bits);
  }
  else {
    printf ("\tFCC is NULL!\n");
  }
  free(chroma);

  fmt.i_width          = var_InheritInteger(vd, "nysa-width");
  fmt.i_height         = var_InheritInteger(vd, "nysa-height");
  fmt.i_rmask          = 0xFF << 16;
  fmt.i_gmask          = 0xFF << 8;
  fmt.i_bmask          = 0xFF << 0;
  fmt.i_x_offset       = 0;
  fmt.i_y_offset       = 0;
  fmt.i_visible_width  = fmt.i_width;
  fmt.i_visible_height = fmt.i_height;

  sys->pitches[0]      = var_InheritInteger(vd, "nysa-pitch");
  sys->lines[0]        = fmt.i_height;

  printf ("\tp_max:   %d\n", PICTURE_PLANE_MAX);
  printf ("\tWidth:   %d\n", fmt.i_width);
  printf ("\tHeight:  %d\n", fmt.i_height);
  printf ("\tPitches: %d\n", sys->pitches[0]);
  printf ("\tLines:   %d\n", sys->lines[0]);

  for (size_t i = 1; i < PICTURE_PLANE_MAX; i++){
      sys->pitches[i]  = sys->pitches[0];
      sys->lines[i]    = sys->lines[0];
  }
  vd->pool             = Pool;
  vd->prepare          = NULL;
  vd->display          = Display;
  vd->control          = Control;
  vd->manage           = NULL;

  return VLC_SUCCESS;
}
static void Close(vlc_object_t *object){
  vout_display_t *vd = (vout_display_t *)object;
  vout_display_sys_t *sys = vd->sys;
  //If we created a pool, delete it
  if (sys->pool){
    picture_pool_Delete(sys->pool);
  }
  free(sys);
}
static picture_pool_t *Pool(vout_display_t *vd, unsigned count){
  //Allow the VLC underlying controls to handle the pool of images
  vout_display_sys_t *sys = vd->sys;
  if (!sys->pool){
    sys->pool = picture_pool_NewFromFormat(&vd->fmt, count);
  }
  return sys->pool;
}
static void Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture){
  vout_display_sys_t *sys = vd->sys;
  printf ("Display Entered\n");
  //XXX: If Nysa insert
  //Check if the LCD is ready
  //If so write an image down to the LCD
  printf ("\tPicture Lines: %d\n", picture->p[0].i_lines);
  printf ("\tBytes Per Line: %d\n", picture->p[0].i_pitch);
  printf ("\tPixels Per Line: %d\n", (picture->p[0].i_pitch / 4));
  printf ("\tVisible Lines: %d\n", picture->p[0].i_visible_lines);
  printf ("\tVisible Pitch: %d\n", picture->p[0].i_visible_pitch);
  sys->lcd->dma_write(picture->p[0].p_pixels);
  printf ("\tFirst 8 Bytes:");
  for (unsigned i = 0; i < 8; i++){
    printf (" %02X", picture->p[0].p_pixels[i]);
  }
  printf ("\n");

  //write picture->p[0].p_pixels
  VLC_UNUSED(vd);
  picture_Release(picture);
  VLC_UNUSED(subpicture);
}
static int Control(vout_display_t *vd, int query, va_list args){
  //vout_display_sys_t *sys = vd->sys;
  VLC_UNUSED(vd);
  VLC_UNUSED(query);
  VLC_UNUSED(args);
  return VLC_SUCCESS;
}
