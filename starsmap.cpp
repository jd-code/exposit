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

#include <math.h>
#include <iostream>

#include "starsmap.h"


namespace exposit {

    void StarsMap::full_update_xind (void) {
	xind.erase(xind.begin(),xind.end());
	StarsMap::iterator mi;
	for (mi=begin() ; mi!=end() ; mi++) {
	    xind.insert(pair<int,VStar*>(mi->second.x, &mi->second));
	}
    }

    void StarsMap::full_expunge (void) {
	{	multimap <int,VStar*>::iterator mi;
	    for (mi=xind.begin() ; mi!=xind.end() ; ) {
		if (mi->second->expunged) {
		    multimap <int,VStar*>::iterator mj = mi;
		    mi++;
		    xind.erase(mj);
		} else
		    mi++;
	    }
	}
	{	iterator mi;
	    for (mi=begin() ; mi!=end() ; ) {
		if (mi->second.expunged) {
		    iterator mj = mi;
		    mi++;
		    erase (mj);
		} else
		    mi++;
	    }
	}
    }

    int StarsMap::find_match (StarsMap const &ref_starmap, double rothint, int &dx, int &dy, double &da0, double &da0_b) {
	StarsMap::iterator li;
	StarsMap::const_iterator lj, start, finish;
	StarsMap &me = *this;

	map <int,double> rep_da;

	double  scos = 0.0,
		ssin = 0.0,
		sdx = 0.0,
		sdy = 0.0,
		spond = 0.0;

	double  a1 =  cos(rothint),
		b1 = -sin(rothint),
		a2 =  sin(rothint),
		b2 =  cos(rothint);

	for (li=me.lower_bound(me.rbegin()->first/2) ; li!=me.end() ; li++) {
//		multimap<int,VStar*> alikes;
	    if (li->second.d0 != 0.0) {
		start  = ref_starmap.lower_bound ( (int) (li->first *0.10)),
		finish = ref_starmap.lower_bound ( (int) (li->first*1.1));
		for (lj=start ; lj!=finish ; lj++) {
		    if (lj->second.d0 != 0.0) {
//			    alikes.insert(pair<int,VStar*>(li->second.distance(lj->second),&(lj->second)));
			double d = 0.01+li->second.distance(lj->second);
			double pond = 1/(d*d);
			// sdx += pond * (li->second.x - lj->second.x);
			// sdy += pond * (li->second.y - lj->second.y);
			// sdx += pond * (li->second.x - ((lj->second.x-w/2)*a1 + (lj->second.y-h/2)*a2 + w/2));
			// sdy += pond * (li->second.y - ((lj->second.x-w/2)*b1 + (lj->second.y-h/2)*b2 + h/2));
			sdx += pond * (((li->second.x-w/2)*a1 + (li->second.y-h/2)*a2 + w/2) - lj->second.x);
			sdy += pond * (((li->second.x-w/2)*b1 + (li->second.y-h/2)*b2 + h/2) - lj->second.y);
			double da = (li->second.a0 - lj->second.a0);

			if(da < 0) da += 2*M_PI;

			rep_da[(int)(da*7200.0/M_PI)] += pond;

			scos += pond * cos(da);
			ssin += pond * sin(da);
			spond += pond;
		    }
		}
	    }
	}

	double cosa = scos / spond;
	double sina = ssin / spond;

	da0 = acos(cosa);
	if (sina<0)
	da0 = 2*M_PI - da0;
	da0_b = da0;
	{	map <int,double>::iterator mi;
	    double vmax = 0;
	    for (mi=rep_da.begin() ; mi!=rep_da.end() ; mi++) {
if (debug)
cout << "  rep[" << mi->first << "] = " << mi->second << endl;
		if (mi->second > vmax) {
		    vmax = mi->second;
		    da0_b = mi->first*M_PI/7200.0;
		}
	    }
	}

	dx = (int) ((sdx / spond) + 0.5);
	dy = (int) ((sdy / spond) + 0.5);
	return 0;
    }

    void StarsMap::setdebug (int d) {
	debug = d;
    }

    int StarsMap::debug = 0;

} // namespace exposit
