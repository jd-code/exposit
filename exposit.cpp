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
#include <map>

#include "draw.h"
#include "graphutils.h"

#define SIMPLECHRONO_STATICS
#include "simplechrono.h"

#include "vstar.h"
#include "starsmap.h"
#include "gp_imagergbl.h"

namespace exposit {

using namespace std;

bool debug = false;
bool globalchrono = true;
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



ImageRGBL *load_image (const char * fname) {
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

    // ImageRGBL input(surface->w, surface->h);
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

    map <string, int> fmatch;
	    
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
	image->render (*screen, screen->w/2, screen->h/2, screen->w/2, screen->h/2, 0, 256);
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

bool watchdir (string &dirname, string &regexp, regex_t *cregexp) {
    bool foundsomething = false;
    DIR * dir;
    dir = opendir (dirname.c_str());
    if (dir == NULL) {
	int e = errno;
	cerr << "could not opendir (\"" << dirname << "\") : " << strerror (e) << endl;

	return false;
    }

    regmatch_t *result = (regmatch_t *) malloc (sizeof(regmatch_t) * (cregexp->re_nsub+1));
    if (result == NULL) {
	cerr << "could not allocate for regexp result set" << endl;

	return false;
    }

    int err_no;
    struct dirent * pde;
    while ((pde = readdir (dir)) != NULL) {
	string fname = dirname;
	fname += "/";
	fname += pde->d_name;

	if ((err_no = regexec(cregexp, pde->d_name, 0, result, 0)) == 0) {
	    map <string, int>::iterator mi = fmatch.find (fname);
	    if (mi == fmatch.end()) {
		fmatch [fname] = 0;
		foundsomething = true;
	    }
	} else if (err_no != REG_NOMATCH) {
	    size_t length = regerror (err_no, cregexp, NULL, 0);; 
	    char *buffer = (char *) malloc(length);;
	    if (buffer == NULL) {
		cerr << "regexp error and could not even malloc for error report [b] !" << endl;
		continue;
	    }
	    regerror (err_no, cregexp, buffer, length);
	    cerr << "regexp error : \"" << regexp << "\" : " << buffer << endl;

	    free (buffer);
	} else {
	    // cout << "     -  " << fname << endl;
	}
    }

    free (result);
    closedir (dir);
    return foundsomething;
}

int gain = -1;
int base = 0;

bool interactfly = true;

bool interact (int &nbimage, bool wemustloop, bool pollingdir = false) {

cout << "************************************************" << endl;
    if (globalchrono) Chrono::dump(cout);
cout << "************************************************" << endl;

    if ((sum_image == NULL) || (screen == NULL))
	return false;

    if (xzoom == -1) {
	wzoom = screen->w/2;
	hzoom = screen->h/2;
	xzoom = (sum_image->w - wzoom) / 2;
	yzoom = (sum_image->h - hzoom) / 2;

//	xzoom = (sum_image->w - sum_image->w/2) / 2;
//	yzoom = (sum_image->h - sum_image->h/2) / 2;
//	wzoom = sum_image->w/2;
//	hzoom = sum_image->h/2;
    }

    if (nbimage != 0) {
	if (gain == -1) {
	    sum_image->fasthistogramme(1);
	    map<int,int>::iterator mi, mj;
	    int vmax = 0;
	    mj = sum_image->hr.begin();
	    for (mi=sum_image->hr.begin() ; mi!=sum_image->hr.end() ; mi++) {
		if (mi->second > vmax) {
		    vmax = mi->second;
		    mj = mi;
		}
	    }
	    for (mi=mj ; (mi!=sum_image->hr.end()) && (log(mi->second) > (0.33*log(vmax))) ; mi++) {
if (debug)
cout << "  = [ " << mi->first << " : " << mi->second << " ]" << endl ;
	    }
	    gain = (mi->first - mj->first) * nbimage;
if (debug) {
cout << "initial gain = " << gain << endl;
cout << "nbimage = " << nbimage << endl;
}
	} else {
	    gain = gain * (nbimage+1)/nbimage;
	}

	sum_image->render (*screen, 0, 0, screen->w/2, screen->h/2, base, gain);
	sum_image->renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2, base, gain,
				xzoom, yzoom, wzoom, hzoom);
	SDL_Rect r;
	r.x = screen->w/2;
	r.y = 0;
	r.w = screen->w/2;
	r.h = screen->h/2;
	SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0, 0, 0));
	sum_image->renderhist (*screen, screen->w/2, 0, screen->w/2, screen->h/2, base, gain);
    }

    SDL_Event event;

    bool redrawzoom = false;
    bool redrawsum = false;
    bool redrawhist = false;
    bool wemustsave = false;

    int nwzoom;
    int nhzoom;

    while (wemustloop) {
	while (SDL_PollEvent(&event)) {
	    /* an event was found */
	    switch (event.type) {
		/* close button clicked */
		case SDL_QUIT:
		    return false;
		    break;

		/* handle the keyboard */
		case SDL_KEYDOWN:
		    switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
			case SDLK_q:
			  return false;
			  break;

			case SDLK_LEFT:
			    xzoom = max(0, xzoom - hzoom/8);
			    redrawzoom = true;
			    break;

			case SDLK_UP:
			    yzoom = max(0, yzoom - hzoom/8);
			    redrawzoom = true;
			    break;

			case SDLK_RIGHT:
			    xzoom = min(sum_image->w - wzoom, xzoom + hzoom/8);
			    redrawzoom = true;
			    break;

			case SDLK_DOWN:
			    yzoom = min(sum_image->h - hzoom, yzoom + hzoom/8);
			    redrawzoom = true;
			    break;

			case SDLK_PAGEUP:
			    nwzoom = max(screen->w/2 , (int)(wzoom / sqrt(2.0)));
			    nhzoom = (nwzoom*screen->h)/screen->w;

			    xzoom += (wzoom - nwzoom)/2;
			    yzoom += (hzoom - nhzoom)/2;

			    wzoom = nwzoom;
			    hzoom = nhzoom;
			    redrawzoom = true;
			    break;

			case SDLK_PAGEDOWN:
			    nwzoom = min(sum_image->w , (int)(wzoom * sqrt(2.0)));
			    nhzoom = (nwzoom*screen->h)/screen->w;

			    xzoom = max(0, xzoom - (nwzoom-wzoom)/2);
			    if (xzoom+nwzoom > sum_image->w)
				xzoom = sum_image->w - nwzoom;
			    wzoom = nwzoom;

			    yzoom = max(0, yzoom - (nhzoom-hzoom)/2);
			    if (yzoom+nhzoom > sum_image->h)
				yzoom = sum_image->h - nhzoom;
			    hzoom = nhzoom;

			    redrawzoom = true;
			    break;


			case SDLK_j:
			    base++;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_i:
			    base+=10;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_h:
			    base--;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_u:
			    base-=10;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;



			case SDLK_l:
			    gain++;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_p:
			    gain+=10;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_k:
			    gain--;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_o:
			    gain-=10;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_t:
			    interactfly = true;
			    break;

			case SDLK_s:
			    wemustsave = true;
			    break;

			case SDLK_g:
			    interactfly = false;
			    break;

			case SDLK_SPACE:
			    wemustloop = false;
			    break;

			default:
			    break;
		    }
		    break;
	    }
	}
	if (nbimage != 0) {
	    if (redrawzoom) {
		sum_image->renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2, base, gain,
					xzoom, yzoom, wzoom, hzoom);
		redrawzoom = false;
	    }
	    if (redrawsum) {
		sum_image->render (*screen, 0, 0, screen->w/2, screen->h/2, base, gain);
		redrawsum = false;
	    }
	    if (redrawhist) {
		SDL_Rect r;
		r.x = screen->w/2;
		r.y = 0;
		r.w = screen->w/2;
		r.h = screen->h/2;
		SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0, 0, 0));
		sum_image->renderhist (*screen, screen->w/2, 0, screen->w/2, screen->h/2, base, gain);
		redrawhist = false;
	    }
	    if (wemustsave) {
static int nbprint = 0;
		char fname[50];
		snprintf (fname, 50, "test_%04d.png", nbprint);
		nbprint++;
		cerr << "saving corrected pic " << fname << "..." << endl;
		sum_image->savecorrected (fname, base, gain);
		cerr << " ... done." << endl;
		wemustsave = false;
	    }
	}
    
	 if (	pollingdir &&
		watchdir (dirname, regexp, cregexp)
	    ) {

	    map <string, int>::iterator mi;
	    for (mi=fmatch.begin() ; mi!=fmatch.end() ; mi++) {
		if (mi->second == 0) {
		    if (ref_image == NULL) {
			ref_image = load_image (mi->first.c_str());
			if (ref_image != NULL) {
			    if (nbimage != 0)
				cerr << "warning, the reference image in use isn't the first in the list" << endl;
if (doublesize) {
    if (ref_image != NULL)
    {	ImageRGBL *imageb = ref_image->doublescale ();
	delete (ref_image);
	ref_image = imageb;
// ref_image->save_png ("ref.png");
    }
}

			    StarsMap* mmap = ref_image->graphe_stars();
			    if (mmap == NULL) {
				cerr << "could not allocate reference stars-map : bailing out ..." << endl;
				return -1;
			    }
			    ref_starmap = *mmap;
cout << "reference star map : " << ref_starmap.size () << " stars." << endl;

			    cout << "reference image is '" << mi->first << "'" << endl;
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

		    if ((ref_image != NULL) && (try_add_pic (mi->first.c_str()) == 0)) {
			nbimage ++;
			redrawzoom = true;
			redrawsum = true;
			redrawhist = true;
			mi->second ++;
		    }
		}
	    }
	}
    }

    return true;
}

} // namespace exposit

using namespace exposit; 
using namespace std;

int simplenanosleep (int ms) {
    timespec rqtp, rmtp;
    rqtp.tv_sec = 0, rqtp.tv_nsec = ms*1000000;

    return nanosleep (&rqtp, &rmtp);
}

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
