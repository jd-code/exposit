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

#ifndef STARSMAP_INCLUDED
#define STARSMAP_INCLUDED

#include <map>

#include "vstar.h"

namespace exposit {

    using namespace std;

    class StarsMap : public multimap <int,VStar> {
	public:
	    int w, h;			// the size of the source image
	    multimap <int,VStar*> xind;	// x-coord sorted index of ourselves
	    StarsMap (void) {}

	    StarsMap (int w, int h) : multimap <int,VStar>(), w(w), h(h) {}

	    void full_update_xind (void);

	    void full_expunge (void);

	    int find_match (StarsMap const &ref_starmap, double rothint, int &dx, int &dy, double &da0, double &da0_b);
	    int find_tuned_match (StarsMap const &ref_starmap, double rothint, int &dx, int &dy, double &da0, double &da0_b);

static int debug;

	    static void setdebug (int d);
    };

} // namespace exposit

#endif // STARSMAP_INCLUDED
