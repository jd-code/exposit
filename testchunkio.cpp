
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
    out.startchunk ("HDR_", 20);

    out.writeWORD (3008);
    out.writeWORD (2000);
    out.writeWORD (24);
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


    int chunk = in.getnextchunk ();

    cout << "chunk = " << strchunk(chunk) << " (" << setbase(16) << setw(8) << setfill('0') << chunk << ")" << endl;
    cout << setbase(10);


    if (chunk == 0x5a505889) {
	int w;
	if (in.readWORD(w))
	    cout << " w = " << w << endl;
	if (in.readWORD(w))
	    cout << " w = " << w << endl;
	if (in.readWORD(w))
	    cout << " w = " << w << endl;
    }

    // while (in && ((chunk = in.getnextchunk ()) != -1)) {
    while (in) {
	chunk = in.getnextchunk ();
	cout << "chunk = " << strchunk(chunk) << " (" << setbase(16) << setw(8) << setfill('0') << chunk << ")" << endl;
	if (chunk == -1) break;
    }

    in.close();

    return 0;
}
