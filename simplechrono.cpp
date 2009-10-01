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
cerr << "ici" << endl;
	for (li=this->begin() ; li!=this->end() ; li++)
	    (*li)->registered = false;
    }
}

