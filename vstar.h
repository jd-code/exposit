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

#ifndef VSTAR_ALLREADYINCLUDED
#define VSTAR_ALLREADYINCLUDED

#include <map>

namespace exposit {

    using namespace std;

    class VStar {
	public:
	    bool expunged;
	    int abs_magnitude;
	    int x, y;
	    double a0; double d0; double mag0;
	    double a1; double d1; double mag1;
	    double a2; double d2; double mag2;

	VStar (int x, int y, int abs_magnitude) : expunged(false), abs_magnitude(abs_magnitude), x(x), y(y),
						  a0(0), d0(0), mag0(0),
						  a1(0), d1(0), mag1(0),
						  a2(0), d2(0), mag2(0)
						{}

	friend int dxxdyy (VStar const &a, VStar const &b);

	int distance_2angles (VStar const &s);

	int distance_3d (VStar const &s);

	int distance_3mags (VStar const &s);

	int distance (VStar const &s);

	void qualify0 (VStar const &s);

	void qualify1 (VStar const &s);

	void qualify2 (VStar const &s);

	bool qualify (multimap<int, VStar*> const &lvstar);
    };

    int dxxdyy (VStar const &a, VStar const &b);

} // namespace exposit

#endif // VSTAR_ALLREADYINCLUDED
