#ifndef GPIMAGERGBLINCLUDED
#define GPIMAGERGBLINCLUDED

#include <SDL.h>

#include <map>

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
	int **r, **g, **b, **l, **msk;
	bool isallocated;
	bool histog_valid;
	int maxr, minr,
	    maxg, ming,
	    maxb, minb,
	    maxl, minl,
	    curmsk;
	map<int,int> hr, hg, hb, hl;

static int debug;
static bool chrono;

	~ImageRGBL ();

	ImageRGBL (int w, int h);
//	    : w(w), h(h),
//	      r(NULL), g(NULL), b(NULL), l(NULL), msk(NULL),
//	      isallocated(false),
//	      histog_valid(false),
//	      maxr(-1), minr(-1), maxg(-1), ming(-1), maxb(-1), minb(-1), maxl(-1), minl(-1),
//	      curmsk (0)
//	{   
//	    allocateall ();
//	}

	ImageRGBL (SDL_Surface & surface);
//	    : w(surface.w), h(surface.h),
//	      r(NULL), g(NULL), b(NULL), l(NULL), msk(NULL),
//	      isallocated(false),
//	      histog_valid(false),
//	      maxr(-1), minr(-1), maxg(-1), ming(-1), maxb(-1), minb(-1), maxl(-1), minl(-1),
//	      curmsk (0)
//	{
//	    allocateall ();
//	    if (isallocated) {
//		int x, y;
//		for (x=0 ; x<w ; x++) for (y=0 ; y<h ; y++)
//		    getpixel (surface, x, y, r[x][y], g[x][y], b[x][y]);
//	    }
//	    setluminance();
//	}

	bool turnmaskon (int defvalue = 1);
	void setluminance (void);
	void setmax (void);

	ImageRGBL *rotate (double ang);
	ImageRGBL *doublescale (void);


	void renderhist (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int gain);

	void renderzoom (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int nblevs, int xs, int ys, int ws, int hs);

	ImageRGBL *subsample (int sub);

	void render (SDL_Surface &surface, int xoff, int yoff, int width, int height, int base, int nblevs);

	bool save_png (const char * fname);
    
	bool savecorrected (const char * fname, int base, int nblevs);

	void fasthistogramme (int step, map<int,int> &hr, map<int,int> &hg, map<int,int> &hb, map<int,int> &hl);

    void fasthistogramme (int step);

    void histogramme (void);

    void substract (ImageRGBL & image);

    void falloff_correct (ImageRGBL & falloffref);

    void substractR (int n);

    void substractG (int n);

    void substractB (int n);

    void maxminize (void);

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
