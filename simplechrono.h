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

#ifndef SIMPLECHRONO_H
#define SIMPLECHRONO_H

#include <time.h>
#include <string>
#include <list>
#include <iostream>
#include <iomanip>

namespace exposit {

    using namespace std;

    class Chrono;

    class ChronoList : public list<Chrono *> {
	public:
	    ChronoList (void) : list<Chrono *>() {}
	    ~ChronoList (void);
    };

    class Chrono {
	public:

    static clock_t total;
    static ChronoList chronolist;


	    string name;
	    clock_t last_t, last_m, cumul;
	    bool running;
	    bool registered;

	    list<Chrono *>::iterator me;

	    Chrono (string name, bool go=false);
	    ~Chrono ();

	    clock_t start (void);

	    clock_t stop (void);

	    friend ostream& operator<< (ostream& out, const Chrono &);

	    static ostream& format_output (ostream &out, const string &name, clock_t last_m, clock_t cumul, clock_t total);


	    static void dump (ostream& out);
    };

    ostream& operator<< (ostream& out, const Chrono &chrono);

#ifdef SIMPLECHRONO_STATICS
    clock_t Chrono::total = 0;
    ChronoList Chrono::chronolist;
#endif

}

#endif // SIMPLECHRONO_H
