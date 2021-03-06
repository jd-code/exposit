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

#define MAINW	1280
// #define MAINW	1024

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <string.h>

#include <regex.h>

#include <math.h>

#include <SDL.h>
#include <SDL_image.h>

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <map>

#include <ext/stdio_filebuf.h>

#include "draw.h"
#include "graphutils.h"

#define SIMPLECHRONO_STATICS
#include "simplechrono.h"

#include "vstar.h"
#include "starsmap.h"
#include "gp_imagergbl.h"
#include "interact.h"

namespace exposit {

using namespace std;

bool debug = false;
bool chrono = false;
bool doublesize = false;
bool finetune = false;		// must-we perform fine tuning ?
SDL_Surface* screen = NULL;

typedef enum {
    LI_NORMAL	= 0,
    LI_UNBAYER	= 1,
    LI_UNBAND	= 2
} LoadImageMode;

int load_image_mode = LI_NORMAL;

int lcrop = 0,
    rcrop = 0,
    tcrop = 0,
    bcrop = 0;

//	inline int max (int a, int b) {
//	    if (a>b)
//		return a;
//	    else
//		return b;
//	}
//	
//	inline int min (int a, int b) {
//	    if (a<b)
//		return a;
//	    else
//		return b;
//	}

int simplenanosleep (int ms) {
    timespec rqtp, rmtp;
    rqtp.tv_sec = 0, rqtp.tv_nsec = ms*1000000;

    return nanosleep (&rqtp, &rmtp);
}

ImageRGBL *load_imageppm (istream &fin, const string& fname, int lcrop, int rcrop, int tcrop, int bcrop) {
    char c;
    string s;
    int i;

    for (i=0 ; i<2 ; i++) {
	if (!fin.get(c))
	    break;
	s += c;
    }
    if (s != "P6") {
	cerr << "load_imageppm: missing P6 signature at begining of file '"  << fname << "'" << endl;
	return NULL;
    }

    while (fin && fin.get(c) && isspace(c)) {}
    fin.unget();
    s = "";
    while (fin && fin.get(c) && isdigit(c))
	s += c;
    fin.unget();
    int width = atoi(s.c_str());

    while (fin && fin.get(c) && isspace(c)) {}
    fin.unget();
    s = "";
    while (fin && fin.get(c) && isdigit(c))
	s += c;
    fin.unget();
    int height = atoi(s.c_str());

    while (fin && fin.get(c) && isspace(c)) {}
    fin.unget();
    s = "";
    while (fin && fin.get(c) && isdigit(c))
	s += c;
    fin.unget();
    int maxval = atoi(s.c_str());
    if ((maxval < 0) || (maxval >= 65536)) {
	cerr << "load_imageppm: '"  << fname << "' maxval was read as : " << maxval << " cannot perform further readings" << endl;
	return NULL;
    }
    int datasize = (maxval < 256) ? 1 : 2;

    if (!(fin && fin.get(c))) {
	cerr << "load_imageppm: '"  << fname << "' some premature end of file ?" << endl;
	return NULL;
    }
    if (!isspace(c)) {
	cerr << "load_imageppm: '"  << fname << "' warning : not a isspace-delimiter at end of header ?" << endl;
    }

    ImageRGBL *image = new ImageRGBL (width-lcrop-rcrop, height-tcrop-bcrop);
    int x, y;
    bool badexit = false;
    int r=0, g=0, b=0;
    for (y=0 ; y<height ; y++) {
	badexit = false;
	for (x=0 ; x<width ; x++) {
	    char c, cc;
	    if (!fin) { badexit = true; break; }
	    switch (datasize) {
		case 1:
		    fin.get(c); r = (int)(unsigned char)c;
		    fin.get(c); g = (int)(unsigned char)c;
		    fin.get(c); b = (int)(unsigned char)c;
		    break;
		case 2:
		    fin.get(c); fin.get(cc); r = 256*((int)(unsigned char)c) + (int)(unsigned char)cc;
		    fin.get(c); fin.get(cc); g = 256*((int)(unsigned char)c) + (int)(unsigned char)cc;
		    fin.get(c); fin.get(cc); b = 256*((int)(unsigned char)c) + (int)(unsigned char)cc;
		    break;
		default:
		    break;
	    }
	    if ((x>=lcrop) && (x<width-rcrop) && (y>=tcrop) && (y<height-bcrop)) {
		image->r[x-lcrop][y-tcrop] = r;
		image->g[x-lcrop][y-tcrop] = g;
		image->b[x-lcrop][y-tcrop] = b;
	    }
	}
	if (badexit) break;
    }
    if (badexit) {
	delete image;
	return NULL;
    }
    image->setluminance();
    image->maxlev = 65536;

    cout << "loaded '" << fname <<"'"
	 << " " << maxval << " maxval"
	 << endl;

    return image;
}

ImageRGBL *load_imageppm (const char * fname, int lcrop, int rcrop, int tcrop, int bcrop) {
    ifstream fin (fname);
    if (!fin) {
	int e = errno;
	cerr << "load_imageppm could not load file as ppm '" << fname <<"' : " << strerror (e) << endl;
	return NULL;
    }
    return load_imageppm (fin, fname, lcrop, rcrop, tcrop, bcrop);
}

class lowermapstringint : public map <string,int> {
  public:
    bool ispresent (const string &s) {
	string t;
	for (size_t i=0 ; i<s.size() ; i++) t += tolower (s[i]);
	if (find(t) != end()) return true;
	return false;
    }
    void insert (const string &s) {
	string t;
	for (size_t i=0 ; i<s.size() ; i++) t += tolower (s[i]);
	(*this)[t] = 1;
    }
    void insert (const char *s) {
	string t;
	for (size_t i=0 ; s[i]!=0 ; i++) t += tolower (s[i]);
	(*this)[t] = 1;
    }
};

lowermapstringint gzippies;
lowermapstringint fitsies;

// sets an initial list of files handled with libcfitsio
void init_fitsfile (void) {
    gzippies.insert ("gz");
    gzippies.insert ("zip");
    gzippies.insert ("z");
    fitsies.insert ("fit");
    fitsies.insert ("fits");
}


map <string,int> matchdcrawfiles;

// sets an initial list of files handled with dcraw
void init_dcrawfile (void) {
    const char* l[] = { "nef", "NEF",
			"cr2", "CR2",
			NULL
		      };
    int i;
    for (i=0 ; l[i] != NULL ; i++) 
	matchdcrawfiles [l[i]] = 1;

}

void adddcrawmatch (string s) {
    size_t p = 0;
    string m;
    while (p<s.size()) {
	while ((p<s.size()) && (!isspace(s[p]))) {
	    m += s[p];
	    p++;
	}
	if (m.size() > 0) {
	    cerr << "adding *." << m << " to dcraw-matched files" << endl;
	    matchdcrawfiles[m] = 1;
	}
	m = "";
	while ((p<s.size()) &&isspace(s[p])) p++;
    }
}

string dcraw_command = "dcraw -t 0 -c -4";

ImageRGBL *dcraw_treat (const char * fname) {
    string command(dcraw_command);
    command += " '";
    command += fname;
    command += "'";
    FILE *pin = popen (command.c_str(), "r");
    if (pin == NULL) {
	int e = errno;
	cerr << "piping to " << command << " went wrong : " << strerror(e) << endl;
	return NULL;
    }
    cerr << "start reading pipe from '" << command << "' ..." << endl ;

    __gnu_cxx::stdio_filebuf<char> pipeco (pin, ios_base::in);
    // basic_filebuf pipeco (pin, ios_base::in, 4096);
    istream in(&pipeco);

    ImageRGBL *image = load_imageppm (in, fname, lcrop, rcrop, tcrop, bcrop);
    pclose (pin);
    cerr << " ... done." << endl;
    return image;
}

void ImageRGBL::upsidedown (void) {
cerr << "inverting" << endl;
    int x, y;
    for (y=0 ; y<h/2 ; y++) {
	int yy = h-1-y;
	for (x=0 ; x<w ; x++) {
	    int rr = r[x][y];
	    int gg = g[x][y];
	    int bb = b[x][y];
	    int ll = l[x][y];
	    r[x][y] = r[x][yy];
	    g[x][y] = g[x][yy];
	    b[x][y] = b[x][yy];
	    l[x][y] = l[x][yy];
	    r[x][yy] = rr;
	    g[x][yy] = gg;
	    b[x][yy] = bb;
	    l[x][yy] = ll;
	}
    }
}

ImageRGBL *load_image (const char * fname, int lcrop, int rcrop, int tcrop, int bcrop) {
    if (strlen (fname) > 3) {
	if (strncmp (fname+strlen (fname)-3, "ppm", 3) == 0) {
	    ImageRGBL * pimage = load_imageppm (fname, lcrop, rcrop, tcrop, bcrop);
	    if (pimage == NULL) return NULL;
	    if (load_image_mode & LI_UNBAND ) pimage->un_BandsArtifacts();
	    if (load_image_mode & LI_UNBAYER) pimage->un_bayer();
	    return pimage;
	}
    }
    {	string s(fname);
	size_t p = s.find_last_of ('.');
	bool attemptfits = false;
	if (p != string::npos) {
	    string se = s.substr(p+1);
	    if ((p>0) && gzippies.ispresent (s.substr(p+1))) {
		size_t q = s.find_last_of ('.', p-1);
		if (q != string::npos) {
		    if (fitsies.ispresent (s.substr(q+1,p-q-1))) {
			attemptfits = true;
		    }
		}
	    } else if ((s.substr(p+1) == "fit") || (s.substr(p+1) == "fits")){
		attemptfits = true;
	    }
	}
	if (attemptfits) {
	    fitsfile * fptr;
	    int status = 0;
	    fits_open_file(&fptr, fname, READONLY, &status);
	    if (status != 0) {
		cerr << "load_image: could not open fit file " << fname << endl;
		return NULL;
	    }
	    ImageRGBL * pimage = new ImageRGBL(fptr);
	    fits_close_file (fptr, &status);
	    if (pimage == NULL) return NULL;
	    if (load_image_mode & LI_UNBAND ) pimage->un_BandsArtifacts();
	    if (load_image_mode & LI_UNBAYER) pimage->un_bayer();
	    return pimage;
	}
    }
    {	string s(fname);
	size_t p = s.find_last_of ('.');
	if (p !=string::npos) {
	    if (matchdcrawfiles.find(s.substr(p+1)) != matchdcrawfiles.end()) {
		ImageRGBL * pimage = dcraw_treat (fname);
		if (pimage == NULL) return NULL;
		if (load_image_mode & LI_UNBAND ) pimage->un_BandsArtifacts();
		if (load_image_mode & LI_UNBAYER) pimage->un_bayer();
		return pimage;
	    }
	}
    }

    SDL_Surface *surface = IMG_Load(fname);
    if (surface == NULL) {
	int e = errno;
	cerr << "(jd) could not load file '" << fname <<"' : " << strerror (e) << endl;
	return NULL;
    }
    cout << "loaded '" << fname <<"'"
	 << " " << (int)surface->format->BitsPerPixel << " bits/px"
	 << " " << (int)surface->format->BytesPerPixel << " bytes/px"
	 << endl;

    ImageRGBL * pimage = new ImageRGBL(*surface, lcrop, rcrop, tcrop, bcrop);
    SDL_FreeSurface (surface);
    if (pimage == NULL) return NULL;
    if (load_image_mode & LI_UNBAND ) pimage->un_BandsArtifacts();
    if (load_image_mode & LI_UNBAYER) pimage->un_bayer();
    return pimage;
}

double radian_to_nicedegrees (double a) {
    double r = (((int)((1800.0*a  )/M_PI)) % 3600)/10.0;
    if (r > 180)
	r -= 360;
    return r;
}

// ----------------------------------------------------------------------------------------

    ImageRGBL *ref_image = NULL;
    StarsMap empty, &ref_starmap = empty, *last_starmap = NULL;
    double last_rot = 0.0;
    int last_dx = 0, last_dy = 0;
    ImageRGBL *sum_image = NULL;

    ImageRGBL *falloff_ref = NULL;
    ImageRGBL *noise_ref = NULL;
    int nbrefnoise = 0;

    string dirname,		// the dir to be polled for files
	   regexp;		// the regexp matching files
    regex_t *cregexp = NULL;	// the compiled regexp

int xzoom = -1;
int yzoom = -1;
int wzoom = -1;
int hzoom = -1;

int try_add_pic (const char * fname, int rothint, bool no_align = false) {
    ImageRGBL *image = load_image (fname, lcrop, rcrop, tcrop, bcrop);

    int maxdt = 20;
    int width = 150;

    if (doublesize) {
	if (image != NULL)
	{	ImageRGBL *imageb = image->doublescale ();
	    delete (image);
	    image = imageb;
	    maxdt *= 2;
	    width *= 2;
	}
    }


    if (image != NULL) {

	static Chrono chrono_rendering("rendering image"); chrono_rendering.start();
	image->render (*screen, screen->w/2, screen->h/2, screen->w/2, screen->h/2, 0, image->maxlev, 1.0);
	chrono_rendering.stop(); if (chrono) cout << chrono_rendering << endl;

	StarsMap *starmap = NULL;

	if (!no_align) {
	    starmap = image->graphe_stars();
	    if (starmap == NULL) {
		cerr << "could not allocate stars-map for " << fname << ". skipped ..." << endl;
		delete (image);
		return -1;
	    }
	    image->study_specularity(*starmap);
	}

	int dx, dy;
	long long diff = 0;
	double firstda0 = 0.0;

	static Chrono chrono_vectordiffing("vector diffing"); chrono_vectordiffing.start();

	if (!no_align) {
	    double da0, da0_b;
	    diff = starmap->find_match (ref_starmap, M_PI*((double)rothint)/180.0, dx, dy, da0, da0_b);

	    cout << " try_add_pic : dv vector-choosed(0) = d[" << dx << "," << dy
		 << "] + rot[ " << setw(6) << fixed << setprecision(1) << radian_to_nicedegrees(da0)
		 << " ~ " << setw(6)                                   << radian_to_nicedegrees (da0_b) << endl;

	    firstda0 = da0_b;

//	    diff = starmap->find_match (ref_starmap, da0_b, dx, dy, da0, da0_b);
//
//	    cout << " try_add_pic : dv vector-choosed(" << firstda0 << ") = d[" << dx << "," << dy
//		 << "] + rot[ " << setw(6) << fixed << setprecision(1) << radian_to_nicedegrees(da0)
//		 << " ~ " << setw(6)                                   << radian_to_nicedegrees (da0_b) << endl;
//
//	    firstda0 = da0_b;

	    diff = starmap->find_tuned_match (ref_starmap, da0_b, dx, dy, da0, da0_b);

	    cout << " try_add_pic : dv vector-choosed(" << firstda0 << ") = d[" << dx << "," << dy
		 << "] + rot[ " << setw(6) << fixed << setprecision(1) << radian_to_nicedegrees(da0)
		 << " ~ " << setw(6)                                   << radian_to_nicedegrees (da0_b) << endl;
	    firstda0 = da0;

	    diff = starmap->find_tuned_match (ref_starmap, da0_b, dx, dy, da0, da0_b);

	    cout << " try_add_pic : dv vector-choosed(" << firstda0 << ") = d[" << dx << "," << dy
		 << "] + rot[ " << setw(6) << fixed << setprecision(1) << radian_to_nicedegrees(da0)
		 << " ~ " << setw(6)                                   << radian_to_nicedegrees (da0_b) << endl;
	    firstda0 = da0;
	}
	chrono_vectordiffing.stop(); if (chrono) cout << chrono_vectordiffing << endl;

	bool compareoldmethod = false;
	if (!no_align && compareoldmethod) {
		// int diff = image->find_match (*ref_image, image->w/3, image->h/3, width, width, dx, dy, maxdt);
		dx = 0, dy = 0;
		diff = image->find_match (*ref_image, xzoom, yzoom, wzoom, hzoom, dx, dy, maxdt);
		cout << " try_add_pic : dv pixmap-choosed = d[" << dx << "," << dy << "] = " << (100.0*((double)diff)/(width*width)) << "%" << endl;
	}

	if (finetune && !no_align) {
	    static Chrono chrono_fine_tuning("fine-tuning"); chrono_fine_tuning.start();
	    double ang;
	    double  delta = 0.2 * M_PI/180.0,
		    step = 0.01 * M_PI/180.0;
//	    double  delta = 0.05 * M_PI/180.0,
//		    step = 0.01 * M_PI/180.0;
	    int wdx,wdy;

	    int gdx = dx, gdy = dy; double gda0 = firstda0;
	    long long mindiff = -1;

	    for (ang = firstda0-delta ; ang < firstda0+delta ; ang += step) {
		ImageRGBL *rot = NULL;
		rot = image->rotate(ang);
		if (rot == NULL)
		    continue;
		wdx = dx, wdy = dy;
bool optimalmatch = true;
		if (optimalmatch==true)
		    diff = rot->optimal_find_match (*ref_image, wdx, wdy, 3);
		else
		    diff = rot->find_match (*ref_image, xzoom, yzoom, wzoom, hzoom, wdx, wdy, 3);

		if ((mindiff == -1) || (diff < mindiff)) {
		    mindiff = diff;
		    gdx = wdx, gdy = wdy, gda0 = ang;
		}
		delete (rot);
	    }
	    if (mindiff != -1)
		dx = gdx, dy = gdy, firstda0 = gda0;
	    else
		cerr << "fine tuning (weirdly) failed ???" << endl ;

	    diff = mindiff;

	    chrono_fine_tuning.stop(); if (chrono) cout << chrono_fine_tuning << endl;

cout << " try_add_pic : dv fine-tuned = d[" << dx << "," << dy
     << "] + rot[ " << fixed << setprecision(5) << ((int)((18000.0*firstda0  )/M_PI))/100.0 << "] = " << diff << endl;
	}


	if (noise_ref != NULL) {
	    static Chrono chrono_dimmingnoise("dimming noise"); chrono_dimmingnoise.start();
	    image->remove_refnoise (*noise_ref, nbrefnoise);
	    chrono_dimmingnoise.stop(); if (chrono) cout << chrono_dimmingnoise << endl;
	}

	if (falloff_ref != NULL) {
	    static Chrono chrono_falloffcorrecting("correcting falloff"); chrono_falloffcorrecting.start();
	    image->falloff_correct (*falloff_ref);
	    chrono_falloffcorrecting.stop(); if (chrono) cout << chrono_falloffcorrecting << endl;
	}

	if (no_align) { dx = 0, dy =0; firstda0 = 0.0; }

	ImageRGBL *rot = NULL;
	if ((!no_align) && (firstda0 != 0.0)) {
	    static Chrono chrono_rotating("rotating image"); chrono_rotating.start();
	    rot = image->rotate(firstda0);
	    chrono_rotating.stop(); if (chrono) cout << chrono_rotating << endl;
	}


if (debug)
cout << " try_add_pic : dv choosed = d[" << dx << "," << dy << "] = " << (100.0*((double)diff)/(width*width)) << "%" << endl;

	static Chrono chrono_finaladding("final image addition"); chrono_finaladding.start();
	if (rot == NULL)
	    sum_image->add (*image, -dx, -dy);
	else
	    sum_image->add (*rot, -dx, -dy);
	chrono_finaladding.stop(); if (chrono) cout << chrono_finaladding << endl;

	static Chrono chrono_cleanup("internal cleanup"); chrono_cleanup.start();
	if (rot != NULL)
	    delete (rot);

	if (last_starmap != NULL)
	    delete (last_starmap);
	last_starmap = starmap;
	last_rot = firstda0;
	last_dx = dx;
	last_dy = dy;
	delete (image);
	chrono_cleanup.stop(); if (chrono) cout << chrono_cleanup << endl;
	return 0;
    }

    return -1;
}


	// JDJDJDJD interact bloc was here

} // namespace exposit

using namespace exposit; 
using namespace std;

void * thread_interact (void *) {
//    debug = 1;
    if (debug) cerr << "entree dans thread_interact" << endl;

    SDL_Init(SDL_INIT_VIDEO);
//    SDL_WM_SetCaption("exposit", "exposit");
    // screen = SDL_SetVideoMode(1024, 768, 0, 0);

    while (ref_image == NULL) {
	simplenanosleep (100);
    }

    if (ref_image != NULL) {
	if (screen == NULL)
	    screen = SDL_SetVideoMode(MAINW, (ref_image->h*MAINW)/ref_image->w, 0, 0);
    }

    if (debug) cerr << "sortie de thread_interact imminente" << endl;

    return NULL;
}

int main (int nb, char ** cmde) {


cerr << "sizeof(int) = " << sizeof(int) << endl << endl;

    pthread_t pth_interact;

    if (pthread_create( &pth_interact, NULL, thread_interact, (void*) NULL) != 0) {
	int e = errno;
	cerr << "could not create interaction thread : " << strerror (e) << endl;
    }

    init_dcrawfile (); // sets an initial list of files handled with dcraw
    init_fitsfile (); // sets the initial list of fits file matches

    int i;
    int nbimage = 0;
    int rothint = 0;
    bool no_align = false;

    for (i=1 ; i<nb ; i++) {
	if (cmde[i][0] != '-') {
	    if (ref_image == NULL) {
		if (nbimage != 0) cerr << "warning, the reference image in use isn't the first in the list" << endl;

		ref_image = load_image (cmde[i], lcrop, rcrop, tcrop, bcrop);

		if (doublesize) {
		    if (ref_image != NULL)
		    {	ImageRGBL *imageb = ref_image->doublescale ();
			delete (ref_image);
			ref_image = imageb;
		// ref_image->save_png ("ref.png");
		    }
		}

		if (ref_image != NULL) {
		    
		    StarsMap * mmap = ref_image->graphe_stars();
		    if (mmap == NULL) {
			cerr << "could not allocate reference stars-map : bailing out ..." << endl;
			return -1;
		    }
		    ref_starmap = *mmap;
cout << "reference star map :" << ref_starmap.size () << " stars." << endl;

		    cout << "reference image is '" << cmde[i] << "'" << endl;
		    if (sum_image == NULL) {
			sum_image = new ImageRGBL (ref_image->w, ref_image->h);
			if (sum_image == NULL) {
			    cerr << "could not allocate sum_image : bailing out ..." << endl;
			    return -1;
			}
			sum_image->zero();
			sum_image->turnmaskon(0);
		    }
		}
	    }

	    if (try_add_pic (cmde[i], rothint, no_align) == 0) {
		nbimage ++;
		if (screen != NULL) {
		    if (!interact (nbimage, interactfly, no_align)) {
			// let's exit
			break;
		    }
		}
	    }
	} else if (strncmp ("-noalign", cmde[i], 8) == 0) {
	    no_align = true;
	} else if (strncmp ("-rot=", cmde[i], 5) == 0) {
	    rothint = atoi (cmde[i]+5);
	} else if (strcmp ("-load-unband", cmde[i]) == 0) {
	    load_image_mode |= LI_UNBAND;
	} else if (strcmp ("-load-unbayer", cmde[i]) == 0) {
	    load_image_mode |= LI_UNBAYER;
	} else if (strcmp ("-load-normal", cmde[i]) == 0) {
	    load_image_mode = LI_NORMAL;
	} else if (strncmp ("-readxpo=", cmde[i], 9) == 0) {
cerr << "loading " << cmde[i]+9 << " ..." << endl;
	    sum_image = new ImageRGBL (cmde[i]+9);
	    if ((sum_image == NULL) || (!sum_image->isallocated)) {
		cerr << "error at allocation of ImageRGBL ?" << endl;
	    } else {
		if (screen == NULL) {
		    screen = SDL_SetVideoMode(MAINW, (sum_image->h*MAINW)/sum_image->w, 0, 0);
		}
	    }

		    
	    cerr << "...done." << endl;
	    nbimage = 1;
	} else if (strncmp ("-hdrtest", cmde[i], 8) == 0) {
	    // JDJDJDJD  this is about trying to make some HDR ?????
	    cerr << "entering gauss" << endl;
	    ImageRGBL *gauss = sum_image->gaussfaster (35, 257 * sum_image->curmsk);
	    // ImageRGBL *gauss = sum_image->gauss (30, 257 * sum_image->curmsk);
	    // sum_image = gauss;
	    cerr << "entering silly" << endl;
	    ImageRGBL *silly = sum_image->silly (*gauss, 6.0, 1.0);
// int mini = silly->shiftnonegative();
// cerr << "shiftnonegative = " << mini << endl;
	    silly->fasthistogramme (5);
	    sum_image = silly;
	    cerr << "entering setluminance" << endl;
	    sum_image->setluminance();
	    cerr << "done..:" << endl;
	} else if (strncmp ("-crop=", cmde[i], 6) == 0) {
	    string s (cmde[i]+6);
	    size_t p = 0, q = 0;
	    q = s.find(',', p);
	    if (q == string::npos)
		lcrop = atoi (s.substr (p).c_str());
	    else {
		lcrop = atoi (s.substr (p,q-p).c_str());
		p = q + 1;	
	    }

	    if (q != string::npos) {
		q = s.find(',', p);
		if (q == string::npos)
		    rcrop = atoi (s.substr (p).c_str());
		else {
		    rcrop = atoi (s.substr (p,q-p).c_str());
		    p = q + 1;	
		}
	    }

	    if (q != string::npos) {
		q = s.find(',', p);
		if (q == string::npos)
		    tcrop = atoi (s.substr (p).c_str());
		else {
		    tcrop = atoi (s.substr (p,q-p).c_str());
		    p = q + 1;	
		}
	    }

	    if (q != string::npos) {
		q = s.find(',', p);
		if (q == string::npos)
		    bcrop = atoi (s.substr (p).c_str());
		else {
		    bcrop = atoi (s.substr (p,q-p).c_str());
		    p = q + 1;	
		}
	    }
	    cerr << "crop = [" << lcrop << "," << rcrop << "," << tcrop << "," << bcrop << "]" << endl;
	} else if (strncmp ("-debug=", cmde[i], 7) == 0) {
	    debug = atoi (cmde[i]+7);
	    ImageRGBL::setdebug (debug);
	    StarsMap::setdebug (debug);
	} else if (strncmp ("-dcraw=", cmde[i], 7) == 0) {
	    dcraw_command = cmde[i]+7;
	} else if (strncmp ("-adddcrawmatch=",cmde[i], 15) == 0) {
	    adddcrawmatch (cmde[i]+15);
	} else if (strncmp ("-doublescale", cmde[i], 12) == 0) {
	    doublesize = true;
	} else if (strncmp ("-noise=", cmde[i], 7) == 0) {
	    ImageRGBL *im = load_image (cmde[i] + 7, lcrop, rcrop, tcrop, bcrop);
	    if (im == NULL) {
		cerr << "could not load noise-reference image : '" << (cmde[i] + 7) << "'" << endl;
	    } else {
		if (noise_ref == NULL) {
		    noise_ref = new ImageRGBL (im->w, im->h);
		    if (noise_ref == NULL) {
			cerr << "could not allocate noise_ref image : bailing out ..." << endl;
			return -1;
		    }
		    noise_ref->zero();
		    if (doublesize) {
			if (noise_ref != NULL) {
			    ImageRGBL *imageb = noise_ref->doublescale ();
			    delete (noise_ref);
			    noise_ref = imageb;
			}
		    }
		}
		if (sum_image == NULL) {
		    sum_image = new ImageRGBL (noise_ref->w, noise_ref->h);
		    if (sum_image == NULL) {
			cerr << "could not allocate sum_image : bailing out ..." << endl;
			return -1;
		    }
		    sum_image->zero();
		    sum_image->turnmaskon(0);
		}
		if (doublesize) {
		    if (im != NULL) {
			ImageRGBL *imageb = im->doublescale ();
			delete (im);
			im = imageb;
		    }
		}
		noise_ref->add (*im, 0, 0);
		noise_ref->setluminance();
		noise_ref->setmax();
		nbrefnoise ++;
		delete (im);
	    }
	} else if (strncmp ("-falloff=", cmde[i], 9) == 0) {
	    ImageRGBL *tempfall = load_image (cmde[i] + 9, lcrop, rcrop, tcrop, bcrop);
	    if (tempfall == NULL)
		cerr << "could not load temp-fall-off image : '" << (cmde[i] + 7) << "'" << endl;
	    else {
		if (falloff_ref == NULL) {
		    falloff_ref = new ImageRGBL (tempfall->w, tempfall->h);
		    if (falloff_ref == NULL) {
			cerr << "could not allocate falloff_ref : bailing out ..." << endl;
			return -1;
		    }
		    falloff_ref->zero();
		    if (doublesize) {
			if (falloff_ref != NULL)
			{	ImageRGBL *imageb = falloff_ref->doublescale ();
			    delete (falloff_ref);
			    falloff_ref = imageb;
		    // ref_image->save_png ("ref.png");
			}
		    }
		}
		if (sum_image == NULL) {
		    sum_image = new ImageRGBL (falloff_ref->w, falloff_ref->h);
		    if (sum_image == NULL) {
			cerr << "could not allocate sum_image : bailing out ..." << endl;
			return -1;
		    }
		    sum_image->zero();
		    sum_image->turnmaskon(0);
		}
		if (doublesize) {
		    if (tempfall != NULL)
		    {	ImageRGBL *imageb = tempfall->doublescale ();
			delete (tempfall);
			tempfall = imageb;
		// ref_image->save_png ("ref.png");
		    }
		}
		falloff_ref->add (*tempfall, 0 ,0);
		falloff_ref->setluminance();
		falloff_ref->setmax();
cerr << "falloff maxl=" << falloff_ref->maxl << " minl=" << falloff_ref->minl << endl;
		delete (tempfall);
	    }
	} else if (strncmp ("-finetune", cmde[i], 9) == 0) {
	    finetune = true;
	} else if (strncmp ("-watch=", cmde[i], 7) == 0) {
	    dirname = cmde[i]+7;
	    regexp = "";
	    size_t pos = dirname.rfind("/");
	    if (pos != string::npos) {
		regexp = dirname.substr(pos + 1);
		dirname = dirname.substr(0, pos);
	    }
	    if (regexp.size() == 0)
	    {
		regexp = ".*\\.jpg";
		cerr << "no regexp supplied, applying default : \"" << regexp << "\"" << endl;
	    }
if (debug) {
cout << "dirname = \"" << dirname << "\"" << endl;
cout << "regexp = \"" << regexp << "\"" << endl;
}
	    cregexp = (regex_t *) malloc(sizeof(regex_t));
	    if (cregexp == NULL) {
		int e = errno;
		cerr << "could not malloc for regexp : " << strerror (e) << endl;
		continue;
	    }
	    memset(cregexp, 0, sizeof(regex_t));

	    int err_no = 0;
	    if ((err_no = regcomp (cregexp, regexp.c_str(), REG_NOSUB)) !=0 ) { /* Compile the regex */
		size_t length = regerror (err_no, cregexp, NULL, 0);; 
		char *buffer = (char *) malloc(length);;
		if (buffer == NULL) {
		    cerr << "regexp error and could not even malloc for error report !" << endl;
		    continue;
		}
		regerror (err_no, cregexp, buffer, length);
		cerr << "regexp error : \"" << regexp << "\" : " << buffer << endl;

		free (buffer);
		regfree (cregexp);
		continue;
	    } 

	    interact (nbimage, true, no_align, true);

	    regfree (cregexp);
	}
    }

    

    interact (nbimage, true, no_align);

//    sum_image->minimize ();
//    sum_image->substract (128);
    if (sum_image != NULL) {
	sum_image->maxminize ();
	sum_image->trunk (gain);
	sum_image->maximize ();
	sum_image->save_png ("test.png");
    }

    SDL_Quit();
    return 0;
}

int oldmain (int nb, char ** cmde) {

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("exposit", "exposit");
    // screen = SDL_SetVideoMode(1024, 768, 0, 0);

    int i;
    int nbimage = 0;
    int rothint = 0;
    bool no_align = false;
    for (i=1 ; i<nb ; i++) {
	if (cmde[i][0] != '-') {
	    if (ref_image == NULL) {
		if (nbimage != 0)
		    cerr << "warning, the reference image in use isn't the first in the list" << endl;
		ref_image = load_image (cmde[i], lcrop, rcrop, tcrop, bcrop);
if (doublesize) {
    if (ref_image != NULL)
    {	ImageRGBL *imageb = ref_image->doublescale ();
	delete (ref_image);
	ref_image = imageb;
// ref_image->save_png ("ref.png");
    }
}

		if (ref_image != NULL) {
		    if (screen == NULL) {
			screen = SDL_SetVideoMode(MAINW, (ref_image->h*MAINW)/ref_image->w, 0, 0);
		    }
		    
		    StarsMap * mmap = ref_image->graphe_stars();
		    if (mmap == NULL) {
			cerr << "could not allocate reference stars-map : bailing out ..." << endl;
			return -1;
		    }
		    ref_starmap = *mmap;
cout << "reference star map :" << ref_starmap.size () << " stars." << endl;

		    cout << "reference image is '" << cmde[i] << "'" << endl;
		    if (sum_image == NULL) {
			sum_image = new ImageRGBL (ref_image->w, ref_image->h);
			if (sum_image == NULL) {
			    cerr << "could not allocate sum_image : bailing out ..." << endl;
			    return -1;
			}
			sum_image->zero();
			sum_image->turnmaskon(0);
		    }
		}
	    }

	    if (try_add_pic (cmde[i], rothint, no_align) == 0) {
		nbimage ++;
		if (screen != NULL) {
		    if (!interact (nbimage, interactfly, no_align)) {
			// let's exit
			break;
		    }
		}
	    }
	} else if (strncmp ("-debug=", cmde[i], 7) == 0) {
	    debug = atoi (cmde[i]+7);
	    ImageRGBL::setdebug (debug);
	    StarsMap::setdebug (debug);
	} else if (strncmp ("-doublescale", cmde[i], 12) == 0) {
	    doublesize = true;
	} else if (strncmp ("-noise=", cmde[i], 7) == 0) {
	    ImageRGBL *im = load_image (cmde[i] + 7, lcrop, rcrop, tcrop, bcrop);
	    if (im == NULL) {
		cerr << "could not load noise-reference image : '" << (cmde[i] + 7) << "'" << endl;
	    } else {
if (doublesize) {
    if (im != NULL)
    {	ImageRGBL *imageb = im->doublescale ();
	delete (im);
	im = imageb;
// ref_image->save_png ("ref.png");
    }
}
	    }
	} else if (strncmp ("-falloff=", cmde[i], 9) == 0) {
	    falloff_ref = load_image (cmde[i] + 9, lcrop, rcrop, tcrop, bcrop);
	    if (falloff_ref == NULL)
		cerr << "could not load fall-off-reference image : '" << (cmde[i] + 7) << "'" << endl;
	    else {
		if (screen == NULL) {
		    screen = SDL_SetVideoMode(MAINW, (falloff_ref->h*MAINW)/falloff_ref->w, 0, 0);
		    if (sum_image == NULL) {
			sum_image = new ImageRGBL (falloff_ref->w, falloff_ref->h);
			if (sum_image == NULL) {
			    cerr << "could not allocate sum_image : bailing out ..." << endl;
			    return -1;
			}
			sum_image->zero();
			sum_image->turnmaskon(0);
		    }
		}
if (doublesize) {
    if (falloff_ref != NULL)
    {	ImageRGBL *imageb = falloff_ref->doublescale ();
	delete (falloff_ref);
	falloff_ref = imageb;
// ref_image->save_png ("ref.png");
    }
}
		falloff_ref->setmax();
	    }
	} else if (strncmp ("-finetune", cmde[i], 9) == 0) {
	    finetune = true;
	} else if (strncmp ("-watch=", cmde[i], 7) == 0) {
	    dirname = cmde[i]+7;
	    regexp = "";
	    size_t pos = dirname.rfind("/");
	    if (pos != string::npos) {
		regexp = dirname.substr(pos + 1);
		dirname = dirname.substr(0, pos);
	    }
	    if (regexp.size() == 0)
	    {
		regexp = ".*\\.jpg";
		cerr << "no regexp supplied, applying default : \"" << regexp << "\"" << endl;
	    }
if (debug) {
cout << "dirname = \"" << dirname << "\"" << endl;
cout << "regexp = \"" << regexp << "\"" << endl;
}
	    cregexp = (regex_t *) malloc(sizeof(regex_t));
	    if (cregexp == NULL) {
		int e = errno;
		cerr << "could not malloc for regexp : " << strerror (e) << endl;
		continue;
	    }
	    memset(cregexp, 0, sizeof(regex_t));

	    int err_no = 0;
	    if ((err_no = regcomp (cregexp, regexp.c_str(), REG_NOSUB)) !=0 ) { /* Compile the regex */
		size_t length = regerror (err_no, cregexp, NULL, 0);; 
		char *buffer = (char *) malloc(length);;
		if (buffer == NULL) {
		    cerr << "regexp error and could not even malloc for error report !" << endl;
		    continue;
		}
		regerror (err_no, cregexp, buffer, length);
		cerr << "regexp error : \"" << regexp << "\" : " << buffer << endl;

		free (buffer);
		regfree (cregexp);
		continue;
	    } 

	    interact (nbimage, true, no_align, true);

	    regfree (cregexp);
	}
    }

    interact (nbimage, true, no_align);

//    sum_image->minimize ();
//    sum_image->substract (128);
    if (sum_image != NULL) {
	sum_image->maxminize ();
	sum_image->trunk (gain);
	sum_image->maximize ();
	sum_image->save_png ("test.png");
    }


    SDL_Quit();
    return 0;
}
