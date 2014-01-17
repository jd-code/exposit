/* 
 * Exposit Copyright (C) 2009,2010 Cristelle Barillon & Jean-Daniel Pauget
 * efficiently stacking astro pics
 *
 * exposit@disjunkt.com  -  http://exposit.disjunkt.com/
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

#ifndef GPIMAGERGBLINCLUDED
#define GPIMAGERGBLINCLUDED

#include <SDL.h>

#include <map>

#include <fitsio.h>

#include "starsmap.h"

namespace exposit {

using namespace std;

class PixelCoord {
    public:
	int x, y;
	PixelCoord (int x, int y) : x(x), y(y) {}
};

class ImageRGBL {

	void freeall (void);

	bool allocateall (void);	

	bool allocate_mask (void);

	inline int dabs (int a, int b) {
	    if (a > b)
		return a-b;
	    else
		return b-a;
	}

    public:
	int w, h;
	int maxlev;	    // 256 for png/jpg, 65536 for ppm (imported raw images)
	int **r, **g, **b, **l, **msk;
	bool isallocated;
	bool histog_valid;
	int histmask;
	int histshift;
	int maxr, minr,
	    maxg, ming,
	    maxb, minb,
	    maxl, minl,
	    curmsk;
	map<int,int> hr, hg, hb, hl;	// histogramms
	int Max;			// if histog_valid, the highest value in h*

static int debug;
static bool chrono;

	~ImageRGBL ();

	ImageRGBL (int w, int h);

	ImageRGBL (SDL_Surface & surface, int lcrop = 0, int rcrop = 0, int tcrop = 0, int bcrop = 0);

	ImageRGBL (fitsfile * fptr, int lcrop = 0, int rcrop = 0, int tcrop = 0, int bcrop = 0);

	ImageRGBL (const char * fname); // loading some xpo file

	bool save_xpo (const char * fnane);


	bool turnmaskon (int defvalue = 1);
	void setluminance (void);
	void setmax (void);

        void un_bayer (void);	// uses luminance in order to build a rgb image

	ImageRGBL *rotate (double ang);
	ImageRGBL *doublescale (void);

	int shiftnonegative (void);
	ImageRGBL *gauss (double ray, int topceil);
	ImageRGBL *gaussfaster (double ray, int topceil);
	ImageRGBL *silly (ImageRGBL &gauss, double strength, double resilient);

    void render_onehist (int base, int noise, int gain, map <int,int> &hs, int Max, int vmax,
		SDL_Surface &surface, int xoff, int yoff, int width, int height,
		Uint32 cline, Uint32 color);

	void renderhist (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int gain);

	void renderzoom (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int nblevs, double gamma, int xs, int ys, int ws, int hs);

	ImageRGBL *subsample (int sub);

	void rendermax (SDL_Surface &surface, int xoff, int yoff, int width, int height);
	void renderseuil (SDL_Surface &surface, int xoff, int yoff, int width, int height, int seuil);
	void rendernodiff (SDL_Surface &surface, int xoff, int yoff, int width, int height);
	void render (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int nblevs, double gamma);

	bool save_png (const char * fname);
    
	bool savecorrected (const char * fname, int base, int nblevs, double gamma);

	void fasthistogramme (int step, map<int,int> &hr, map<int,int> &hg, map<int,int> &hb, map<int,int> &hl);

    void fasthistogramme (int step);

    void histogramme (void);

    void substract (ImageRGBL & image);

    void remove_refnoise (ImageRGBL & refnoise, int n);

    void falloff_correct (ImageRGBL & falloffref);

    void substractR (int n);

    void substractG (int n);

    void substractB (int n);

    void maxminize (void);

    int averageabsR (void);

    long long diff (ImageRGBL &ref, int xr, int yr, int width, int height, int dx, int dy);

    long long diff (ImageRGBL &ref, int xr, int yr, int width, int height, int dx, int dy, int cut);

    long long optimaldiff (ImageRGBL &ref, int dx, int dy);


    long long find_match (ImageRGBL &ref, int x, int y, int width, int height, int &dx, int &dy, int maxdd);

    long long optimal_find_match (ImageRGBL &ref, int &dx, int &dy, int maxdd);


    void maximize (void);

    void minimize (void);

    void zero (void);

    void trunk (int n);

    void substract (int n);

    void add (ImageRGBL &image, int dx, int dy);

    void brighters (size_t nb, multimap <int, PixelCoord> &mm);


static multimap <int, PixelCoord> scpec_mmap;

    void initspecularite (int m);

    int specularite (int x, int y);

    void study_specularity (StarsMap const & sm);

    int conic_sum (int x, int y);

#ifdef ORIGINALSETTINGS
#   define SUBSBASE 4
#   define NBMAXSPOTS 2000
#   define BRIGHTLISTDIVISOR 10
#else
#   define SUBSBASE 8
#   define NBMAXSPOTS 2000
#   define BRIGHTLISTDIVISOR 20
#endif

    StarsMap * graphe_stars (void);

    static void setdebug (int d);
    static void setchrono (bool c);
};

} // namepsace exposit

#endif // GPIMAGERGBLINCLUDED
