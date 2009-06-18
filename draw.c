/* libdrawing for SDL. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "draw.h"

/* Modify these parameters if you want the library to try using higher
 *    resolutions or color depths. */
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_DEPTH 16
#define NUM_LEVELS 256

#define SGN(x) ((x)>0 ? 1 : ((x)==0 ? 0:(-1)))
#define ABS(x) ((x)>0 ? (x) : (-x))

static SDL_Surface *screen; /* initialized by Draw_init */

#define FG_COLOR colors[0]
#define BG_COLOR colors[nlevels-1];

static unsigned char gamma_table[256];

int Draw_antialias_mode = 1;	/* use Wu antialiasing */
double Draw_gamma = 2.35;   /* looks good enough */

int Draw_init(SDL_Surface *s)
{
  int i;

    screen = s;
/*
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);
  screen = SDL_SetVideoMode(DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_DEPTH,
	        SDL_HWSURFACE|SDL_ANYFORMAT|SDL_DOUBLEBUF);
  if (screen == NULL) {
    fprintf(stderr, "Couldn't set %dx%dx%d bpp video mode: %s\n",
        DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_DEPTH,
        SDL_GetError());
    exit(1);
  }
*/
  /* generate gamma correction table */
  for (i=0; i<NUM_LEVELS; i++)
    gamma_table[i] = (int) (NUM_LEVELS-1)*pow((double)i/((double)(NUM_LEVELS-1)), 1.0/Draw_gamma);
/*  return(screen->format->BitsPerPixel); */
    return 0;
}

int Draw_getmaxx(void)
{
  return(screen->w-1);
}

int Draw_getmaxy(void)
{
  return(screen->h-1);
}

Uint32 Draw_mapcolor(Uint8 r, Uint8 g, Uint8 b)
{
#if 1
  return(SDL_MapRGB(screen->format, gamma_table[r], gamma_table[g], gamma_table[b]));
#else
  double rf, gf, bf, Y;
  /* YIQ gray level */
  rf = (double)r/255;
  gf = (double)g/255;
  bf = (double)b/255;
  Y = 0.299*rf + 0.587*gf + 0.114*bf;
  Y *= 255;
  return(SDL_MapRGB(screen->format, gamma_table[(int)Y], gamma_table[(int)Y],
	    gamma_table[(int)Y]));
#endif
}

void Draw_pixel(Uint32 x, Uint32 y, Uint32 color)
{
  Uint32 bpp, ofs;

  bpp = screen->format->BytesPerPixel;
  ofs = screen->pitch*y + x*bpp;

  SDL_LockSurface(screen);
  memcpy(screen->pixels + ofs, &color, bpp);
  SDL_UnlockSurface(screen);
}

/* Basic unantialiased Bresenham line algorithm */
static void bresenham_line(Uint32 x1, Uint32 y1, Uint32 x2, Uint32 y2,
	       Uint32 color)
{
  int lg_delta, sh_delta, cycle, lg_step, sh_step;

  lg_delta = x2 - x1;
  sh_delta = y2 - y1;
  lg_step = SGN(lg_delta);
  lg_delta = ABS(lg_delta);
  sh_step = SGN(sh_delta);
  sh_delta = ABS(sh_delta);
  if (sh_delta < lg_delta) {
    cycle = lg_delta >> 1;
    while (x1 != x2) {
      Draw_pixel(x1, y1, color);
      cycle += sh_delta;
      if (cycle > lg_delta) {
    cycle -= lg_delta;
    y1 += sh_step;
      }
      x1 += lg_step;
    }
    Draw_pixel(x1, y1, color);
  }
  cycle = sh_delta >> 1;
  while (y1 != y2) {
    Draw_pixel(x1, y1, color);
    cycle += lg_delta;
    if (cycle > sh_delta) {
      cycle -= sh_delta;
      x1 += lg_step;
    }
    y1 += sh_step;
  }
  Draw_pixel(x1, y1, color);
}

static void wu_line(Uint32 x0, Uint32 y0, Uint32 x1, Uint32 y1,
	       Uint32 fgc, Uint32 bgc, int nlevels, int nbits)
{
  Uint32 intshift, erracc,erradj;
  Uint32 erracctmp, wgt, wgtcompmask;
  int dx, dy, tmp, xdir, i;
  Uint32 colors[nlevels];
  SDL_Rect r;
  SDL_Color fg, bg;

  SDL_GetRGB(fgc, screen->format, &fg.r, &fg.g, &fg.b);
  SDL_GetRGB(bgc, screen->format, &bg.r, &bg.g, &bg.b);

  /* generate the colors by linear interpolation, applying gamma correction */
  for (i=0; i<nlevels; i++) {
    Uint8 r, g, b;

    r = gamma_table[fg.r - (i*(fg.r - bg.r))/(nlevels-1)];
    g = gamma_table[fg.g - (i*(fg.g - bg.g))/(nlevels-1)];
    b = gamma_table[fg.b - (i*(fg.b - bg.b))/(nlevels-1)];
    colors[i] = SDL_MapRGB(screen->format, r, g, b);
  }
  if (y0 > y1) {
    tmp = y0; y0 = y1; y1 = tmp;
    tmp = x0; x0 = x1; x1 = tmp;
  }
  /* draw the initial pixel in the foreground color */
  Draw_pixel(x0, y0, FG_COLOR);

  dx = x1 - x0;
  xdir = (dx >= 0) ? 1 : -1;
  dx = (dx >= 0) ? dx : -dx;

  /* special-case horizontal, vertical, and diagonal lines which need no
 *      weighting because they go right through the center of every pixel. */
  if ((dy = y1 - y0) == 0) {
    /* horizontal line */
    r.x = (x0 < x1) ? x0 : x1;
    r.y = y0;
    r.w = dx;
    r.h = 1;
    SDL_FillRect(screen, &r, FG_COLOR);
    return;
  }

  if (dx == 0) {
    /* vertical line */
    r.x = x0;
    r.y = y0;
    r.w = 1;
    r.h = dy;
    SDL_FillRect(screen, &r, FG_COLOR);
    return;
  }

  if (dx == dy) {
    for (; dy != 0; dy--) {
      x0 += xdir;
      y0++;
      Draw_pixel(x0, y0, FG_COLOR);
    }
    return;
  }
  /* line is not horizontal, vertical, or diagonal */
  erracc = 0;		/* err. acc. is initially zero */
  /* # of bits by which to shift erracc to get intensity level */
  intshift = 32 - nbits;
  /* mask used to flip all bits in an intensity weighting */
  wgtcompmask = nlevels - 1;
  /* x-major or y-major? */
  if (dy > dx) {
    /* y-major.  Calculate 16-bit fixed point fractional part of a pixel that
 *        X advances every time Y advances 1 pixel, truncating the result so that
 *               we won't overrun the endpoint along the X axis */
    erradj = ((Uint64)dx << 32) / (Uint64)dy;
    /* draw all pixels other than the first and last */
    while (--dy) {
      erracctmp = erracc;
      erracc += erradj;
      if (erracc <= erracctmp) {
    /* rollover in error accumulator, x coord advances */
    x0 += xdir;
      }
      y0++;	    /* y-major so always advance Y */
      /* the nbits most significant bits of erracc give us the intensity
 *   weighting for this pixel, and the complement of the weighting for
 *	 the paired pixel. */
      wgt = erracc >> intshift;
      Draw_pixel(x0, y0, colors[wgt]);
      Draw_pixel(x0+xdir, y0, colors[wgt^wgtcompmask]);
    }
    /* draw the final pixel, which is always exactly intersected by the line
 *        and so needs no weighting */
    Draw_pixel(x1, y1, FG_COLOR);
    return;
  }
  /* x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
 *      that Y advances each time X advances 1 pixel, truncating the result so
 *           that we won't overrun the endpoint along the X axis. */
  erradj = ((Uint64)dy << 32) / (Uint64)dx;
  /* draw all pixels other than the first and last */
  while (--dx) {
    erracctmp = erracc;
    erracc += erradj;
    if (erracc <= erracctmp) {
      /* accumulator turned over, advance y */
      y0++;
    }
    x0 += xdir;		/* x-major so always advance X */
    /* the nbits most significant bits of erracc give us the intensity
 *        weighting for this pixel, and the complement of the weighting for
 *               the paired pixel. */
    wgt = erracc >> intshift;
    Draw_pixel(x0, y0, colors[wgt]);
    Draw_pixel(x0, y0+1, colors[wgt^wgtcompmask]);
  }
  /* draw final pixel, always exactly intersected by the line and doesn't
 *      need to be weighted. */
  Draw_pixel(x1, y1, FG_COLOR);
}

void Draw_line(Uint32 x1, Uint32 y1, Uint32 x2, Uint32 y2,
	       Uint32 color)
{
  if (Draw_antialias_mode == 0) 
    bresenham_line(x1, y1, x2, y2, color);
  else if (Draw_antialias_mode == 1)
    wu_line(x1, y1, x2, y2, color, 0, 256, 8);
}

void Draw_screenupdate(void)
{
  SDL_UpdateRect(screen, 0, 0, 0, 0);
}

