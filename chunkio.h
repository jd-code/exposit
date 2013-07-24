
#include <fstream>
#include <string>

namespace chunkio {

    using namespace std;

    string strchunk (int l);

    class ofchunk {
      private:
	ofstream ofs;
	int remainingchunklen;
      public:
	ofchunk (const char * fname, ios_base::openmode mode = ios_base::out);
	bool startchunk (const char * chunkname, int len);
	bool endchunk (void);
	bool isok (void);
	bool writeLONG (int l);
	bool writeWORD (int l);
	bool writeBYTE (int l);
	bool writeBYTES (const char *p, int l);
	void close (void);
	bool operator ! () const;
	operator void * () const;
	~ofchunk (void);
    };

    class ifchunk {
      private:
	ifstream ifs;
	int remainingchunklen;
      public:
	ifchunk (const char * fname, ios_base::openmode mode = ios_base::out);
	bool isok (void);
	bool operator ! () const;
	operator void * () const;
	int getnextchunk (void);
	int get_remchunklen (void);
	bool readLONG (int &l);
	bool readWORD (int &l);
	bool readBYTE (int &l);
	void close (void);
	~ifchunk (void);

//	bool startchunk (const char * chunkname, int len);
//	bool endchunk (void);
//	bool isok (void);
//	bool writeLONG (int l);
//	bool writeBYTES (const char *p, int l);
    };

}

