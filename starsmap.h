#ifndef STARSMAP_INCLUDED
#define STARSMAP_INCLUDED

#include <map>

#include "vstar.h"

namespace exposit {

    using namespace std;

    class StarsMap : public multimap <int,VStar> {
	public:
	    int w, h;
	    multimap <int,VStar*> xind;
	    StarsMap (void) {}

	    StarsMap (int w, int h) : multimap <int,VStar>(), w(w), h(h) {}

	    void full_update_xind (void);

	    void full_expunge (void);

	    int find_match (StarsMap const &ref_starmap, double rothint, int &dx, int &dy, double &da0, double &da0_b);

static int debug;

	    static void setdebug (int d);
    };

} // namespace exposit

#endif // STARSMAP_INCLUDED
