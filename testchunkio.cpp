
#include <errno.h>
#include <string.h>

#include <iostream>
#include <iomanip>

#include "chunkio.h"

using namespace std;
using namespace chunkio;

int main (void) {

    ofchunk out ("tagada.chk");

    out.writeBYTES ("\211XPO", 4);

    out.startchunk ("HDR_", 22);

	    out.writeWORD (3008);
	    out.writeWORD (2000);
	    out.writeWORD (24);

	    out.writeLONG (42);
	    out.writeLONG (87654321);
	    out.writeLONG (-87654321);

    out.endchunk ();

    out.startchunk ("LI_8", 4);
    out.endchunk ();
    out.startchunk ("LI16", 4);
    out.endchunk ();
    out.startchunk ("LI32", 4);
    out.endchunk ();

    out.startchunk ("MK_8", 4);
    out.endchunk ();
    out.startchunk ("MK16", 4);
    out.endchunk ();

    out.startchunk ("END_", 4);
    out.endchunk ();

    out.close();


    ifchunk in ("tagada.chk");
    if (!in.isok()) {
	cerr << "in pas bon : " << strerror (errno) << endl;
	return 1;
    }

    int mark;
    if (!in.readLONG (mark)) {
	cerr << "in pas bon : " << strerror (errno) << endl;
	return 1;
    }
    
    cout << "mark = " << strchunk(mark) << " (" << setbase(16) << setw(8) << setfill('0') << mark << ")" << endl;
    cout << setbase(10);


    int chunk;



    // while (in && ((chunk = in.getnextchunk ()) != -1)) {
    while (in) {
	chunk = in.getnextchunk ();
	cout << "chunk = " << strchunk(chunk) << " (" << setbase(16) << setw(8) << setfill('0') << chunk << ")" << endl;
	cout << setbase(10);
//	    if (chunk == 0x5a505889) {
	    if (chunk == 0x5f524448) {
		int w;
		int l;
		if (in.readWORD(w)) cout << " w = " << w << endl;
		if (in.readWORD(w)) cout << " w = " << w << endl;
		if (in.readWORD(w)) cout << " w = " << w << endl;
		if (in.readLONG(l)) cout << " l = " << l << endl;
		if (in.readLONG(l)) cout << " l = " << l << endl;
		if (in.readLONG(l)) cout << " l = " << l << endl;
	    }

	if (chunk == -1) break;
    }

    in.close();

    return 0;
}
