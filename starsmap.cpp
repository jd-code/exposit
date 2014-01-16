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
#include <list>
#include <iostream>

#include "starsmap.h"


namespace exposit {

    inline int abs (int i) {
	return i<0 ? -i : i;
    }

    double average (list<double> const &l) {
	if (l.empty()) return 0.0;
	list<double>::const_iterator li;
	double moy = 0.0;
        for (li=l.begin() ; li!=l.end() ; li++) moy += *li;
	return moy / l.size();
    }

    double average_and_stddev (list<double> const &l, double &stddev) {
	if (l.empty()) return 0.0;
	list<double>::const_iterator li;
	double moy = average (l);
	stddev = 0.0;
	for (li=l.begin() ; li!=l.end() ; li++) {
	    double d = *li - moy;
	    stddev += d * d;
	}
	stddev = sqrt (stddev / l.size());	
	return moy;
    }

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

    int StarsMap::find_tuned_match (StarsMap const &ref_starmap, double rothint, int &dx, int &dy, double &da0, double &da0_b) {
	StarsMap::iterator li;
	StarsMap::const_iterator lj, start, finish;
	StarsMap &me = *this;


	double  scos = 0.0,
		ssin = 0.0,
		sdx = 0.0,
		sdy = 0.0,
		pond = 0.0,
		spond = 0.0;

	double  a1 =  cos(rothint),
		b1 = -sin(rothint),
		a2 =  sin(rothint),
		b2 =  cos(rothint);

	// first, we try to build a matching list of VStars

	map<VStar*,VStar*> match;				// the matching list
	multimap <double,map<VStar*,VStar*>::iterator> refine;	// a distance-indexed list of the above for further refining
	list<double> ldisteucl;					// for qualifying the above coincidence attempt

	if (xind.empty()) {
	    cerr << "the x-index of stars is emty ??" << endl
		 << "me.size() = " << me.size() << endl;
	    return -1;
	}

	for (li=me.begin() ; li!=me.end() ; li++) {	// for each star from ourselves (starmap)
	    int projx, projy;
	    bool pushed = false;
	    projx = (li->second.x-w/2)*a1 + (li->second.y-h/2)*a2 + w/2 - dx;	// we project to the predicted rotation and drift
	    projy = (li->second.x-w/2)*b1 + (li->second.y-h/2)*b2 + h/2 - dy;

	    if ((projx < w/15) || (projy < w/15) || (projx > w-w/15) || (projy > h-w/15))   // it the projection is out of bounds we bail out for that one
		continue;

	    multimap <int,VStar*>::const_iterator lj, mj;	// we're going to fetch the closest star in euclidian distance to the predicted proj point
	    mj = ref_starmap.xind.lower_bound(projx);		// we use the x-index to find fast
	    if (mj == ref_starmap.xind.end()) mj --;		// in case we're out of the bound we use the last of the map as starting value
	    int mindis = mj->second->distance_eucl(projx, projy);
	    if (abs(mj->second->y - projy) < w/20)
		match[&li->second] = mj->second, pushed = true;

	    int widthlim = w / 20;
	    for (lj=mj ; (lj!=ref_starmap.xind.end()) && (lj->second->x-projx < widthlim) ; lj++) { // we explore on one side
		if (abs(lj->second->y - projy) < w/20) {    // targets must be close (useful when "distance" is used instead of "distance_eucl")
		    int d = lj->second->distance_eucl(projx, projy);
		    if (d < mindis) {
			mindis = d;
			match[&li->second] = lj->second, pushed = true;
		    }
		}
	    }
	    lj=mj;
	    while (projx - lj->second->x < widthlim) {						    // we explore the other side
		if (abs(lj->second->y - projy) < w/20) {    // targets must be close (useful when "distance" is used instead of "distance_eucl")
		    int d = lj->second->distance_eucl(projx, projy);
		    if (d < mindis) {
			mindis = d;
			match[&li->second] = lj->second, pushed = true;
		    }
		}
		if (lj == ref_starmap.xind.begin())
		    break;
		lj--;
	    }
	    if (pushed) {
		map<VStar*,VStar*>::iterator mi = match.find (&li->second);
		if (mi != match.end()) {	// here we index the matches found by order of closeness
		    // refine[match[&li->second]->distance_eucl(projx, projy)] = mi;
		    double d = mi->second->distance_eucl(projx, projy);
		    refine.insert (pair<double,map<VStar*,VStar*>::iterator> (d, mi));
		    ldisteucl.push_back (d);
		} else {
		    cerr << "StarsMap::find_tuned_match : error : we don't find what we just pushed-in ?" << endl;
		}
	    }
	}
	{   double aver, stddev = 0.0;
	    aver = average_and_stddev (ldisteucl, stddev);
	    cerr << "  original dist_eucl_moy = " << aver << " stddev = " << stddev << endl;
	}
	// we refine the match-list using the closeness index
	{
	    size_t tot = refine.size(), i;
	    
	// we're about to drop the 10% worst matches
#define CLOSENESSPERCENTILE 0.9
	    multimap <double,map<VStar*,VStar*>::iterator>::iterator ri;

	    // for statistics ...
	    list<double> ldtistrefined;

	    for (ri=refine.begin(), i=0 ; (i<tot*CLOSENESSPERCENTILE) && (ri!=refine.end()) ; ri++,i++)
		ldtistrefined.push_back (ri->first);	// for statistics only

	    for ( ; ri!=refine.end() ; ri++)
		match.erase (ri->second);

	    refine.erase (refine.begin(), refine.end());
	    {   double aver, stddev = 0.0;
		aver = average_and_stddev (ldtistrefined, stddev);
		cerr << "   refined dist_eucl_moy = " << aver << " stddev = " << stddev << endl;
	    }
	}

	// calculating the starmap rotation angle by comparing matching bi-points
	{   map<VStar*,VStar*>::iterator li, lj;
	    int n = 0;

	    spond = 0.0;
	    for (li=match.begin() ; li!=match.end() ; li++) {		// we go the full matching point list ...
// cerr << "d = " << li->first->distance_eucl(*li->second) << endl;
		double d1 = li->first->distance(*li->second);	// the VStar distance is used for ponderation
		for (lj=li, lj++ ; lj!=match.end() ; lj++) {		// ... toward the second half of the list
		    int firstdx = li->first->x - lj->first->x,
			firstdy = li->first->y - lj->first->y;
		    int seconddx = li->second->x - lj->second->x,
			seconddy = li->second->y - lj->second->y;
		    double norm1 = sqrt (firstdx*firstdx+firstdy*firstdy),
			   norm2 = sqrt (seconddx*seconddx+seconddy*seconddy);
		   
// only vectors as long as half (or more) the width of the pic will be used for rotation matching
// #define VECTORMATCHMINIMUMLENGTH 0.75
#define VECTORMATCHMINIMUMLENGTH 0.5
		    if ((norm1 < w*VECTORMATCHMINIMUMLENGTH) || (norm2 < w*VECTORMATCHMINIMUMLENGTH))
			continue; 
		    double d2 = lj->first->distance(*lj->second);   // the VStar distance is used for ponderation
		    d2 *= d1;
		    if (d2 < 0.0001) d2 = 0.0001;
		    pond = 1/(d2*d2);
		    n++;
		    scos += pond * (firstdx*seconddx + firstdy*seconddy) / (norm2 * norm1);
		    ssin += pond * (-firstdx*seconddy + firstdy*seconddx) / (norm2 * norm1);
		    spond += pond;
		}
	    }

	    double cosa = scos / spond;
	    double sina = ssin / spond;
// JDJDJDJD those values could be reported to a structure ?
//    cerr << match.size() << "points,  " << n << " bi-points" << endl;
//    cerr << "rotm = [ " << scos << " : " << ssin << " ] / " << spond << endl;
//    cerr << "rotm = [ " << cosa << " : " << sina << " ]    sqsum = " << cosa*cosa + sina*sina << endl;
	    da0 = acos(cosa);
	    if (sina<0)
		da0 = 2*M_PI - da0;
	    da0_b = da0;
	}

	a1 =  cos(da0),
	b1 = -sin(da0),
	a2 =  sin(da0),
	b2 =  cos(da0);

	{   map<VStar*,VStar*>::iterator li;
	    list<double> l_spec_dist;	// used for statistic collection
	    spond = 0.0;
	    for (li=match.begin() ; li!=match.end() ; li++) {
		double d = li->first->distance(*li->second);	// the VStar distance is used for ponderation
		l_spec_dist.push_back (d);  // statistic collection
		if (d<0.001) d = 0.001;
		pond = 1/d*d;
		sdx += pond * ((li->first->x-w/2)*a1 + (li->first->y-h/2)*a2) +w/2 - li->second->x;
		sdy += pond * ((li->first->x-w/2)*b1 + (li->first->y-h/2)*b2) +h/2 - li->second->y;
		spond += pond;
	    }
	    dx = sdx / spond;
	    dy = sdy / spond;

	    {   double aver, stddev = 0.0;
		aver = average_and_stddev (l_spec_dist, stddev);
		cerr << "  specular dist_eucl_moy = " << aver << " stddev = " << stddev << endl;
	    }
	    
	}

	return 0;
    }

    int StarsMap::find_match (StarsMap const &ref_starmap, double rothint, int &dx, int &dy, double &da0, double &da0_b) {
	StarsMap::iterator li;
	StarsMap::const_iterator lj, start, finish;
	StarsMap &me = *this;

//#define ANGULAR_RES 7200.0
#define ANGULAR_RES 72000.0

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

	if (ref_starmap.empty()) {
	    cerr << "StarsMap::find_match ref_starmap is empty !" << endl;
	    return 0;
	}
	if (me.empty()) {
	    cerr << "StarsMap::find_match I'm empty !" << endl;
	    return 0;
	}

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

			rep_da[(int)(da*ANGULAR_RES/M_PI)] += pond;

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
		    da0_b = mi->first*M_PI/ANGULAR_RES;
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
