
#include <string.h>

#include <iostream>

#include "chunkio.h"


namespace chunkio {
    string strchunk (int l) {
	string s;
	s += (char)(l & 0xff);
	s += (char)((l & 0xff00) >> 8);
	s += (char)((l & 0xff0000) >> 16);
	s += (char)((l & 0xff000000) >> 24);
	return s;
    }


    ofchunk::ofchunk (const char * fname, ios_base::openmode mode /* = ios_base::out */) :
	ofs (fname, mode | ios::binary),
	remainingchunklen (-1)
    {
    }

    void ofchunk::close (void) {
	ofs.close();
    }

    ofchunk::~ofchunk (void) {
	ofs.close();
    }

    bool ofchunk::isok (void) {
	return (ofs) ? true : false;
    }

    bool ofchunk::operator ! () const {
	return (!ofs);
    }

    ofchunk::operator void * () const {
	if (ofs)
	    return (void *)this;
	else
	    return NULL;
    }

    bool ofchunk::writeLONG (int l) {
	if (!isok())
	    return false;
	if ((remainingchunklen >= 0) && (remainingchunklen < 4))
	    return false;

	unsigned char buf[4];
	buf [0] = l & 0xff;
	buf [1] = (l & 0xff00) >> 8;
	buf [2] = (l & 0xff0000) >> 16;
	buf [3] = (l & 0xff000000) >> 24;
	ofs.write ((char *)&buf[0], 4);
	if (!isok())
	    return false;
	if (remainingchunklen >= 0)
	    remainingchunklen -= 4;
	return true;
    }

    bool ofchunk::writeBYTE (int l) {
	if (!isok())
	    return false;
	if ((remainingchunklen >= 0) && (remainingchunklen < 1))
	    return false;

	unsigned char buf[4];
	buf [0] = l & 0xff;
	ofs.write ((char *)&buf[0], 1);
	if (!isok())
	    return false;
	if (remainingchunklen >= 0)
	    remainingchunklen -= 1;
	return true;
    }

    bool ofchunk::writeWORD (int l) {
	if (!isok())
	    return false;
	if ((remainingchunklen >= 0) && (remainingchunklen < 2))
	    return false;

	unsigned char buf[4];
	buf [0] = l & 0xff;
	buf [1] = (l & 0xff00) >> 8;
	ofs.write ((char *)&buf[0], 2);
	if (!isok())
	    return false;
	if (remainingchunklen >= 0)
	    remainingchunklen -= 2;
	return true;
    }

    bool ofchunk::writeBYTES (const char * p, int l) {
	if (!isok())
	    return false;
	if ((remainingchunklen >= 0) && (remainingchunklen < l))
	    return false;
	ofs.write (p, l);
	if (!isok())
	    return false;
	if (remainingchunklen >= 0)
	    remainingchunklen -= l;
	return true;
    }

    bool ofchunk::startchunk (const char * chunkname, int len) {
	if (remainingchunklen != -1)
	    return false;
	if (strlen (chunkname) != 4)
	    return false;
	if (!writeLONG (len))
	    return false;
	remainingchunklen = len;
	return writeBYTES (chunkname, 4);
    }
    
    bool ofchunk::endchunk (void) {
	if (!ofs) return false;
	if (remainingchunklen < 0)
	    return false;
	for (int i=0 ; i<remainingchunklen ; i++)
	    ofs.put ((char)0);
	remainingchunklen = -1;
	return true;
    }

    // -----------------------------------------------------------------------

    ifchunk::ifchunk (const char * fname, ios_base::openmode mode /* = ios_base::out */) :
	ifs (fname, mode | ios::binary),
	remainingchunklen (-1)
    {
    }

    void ifchunk::close (void) {
	ifs.close();
    }

    ifchunk::~ifchunk (void) {
	ifs.close();
    }

    bool ifchunk::isok (void) {
	return (ifs) ? true : false;
    }

    bool ifchunk::operator ! () const {
	return (!ifs);
    }

    ifchunk::operator void * () const {
	if (ifs)
	    return (void *)this;
	else
	    return NULL;
    }



    bool ifchunk::readLONG (int &l) {
	if (!isok())
	    return false;
	if ((remainingchunklen >= 0) && (remainingchunklen < 4))
	    return false;

	unsigned char buf[4];
	ifs.read ((char *)&buf[0], 4);
	if (!isok())
	    return false;

	if (!isok())
	    return false;
	if (remainingchunklen >= 0)
	    remainingchunklen -= 4;
	l = buf [0] + (buf [1] << 8) + (buf[2] << 16) + (buf[3] << 24);
	return true;
    }

    bool ifchunk::readBYTE (int &l) {
	if (!isok())
	    return false;
	if ((remainingchunklen >= 0) && (remainingchunklen < 1))
	    return false;

	unsigned char buf[1];
	ifs.read ((char *)&buf[0], 1);
	if (!isok())
	    return false;

	if (!isok())
	    return false;
	if (remainingchunklen >= 0)
	    remainingchunklen -= 1;
	l = buf [0];
	return true;
    }

    bool ifchunk::readWORD (int &l) {
	if (!isok())
	    return false;
	if ((remainingchunklen >= 0) && (remainingchunklen < 2))
	    return false;

	unsigned char buf[2];
	ifs.read ((char *)&buf[0], 2);
	if (!isok())
	    return false;

	if (!isok())
	    return false;
	if (remainingchunklen >= 0)
	    remainingchunklen -= 2;
	l = buf [0] + (buf [1] << 8);
	return true;
    }

    int ifchunk::getnextchunk (void) {
	if (remainingchunklen > 0) {
	    ifs.seekg (remainingchunklen, ios::cur);
	}
	remainingchunklen = -1;

	if (!ifs)
	    return -1;

	int l=0;
	if (!readLONG (remainingchunklen))
	    return -1;
	if (readLONG (l))
	    return l;
	else
	    return -1;
    }

    int ifchunk::get_remchunklen (void) {
	return remainingchunklen;
    }


}   // namespace chunkio
