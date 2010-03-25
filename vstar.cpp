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

#include "vstar.h"

namespace exposit {

    using namespace std;

    int VStar::distance_2angles (VStar const &s) {
	double	da1 = a1 - s.a1,
		da2 = a2 - s.a2,
		d = sqrt (da1*da1 + da2*da2);
		d *= 180.0/M_PI;
	return (int) d;
    }

    int VStar::distance_3d (VStar const &s) {
	double	da0 = d0 - s.d0,
		da1 = d1 - s.d1,
		da2 = d2 - s.d2,
		d = sqrt (da0*da0 + da1*da1 + da2*da2);
		d /= 10;
	return (int) d;
    }


    int VStar::distance_3mags (VStar const &s) {
	double	da0 = mag0 - s.mag0,
		da1 = mag1 - s.mag1,
		da2 = mag2 - s.mag2,
		d = sqrt (da0*da0 + da1*da1 + da2*da2);
		d *= 100;
	return (int) d;
    }

    int VStar::distance (VStar const &s) {
	double	D0 = distance_2angles (s),
		D1 = distance_3d (s),
		d = sqrt (D0*D0 + D1*D1);
	return (int) d;
    }

    void VStar::qualify0 (VStar const &s) {
	int dx = s.x - x,
	    dy = s.y - y,
	    dd = dxxdyy (*this, s);
	d0 = sqrt(dd);
	if (d0 < 2.0) {
	    a0 = 0, d0 = 0, mag0 = 0;
	    return;
	}

	a0 = acos (dx/d0);
	if (dy<0)
	    a0 = 2 * M_PI - a0;
	mag0 = s.abs_magnitude / abs_magnitude;
    }

    void VStar::qualify1 (VStar const &s) {
	int dx = s.x - x,
	    dy = s.y - y,
	    dd = dxxdyy (*this, s);
	d1 = sqrt(dd);
	if ((d0==0.0) || (d1 < 2.0)) {
	    a1 = 0, d1 = 0, mag1 = 0;
	    return;
	}

	a1 = acos (dx/d1);
	if (dy<0)
	    a1 = 2 * M_PI - a1;
	a1 -= a0;
	if (a1 < 0)
	    a1 += 2 * M_PI;
	mag1 = s.abs_magnitude / abs_magnitude;
    }

    void VStar::qualify2 (VStar const &s) {
	int dx = s.x - x,
	    dy = s.y - y,
	    dd = dxxdyy (*this, s);
	d2 = sqrt(dd);
	if ((d0==0.0) || (d2 < 2.0)) {
	    a2 = 0, d2 = 0, mag2 = 0;
	    return;
	}

	a2 = acos (dx/d2);
	if (dy<0)
	    a2 = 2 * M_PI - a2;
	a2 -= a0;
	if (a2 < 0)
	    a2 += 2 * M_PI;
	mag2 = s.abs_magnitude / abs_magnitude;
    }

    bool VStar::qualify (multimap<int, VStar*> const &lvstar) {
	if (lvstar.size() < 3)
	    return false;
	multimap<int, VStar*>::const_iterator mi, mm=lvstar.end();
	int i;
	int vmax = 0;
	for (i=0,mi=lvstar.begin() ; (i<3)&&(mi!=lvstar.end()) ;i++,mi++) {
	    if (mi->second->abs_magnitude >= vmax) {
		mm = mi;
		vmax = mi->second->abs_magnitude;
	    }
	}
	qualify0 (*(mm->second));
	for (i=0,mi=lvstar.begin() ; (i<2)&&(mi!=lvstar.end()) ;mi++) {
	    if (mi != mm) {
		switch (i) {
		    case 0:
			qualify1 (*(mi->second));
			i++;
			break;
		    case 1:
			qualify2 (*(mi->second));
			i++;
			break;
		}
	    }
	}
	if (a2<a1) {
	    swap (a2,a1);
	    swap (d2,d1);
	    swap (mag2,mag1);
	}
	a2 -= a1;
	if (a1 < 0)
	    a1 += 2 * M_PI;
	return true;
    }


    int dxxdyy (VStar const &a, VStar const &b) {
	int dx = a.x - b.x,
	    dy = a.y - b.y;
	return dx*dx + dy*dy;
    }
} // namespace exposit
