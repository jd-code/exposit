/* 
 * $Id: graphutils.c 520 2005-04-19 21:27:25Z jd $
 * GrapeFruit Copyright (C) 2002,2003 Cristelle Barillon & Jean-Daniel Pauget
 * a whole set of graphical utilities for SDL
 *
 * grapefruit@disjunkt.com  -  http://grapefruit.disjunkt.com/
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * you can also try the web at http://www.gnu.org/
 *
 */

/* this one is from the SDL tutorial */
/*
 *  * Set the pixel at (x, y) to the given value
 *   * NOTE: The surface must be locked before calling this!
 *    */

#include <string.h>

#include "graphutils.h"
#include "jeuchar.h"

void SDLF_putpixel (SDL_Surface *surface, int x, int y, Uint32 pixel)	    // JDJDJDJD le test vis à vis du clipping est manquant
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

void rvline (SDL_Surface *Sdest, int x1, int y1, int x2, int y2, Uint32 pixel)
{
    if (y1 > y2) {
	 rvline (Sdest, x2, y2, x1, y1, pixel);
	 return;
    } else {
	int x, y;
	int minx = Sdest->clip_rect.x,
	    miny = Sdest->clip_rect.y,
	    maxx = minx + Sdest->clip_rect.w,
	    maxy = miny + Sdest->clip_rect.h;
	double p = (x2-x1) / (double)(y2-y1);

	for (y=y1 ; y!=y2 ; y++) {
	    x = x1 + (int) ((double)(y - y1) * p);
	    if ((x>=minx) && (y>=miny) && (x<maxx) && (y<maxy))
		SDLF_putpixel (Sdest, x, y, pixel);
	}
    }
} 

void rhline (SDL_Surface *Sdest, int x1, int y1, int x2, int y2, Uint32 pixel)
{
    if (x1 > x2) {
	 rhline (Sdest, x2, y2, x1, y1, pixel);
	 return;
    } else {
	int x, y;
	int minx = Sdest->clip_rect.x,
	    miny = Sdest->clip_rect.y,
	    maxx = minx + Sdest->clip_rect.w,
	    maxy = miny + Sdest->clip_rect.h;
	double p = (y2-y1) / (double)(x2-x1);

	for (x=x1 ; x!=x2 ; x++) {
	    y = y1 + (int) ((double)(x - x1) * p);
	    if ((x>=minx) && (y>=miny) && (x<maxx) && (y<maxy))
		SDLF_putpixel (Sdest, x, y, pixel);
	}
    }
}

void line (SDL_Surface *Sdest, int x1, int y1, int x2, int y2, Uint32 pixel)
{
    int dx = (x1>x2) ? x1-x2 : x2-x1;
    int dy = (y1>y2) ? y1-y2 : y2-y1;
    if (dx > dy)
	rhline (Sdest, x2, y2, x1, y1, pixel);
    else
	rvline (Sdest, x2, y2, x1, y1, pixel);
}
    
void SDLF_putstr (SDL_Surface *surface, int x, int y, Uint32 pixel, const char *texte)
{       
    int minx, miny, maxx, maxy;

    if (texte == NULL) return;

    if ((x>surface->w) || (y>surface->h))
	return ;

    minx = surface->clip_rect.x,
    miny = surface->clip_rect.y,
    maxx = minx + surface->clip_rect.w,
    maxy = miny + surface->clip_rect.h;

    
    {	int t,g;
	int l = strlen (texte);
//	int ominx = (x<0) ? 0 : x;
//	int ominy = (y<0) ? 0 : y;
	int omaxx = x + l * 8;
	int omaxy = y + 16;

	if ((omaxx < minx) || (omaxy < miny))
	    return ;

	if ( SDL_MUSTLOCK(surface) ) {
	    if ( SDL_LockSurface(surface) < 0 ) {
		/* JDJDJDJD fprintf(stderr, "Can't lock surface: %s\n", SDL_GetError()); */
		fprintf(stderr, "Can't lock surface: \n");
		return;
	    }
	}
	
        while (*(const unsigned char *)texte != 0) {
	    for (t=0 ; t<16 ; t++)
		if (((t+y)>=miny) && ((t+y)<maxy)) {
		    for (g=0 ; g<8 ; g++)
			if (((x+g)>=minx) && ((x+g)<maxx))
			    if (HIPjeuchar[(int)(*(const unsigned char *)texte)][t] & (0x80 >> g))
				SDLF_putpixel (surface, x+g,y+t, pixel);
                }
	    texte ++;
	    x += 8;
	    if (x > maxx) break;
        }

//	SDL_UpdateRect(surface, ominx, ominy, omaxx-ominx, omaxy-ominy);
	
	if ( SDL_MUSTLOCK(surface) ) {
	    SDL_UnlockSurface(surface);
	}
    }
}

void SDLF_putstrA (SDL_Surface *surface, int x, int y, Uint32 pixel, const char *texte)
{       
    if (texte == NULL) return;

    if ((x>surface->w) || (y>surface->h))
	return ;

    {	int t,g;
	int l = strlen (texte);
//	int minx = (x<0) ? 0 : x;
//	int miny = (y<0) ? 0 : y;
	int maxx = x + l * 8;
	int maxy = y + 16;

	if ((maxx < 0) || (maxy < 0))
	    return ;

	if ( SDL_MUSTLOCK(surface) ) {
	    if ( SDL_LockSurface(surface) < 0 ) {
		/* JDJDJDJD fprintf(stderr, "Can't lock surface: %s\n", SDL_GetError()); */
		fprintf(stderr, "Can't lock surface: \n");
		return;
	    }
	}
	
        while (*(const unsigned char *)texte != 0) {
	    for (t=0 ; t<16 ; t++)
		if (((t+y)>=0) && ((t+y)<surface->h)) {
		    for (g=0 ; g<8 ; g++)
			if (((x+g)>=0) && ((x+g)<surface->w))
			    if (HIPjeucharA[(int)(*(const unsigned char *)texte)][t] & (0x80 >> g))
				SDLF_putpixel (surface, x+g,y+t, pixel);
                }
	    texte ++;
	    x += 8;
	    if (x > surface->w) break;
        }

//	SDL_UpdateRect(surface, minx, miny, maxx-minx, maxy-miny);
	
	if ( SDL_MUSTLOCK(surface) ) {
	    SDL_UnlockSurface(surface);
	}
    }
}

// #################################################################################

SDL_Surface *SDL_CreateMimicSurface(Uint32 flags, int width, int height, SDL_Surface const * from)
{
    SDL_PixelFormat *format = from->format;
    SDL_Surface *temp = SDL_CreateRGBSurface (flags, width, height,
			         format->BitsPerPixel,
			         format->Rmask, format->Gmask, format->Bmask,
				 format->Amask);
    if (format->BitsPerPixel == 8) {
	if (format->palette != NULL)
	    SDL_SetPalette (temp, SDL_LOGPAL, format->palette->colors, 0, format->palette->ncolors);
	else
	    fprintf (stderr, "SDL_CreateMimicSurface: the from->format->palette is NULL ??\n");
    }
    return temp;
}

void MultAlpha (SDL_Surface *s, Uint32 m)
{
    int i, j;
    Uint32 *q = (Uint32*) s->pixels,
	   *p;
    Uint32 Amask = s->format->Amask;
    Uint32 Anotmask = ~(s->format->Amask);
    Uint8  Ashift = s->format->Ashift;
    Uint32 a;

    for (j=0 ; j<s->h ; j++) {
	p = q;
	for (i=0 ; i<s->w ; i++) {
	    a = (*p & Amask) >> Ashift;
	    a *= m;
	    a >>= 8;
	    *p = (*p & Anotmask) | (a << Ashift);
	    p++;
	}
	q += s->pitch >> 2;
    }
}

SDL_Surface *ShadowAlpha (Uint32 flags, SDL_Surface const * from, Uint32 m)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int rmask = 0xff000000;
    int gmask = 0x00ff0000;
    int bmask = 0x0000ff00;
    int amask = 0x000000ff;
#else
    int rmask = 0x000000ff;
    int gmask = 0x0000ff00;
    int bmask = 0x00ff0000;
    int amask = 0xff000000;
#endif
 
    SDL_Surface *dest = SDL_CreateRGBSurface (flags, from->w, from->h, 32, rmask, gmask, bmask, amask);

    int i, j;
    Uint32 *qf = (Uint32*) from->pixels,
	   *qd = (Uint32*) dest->pixels,
	   *pf, *pd;

    size_t pitchf = from->pitch >> 2,
	   pitchd = dest->pitch >> 2;
	   
    Uint32 fAmask = from->format->Amask;
    // Uint32 fAnotmask = ~fAmask;
    Uint8  fAshift = from->format->Ashift;

    // Uint32 dAmask = dest->format->Amask;
    Uint32 dAnotmask = ~fAmask;
    Uint8  dAshift = dest->format->Ashift;

    // Uint32 TransBl = SDL_MapRGBA (dest->format, 0, 0, 0, 0);	// transparent black
    Uint32 OpaquBl = SDL_MapRGBA (dest->format, 0, 0, 0, m);	// opaque black

    Uint32 a;

    for (j=0 ; j<from->h ; j++) {
	pf = qf;
	pd = qd;
	for (i=0 ; i<from->w ; i++) {
	    a = (*pf & fAmask) >> fAshift;
	    a *= m;
	    a >>= 8;
	    *pd = (OpaquBl & dAnotmask) | (a << dAshift);
	    pf++;
	    pd++;
	}
	// qf += from->pitch >> 2;	// should be factorized !
	// qd += from->pitch >> 2;	// should be factorized ! and should probably be dest in place of from
	qf += pitchf,
	qd += pitchd;

    }
    return dest;
}

void BrightToAlpha (SDL_Surface *s)
{
    int i, j;
    Uint32 *q = (Uint32*) s->pixels,
	   *p;
    // Uint32 Amask = s->format->Amask;
    Uint32 Rmask = s->format->Rmask;	    // we use only the red, right now... JDJDJD
    Uint32 Anotmask = ~(s->format->Amask);
    Uint8  Ashift = s->format->Ashift;
    Uint8  Rshift = s->format->Rshift;
    Uint32 a;

    for (j=0 ; j<s->h ; j++) {
	p = q;
	for (i=0 ; i<s->w ; i++) {
	    a = (*p & Rmask) >> Rshift;
	    a = 255-a;
	    *p = (*p & Anotmask) | (a << Ashift);
	    p++;
	}
	q += s->pitch >> 2;
    }
}

void AlphaMaxBlit (SDL_Surface const * from, SDL_Surface *dest, SDL_Rect * rdst)
{
    int i, j, af, ad;

    Uint8  fAshift = from->format->Ashift;
    Uint32 fAmask = from->format->Amask;
    Uint8  dAshift = dest->format->Ashift;
    Uint32 dAmask = dest->format->Amask;
    Uint32 dNAmask = ~dest->format->Amask;

    int xfdeb = 0, xddeb, xdend, wt;
    int yfdeb = 0, yddeb, ydend, ht;

    Uint32 *qf = (Uint32*) from->pixels,
	   *qd = (Uint32*) dest->pixels,
	   *pf, *pd;

    size_t  fpi = from->pitch >> 2,
	    dpi = dest->pitch >> 2;

    xdend = rdst->x + ((from->w < rdst->w) ? from->w : rdst->w); // on prend le rectangle le moins large -> 1ere coordonee de fin
    xdend = (xdend > dest->w) ? dest->w : xdend; // on s'arrete a la fin de la destination
    xddeb = rdst->x;
    if (xddeb < 0) {	// faut-il tronquer a gauche ?
	xfdeb -= rdst->x;
	xddeb = 0;
    }
    wt = xdend - xddeb;
    
    ydend = rdst->y + ((from->h < rdst->h) ? from->h : rdst->h); // on prend le rectangle le moins large -> 1ere coordonee de fin
    ydend = (ydend > dest->h) ? dest->h : ydend; // on s'arrete a la fin de la destination
    yddeb = rdst->y;
    if (yddeb < 0) {	// faut-il tronquer en haut ?
	yfdeb -= rdst->y;
	yddeb = 0;
    }
    ht = ydend - yddeb;
    
    qd += xddeb + yddeb * dpi;
    qf += xfdeb + yfdeb * fpi;
	    
    for (j=0 ; j<ht ; j++) {
	pf = qf;
	pd = qd;
	for (i=0 ; i<wt ; i++) {
	    af = (*pf & fAmask) >> fAshift;
	    ad = (*pd & dAmask) >> dAshift;
	    ad = (ad > af) ? ad : af ;

	    *pd &= dNAmask;
	    *pd |= (ad << dAshift);

	    pf++;
	    pd++;
	}
	qf += fpi;
	qd += dpi;
    }
}

void ShadowBlit (SDL_Surface const * from, SDL_Surface *dest)
{
    int i, j;
    Uint32 *qf = (Uint32*) from->pixels,
	   *qd = (Uint32*) dest->pixels,
	   *pf, *pd;

    size_t pitchf = from->pitch >> 2,
	   pitchd = dest->pitch >> 2;
	   
    Uint32 fAmask = from->format->Amask;
    // Uint32 fAnotmask = ~fAmask;
    Uint8  fAshift = from->format->Ashift;

    // Uint32 dAmask = dest->format->Amask;
    // Uint32 dAnotmask = ~fAmask;
    // Uint8  dAshift = dest->format->Ashift;

    // Uint32 TransBl = SDL_MapRGBA (dest->format, 0, 0, 0, 0);	// transparent black

    Uint32 a;

    for (j=0 ; j<from->h ; j++) {
	pf = qf;
	pd = qd;
	for (i=0 ; i<from->w ; i++) {
	    a = (*pf & fAmask) >> fAshift;
	    if (a)		// JDJDJDJD peut-être le MAX des deux alphas serait le bon ?
		*pd = *pf;
	    pf++;
	    pd++;
	}
	// qf += from->pitch >> 2;	// should be factorized !
	// qd += dest->pitch >> 2;	// should be factorized !
	qf += pitchf,
	qd += pitchd;
    }
}

SDL_Surface * Create32bSurface (Uint32 flags, int width, int height)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
       int rmask = 0xff000000;
       int gmask = 0x00ff0000;
       int bmask = 0x0000ff00;
       int amask = 0x000000ff;
#else
       int rmask = 0x000000ff;
       int gmask = 0x0000ff00;
       int bmask = 0x00ff0000;
       int amask = 0xff000000;
#endif
    return SDL_CreateRGBSurface (flags, width, height, 32, rmask, gmask, bmask, amask);
}


