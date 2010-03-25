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

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <string.h>

#include <regex.h>

#include <math.h>

#include <SDL.h>
#include <SDL/SDL_image.h>

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

ImageRGBL *load_imageppm (istream &fin, const string& fname) {
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

    ImageRGBL *image = new ImageRGBL (width, height);
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
	    image->r[x][y] = r;
	    image->g[x][y] = g;
	    image->b[x][y] = b;
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

ImageRGBL *load_imageppm (const char * fname) {
    ifstream fin (fname);
    if (!fin) {
	int e = errno;
	cerr << "load_imageppm could not load file as ppm '" << fname <<"' : " << strerror (e) << endl;
	return NULL;
    }
    return load_imageppm (fin, fname);
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

string dcraw_command = "dcraw -c -4";

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

    ImageRGBL *image = load_imageppm (in, fname);
    pclose (pin);
    cerr << " ... done." << endl;
    return image;
}

ImageRGBL *load_image (const char * fname) {
    if (strlen (fname) > 3) {
	if (strncmp (fname+strlen (fname)-3, "ppm", 3) == 0)
	    return load_imageppm (fname);
    }
    {	string s(fname);
	size_t p = s.find_last_of ('.');
	if (p !=string::npos) {
	    if (matchdcrawfiles.find(s.substr(p+1)) != matchdcrawfiles.end()) {
		return dcraw_treat (fname);
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

    ImageRGBL *image = new ImageRGBL(*surface);

    SDL_FreeSurface (surface);

    return image;
}

// ----------------------------------------------------------------------------------------

    ImageRGBL *ref_image = NULL;
    StarsMap empty, &ref_starmap = empty;
    ImageRGBL *sum_image = NULL;

    list <ImageRGBL *> lnoise_ref;

    ImageRGBL *falloff_ref = NULL;

    string dirname,		// the dir to be polled for files
	   regexp;		// the regexp matching files
    regex_t *cregexp = NULL;	// the compiled regexp

int xzoom = -1;
int yzoom = -1;
int wzoom = -1;
int hzoom = -1;

int try_add_pic (const char * fname) {
    ImageRGBL *image = load_image (fname);

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
	image->render (*screen, screen->w/2, screen->h/2, screen->w/2, screen->h/2, 0, image->maxlev);
	chrono_rendering.stop(); if (chrono) cout << chrono_rendering << endl;

	StarsMap *starmap = image->graphe_stars();
	if (starmap == NULL) {
	    cerr << "could not allocate stars-map for " << fname << ". skipped ..." << endl;
	    delete (image);
	    return -1;
	}
	image->study_specularity(*starmap);

	int dx, dy;
	long long diff;
	double firstda0 = 0.0;

	static Chrono chrono_vectordiffing("vector diffing"); chrono_vectordiffing.start();

	{
	    double da0, da0_b;
	    diff = starmap->find_match (ref_starmap, 0.0, dx, dy, da0, da0_b);

	    cout << " try_add_pic : dv vector-choosed(0) = d[" << dx << "," << dy
		 << "] + rot[ " << ((int)((1800.0*da0  )/M_PI))/10.0
		 << " ~ " <<       ((int)((1800.0*da0_b)/M_PI))/10.0 << " ] " << endl;

	    firstda0 = da0_b;
	    diff = starmap->find_match (ref_starmap, da0_b, dx, dy, da0, da0_b);

	    cout << " try_add_pic : dv vector-choosed(" << firstda0 << ") = d[" << dx << "," << dy
		 << "] + rot[ " << ((int)((1800.0*da0  )/M_PI))/10.0
		 << " ~ " <<       ((int)((1800.0*da0_b)/M_PI))/10.0 << " ] " << endl;
	}
	chrono_vectordiffing.stop(); if (chrono) cout << chrono_vectordiffing << endl;

	bool compareoldmethod = false;
	if (compareoldmethod) {
		// int diff = image->find_match (*ref_image, image->w/3, image->h/3, width, width, dx, dy, maxdt);
		dx = 0, dy = 0;
		diff = image->find_match (*ref_image, xzoom, yzoom, wzoom, hzoom, dx, dy, maxdt);
		cout << " try_add_pic : dv pixmap-choosed = d[" << dx << "," << dy << "] = " << (100.0*((double)diff)/(width*width)) << "%" << endl;
	}

	if (finetune) {
	    static Chrono chrono_fine_tuning("fine-tuning"); chrono_fine_tuning.start();
	    double ang;
	    double  delta = 0.05 * M_PI/180.0,
		    step = 0.01 * M_PI/180.0;
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


	if (!lnoise_ref.empty()) {
	    static Chrono chrono_derefnoising("substracting reference noise"); chrono_derefnoising.start();

	    image->substract(**(lnoise_ref.begin()));
	    chrono_derefnoising.stop(); if (chrono) cout << chrono_derefnoising << endl;
	}

	if (falloff_ref != NULL) {
	    static Chrono chrono_falloffcorrecting("correcting falloff"); chrono_falloffcorrecting.start();
	    image->falloff_correct (*falloff_ref);
	    chrono_falloffcorrecting.stop(); if (chrono) cout << chrono_falloffcorrecting << endl;
	}

	ImageRGBL *rot = NULL;
	if (firstda0 != 0.0) {
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
	delete (starmap);
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
    debug = 1;
    if (debug) cerr << "entree dans thread_interact" << endl;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("exposit", "exposit");
    // screen = SDL_SetVideoMode(1024, 768, 0, 0);

    while (ref_image == NULL) {
	simplenanosleep (100);
    }

    if (ref_image != NULL) {
	if (screen == NULL)
	    screen = SDL_SetVideoMode(1280, (ref_image->h*1280)/ref_image->w, 0, 0);
    }

    if (debug) cerr << "sortie de thread_interact imminente" << endl;

    return NULL;
}

int main (int nb, char ** cmde) {

    pthread_t pth_interact;

    if (pthread_create( &pth_interact, NULL, thread_interact, (void*) NULL) != 0) {
	int e = errno;
	cerr << "could not create interaction thread : " << strerror (e) << endl;
    }

    init_dcrawfile (); // sets an initial list of files handled with dcraw

    int i;
    int nbimage = 0;
    for (i=1 ; i<nb ; i++) {
	if (cmde[i][0] != '-') {
	    if (ref_image == NULL) {
		if (nbimage != 0) cerr << "warning, the reference image in use isn't the first in the list" << endl;

		ref_image = load_image (cmde[i]);

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

	    if (try_add_pic (cmde[i]) == 0) {
		nbimage ++;
		if (screen != NULL) {
		    if (!interact (nbimage, interactfly)) {
			// let's exit
			break;
		    }
		}
	    }
	} else if (strncmp ("-debug=", cmde[i], 7) == 0) {
	    debug = atoi (cmde[i]+7);
	    ImageRGBL::setdebug (debug);
	    StarsMap::setdebug (debug);
	} else if (strncmp ("-dcraw=", cmde[i], 7) == 0) {
	    dcraw_command = cmde[i]+7;
	} else if (strncmp ("-doublescale", cmde[i], 12) == 0) {
	    doublesize = true;
	} else if (strncmp ("-noise=", cmde[i], 7) == 0) {
	    ImageRGBL *im = load_image (cmde[i] + 7);
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
		lnoise_ref.push_back (im);
	    }
	} else if (strncmp ("-falloff=", cmde[i], 9) == 0) {
	    falloff_ref = load_image (cmde[i] + 9);
	    if (falloff_ref == NULL)
		cerr << "could not load fall-off-reference image : '" << (cmde[i] + 7) << "'" << endl;
	    else {
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

	    interact (nbimage, true, true);

	    regfree (cregexp);
	}
    }

    interact (nbimage, true);

//    sum_image->minimize ();
//    sum_image->substract (128);
    if (sum_image != NULL) {
	sum_image->maxminize ();
	sum_image->trunk (gain);
	sum_image->maximize ();
	sum_image->save_png ("test.png");
    }

    {	list <ImageRGBL *>::iterator li;
	for (li=lnoise_ref.begin() ; li!=lnoise_ref.end() ; li++) {
	    delete (*li);
	}
	lnoise_ref.erase (lnoise_ref.begin(), lnoise_ref.end());
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
    for (i=1 ; i<nb ; i++) {
	if (cmde[i][0] != '-') {
	    if (ref_image == NULL) {
		if (nbimage != 0)
		    cerr << "warning, the reference image in use isn't the first in the list" << endl;
		ref_image = load_image (cmde[i]);
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
			screen = SDL_SetVideoMode(1280, (ref_image->h*1280)/ref_image->w, 0, 0);
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

	    if (try_add_pic (cmde[i]) == 0) {
		nbimage ++;
		if (screen != NULL) {
		    if (!interact (nbimage, interactfly)) {
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
	    ImageRGBL *im = load_image (cmde[i] + 7);
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
		lnoise_ref.push_back (im);
	    }
	} else if (strncmp ("-falloff=", cmde[i], 9) == 0) {
	    falloff_ref = load_image (cmde[i] + 9);
	    if (falloff_ref == NULL)
		cerr << "could not load fall-off-reference image : '" << (cmde[i] + 7) << "'" << endl;
	    else {
		if (screen == NULL) {
		    screen = SDL_SetVideoMode(1280, (falloff_ref->h*1280)/falloff_ref->w, 0, 0);
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

	    interact (nbimage, true, true);

	    regfree (cregexp);
	}
    }

    interact (nbimage, true);

//    sum_image->minimize ();
//    sum_image->substract (128);
    if (sum_image != NULL) {
	sum_image->maxminize ();
	sum_image->trunk (gain);
	sum_image->maximize ();
	sum_image->save_png ("test.png");
    }

    {	list <ImageRGBL *>::iterator li;
	for (li=lnoise_ref.begin() ; li!=lnoise_ref.end() ; li++) {
	    delete (*li);
	}
	lnoise_ref.erase (lnoise_ref.begin(), lnoise_ref.end());
    }

    SDL_Quit();
    return 0;
}
