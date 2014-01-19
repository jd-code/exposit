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

#include <vector>
#include <string>
#include <map>

#include "graphutils.h"

#include "gp_imagergbl.h"
#include "simplechrono.h"

namespace exposit {

    // JDJDJDJD ugly
    extern int
	lcrop, rcrop, tcrop, bcrop;

    map <string, int> fmatch;
	    
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

using namespace std;

bool globalchrono = true;

bool menuactive = true;
bool zoomactive = false;

int base = 0;
vector<string> mainmenu;
vector<string> gainsmenu;

vector<string> *pcurmenu = &mainmenu;
int curmenuentry = 0, previousmenuentry = 0;
bool redrawmenu = false;

bool dialogactive = false;


//    string &dialogtext = dialogexistq;
//    int curdialogentry = 0;
//    vector<string> &curdialog = yesno;

// JDJDJDJD this part is uggly and needs some proper reorganisation !!!
// resoudre ce truc avec le .h qui va bien ... JDJDJDJD
extern SDL_Surface* screen;

extern ImageRGBL *ref_image;
extern ImageRGBL *sum_image;

extern    string dirname;		// the dir to be polled for files
extern	  string regexp;		// the regexp matching files
extern    regex_t *cregexp;		// the compiled regexp

extern int xzoom;
extern int yzoom;
extern int wzoom;
extern int hzoom;

extern bool debug;

extern bool doublesize;

extern StarsMap empty, &ref_starmap, *last_starmap;
extern double last_rot;
extern int last_dx, last_dy;

ImageRGBL *load_image (const char * fname, int lcrop, int rcrop, int tcrop, int bcrop);
int try_add_pic (const char * fname, int rothint);

int simplenanosleep (int ms);
// ... ---------------------

int gain = -1;
double gamma = 1.0;
bool interactfly = true;

bool render_menu (SDL_Surface &surface, int xoff, int yoff, int width, int height, 
		  vector<string> &menu, int active, int pressed
		 ) {
    Uint32 color_normal = SDL_MapRGB(surface.format, 192, 192, 255);
    Uint32 color_active = SDL_MapRGB(surface.format, 255, 255, 255);
    Uint32 color_pressed = SDL_MapRGB(surface.format, 255, 121, 128);
    Uint32 color;

    int i;
    vector<string>::iterator li;

    for (li=menu.begin(),i=0 ; li!=menu.end() ; li++,i++) {
	if (i == active)
	    color = color_active;
	else if (i == pressed)
	    color = color_pressed;
	else
	    color = color_normal;
	grapefruit::SDLF_putstr (&surface, xoff+(width-8*li->size())/2, yoff+24+16*i, color, li->c_str());
    }

    SDL_UpdateRect(&surface, 0, 0, 0, 0);
    return true;
}

class Dialog {
    public:
    	string question;
	vector<string> answers;
	int curdialogentry,
	defaultentry;

	static void installdialog (Dialog &d);

    	Dialog (const string &question, const string &answ, int defaultentry=0)
	    : question(question), curdialogentry(defaultentry), defaultentry(defaultentry)
	{
	     size_t p=0;
	     while (p < answ.size()) {
		size_t q = answ.find('|', p);
		if (q != string::npos) {
		    answers.push_back (answ.substr (p, q-p));
		    p = q+1;
		} else {
		    answers.push_back (answ.substr (p));
		    break;
		}
	     }
	}
	bool escape (void) {
	    return action (defaultentry);
	}
	bool left (void) {
	    if (curdialogentry > 0) {
		curdialogentry--;
		return true;
	    }
	    return false;
	}
	bool right (void) {
	    if (curdialogentry < (int)answers.size()-1) {
		curdialogentry++;
		return true;
	    }
	    return false;
	}
	bool enter (void) {
	    return action (curdialogentry);

	}
	virtual bool action (int entry) = 0;
	bool render_dialog (SDL_Surface &surface, int xoff, int yoff, int width, int height) {
	    Uint32 color_normal = SDL_MapRGB(surface.format, 192, 192, 255);
	    Uint32 color_active = SDL_MapRGB(surface.format, 255, 255, 255);
//	    Uint32 color_pressed = SDL_MapRGB(surface.format, 255, 121, 128);
	    Uint32 color = color_normal;

	    
	    grapefruit::SDLF_putstr (&surface, xoff+(width-8*question.size())/2, yoff+24, color, question.c_str());

	    int i;
	    vector<string>::iterator li;
	    size_t larg = 0, curlarg = 0;
	    for (li=answers.begin(),i=0 ; li!=answers.end() ; li++,i++) {
		larg += li->size() * 8 + 16;
	    }
	    larg -= 16;

	    for (li=answers.begin(),i=0 ; li!=answers.end() ; li++,i++) {
		if (i == curdialogentry)
		    color = color_active;
//		else if (i == pressed)
//		    color = color_pressed;
		else
		    color = color_normal;
		grapefruit::SDLF_putstr (&surface, xoff + (width-larg)/2 + curlarg, yoff+24*2, color, li->c_str());
		curlarg += li->size()*8 + 16;
	    }

	    SDL_UpdateRect(&surface, 0, 0, 0, 0);
	    return true;
	}

};

class Dexit : public Dialog {
    public:
    	Dexit (void) : Dialog ("do you really want to exit exposit ?", "yes|no", 1) {}
	virtual bool action (int entry) {
	    if (entry == 0) { // ask for exit
		exit (0);
	    }
	    menuactive = true;
	    return true;
	}
};

Dexit exityesno;
Dialog &curdialog = exityesno;

void Dialog::installdialog (Dialog &d) {
    dialogactive = true;
    curdialog = d;
    d.curdialogentry = d.defaultentry;
}


bool interact (int &nbimage, bool wemustloop, bool pollingdir = false) {

    if (globalchrono) Chrono::dump(cout);

//    if ((sum_image == NULL) || (screen == NULL))
    if (screen == NULL)
	return false;

{   static bool mainmenuinit = true;
    if (mainmenuinit) {
	mainmenu.push_back ("add next frame");
	mainmenu.push_back ("zoom adjust");
	mainmenu.push_back ("gain levels");
	mainmenu.push_back ("examine stack");
	mainmenu.push_back ("save stack");
	mainmenu.push_back ("new stack");
	mainmenu.push_back ("exit");

	gainsmenu.push_back ("overall gain");
	gainsmenu.push_back ("overall ceil");
	gainsmenu.push_back ("red gain");
	gainsmenu.push_back ("red ceil");
	gainsmenu.push_back ("green gain");
	gainsmenu.push_back ("green ceil");
	gainsmenu.push_back ("blue gain");
	gainsmenu.push_back ("blue ceil");
	gainsmenu.push_back ("back");


	mainmenuinit = false;
    }
}
    if (sum_image != NULL) {
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
    }

    if (nbimage != 0) {
	if (gain == -1) {
	    sum_image->fasthistogramme(1);
	    map<int,int>::iterator mi, mj;
	    int vmax = 0;
	    mj = sum_image->hr.begin();
	    for (mi=sum_image->hr.begin(),mi++ ; mi!=sum_image->hr.end() ; mi++) {
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

	sum_image->render (*screen, 0, 0, screen->w/2, screen->h/2, base, gain, gamma);
	sum_image->renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2, base, gain, gamma,
				xzoom, yzoom, wzoom, hzoom);

	ref_starmap.renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2,
				xzoom, yzoom, wzoom, hzoom,
				sum_image->w/2, sum_image->h/2,
				0.0, 0, 0,
				255,0,0);
	if (last_starmap != NULL)
	    last_starmap->renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2,
                                xzoom, yzoom, wzoom, hzoom,
				sum_image->w/2, sum_image->h/2,
				last_rot, last_dx, last_dy,
				0,0,255);

	SDL_Rect r;
	r.x = screen->w/2;
	r.y = 0;
	r.w = screen->w/2;
	r.h = screen->h/2;
	SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0, 0, 0));
	sum_image->renderhist (*screen, screen->w/2, 0, screen->w/2, screen->h/2, base, gain);
	render_menu (*screen, screen->w/2, 0, screen->w/2, screen->h/2, *pcurmenu, curmenuentry, -1);
    }

    SDL_Event event;

    bool redrawzoom = false;
    bool redrawsum = false;
    bool redrawhist = false;
    bool wemustsave = false;
    bool wemustsaveXPO = false;

    int nwzoom;
    int nhzoom;

    while (wemustloop) {
	while (SDL_PollEvent(&event)) {
	    vector<string> &curmenu = *pcurmenu;
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
			    if (zoomactive) {
				zoomactive = false;
				menuactive = true;
				redrawzoom = true;	
				redrawmenu = true;	
			    } else if (dialogactive) {
				curdialog.escape();
				dialogactive = false;
				redrawmenu = true;
			    } else {
				return false;
			    }
			    break;

			case SDLK_LEFT:
			    if (menuactive) {
			    } else if (dialogactive) {
				if (curdialog.left()) redrawmenu = true;
			    } else if (zoomactive) {
				xzoom = max(0, xzoom - hzoom/8);
				redrawzoom = true;
			    }
			    break;

			case SDLK_UP:
			    if (menuactive) {
				if (curmenuentry > 0) {
				    curmenuentry --;
				    redrawmenu = true;
				}
			    } else if (zoomactive) {
				yzoom = max(0, yzoom - hzoom/8);
				redrawzoom = true;
			    }
			    break;

			case SDLK_RIGHT:
			    if (menuactive) {
			    } else if (dialogactive) {
				if (curdialog.right()) redrawmenu = true;
			    } else if (zoomactive) {
				xzoom = min(sum_image->w - wzoom, xzoom + hzoom/8);
				redrawzoom = true;
			    }
			    break;

			case SDLK_DOWN:
			    if (menuactive) {
				if (curmenuentry < (int)curmenu.size()-1) {
				    curmenuentry ++;
				    redrawmenu = true;
				}
			    } else if (zoomactive) {
				yzoom = min(sum_image->h - hzoom, yzoom + hzoom/8);
				redrawzoom = true;
			    }
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

			case SDLK_b:
			    gamma *= 1.02;
			    redrawzoom = true;
			    redrawsum = true;
			    break;

			case SDLK_n:
			    gamma /= 1.02;
			    redrawzoom = true;
			    redrawsum = true;
			    break;

			case SDLK_j:
			    base += sum_image->Max/(screen->w/2);
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_i:
			    base += sum_image->Max/20;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_h:
			    base -= sum_image->Max/(screen->w/2);
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_u:
			    base -= sum_image->Max/20;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;



			case SDLK_l:
			    gain += sum_image->Max/(screen->w/2);
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_p:
			    gain += sum_image->Max/20;
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_k:
			    gain -= sum_image->Max/(screen->w/2);
			    redrawzoom = true;
			    redrawsum = true;
			    redrawhist = true;
			    break;

			case SDLK_o:
			    gain -= sum_image->Max/20;
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

			case SDLK_w:
			    wemustsaveXPO = true;
			    break;

			case SDLK_g:
			    interactfly = false;
			    break;

			case SDLK_RETURN:
			case SDLK_SPACE:
			    if (menuactive) {
				if (curmenu[curmenuentry] == "zoom adjust") {
				    zoomactive = true;
				    menuactive = false;
				    redrawmenu = true;
				    redrawzoom = true;
				} else if (curmenu[curmenuentry] == "exit") {
				    redrawmenu = true;
				    menuactive = false;

				    Dialog::installdialog (exityesno);
				} else if (curmenu[curmenuentry] == "add next frame") {
				    // we start the next computation
				    wemustloop = false;
				} else if (curmenu[curmenuentry] == "gain levels") {
				    menuactive = true;
				    redrawmenu = true;
				    previousmenuentry = curmenuentry;
				    curmenuentry = 0;
				    pcurmenu = &gainsmenu;
				    continue;
				} else if (curmenu[curmenuentry] == "back") {
				    menuactive = true;
				    redrawmenu = true;
				    curmenuentry = previousmenuentry;
				    pcurmenu = &mainmenu;
				    continue;
				}
			    } else if (zoomactive) {
				zoomactive = false;
				menuactive = true;
				redrawzoom = true;	
				redrawmenu = true;	
			    } else if (dialogactive) {
				curdialog.enter();
				dialogactive = false;
				redrawmenu = true;
			    } else {
				// we start the next computation
				wemustloop = false;
			    }
			    break;

			default:
			    break;
		    }
		    break;
	    }
	}
	if (nbimage != 0) {
	    if (redrawzoom) {
		sum_image->renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2, base, gain, gamma,
					xzoom, yzoom, wzoom, hzoom);
		ref_starmap.renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2,
					xzoom, yzoom, wzoom, hzoom, 
					sum_image->w/2, sum_image->h/2,
					0.0, 0, 0,
					255,0,0);
		if (last_starmap != NULL)
		    last_starmap->renderzoom (*screen, 0, screen->h/2, screen->w/2, screen->h/2,
					xzoom, yzoom, wzoom, hzoom,
					sum_image->w/2, sum_image->h/2,
					last_rot, last_dx, last_dy,
					0,0,255);

		if (zoomactive) {
		    Uint32 color = SDL_MapRGB(screen->format, 192, 192, 255);
		    string text ("zoom adjust");
		    grapefruit::SDLF_putstr (screen, (screen->w/2-8*text.size())/2, screen->h/2+24, color, text.c_str());
		    SDL_UpdateRect(screen, 0, 0, 0, 0);
		}
		redrawzoom = false;
	    }
	    if (redrawsum) {
		sum_image->render (*screen, 0, 0, screen->w/2, screen->h/2, base, gain, gamma);
		redrawsum = false;
	    }
	    if (redrawmenu || redrawhist) {
		redrawhist = true,
		redrawmenu  = true;
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
	    if (redrawmenu) {
		if (menuactive) {
		    render_menu (*screen, screen->w/2, 0, screen->w/2, screen->h/2, *pcurmenu, curmenuentry, -1);
		}
		if (dialogactive) {
		    curdialog.render_dialog (*screen, screen->w/2, 0, screen->w/2, screen->h/2);
		}
		redrawmenu = false;
	    }
	    if (wemustsave) {
static int nbprint = 0;
		char fname[50];
		snprintf (fname, 50, "test_%04d.png", nbprint);
		nbprint++;
		cerr << "saving corrected pic " << fname << "..." << endl;
		sum_image->savecorrected (fname, base, gain, gamma);
		cerr << " ... done." << endl;
		wemustsave = false;
	    }
	    if (wemustsaveXPO) {
static int nbprint = 0;
		char fname[50];
		snprintf (fname, 50, "test_%04d.xpo", nbprint);
		nbprint++;
		cerr << "saving XPO pic " << fname << "..." << endl;
		sum_image->save_xpo (fname);
		cerr << " ... done." << endl;
		wemustsaveXPO = false;
	    }
	}
    
	 if (	pollingdir &&
		watchdir (dirname, regexp, cregexp)
	    ) {

	    map <string, int>::iterator mi;
	    for (mi=fmatch.begin() ; mi!=fmatch.end() ; mi++) {
		if (mi->second == 0) {
		    if (ref_image == NULL) {
			ref_image = load_image (mi->first.c_str(), lcrop, rcrop, tcrop, bcrop);
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

		    if ((ref_image != NULL) && (try_add_pic (mi->first.c_str(), 0) == 0)) {
			nbimage ++;
			redrawzoom = true;
			redrawsum = true;
			redrawhist = true;
			mi->second ++;
		    }
		}
	    }
	}
	simplenanosleep (50);
    }

    return true;
}


} // namespace exposit
