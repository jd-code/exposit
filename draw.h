/* Rafael R. Sevilla                                dido@pacific.net.ph */

#ifndef _DRAWING_H
#define _DRAWING_H

/* Header file for libdrawing-SDL */

#include <SDL/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif


/* extern int Draw_init(void); */
extern int Draw_init(SDL_Surface *s);
extern int Draw_getmaxx(void);
extern int Draw_getmaxy(void);
extern Uint32 Draw_mapcolor(Uint8 r, Uint8 g, Uint8 b);
extern void Draw_line(Uint32 x1, Uint32 y1, Uint32 x2, Uint32 y2,
	      Uint32 color);
extern void Draw_pixel(Uint32 x, Uint32 y, Uint32 color);
extern void Draw_screenupdate(void);

#ifdef __cplusplus
}
#endif

#endif

