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

#include "simplechrono.h"

namespace exposit {

    using namespace std;


    Chrono::Chrono (string name, bool go /* =false */) :
	name(name),
	last_t(0), last_m(0), cumul(0),
	running (false), registered(false) {
	    if (go) start();
	chronolist.push_back(this);
	me = chronolist.end();
	me --;
	registered = true;
    }

    Chrono::~Chrono () {
	if (registered)
	    chronolist.erase(me);
	registered = false;
    }

    clock_t Chrono::start (void) {
	if (running == true) {
	    cerr << "chrono \"" << name << "\" already started : skipped" << endl;
	    return last_t;
	}
	running = true;
	return last_t = clock();
    }

    clock_t Chrono::stop (void) {
	if (running == false) {
	    cerr << "chrono \"" << name << "\" already stopped : skipped" << endl;
	    return last_t;
	}
	clock_t now = clock();
	last_m = now - last_t;
	last_t = now;
	cumul += last_m;
	total += last_m;
	running = false;
	return now;
    }

    ostream& Chrono::format_output (ostream &out, const string &name, clock_t last_m, clock_t cumul, clock_t total) {
	ios::fmtflags state = out.flags();
	float percent = (total == 0) ? 0.0 : ((int)(cumul*1000/total))/10.0;
	out << setw(40) << name << " " << setw(6) << (last_m*1000L)/CLOCKS_PER_SEC << " ms "
	    << " ( cumul = " << setw(7) << (cumul*1000L)/CLOCKS_PER_SEC << " ms "
	    << setw(5) << fixed << setprecision(1) << percent << "% tot)";
	out.flags(state);
	return out;
    }

    void Chrono::dump (ostream& out) {
	list<Chrono *>::iterator li;
	clock_t lasttotal = 0,
		cumultotal = 0;
	for (li=chronolist.begin() ; li!=chronolist.end() ; li++) {
	    out << (**li) << endl;
	    lasttotal += (*li)->last_m;
	    cumultotal += (*li)->cumul;
	}
	format_output (out, "total", lasttotal, cumultotal, total) << endl;
    }

    ostream& operator<< (ostream& out, const Chrono &chrono) {
	return Chrono::format_output (out, chrono.name, chrono.last_m, chrono.cumul, chrono.total);
    }

    ChronoList::~ChronoList (void) {
	list<Chrono *>::iterator li;
	for (li=this->begin() ; li!=this->end() ; li++)
	    (*li)->registered = false;
    }
}

