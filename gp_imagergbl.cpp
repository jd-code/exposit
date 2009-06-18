#include <math.h>

#define PNG_DEBUG 3
#include <png.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include "draw.h"
#include "graphutils.h"

#include "simplechrono.h"
#include "gp_imagergbl.h"

namespace exposit {

int ImageRGBL::debug = 0;
bool ImageRGBL::chrono = false;

    void putpixel(SDL_Surface &surface, int x, int y, int r, int g, int b)
    {
	Uint32 pixel = SDL_MapRGB(surface.format, r, g, b);

	int bpp = surface.format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface.pixels + y * surface.pitch + x * bpp;

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

    Uint32 getpixel(SDL_Surface &surface, int x, int y, int &r, int &g, int &b)
    {
	int bpp = surface.format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface.pixels + y * surface.pitch + x * bpp;
	Uint32 v = 0;

	switch(bpp) {
	case 1:
	    v = *p;
	    break;

	case 2:
	    v = *(Uint16 *)p;
	    break;

	case 3:
	    if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
		v = p[0] << 16 | p[1] << 8 | p[2];
	    else
		v = p[0] | p[1] << 8 | p[2] << 16;
	    break;

	case 4:
	    v = *(Uint32 *)p;
	    break;

	default:
	    v = 0;
	    break;       /* shouldn't happen, but avoids warnings */
	}
	Uint8 rr, gg, bb;
	SDL_GetRGB(v, surface.format, &rr, &gg, &bb);
	r = (int)rr;
	g = (int)gg;
	b = (int)bb;
	return v;
    }

    void ImageRGBL::freeall (void) {
	int i;
	if (r != NULL) { for (i=0 ; i<w ; i++) { if (r[i]!=NULL) free (r[i]); } }
	if (g != NULL) { for (i=0 ; i<w ; i++) { if (g[i]!=NULL) free (g[i]); } }
	if (b != NULL) { for (i=0 ; i<w ; i++) { if (b[i]!=NULL) free (b[i]); } }
	if (l != NULL) { for (i=0 ; i<w ; i++) { if (l[i]!=NULL) free (l[i]); } }
	if (msk != NULL) { for (i=0 ; i<w ; i++) { if (msk[i]!=NULL) free (msk[i]); } }
	if (r != NULL) free (r);
	if (g != NULL) free (g);
	if (b != NULL) free (b);
	if (l != NULL) free (l);
	if (msk != NULL) free (msk);
    }

    bool ImageRGBL::allocateall (void) {   
	if (debug) cout << "allocateall " << w << " x " << h << endl;

	for (;;)
	{	
	    r = (int **)malloc(w * sizeof(int *)); if (r == NULL) break;
	    g = (int **)malloc(w * sizeof(int *)); if (g == NULL) break;
	    b = (int **)malloc(w * sizeof(int *)); if (b == NULL) break;
	    l = (int **)malloc(w * sizeof(int *)); if (l == NULL) break;

	    int i; bool all_ok = true;

	    for (i=0 ; i<w ; i++) {
		r[i] = (int *)malloc (h * sizeof (int)); if (r[i] == NULL) { all_ok = false; break; }
		g[i] = (int *)malloc (h * sizeof (int)); if (g[i] == NULL) { all_ok = false; break; }
		b[i] = (int *)malloc (h * sizeof (int)); if (b[i] == NULL) { all_ok = false; break; }
		l[i] = (int *)malloc (h * sizeof (int)); if (l[i] == NULL) { all_ok = false; break; }
	    }
	    if (all_ok)
		isallocated = true;
	    break;
	}
	if (isallocated) {
	    if (debug) cout << "allocated." << endl;
	} else
	    freeall ();
	histog_valid = false;
	return isallocated;
    }
    
    bool ImageRGBL::allocate_mask (void) {
	if (debug) cout << "allocate_mask " << w << " x " << h << endl;
	msk = (int **)malloc(w * sizeof(int *));
	if (msk == NULL) {
	    return false;
	}

	int i; bool all_ok = true;

	for (i=0 ; i<w ; i++) {
	    msk[i] = (int *)malloc (h * sizeof (int)); if (msk[i] == NULL) { all_ok = false; break; }
	}
	if (!all_ok) {
	    for (i=0 ; i<w ; i++) {
		if (msk[i] != NULL) free (msk[i]);
	    }
	    free (msk);
	    msk = NULL;
	    return false;
	}
	if (debug) cout << "allocated." << endl;
	return true;
    }

    ImageRGBL::ImageRGBL (int w, int h)
	: w(w), h(h),
	  r(NULL), g(NULL), b(NULL), l(NULL), msk(NULL),
	  isallocated(false),
	  histog_valid(false),
	  maxr(-1), minr(-1), maxg(-1), ming(-1), maxb(-1), minb(-1), maxl(-1), minl(-1),
	  curmsk (0)
    {   
	allocateall ();
    }

    ImageRGBL::ImageRGBL (SDL_Surface & surface)
	: w(surface.w), h(surface.h),
	  r(NULL), g(NULL), b(NULL), l(NULL), msk(NULL),
	  isallocated(false),
	  histog_valid(false),
	  maxr(-1), minr(-1), maxg(-1), ming(-1), maxb(-1), minb(-1), maxl(-1), minl(-1),
	  curmsk (0)
    {
	allocateall ();
	if (isallocated) {
	    int x, y;
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
		getpixel (surface, x, y, r[x][y], g[x][y], b[x][y]);
	}
	setluminance();
    }

    bool ImageRGBL::turnmaskon (int defvalue /* = 1 */) {
	if (!allocate_mask())
	    return false;
	int x,y;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
	    msk[x][y] = defvalue;
	curmsk = defvalue;
	return true;
    }

    void ImageRGBL::setluminance (void) {
	int x,y;
	if (msk == NULL) {
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
		l[x][y] = r[x][y] + g[x][y] + b[x][y];
	} else {
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
		if (msk[x][y] == curmsk)
		    l[x][y] = r[x][y] + g[x][y] + b[x][y];
	}
    }

    void ImageRGBL::setmax (void) {
	int x=0, y=0;
	    maxr = r[x][y]; minr = r[x][y];
	    maxg = g[x][y]; ming = g[x][y];
	    maxb = b[x][y]; minb = b[x][y];
	    maxl = l[x][y]; minl = l[x][y];
       
	if (msk != NULL) {
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
		if (msk[x][y] == curmsk) {
		    maxr = r[x][y]; minr = r[x][y];
		    maxg = g[x][y]; ming = g[x][y];
		    maxb = b[x][y]; minb = b[x][y];
		    maxl = l[x][y]; minl = l[x][y];
		    break;
		}
	    }
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
		if (msk[x][y] == curmsk) {
		    maxr = max(maxr, r[x][y]); minr = min(minr, r[x][y]);
		    maxg = max(maxg, g[x][y]); ming = min(ming, g[x][y]);
		    maxb = max(maxb, b[x][y]); minb = min(minb, b[x][y]);
		    maxl = max(maxl, l[x][y]); minl = min(minl, l[x][y]);
		}
	    }
	} else {
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
		maxr = max(maxr, r[x][y]); minr = min(minr, r[x][y]);
		maxg = max(maxg, g[x][y]); ming = min(ming, g[x][y]);
		maxb = max(maxb, b[x][y]); minb = min(minb, b[x][y]);
		maxl = max(maxl, l[x][y]); minl = min(minl, l[x][y]);
	    }
	}
    }

    ImageRGBL *ImageRGBL::rotate (double ang) {
	ImageRGBL *rot = new ImageRGBL (w, h);
	if (rot == NULL) {
	    cerr << "could not allocate rotation image " << w << " x " << h << endl;
	    return NULL;
	}

	if (rot->turnmaskon(0) == false) {
	    cerr << "could not allocate rotation image mask " << w << " x " << h << endl;
	    delete (rot);
	    return NULL;
	}

	double  a1 =  cos(ang),
		b1 = -sin(ang),
		a2 =  sin(ang),
		b2 =  cos(ang);

	int x,y;
	int ww = w/2, hh = h/2;

	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
	    int xx = (int)((x-ww)*a1 + (y-hh)*b1 + ww),
		yy = (int)((x-ww)*a2 + (y-hh)*b2 + hh);
	    if ((xx>0) && (yy>0) && (xx<w) && (yy<h)) {
		rot->r[x][y] = r[xx][yy];
		rot->g[x][y] = g[xx][yy];
		rot->b[x][y] = b[xx][yy];
		rot->l[x][y] = l[xx][yy];
		rot->msk[x][y] = 1;
	    } else {
		rot->r[x][y] = 0;
		rot->g[x][y] = 0;
		rot->b[x][y] = 0;
		rot->l[x][y] = 0;
		rot->msk[x][y] = 0;
	    }
	}

	rot->curmsk = 1;
	return rot;
    }

    ImageRGBL *ImageRGBL::doublescale (void) {
	ImageRGBL *dbl = new ImageRGBL (w*2-2, h*2-2);
	if (dbl->isallocated == false) {
	    delete (dbl);
	    dbl = NULL;
	}
	if (dbl == NULL) {
	    cerr << "could not allocate doublescale image " << w*2-1 << " x " << h*2-1 << endl;
	    return NULL;
	}
	
	int x, y, xx, yy;

	for (x=0 ; x<w-1 ; x++) {
	    xx = 2*x;
	    for (y=0 ; y<h-1 ; y++) {
		yy = 2*y;

		dbl->r[xx  ][yy  ] = r[x][y];
		dbl->r[xx+1][yy  ] = (r[x][y] + r[x+1][y]) >> 1;
		dbl->r[xx  ][yy+1] = (r[x][y] + r[x][y+1]) >> 1;
		// dbl->r[xx+1][yy+1] = (r[x][y] + r[x+1][y+1]) >> 1;
		dbl->r[xx+1][yy+1] = (r[x][y] + r[x+1][y+1] + r[x+1][y] + r[x][y+1]) >> 2;

		dbl->g[xx  ][yy  ] = g[x][y];
		dbl->g[xx+1][yy  ] = (g[x][y] + g[x+1][y]) >> 1;
		dbl->g[xx  ][yy+1] = (g[x][y] + g[x][y+1]) >> 1;
		// dbl->g[xx+1][yy+1] = (g[x][y] + g[x+1][y+1]) >> 1;
		dbl->g[xx+1][yy+1] = (g[x][y] + g[x+1][y+1] + g[x+1][y] + g[x][y+1]) >> 2;

		dbl->b[xx  ][yy  ] = b[x][y];
		dbl->b[xx+1][yy  ] = (b[x][y] + b[x+1][y]) >> 1;
		dbl->b[xx  ][yy+1] = (b[x][y] + b[x][y+1]) >> 1;
		// dbl->b[xx+1][yy+1] = (b[x][y] + b[x+1][y+1]) >> 1;
		dbl->b[xx+1][yy+1] = (b[x][y] + b[x+1][y+1] + b[x+1][y] + b[x][y+1]) >> 2;
	    }
	}
	dbl->setluminance();

	return dbl;
    }

    ImageRGBL::~ImageRGBL () {
	freeall ();
    }

    void ImageRGBL::renderhist (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int gain) {
//	    map<int,int> hr, hg, hb, hl;
	map<int,int>::iterator mi, mi_max;
	int vmax;
	int rnoise, gnoise, bnoise;
	double rmax, gmax, bmax;

	fasthistogramme (1);

	vmax = 0;
	for (mi=hr.begin() ; mi!=hr.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	rnoise = mi_max->first -1;
	rmax = log(vmax);

	vmax = 0;
	for (mi=hg.begin() ; mi!=hg.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	gnoise = mi_max->first -1;
	gmax = log(vmax);

	vmax = 0;
	for (mi=hb.begin() ; mi!=hb.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	bnoise = mi_max->first -1;
	bmax = log(vmax);

	mi = hr.end(); mi--;
	int Max = mi->first;
	mi = hg.end(); mi--;
	Max = max (Max, mi->first);
	mi = hb.end(); mi--;
	Max = max (Max, mi->first);


	Draw_init (&surface);
       

	int predx = 0, predy = height/4;
	Uint32 cline = SDL_MapRGB(surface.format, 128, 128, 128);
	Uint32 color = SDL_MapRGB(surface.format, 255, 0, 0);
	Draw_line (xoff+((base+rnoise)*width)/Max, yoff+predy,      xoff+((base+rnoise)*width)/Max, yoff+predy-height/4, cline);
	Draw_line (xoff+((base+rnoise+gain)*width)/Max, yoff+predy, xoff+((base+rnoise+gain)*width)/Max, yoff+predy-height/4, cline);
	for (mi=hr.begin() ; (mi!=hr.end() && (mi->first < base+rnoise+gain)) ; mi++);
	if (mi!=hr.end()) {
	    stringstream s;
	    s << rnoise << " " << base << " " << gain << " " << ((int)((1000*log(mi->second)) / rmax))/10 << "%" << ends;
	    grapefruit::SDLF_putstr (&surface, xoff+128, yoff+predy-height/4+32, color, s.str().c_str());
	}
	for (mi=hr.begin() ; mi!=hr.end() ; mi++) {
	    int nx = (mi->first * width)/Max;
	    int ny = (int)(height/4 - (log(mi->second) * height/4)/rmax);
	    Draw_line (xoff+predx, yoff+predy, xoff+nx, yoff+ny, color);
// cout << predx << " " << predy << " " << nx << " " << ny << endl;
	    predx = nx, predy = ny;
	}

	predx = 0, predy = 2*height/4;
	color = SDL_MapRGB(surface.format, 0, 255, 0);
	Draw_line (xoff+((base+gnoise)*width)/Max, yoff+predy,      xoff+((base+gnoise)*width)/Max, yoff+predy-height/4, cline);
	Draw_line (xoff+((base+gnoise+gain)*width)/Max, yoff+predy, xoff+((base+gnoise+gain)*width)/Max, yoff+predy-height/4, cline);
	for (mi=hg.begin() ; (mi!=hg.end() && (mi->first < base+rnoise+gain)) ; mi++);
	if (mi!=hg.end()) {
	    stringstream s;
	    s << rnoise << " " << base << " " << gain << " " << ((int)((1000*log(mi->second)) / rmax))/10 << "%" << ends;
	    grapefruit::SDLF_putstr (&surface, xoff+128, yoff+predy-height/4+32, color, s.str().c_str());
	}
	for (mi=hg.begin() ; mi!=hg.end() ; mi++) {
	    int nx = (mi->first * width)/Max;
	    int ny = (int)(2*height/4 - (log(mi->second) * height/4)/gmax);
	    Draw_line (xoff+predx, yoff+predy, xoff+nx, yoff+ny, color);
// cout << predx << " " << predy << " " << nx << " " << ny << endl;
	    predx = nx, predy = ny;
	}

	predx = 0, predy = 3*height/4;
	color = SDL_MapRGB(surface.format, 0, 0, 255);
	Draw_line (xoff+((base+bnoise)*width)/Max, yoff+predy,      xoff+((base+bnoise)*width)/Max, yoff+predy-height/4, cline);
	Draw_line (xoff+((base+bnoise+gain)*width)/Max, yoff+predy, xoff+((base+bnoise+gain)*width)/Max, yoff+predy-height/4, cline);
	for (mi=hb.begin() ; (mi!=hb.end() && (mi->first < base+rnoise+gain)) ; mi++);
	if (mi!=hb.end()) {
	    stringstream s;
	    s << rnoise << " " << base << " " << gain << " " << ((int)((1000*log(mi->second)) / rmax))/10 << "%" << ends;
	    grapefruit::SDLF_putstr (&surface, xoff+128, yoff+predy-height/4+32, color, s.str().c_str());
	}
	for (mi=hb.begin() ; mi!=hb.end() ; mi++) {
	    int nx = (mi->first * width)/Max;
	    int ny = (int)(3*height/4 - (log(mi->second) * height/4)/bmax);
	    Draw_line (xoff+predx, yoff+predy, xoff+nx, yoff+ny, color);
// cout << predx << " " << predy << " " << nx << " " << ny << endl;
	    predx = nx, predy = ny;
	}

	SDL_UpdateRect(&surface, 0, 0, 0, 0);
    }


    void ImageRGBL::renderzoom (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int nblevs, int xs, int ys, int ws, int hs) {
	int x,y;
	double xr = (double)ws / width,
	       yr = (double)hs / height;

//	    map<int,int> hr, hg, hb, hl;
	map<int,int>::iterator mi, mi_max;
	int vmax;
	int rnoise, gnoise, bnoise;

	fasthistogramme (1);
	vmax = 0;
	for (mi=hr.begin() ; mi!=hr.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	rnoise = mi_max->first -1;
	vmax = 0;
	for (mi=hg.begin() ; mi!=hg.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	gnoise = mi_max->first -1;
	vmax = 0;
	for (mi=hb.begin() ; mi!=hb.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	bnoise = mi_max->first -1;
if (debug)
cout << " renderzoom: " << rnoise << " " << gnoise << " " << bnoise << endl;

	double gain = 256.0/nblevs;

	SDL_LockSurface(&surface);
	for (x=0 ; x<width ; x++) for (y=0 ; y<height ; y++) {
	    int i, j, n = 0;
	    int tr = 0, tg = 0, tb = 0;
	    int sx = (int)(x * xr + xs),
		sy = (int)(y * yr + ys);
	    for (i=0 ; i<(xr) ; i++) for (j=0 ; j<(yr) ; j++) {
		tr += max(0, r[sx+i][sy+j] - (rnoise+base));
		tg += max(0, g[sx+i][sy+j] - (gnoise+base));
		tb += max(0, b[sx+i][sy+j] - (bnoise+base));
		n ++;
	    }
	    putpixel (surface, x+xoff, y+yoff, min(255,(int)((gain * tr))/n), min(255,(int)((gain * tg)/n)), min(255,(int)((gain * tb)/n)));
	}
	putpixel (surface, 10,10, 255,255,255);
	putpixel (surface, 20,20, 255,0,255);
	putpixel (surface, 10,20, 255,0,0);
	SDL_UnlockSurface(&surface);
	SDL_UpdateRect(&surface, 0, 0, 0, 0);
    }

    ImageRGBL *ImageRGBL::subsample (int sub) {
	if (sub < 0) {
	    cerr << "could not subsample(" << sub << ") : sub not valid" << endl;
	    return NULL;
	}
	int width = w/sub,
	    height = h/sub;
	double xr = (double)w / width,
	       yr = (double)h / height;

	ImageRGBL *image = new ImageRGBL (width, height);
	if (image == NULL) {
	    cerr << "could not subsample(" << sub << ") ???" << endl;
	    return NULL;
	}

	int x, y;
	for (x=0 ; x<width ; x++) for (y=0 ; y<height ; y++) {
	    int i, j, n = 0;
	    int tr = 0, tg = 0, tb = 0;
	    int sx = (int)(x * xr),
		sy = (int)(y * yr);
	    for (i=0 ; i<(xr) ; i++) for (j=0 ; j<(yr) ; j++) {
		tr += max(0, r[sx+i][sy+j]);
		tg += max(0, g[sx+i][sy+j]);
		tb += max(0, b[sx+i][sy+j]);
		n ++;
	    }
	    image->r[x][y] = tr/n;
	    image->g[x][y] = tg/n;
	    image->b[x][y] = tb/n;
	}
	image->setluminance();

	return image;
    }

    void ImageRGBL::render (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int nblevs) {
	int x,y;
	double xr = (double)w / width,
	       yr = (double)h / height;

//	    map<int,int> hr, hg, hb, hl;
	map<int,int>::iterator mi, mi_max;
	int vmax;
	int rnoise, gnoise, bnoise;

	fasthistogramme (5);
	vmax = 0;
	for (mi=hr.begin() ; mi!=hr.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	rnoise = mi_max->first -1;
	vmax = 0;
	for (mi=hg.begin() ; mi!=hg.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	gnoise = mi_max->first -1;
	vmax = 0;
	for (mi=hb.begin() ; mi!=hb.end() ; mi++)
	    if (mi->second > vmax) { vmax = mi->second; mi_max = mi; }
	bnoise = mi_max->first -1;
if (debug)
cout << " render: " << rnoise << " " << gnoise << " " << bnoise << endl;

	double gain = 256.0/nblevs;

	SDL_LockSurface(&surface);
	for (x=0 ; x<width ; x++) for (y=0 ; y<height ; y++) {
	    int i, j, n = 0;
	    int tr = 0, tg = 0, tb = 0;
	    int sx = (int)(x * xr),
		sy = (int)(y * yr);
	    for (i=0 ; i<(xr) ; i++) for (j=0 ; j<(yr) ; j++) {
		tr += max(0, r[sx+i][sy+j] - (rnoise+base));
		tg += max(0, g[sx+i][sy+j] - (gnoise+base));
		tb += max(0, b[sx+i][sy+j] - (bnoise+base));
		n ++;
	    }
	    putpixel (surface, x+xoff, y+yoff, min(255,(int)((gain * tr)/n)), min(255,(int)((gain * tg)/n)), min(255,(int)((gain * tb)/n)));
	}
	putpixel (surface, 10,10, 255,255,255);
	putpixel (surface, 20,20, 255,0,255);
	putpixel (surface, 10,20, 255,0,0);
	SDL_UnlockSurface(&surface);
	SDL_UpdateRect(&surface, 0, 0, 0, 0);
    }

    bool ImageRGBL::save_png (const char * fname) {
	png_structp png_ptr;
	png_infop info_ptr;

	png_bytep * row_pointers = (png_bytep *) malloc (h * sizeof(png_bytep));
	if (row_pointers == NULL) {
	    cerr << "save_png : could not allocate row_pointers" << endl;
	    return false;
	}

	int y;

	for (y=0 ; y<h ; y++) {
	    row_pointers[y] = (png_bytep) malloc (w * sizeof(png_byte) * 4); 
	    int x;
	    png_bytep p = row_pointers[y];
	    if (p == NULL) {
		cerr << "save_png : could not allocate row itself" << endl;
		int i;
		for (i=0 ; i<y ; i++) free (row_pointers[i]);
		free (row_pointers);
		return false;
	    }
	    for (x=0 ; x<w ; x++) {
		*p++ = (png_byte)((r[x][y] > 255) ? 255 : r[x][y]);
		*p++ = (png_byte)((g[x][y] > 255) ? 255 : g[x][y]);
		*p++ = (png_byte)((b[x][y] > 255) ? 255 : b[x][y]);
		*p++ = 255;
	    }
	}


	/* create file */
	FILE *fp = fopen(fname, "wb");
	if (!fp) {
	    cerr << "[write_png_file] File " << fname << " could not be opened for writing" << endl;
	    for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);
	    return false;
	}

	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!png_ptr) {
	    cerr << "[write_png_file] png_create_write_struct failed" << endl;
	    for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);
	    return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
	    cerr << "[write_png_file] png_create_info_struct failed" << endl;
	    for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);
	    return false;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
	    cerr << "[write_png_file] Error during init_io" << endl;
	    for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);
	    return false;
	}

	png_init_io(png_ptr, fp);


	/* write header */
	if (setjmp(png_jmpbuf(png_ptr))) {
	    cerr << "[write_png_file] Error during writing header" << endl;
	    for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);
	    return false;
	}

	png_set_IHDR(png_ptr, info_ptr, w, h,
		     8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr))) {
	    cerr << "[write_png_file] Error during writing bytes" << endl;
	    for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);
	    return false;
	}

	png_write_image(png_ptr, row_pointers);


	/* end write */
	if (setjmp(png_jmpbuf(png_ptr))) {
	    cerr << "[write_png_file] Error during end of write" << endl;
	    for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);
	    return false;
	}

	png_write_end(png_ptr, NULL);

	/* cleanup heap allocation */
	for (y=0 ; y<h ; y++) free (row_pointers[y]); free (row_pointers);

	fclose(fp);
	return true;
    }

//    void histogramme (map<int,int> &hr, map<int,int> &hg, map<int,int> &hb, map<int,int> &hl) {
//	int x, y;
//	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
//	    hr[r[x][y]] ++;
//	    hg[g[x][y]] ++;
//	    hb[b[x][y]] ++;
//	    hl[l[x][y]] ++;
//	}
//    }

    void ImageRGBL::fasthistogramme (int step, map<int,int> &hr, map<int,int> &hg, map<int,int> &hb, map<int,int> &hl) {
	int x, y;
	hr.erase(hr.begin(), hr.end());
	hg.erase(hg.begin(), hg.end());
	hb.erase(hb.begin(), hb.end());
	hl.erase(hl.begin(), hl.end());
	if (msk == NULL) {
cout << "fasthistogramme sans curmsk" << endl;
	    for (x=0 ; x<w ; x+=step) for (y=0 ; y<h ; y+=step) {
		hr[r[x][y]] ++;
		hg[g[x][y]] ++;
		hb[b[x][y]] ++;
		hl[l[x][y]] ++;
	    }
	} else {
cout << "fasthistogramme avec curmsk = " << curmsk << endl;
	    for (x=0 ; x<w ; x+=step) for (y=0 ; y<h ; y+=step) {
		if (msk[x][y] == curmsk) {
		    hr[r[x][y]] ++;
		    hg[g[x][y]] ++;
		    hb[b[x][y]] ++;
		    hl[l[x][y]] ++;
		}
	    }
	}
    }

    void ImageRGBL::fasthistogramme (int step) {
	if (histog_valid)
	    return;
	fasthistogramme (step, hr, hg, hb, hl);
	histog_valid = true;
    }

    void ImageRGBL::histogramme (void) {
	fasthistogramme (1);
    }

    void ImageRGBL::substract (ImageRGBL & image) {
	int x, y, 
	    minw = min(w, image.w), 
	    minh = min(h, image.h);
	for (x=0 ; x<minw ; x++) for (y=0 ; y<minh ; y++) {
	    r[x][y] -= image.r[x][y];
	    g[x][y] -= image.g[x][y];
	    b[x][y] -= image.b[x][y];
	    l[x][y] -= image.l[x][y];
	}
	histog_valid = false;
    }

    void ImageRGBL::falloff_correct (ImageRGBL & falloffref) {
	int x, y, 
	    minw = min(w, falloffref.w), 
	    minh = min(h, falloffref.h);
	for (x=0 ; x<minw ; x++) for (y=0 ; y<minh ; y++) {
	    r[x][y] = (int)((falloffref.maxr/(double)falloffref.r[x][y]) * r[x][y]);
	    g[x][y] = (int)((falloffref.maxg/(double)falloffref.g[x][y]) * g[x][y]);
	    b[x][y] = (int)((falloffref.maxb/(double)falloffref.b[x][y]) * b[x][y]);
	    l[x][y] = r[x][y] + g[x][y] + b[x][y];
	}
	histog_valid = false;
    }

    void ImageRGBL::substractR (int n) {
	int x, y;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
	    if (r[x][y]>n) r[x][y] -= n; else r[x][y] = 0;
	histog_valid = false;
    }

    void ImageRGBL::substractG (int n) {
	int x, y;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
	    if (g[x][y]>n) g[x][y] -= n; else g[x][y] = 0;
	histog_valid = false;
    }

    void ImageRGBL::substractB (int n) {
	int x, y;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
	    if (b[x][y]>n) b[x][y] -= n; else b[x][y] = 0;
	histog_valid = false;
    }

    void ImageRGBL::maxminize (void) {
	map<int,int> hr, hg, hb, hl;
	map<int,int>::iterator mi, mi_max;
	int max;

	fasthistogramme (1, hr, hg, hb, hl);

	max = 0;
	for (mi=hr.begin() ; mi!=hr.end() ; mi++)
	    if (mi->second > max) { max = mi->second; mi_max = mi; }
	substractR (mi_max->first);    

	max = 0;
	for (mi=hg.begin() ; mi!=hg.end() ; mi++)
	    if (mi->second > max) { max = mi->second; mi_max = mi; }
	substractG (mi_max->first);    

	max = 0;
	for (mi=hb.begin() ; mi!=hb.end() ; mi++)
	    if (mi->second > max) { max = mi->second; mi_max = mi; }
	substractB (mi_max->first);    
	histog_valid = false;
    }


// ################# JDJDJDJDJD verifier que les deux diffs sont toujourts identiques !!!!
    int ImageRGBL::diff (ImageRGBL &ref, int xr, int yr, int width, int height, int dx, int dy) {
	int x, y, xc=xr-dx, yc=yr-dy;
////////	int x, y, xc=xr+dx, yc=yr+dy;
	int diff = 0;

	for (x=0 ; x<width ; x++) for (y=0 ; y<height ; y++)
	    diff += dabs (l[xc+x][yc+y], ref.l[xr+x][yr+y]);
//	    diff += dabs (l[xr+x][yr+y], ref.l[xc+x][yc+y]);
	return diff;
    }

// ################# JDJDJDJDJD verifier que les deux diffs sont toujourts identiques !!!!

    int ImageRGBL::diff (ImageRGBL &ref, int xr, int yr, int width, int height, int dx, int dy, int cut) {
	int x, y, xc=xr-dx, yc=yr-dy;
	int diff = 0;

	for (x=0 ; x<width ; x++) {
	    for (y=0 ; y<height ; y++) {
		diff += dabs (l[xc+x][yc+y], ref.l[xr+x][yr+y]);
//		diff += dabs (l[xr+x][yr+y], ref.l[xc+x][yc+y]);
		if (diff > cut) break;
	    }
	    if (diff > cut) break;
	}
	return diff;
    }
////////    int ImageRGBL::diff (ImageRGBL &ref, int xr, int yr, int width, int height, int dx, int dy, int cut) {
////////	int x, y, xc=xr-dx, yc=yr-dy;
////////	int diff = 0;
////////
////////	for (x=0 ; x<width ; x++) {
////////	    for (y=0 ; y<height ; y++) {
////////		diff += dabs (l[xc+x][yc+y], ref.l[xr+x][yr+y]);
//////////		diff += dabs (l[xr+x][yr+y], ref.l[xc+x][yc+y]);
////////		if (diff > cut) break;
////////	    }
////////	    if (diff > cut) break;
////////	}
////////	return diff;
////////    }



    int ImageRGBL::find_match (ImageRGBL &ref, int x, int y, int width, int height, int &dx, int &dy, int maxdd) {
	int dd;
	int min, m;
	int gdx = 0, gdy = 0;
	int odx = dx, ody = dy;

	dx = 0, dy = 0;
	min = diff (ref, x, y, width, height, dx, dy);
//	cout << "      [" << dx << "," << dy << "]" << endl;

	for (dd=1 ; dd<= maxdd ; dd++) {
	    dx = dd, dy = 0;
	    int vy = +1, vx = -1;
	    int n;
	    for (n=0; n<4*dd ; n++) {
		m = diff (ref, x, y, width, height, -odx-dx, -ody-dy, min);
		if (m < min)
		    gdx = dx, gdy = dy, min = m;
//		cout << "      [" << dx << "," << dy << "] = " << m << endl;
		dx += vx, dy += vy;
		if ((dx == dd) || (dx == -dd)) vx *= -1;
		if ((dy == dd) || (dy == -dd)) vy *= -1;
	    }
	}
	dx = odx+gdx, dy = ody+gdy;
	return min;
    }

    void ImageRGBL::maximize (void) {
	int x, y, m = 0;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
	    m = max (m, r[x][y]);
	    m = max (m, g[x][y]);
	    m = max (m, b[x][y]);
	}

	if (m != 0) {
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
		r[x][y] = (int)(((double)r[x][y] * 255) / m);
		g[x][y] = (int)(((double)g[x][y] * 255) / m);
		b[x][y] = (int)(((double)b[x][y] * 255) / m);
	    }
	}
	histog_valid = false;
    }

    void ImageRGBL::minimize (void) {
	int x, y, m = r[0][0];
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
	    m = min (m, r[x][y]);
	    m = min (m, g[x][y]);
	    m = min (m, b[x][y]);
	}

	if (m != 0) {
	    for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
		r[x][y] -= m;
		g[x][y] -= m;
		b[x][y] -= m;
	    }
	}
	histog_valid = false;
    }

    void ImageRGBL::zero (void) {
	int x, y;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
	    r[x][y] = 0;
	    g[x][y] = 0;
	    b[x][y] = 0;
	    l[x][y] = 0;
	}
	histog_valid = false;
    }

    void ImageRGBL::trunk (int n) {
	int x, y;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
	    r[x][y] = min (n, r[x][y]);
	    g[x][y] = min (n, g[x][y]);
	    b[x][y] = min (n, b[x][y]);
	}
	histog_valid = false;
    }

    void ImageRGBL::substract (int n) {
	int x, y;
	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
	    if (r[x][y]>n) r[x][y] -= n; else r[x][y] = 0;
	    if (g[x][y]>n) g[x][y] -= n; else g[x][y] = 0;
	    if (b[x][y]>n) b[x][y] -= n; else b[x][y] = 0;
	}
	histog_valid = false;
    }

    void ImageRGBL::add (ImageRGBL &image, int dx, int dy) {
	int startx = max (0, -dx),
	    starty = max (0, -dy),
	    endx = min (image.w, w-dx),
	    endy = min (image.h, h-dy),
	    x, y;

	if (msk != NULL) {
	    if (image.msk != NULL) {
		for (x=startx ; x<endx ; x++) for (y=starty ; y<endy ; y++) {
if ((x+dx < 0) || (x+dx >= w)) cerr << "x(left) oor : " << x+dx << endl;
if ((y+dy < 0) || (y+dy >= h)) cerr << "h(left) oor : " << y+dy << endl;
if ((x<0) || (x>image.w)) cerr << "x(right) oor : " << x << endl;
if ((y<0) || (y>image.h)) cerr << "y(right) oor : " << y << endl;
		    if (image.msk[x][y] > 0) {
			r[x+dx][y+dy] += image.r[x][y];
			g[x+dx][y+dy] += image.g[x][y];
			b[x+dx][y+dy] += image.b[x][y];
			l[x+dx][y+dy] += image.l[x][y];
			msk[x+dx][y+dy] += image.msk[x][y];
		    }
		}
		curmsk += image.curmsk;
	    } else {
		for (x=startx ; x<endx ; x++) for (y=starty ; y<endy ; y++) {
if ((x+dx < 0) || (x+dx >= w)) cerr << "x(left) oor : " << x+dx << endl;
if ((y+dy < 0) || (y+dy >= h)) cerr << "h(left) oor : " << y+dy << endl;
if ((x<0) || (x>image.w)) cerr << "x(right) oor : " << x << endl;
if ((y<0) || (y>image.h)) cerr << "y(right) oor : " << y << endl;
		    r[x+dx][y+dy] += image.r[x][y];
		    g[x+dx][y+dy] += image.g[x][y];
		    b[x+dx][y+dy] += image.b[x][y];
		    l[x+dx][y+dy] += image.l[x][y];
		    msk[x+dx][y+dy] ++;
		}
		curmsk ++;
	    }
	} else {
	    if (image.msk != NULL) {
		for (x=startx ; x<endx ; x++) for (y=starty ; y<endy ; y++) {
if ((x+dx < 0) || (x+dx >= w)) cerr << "x(left) oor : " << x+dx << endl;
if ((y+dy < 0) || (y+dy >= h)) cerr << "h(left) oor : " << y+dy << endl;
if ((x<0) || (x>image.w)) cerr << "x(right) oor : " << x << endl;
if ((y<0) || (y>image.h)) cerr << "y(right) oor : " << y << endl;
		    if (image.msk[x][y] > 0) {
			r[x+dx][y+dy] += image.r[x][y];
			g[x+dx][y+dy] += image.g[x][y];
			b[x+dx][y+dy] += image.b[x][y];
			l[x+dx][y+dy] += image.l[x][y];
		    }
		}
	    } else {
		for (x=startx ; x<endx ; x++) for (y=starty ; y<endy ; y++) {
if ((x+dx < 0) || (x+dx >= w)) cerr << "x(left) oor : " << x+dx << endl;
if ((y+dy < 0) || (y+dy >= h)) cerr << "h(left) oor : " << y+dy << endl;
if ((x<0) || (x>image.w)) cerr << "x(right) oor : " << x << endl;
if ((y<0) || (y>image.h)) cerr << "y(right) oor : " << y << endl;
		    r[x+dx][y+dy] += image.r[x][y];
		    g[x+dx][y+dy] += image.g[x][y];
		    b[x+dx][y+dy] += image.b[x][y];
		    l[x+dx][y+dy] += image.l[x][y];
		}
	    }
	}
	histog_valid = false;
    }

    void ImageRGBL::brighters (size_t nb, multimap <int, PixelCoord> &mm) {
	int x, y;
	int min = -1;

	if (nb <= 1)
	    return;

	for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++) {
	    if (l[x][y] > min) {
		mm.insert(pair<int, PixelCoord>(l[x][y],PixelCoord(x,y)));
		if (mm.size() > nb) {
		    mm.erase(mm.begin());
		}
		min = mm.begin()->first;
	    }
	}
    }


multimap <int, PixelCoord> ImageRGBL::scpec_mmap;

    void ImageRGBL::initspecularite (int m) {
static int spec_max = -1;
	if (m <= spec_max)
	    return;
	int x, y;
	for (x=-m-1 ; x< m+2 ; x++) for (y=-m-1 ; y<m+2 ; y++) {
	    int d = (int)sqrt(x*x + y*y);
	    if ((d < m) && (d > spec_max))
		scpec_mmap.insert(pair<int, PixelCoord>(d+1,PixelCoord(x,y)));
	}
	spec_max = m;
cout << "initspecularite : size = " << scpec_mmap.size() << endl;
    }

    int ImageRGBL::specularite (int x, int y) {
	multimap <int, PixelCoord>::iterator mi;
	int curd = 1;
	for (mi=scpec_mmap.begin() ; mi!=scpec_mmap.end() ; mi++) {
	    if ((curd != 1) && (mi->first != curd))
		break;
	    if (l[mi->second.x+x][mi->second.y+y] > 128) {
		curd = mi->first+1;
		mi = scpec_mmap.upper_bound(curd-1);
		continue;
	    }
	}
	return curd;
    }

    void ImageRGBL::study_specularity (StarsMap const & sm) {
	StarsMap::const_iterator mi;

static bool initialise = true;
	if (initialise) {
	    initspecularite (16);
	    initialise = false;
	}

	static Chrono chrono_specularity("studying image specularity"); chrono_specularity.start();
	int n=0;
	double av_ratio=0;

	for (mi=sm.begin() ; mi!=sm.end() ; mi++) {
	    int spec = specularite (mi->second.x, mi->second.y);
//	    cout << " : " << mi->second.abs_magnitude
//		 << " : " << spec
//		 << " : " << (double)((spec != 0) ? (double)mi->second.abs_magnitude / spec : -1.0)
//		 << " : " << (double)((spec != 0) ? (double)mi->second.abs_magnitude / (spec*spec) : -1.0)
//		 << endl;
	    if (spec > 4) {
		n++;
		av_ratio += mi->second.abs_magnitude / (spec*spec);
	    }
	}
	chrono_specularity.stop(); if (chrono) cout << chrono_specularity << endl;

	ios::fmtflags state = cout.flags();
	cout << fixed << "av_ratio = " << av_ratio/n << endl;
	cout.flags(state);
    }

    int ImageRGBL::conic_sum (int x, int y) {
static    bool conic_pond_init = true;
#define CONIC_S 17
#define CONIC_S2 8
static    int conic_pond [CONIC_S][CONIC_S];

	if (conic_pond_init) {
if (debug)
cout << "initialising conic_pond ..." << endl;
	    int i, j;
	    double dmax = CONIC_S2;
	    for (i=0 ; i<CONIC_S ; i++) for (j=0 ; j<CONIC_S ; j++) {
		int dx = i-CONIC_S2, dy = j-CONIC_S2;
		double d = sqrt (dx*dx + dy*dy);
		d = dmax-d;
		if (d<0) d = 0.0;

		conic_pond [i][j] = (int)((256.0*d*d*d*d)/(dmax*dmax*dmax*dmax));
		// conic_pond [i][j] = (int)(sqrt(CONIC_S2*CONIC_S2*2) - sqrt((i-CONIC_S2)*(i-CONIC_S2)+(j-CONIC_S2)*(j-CONIC_S2)));
		// conic_pond [i][j] = (int)((1024*conic_pond [i][j]*conic_pond [i][j]*conic_pond [i][j])/(sqrt(CONIC_S2*CONIC_S2*2) * sqrt(CONIC_S2*CONIC_S2*2) * sqrt(CONIC_S2*CONIC_S2*2)));
	    }
	    conic_pond_init = false;
if (debug)
cerr << "... done." << endl;
	}
	int s = 0;
	int i, j;
	x-=CONIC_S2, y-=CONIC_S2;
	for (i=0 ; i<CONIC_S ; i++) for (j=0 ; j<CONIC_S ; j++) {
	    s += l[i+x][j+y] * conic_pond [i][j];
	}
	return s;
    }

#ifdef ORIGINALSETTINGS
#   define SUBSBASE 4
#   define NBMAXSPOTS 2000
#   define BRIGHTLISTDIVISOR 10
#else
#   define SUBSBASE 8
#   define NBMAXSPOTS 2000
#   define BRIGHTLISTDIVISOR 20
#endif

    StarsMap * ImageRGBL::graphe_stars (void) {
	static Chrono chrono_subsampling("subsampling image to subsbase"); chrono_subsampling.start();
	StarsMap *mmap;
	mmap = new StarsMap (w, h);
	if (mmap == NULL)
	    return NULL;

	// on construit une image degradee pour trouver vite les points chauds
	ImageRGBL *image_r64 = this->subsample(SUBSBASE);
	if (image_r64 == NULL) {
	    delete mmap;
	    return NULL;
	}

	chrono_subsampling.stop(); if (chrono) cout << chrono_subsampling << endl;
	static Chrono chrono_hotspoting("raw hotspoting"); chrono_hotspoting.start();
	
	multimap <int, PixelCoord> mm;	// ca va servir a stocker les points chauds

	// on cherche les points chauds
	image_r64->brighters (NBMAXSPOTS, mm);

	chrono_hotspoting.stop(); if (chrono) cout << chrono_hotspoting << endl;
	static Chrono chrono_isolating("isolating hotspots"); chrono_isolating.start();

	multimap <int, PixelCoord>::iterator mmi;

	// on va remplir la liste des points les plus chauds
	StarsMap &lvstar = *mmap;

	// isolement ponctuel des points chauds et insertion dans le graphe
	for (mmi=mm.begin() ; mmi!=mm.end() ; mmi++) {
	    int i, j;
	    int x=-1, y=-1;
	    int vmax = -1, s;
// cerr << "conic_search [" << SUBSBASE*mmi->second.x << " , " << SUBSBASE*mmi->second.y << "]" << endl;
	    // si pas trop pres des bords (JDJDJDJD a ameliorer)
	    if (	(SUBSBASE*mmi->second.x > 2*(CONIC_S+SUBSBASE)) 
		 && (SUBSBASE*mmi->second.y > 2*(CONIC_S+SUBSBASE))
		 && (SUBSBASE*mmi->second.x+2*(SUBSBASE+CONIC_S) < w-SUBSBASE)
		 && (SUBSBASE*mmi->second.y+2*(SUBSBASE+CONIC_S) < h-SUBSBASE)
	       ) {
		// recherche du point le plus chaud par convolution d'un cone autour de l'endroit
		bool oldsystematicmethod = false;
		if (oldsystematicmethod) {
		    for (i=(SUBSBASE*mmi->second.x)-CONIC_S ; i<(SUBSBASE*mmi->second.x)+SUBSBASE+CONIC_S ; i++) {
			for (j=(SUBSBASE*mmi->second.y)-CONIC_S ; j<(SUBSBASE*mmi->second.y)+SUBSBASE+CONIC_S ; j++) {
    // cerr << "conic_sum("<<i<<","<<j<<")"<<endl;
			    s = conic_sum (i, j);
			    if (s > vmax) {
				vmax = s,
				x = i, y = j;
			    }
			}
		    }
		} else {
		    multimap <int, PixelCoord> lhs; // localhotspots
		    for (i=(SUBSBASE*mmi->second.x)-CONIC_S ; i<(SUBSBASE*mmi->second.x)+SUBSBASE+CONIC_S ; i++) {
			for (j=(SUBSBASE*mmi->second.y)-CONIC_S ; j<(SUBSBASE*mmi->second.y)+SUBSBASE+CONIC_S ; j++) {
			    lhs.insert(pair<int, PixelCoord>(l[i][j],PixelCoord(i,j)));
			}
		    }
		    multimap <int, PixelCoord>::reverse_iterator mmj;
		    int i,
			n = lhs.size() / BRIGHTLISTDIVISOR;
		    for (mmj=lhs.rbegin(),i=0 ; (i<n) && (mmj!=lhs.rend()) ; i++,mmj++) {
			s = conic_sum (mmj->second.x, mmj->second.y);
			if (s > vmax) {
			    vmax = s,
			    x = mmj->second.x, y = mmj->second.y;
			}
		    }
		}

		if (false) // determination du centre methode barycentre de la zone (pas terrible)
		{
		    int stot = 0, sx = 0, sy = 0;
		    for (i=x-CONIC_S ; i<x+CONIC_S ; i++) {
			for (j=y-CONIC_S ; j<y+CONIC_S ; j++) {
			    int pond = (l[i][j]*l[i][j])/(256*3);
			    stot +=   pond;
			    sx += i * pond;
			    sy += j * pond;
    // cerr << "conic_sum("<<i<<","<<j<<")"<<endl;
			}
		    }
		    x = sx / stot;
		    y = sy / stot;
		    vmax = conic_sum (x, y);
		}
// cerr << "pixel["<<x<<","<<y<<"]" << endl;

		if (x!=-1)
		    lvstar.insert (pair<int,VStar> (vmax,VStar(x,y,vmax)));
	    }
	}
	chrono_isolating.stop(); if (chrono) cout << chrono_isolating << endl;
	static Chrono chrono_unifying("unifiying hotspots list"); chrono_unifying.start();

	// on enleve les doublons potentiels en supprimant les points trop proches
	// ca elemine certainement des etoiles jumelles/triples ...
	bool oldmethod_noindex = false;
	if (oldmethod_noindex) {
	    multimap <int,VStar>::iterator li;
	    int ceil = CONIC_S2 * CONIC_S2;
	    for (li=lvstar.begin() ; li!=lvstar.end() ; li++) {
    // cerr << "  lvstar.size=" << lvstar.size() << endl ;
		multimap <int,VStar>::iterator lj;
		for (lj=li,lj++ ; lj!=lvstar.end() ; ) {
		    if (dxxdyy (li->second, lj->second) < ceil) {
			multimap <int,VStar>::iterator lk = lj;
			if (lk->first > li->first)
			    li->second = lk->second;
			lj++;
			lvstar.erase(lk);
		    } else
			lj++;

		}
	    }
	} else {
	    int ceil = CONIC_S2 * CONIC_S2;

	    lvstar.full_update_xind ();

	    StarsMap::iterator mi;
	    for (mi=lvstar.begin() ; mi!=lvstar.end() ; mi++) {
		if (mi->second.expunged) continue;
		multimap <int,VStar*>::iterator mj,
			    start  = lvstar.xind.lower_bound(mi->second.x - ceil - 1),
			    finish = lvstar.xind.lower_bound(mi->second.x + ceil + 1);
		for (mj=start ; mj!=finish ; mj++) {
		    if (mj->second->expunged) continue;
		    if (&mi->second == mj->second) continue; // ne pas se supprimer soi-meme
		    if (dxxdyy (mi->second, *(mj->second)) < ceil) {
			if (mi->second.abs_magnitude > mj->second->abs_magnitude) {
			    mj->second->expunged = true;
			} else {
			    mi->second.expunged = true;
			    break;
			}
		    }
		}
	    }
	    lvstar.full_expunge();
if (debug)
cout << "lvstar.size() = " << lvstar.size() << endl;
	}
	chrono_unifying.stop(); if (chrono) cout << chrono_unifying << endl;
	static Chrono chrono_neighbouring("finding neighbours and qualifying"); chrono_neighbouring.start();

	// lvstar.erase(lvstar.begin(),lvstar.lower_bound(lvstar.rbegin()->first/2));
#ifdef OUTPUTCINFOS
ofstream cinfo ("/tmp/stinfo1.txt");
#endif

	multimap <int,VStar>::iterator li;
	for (li=lvstar.begin() ; li!=lvstar.end() ; li++) {
	    int x = li->second.x,
		y = li->second.y;
#ifdef OUTPUTCINFOS
cinfo << "mag[" << li->first << " = star["<<x<<","<<y<<"]" << endl;
#endif

	    bool uggly_red_cross_on_final = false;
	    if (uggly_red_cross_on_final)
	    {   int i,j;
		for (i=x-20 ; i<x+20 ; i++)
		    r[i][y] = 255;
		for (j=y-20 ; j<y+20 ; j++)
		    r[x][j] = 255;
	    }
	    
	    // la recherche des voisines
	    multimap<int,VStar*> neighb;

	    // au minimum a 95% de la magnitude de l'etoile coeur
	    bool oldmethod_noindex2 = false;
	    if (oldmethod_noindex2) {
		multimap <int,VStar>::iterator  lj,
						start = lvstar.lower_bound((int)(li->first*0.95)),
						finish = lvstar.end();
						//finish = lvstar.lower_bound((int)(li->first*1.05));
		for (lj=start ; lj!=finish ; lj++) {
		    if (lj == li)
			continue;
		    int d = dxxdyy(li->second, lj->second);
		    neighb.insert (pair<int,VStar*>(d, &(lj->second)));
		    //    if ((neighb.size()<3) || (d<neighb.begin()->first)) {
		    //	if (neighb.size() >= 3) {
		    //	    map<int,VStar*>::iterator mi = neighb.end();
		    //	    mi --;
		    //	    neighb.erase(mi);
		    //	}
		    //	neighb[d] = &(*lj);
		    //    }
		}
	    } else {
		int d;
		multimap <int, VStar*>::iterator
		    lj,
		    middle = lvstar.xind.lower_bound (li->second.x);

		for (lj=middle ; (lj!=lvstar.xind.end()) && ((neighb.size() < 3) || ((lj->second->x-li->second.x) < neighb.rbegin()->first)) ; lj++) {
		    if (&li->second == lj->second) continue;
		    // we want minimum a 0.95% of current magnitude star
		    if (lj->second->abs_magnitude < 0.95*li->first) continue;

		    d = dxxdyy(li->second, *(lj->second));
		    neighb.insert (pair<int,VStar*>(d, lj->second));
		}

		for (lj=middle ; (lj!=lvstar.xind.begin()) && ((neighb.size() < 3) || ((li->second.x-lj->second->x) < neighb.rbegin()->first)) ; lj--) {
		    if (&li->second == lj->second) continue;
		    // we want minimum a 0.95% of current magnitude star
		    if (lj->second->abs_magnitude < 0.95*li->first) continue;

		    d = dxxdyy(li->second, *(lj->second));
		    neighb.insert (pair<int,VStar*>(d, lj->second));
		}





//		lj=middle;
//		int oldmax = d;
//		bool weneedthreeonly = false;
//		if (!neighb.empty())
//		    weneedthreeonly = true;
//
//		while (lj!=lvstar.xind.begin()) {
//		    if ((&li->second == lj->second) || (lj->second->abs_magnitude < 0.95*li->first)) {
//			lj--;
//			continue;
//		    }
//		    d = dxxdyy(li->second, *(lj->second));
//		    neighb.insert (pair<int,VStar*>(d, lj->second));
//		    if (weneedthreeonly && (neighb.size() >= 3)) break;
//		    if ((!weneedthreeonly) && (lj->second->x > oldmax)) break;
//		    lj--;
//		} 

	    }

	    li->second.qualify (neighb);

//	    bool drawthestuff = false;	    // JDJDJDJD
//		    if (drawthestuff) {
//			SDL_LockSurface(screen);
//	
//		    
//			Draw_init (screen);
//	
//			// Uint32 color = SDL_MapRGB(screen->format, 0, 255, 0);
//			int dcol = (int)((256*(li->first-lvstar.begin()->first))/(lvstar.rbegin()->first - lvstar.begin()->first));
//			dcol = min(dcol,255);
//			Uint32 color = SDL_MapRGB(screen->format, (dcol<128)?255-2*dcol:0, (dcol<128)?2*dcol:512-(2*dcol), (dcol<128)?0:2*(dcol-128));
//	
//			multimap<int,VStar*>::iterator mi;
//			int i = 0;
//			for (i=0,mi=neighb.begin() ; (i<3) && (mi!= neighb.end()) ; i++,mi++) {
//			    Draw_line(screen->w/2+(li->second.x*screen->w/2)/w,
//				      screen->h/2+(li->second.y*screen->h/2)/h,
//				      screen->w/2+(mi->second->x*screen->w/2)/w,
//				      screen->h/2+(mi->second->y*screen->h/2)/h,	color);
//			}
//	
//	
//			color = SDL_MapRGB(screen->format, 255, 0, 0);
//	
//			Draw_line(screen->w/2+(x*screen->w/2)/w - 4,
//				  screen->h/2+(y*screen->h/2)/h - 4,
//				  screen->w/2+(x*screen->w/2)/w + 4,
//				  screen->h/2+(y*screen->h/2)/h - 4,	color);
//	
//			Draw_line(screen->w/2+(x*screen->w/2)/w - 4,
//				  screen->h/2+(y*screen->h/2)/h + 4,
//				  screen->w/2+(x*screen->w/2)/w + 4,
//				  screen->h/2+(y*screen->h/2)/h + 4,	color);
//	
//			Draw_line(screen->w/2+(x*screen->w/2)/w - 4,
//				  screen->h/2+(y*screen->h/2)/h - 4,
//				  screen->w/2+(x*screen->w/2)/w - 4,
//				  screen->h/2+(y*screen->h/2)/h + 4,	color);
//	
//			Draw_line(screen->w/2+(x*screen->w/2)/w + 4,
//				  screen->h/2+(y*screen->h/2)/h - 4,
//				  screen->w/2+(x*screen->w/2)/w + 4,
//				  screen->h/2+(y*screen->h/2)/h + 4,	color);
//	
//	//		putpixel (*screen, screen->w/2+(x*screen->w/2)/w,
//	//				   screen->h/2+(y*screen->h/2)/h,
//	//			    0, 255, 0);
//			SDL_UnlockSurface(screen);
//			SDL_UpdateRect(screen, 0, 0, 0, 0);
//		    }
	}
	chrono_neighbouring.stop(); if (chrono) cout << chrono_neighbouring << endl;

	bool trytogetsomeoldinfosnotneededanymore = false;
	if (trytogetsomeoldinfosnotneededanymore) {
	    ofstream cinfo2 ("/tmp/stinfo2.txt");
	    // for (li=lvstar.begin() ; li!=lvstar.end() ; li++)
	    for (li=lvstar.lower_bound(lvstar.rbegin()->first/2) ; li!=lvstar.end() ; li++) {
		multimap<int,VStar*> alikes;
		multimap <int,VStar>::iterator lj,
						start = lvstar.lower_bound((int)(li->first*0.10)),
						finish = lvstar.lower_bound((int)(li->first*1.1));
		if (li->second.d0 != 0.0) {
		    for (lj=start ; lj!=finish ; lj++) {
			if ((lj != li) && (lj->second.d0 != 0.0))
			    alikes.insert(pair<int,VStar*>(li->second.distance(lj->second),&(lj->second)));
		    }
		}

		multimap<int,VStar*>::iterator mi;
		for (mi=alikes.begin() ; mi!=alikes.end() ; mi++) {
		    if (mi->second->d0 != 0.0) {
			cinfo2 << "      d = " << mi->first << endl;
		    }
		}
	    }
	}

	delete (image_r64);
	return mmap;
    }

} // namepsace exposit
