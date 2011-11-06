/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>		/* sprintf */

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <ext/stdio_filebuf.h>	// __gnu_cxx::stdio_filebuf

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#include <SDL.h>
#include <SDL/SDL_image.h>

#define SIMPLECHRONO_STATICS
#include "simplechrono.h"

#include "gp_imagergbl.h"

#include "show.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))


namespace std {

using namespace exposit;

SDL_Surface* screen = NULL;


size_t len = 0;


unsigned int     n_buffers       = 0;

bool V4L2Cap::enumerate_menu (struct v4l2_queryctrl &queryctrl, struct v4l2_querymenu &querymenu) {
    cout << "  Menu items:" << endl;

    memset (&querymenu, 0, sizeof (querymenu));
    querymenu.id = queryctrl.id;

    for (querymenu.index = queryctrl.minimum;
	    querymenu.index <= (unsigned int)queryctrl.maximum;
	    querymenu.index++) {
	if (0 == ioctl (fd, VIDIOC_QUERYMENU, &querymenu)) {
	    cout << querymenu.name << endl;
	} else {
	    perror ("VIDIOC_QUERYMENU");
	    return false;
	}
    }
    return true;
}

void V4L2Cap::list_controls (void)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_querymenu querymenu;


    memset (&queryctrl, 0, sizeof (queryctrl));

    cout << " --- public base" << endl;
    for (queryctrl.id = V4L2_CID_BASE;
	 queryctrl.id < V4L2_CID_LASTP1;
	 queryctrl.id++) {
	if (0 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
	    if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
		    continue;

	    cout << "        " << setw(20) << queryctrl.name << " ";

	    if (queryctrl.type == V4L2_CTRL_TYPE_MENU) {
		    cout << endl;
		    enumerate_menu (queryctrl, querymenu);
	    }

	    // getting the current value
	    struct v4l2_control control;

	    memset (&control, 0, sizeof (control));
	    control.id = queryctrl.id;

	    if (0 == ioctl (fd, VIDIOC_G_CTRL, &control)) {
		cout << " = " << control.value << endl;
	    } else {
		cout << " error getting value" << endl;
	    }
	} else {
	    if (errno == EINVAL)
		    continue;

	    perror ("VIDIOC_QUERYCTRL");
	    continue;
	}

    }

    cout << " --- private base" << endl;

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;;
	 queryctrl.id++) {
	    if (0 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
		    if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
			    continue;

		    cout << "        " << setw(20) << queryctrl.name << " ";

		    if (queryctrl.type == V4L2_CTRL_TYPE_MENU) {
			    cout << endl;
			    enumerate_menu (queryctrl, querymenu);
		    }

		// getting the current value
		struct v4l2_control control;

		memset (&control, 0, sizeof (control));
		control.id = queryctrl.id;

		if (0 == ioctl (fd, VIDIOC_G_CTRL, &control)) {
		    cout << " = " << control.value << endl;
		} else {
		    cout << " error getting value" << endl;
		}
	    } else {
		    if (errno == EINVAL)
			    break;

		    perror ("VIDIOC_QUERYCTRL");
		    continue;
	    }
    }
}


bool V4L2Cap::errno_exit (const string & s)
{
    int e = errno;
    cerr << "V4L2Cap " << s << " error : " << strerror (e) << endl;
    return false;
}

int xioctl (int fd, int request, void * arg) {
        int r;

        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);

        return r;
}

int V4L2Cap::read_frame	(void)
{
        struct v4l2_buffer buf;
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
    		if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
            		switch (errno) {
            		case EAGAIN:
                    		return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("read");
			}
		}

if (debug) cerr << "1";
    		process_image (buffers[0].start);

		break;

	case IO_METHOD_MMAP:
		CLEAR (buf);

            	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            	buf.memory = V4L2_MEMORY_MMAP;

    		if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
            		switch (errno) {
            		case EAGAIN:
                    		return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
			}
		}

                assert (buf.index < n_buffers);

if (debug) cerr << "2";

		len = buffers[buf.index].length;
	        process_image (buffers[buf.index].start);

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;

	case IO_METHOD_USERPTR:
		CLEAR (buf);

    		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start
			    && buf.length == buffers[i].length)
				break;

		assert (i < n_buffers);

if (debug) cerr << "2";
    		process_image ((void *) buf.m.userptr);

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;
	}

	return 1;
}

bool V4L2Cap::stop_capturing (void) {
    enum v4l2_buf_type type;
    state = false;

    switch (io) {
    case IO_METHOD_READ:
	/* Nothing to do. */
	break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
	    return errno_exit ("VIDIOC_STREAMOFF");

	break;
    }
    state = true;
    return true;
}

bool V4L2Cap::start_capturing (void) {
    unsigned int i;
    enum v4l2_buf_type type;

    state = false;

    switch (io) {
	case IO_METHOD_READ:
	    /* Nothing to do. */
	    break;

	case IO_METHOD_MMAP:
	    for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
		    return errno_exit ("VIDIOC_QBUF");
	    }
	    
	    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	    if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
		return errno_exit ("VIDIOC_STREAMON");

	    break;

	case IO_METHOD_USERPTR:
	    for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_USERPTR;
		buf.index       = i;
		buf.m.userptr	= (unsigned long) buffers[i].start;
		buf.length      = buffers[i].length;

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
		    return errno_exit ("VIDIOC_QBUF");
	    }

	    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	    if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
		    return errno_exit ("VIDIOC_STREAMON");

	    break;
    }
    state = true;
    return true;
}

bool V4L2Cap::uninit_device (void) {
    unsigned int i;
    bool badstuffoccured = false;

    switch (io) {
	case IO_METHOD_READ:
	    free (buffers[0].start);
	    break;

	case IO_METHOD_MMAP:
	    for (i = 0; i < n_buffers; ++i) {
		if (-1 == munmap (buffers[i].start, buffers[i].length))
		    badstuffoccured = true;
	    }
	    if (badstuffoccured) {
		return errno_exit ("uninit_device munmap");
	    }
	    break;

	case IO_METHOD_USERPTR:
	    for (i = 0; i < n_buffers; ++i)
		free (buffers[i].start);
	    break;
    }

    free (buffers);
    if (badstuffoccured)
	return false;

    return true;
}

bool V4L2Cap::init_read (size_t buffer_size)
{
    state = false;
    buffers = (buffer *) calloc (1, sizeof (*buffers));

    if (!buffers) {
	cerr << "V4L2Cap::init_read 1 could not allocate " << sizeof (*buffers) << " bytes" << endl;
	return false;
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc (buffer_size);

    if (!buffers[0].start) {
	cerr << "V4L2Cap::init_read 2 could not allocate " << buffer_size << " bytes" << endl;
	return false;
    }
    state = true;
    return true;
}

bool V4L2Cap::init_mmap (void)
{
    struct v4l2_requestbuffers req;

    CLEAR (req);

    req.count               = 4;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_MMAP;

    state = false;
    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
	if (EINVAL == errno) {
	    cerr << dev_name << " does not support memory mapping" << endl;
	    return false;
	} else {
	    return errno_exit ("VIDIOC_REQBUFS");
	}
    }

    if (req.count < 2) {
	cerr << "Insufficient buffer memory on " << dev_name << endl;
	return false;
    }

    buffers = (buffer *) calloc (req.count, sizeof (*buffers));

    if (!buffers) {
	cerr << "V4L2Cap could not allocate " << sizeof (*buffers) << " bytes" << endl;
	return false;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
	struct v4l2_buffer buf;

	CLEAR (buf);

	buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory      = V4L2_MEMORY_MMAP;
	buf.index       = n_buffers;

	if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
	    return errno_exit ("VIDIOC_QUERYBUF");

	buffers[n_buffers].length = buf.length;
	buffers[n_buffers].start =
		mmap (NULL /* start anywhere */,
		      buf.length,
		      PROT_READ | PROT_WRITE /* required */,
		      MAP_SHARED /* recommended */,
		      fd, buf.m.offset);

	if (MAP_FAILED == buffers[n_buffers].start)
	    return errno_exit ("mmap");
    }
    state = true;
    return true;
}

bool V4L2Cap::init_userp (size_t buffer_size)
{
    struct v4l2_requestbuffers req;
    unsigned int page_size;

    page_size = getpagesize ();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    CLEAR (req);

    req.count               = 4;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
	if (EINVAL == errno) {
	    cerr << dev_name << " does not support user pointer i/o" << endl;
	    return false;
	} else {
	    return errno_exit ("VIDIOC_REQBUFS");
	}
    }

    buffers = (buffer *) calloc (4, sizeof (*buffers));

    if (!buffers) {
	cerr << "V4L2Cap could not allocate " << 4*sizeof (*buffers) << " bytes" << endl;
	return false;
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
	buffers[n_buffers].length = buffer_size;
	buffers[n_buffers].start = memalign (/* boundary */ page_size,
					     buffer_size);

	if (!buffers[n_buffers].start) {
	    cerr << "V4L2Cap could not memalign " << 4*sizeof (*buffers) << " bytes" << endl;
	    return false;
	}
    }
    return true;
}

bool V4L2Cap::init_device (void) {
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    state = false;
    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
	if (EINVAL == errno) {
	    cerr << dev_name << " is no V4L2 device" << endl;
	    return false;
	} else {
	    return errno_exit ("VIDIOC_QUERYCAP");
	}
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
	cerr << dev_name << " is no video capture device" << endl;
	return false;
    }

    switch (io) {
	case IO_METHOD_READ:
	    if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
		cerr << dev_name << " does not support read i/o (try IO_METHOD_MMAP or IO_METHOD_USERPTR ?)" << endl;
		return false;
	    }
	    break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
	    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		cerr << dev_name << " does not support streaming i/o (try IO_METHOD_READ ?)" << endl;
		return false;
	    }
	    break;
    }


    /* Select video input, video standard and tune here. */


    CLEAR (cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c = cropcap.defrect; /* reset to default */

	if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
	    switch (errno) {
		case EINVAL:
		    cerr << dev_name << " VIDIOC_S_CROP Cropping not supported ? ignored" << endl;
		    /* Cropping not supported. */
		    break;
		default:
		    cerr << dev_name << " VIDIOC_S_CROP error reported but ignored" << endl;
		    /* Errors ignored. */
		    break;
	    }
	}
    } else {	
	cerr << dev_name << " VIDIOC_CROPCAP error reported but ignored" << endl;
	/* Errors ignored. */
    }


    CLEAR (fmt);

//JDJDJDJD    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//JDJDJDJD    fmt.fmt.pix.width       = 320;
//JDJDJDJD    fmt.fmt.pix.height      = 200;
//JDJDJDJD//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MPEG;
//JDJDJDJD    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
//JDJDJDJD//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB444;
//JDJDJDJD    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
//JDJDJDJD/*        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
//JDJDJDJD    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
//JDJDJDJD*/
//JDJDJDJD    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
//JDJDJDJD	return errno_exit ("VIDIOC_S_FMT");
//JDJDJDJD    /* Note VIDIOC_S_FMT may change width and height. */

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 640;
    fmt.fmt.pix.height      = 480;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MPEG;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB444;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
/*        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
*/
    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
	return errno_exit ("VIDIOC_S_FMT");
    /* Note VIDIOC_S_FMT may change width and height. */

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl (fd, VIDIOC_G_FMT, &fmt))
	return errno_exit ("VIDIOC_G_FMT");

    active_fmt = fmt;

    cerr << " pix.pixelformat = "
		<< (char)(fmt.fmt.pix.pixelformat & 0xff)
		<< (char)((fmt.fmt.pix.pixelformat >> 8 ) & 0xff)
		<< (char)((fmt.fmt.pix.pixelformat >> 16 ) & 0xff)
		<< (char)((fmt.fmt.pix.pixelformat >> 24 ) & 0xff)
		<< endl
	 << "    .field = " << fmt.fmt.pix.field << endl
	 << "    .width = " << fmt.fmt.pix.width << endl
	 << "    .height = " << fmt.fmt.pix.height << endl << endl;


    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
	fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
	fmt.fmt.pix.sizeimage = min;

    switch (io) {
	case IO_METHOD_READ:
	    return init_read (fmt.fmt.pix.sizeimage);
	    break;

	case IO_METHOD_MMAP:
	    return init_mmap ();
	    break;

	case IO_METHOD_USERPTR:
	    return init_userp (fmt.fmt.pix.sizeimage);
	    break;
    }
    return false;
}

bool V4L2Cap::close_device (void)
{
    if (-1 == close (fd))
	return errno_exit ("close");

    fd = -1;
    return true;
}

bool V4L2Cap::open_device (void)
{
    struct stat st; 

    if (-1 == stat (dev_name.c_str(), &st)) {
	int e = errno;
	cerr << "Cannot identify '" << dev_name << "' : " << strerror (e) << endl;
	return false;
    }

    if (!S_ISCHR (st.st_mode)) {
	cerr << "not seen as a device '" << dev_name << "'" << endl; 
	return false;
    }

    fd = open (dev_name.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);
cerr << "open : fd = " << fd << endl;
    if (-1 == fd) {
	int e = errno;
	cerr << "Cannot open '" << dev_name << "' : " << strerror (e) << endl;
	return false;
    }
    return true;
}

bool operator ! (V4L2Cap const & v) {
    return !v.state;
}

V4L2Cap::V4L2Cap (string dev_name) :
    debug(false),
    dev_name(dev_name),
    fd(-1),
    buffers(NULL),
    io(IO_METHOD_MMAP)
{
    state = false;
}




class V4L2MonProcess : public V4L2Cap {
    public:
	virtual void process_image (void *p);
	V4L2MonProcess (string dev_name) : V4L2Cap (dev_name) {}
};

void V4L2MonProcess::process_image (void *p)
{

static bool first_time = true;
static	    __gnu_cxx::stdio_filebuf<char> *pipeco = NULL;
static ostream* pgphotostream = NULL;
    if (first_time) {
	first_time = false;
	//const char* command = "gphoto2 --shell 2> /dev/null 2>&1";
	const char* command = "gphoto2 --shell";
	FILE * pgphoto = popen (command, "w");
	if (pgphoto == NULL) {
	    int e = errno;
	    cerr << "error : " << command << " failed : " << strerror (e) << endl;
	} else {
	    pipeco = new __gnu_cxx::stdio_filebuf<char>(pgphoto, ios_base::out);
	    if (pipeco == NULL) {
		cerr << "could not create pipeco ..." << endl;
	    } else {
		pgphotostream = new ostream (pipeco);
		if (pgphotostream == NULL)
		    cerr << "could not create pgphotostream ..." << endl;
	    }
	}
    }


static int i = 0;
	i++;

	if (i > 1) {

	if (len != 0) {
	    bool anciencode=false;
	    if (anciencode)
	    {	char name[30];
		snprintf (name, 25, "frame_%03d.jpg", i);
		FILE *fout = fopen (name, "w"); 
		if (fout == NULL) {
		    int e = errno;
		    cerr << "could not open " << name << " : " << strerror(e) << endl;
		}
		fwrite (p, len, 1, fout);
		fclose (fout);
	    } else {	// nouveau code, donc ...
static ImageRGBL *prepic = NULL, *curpic = NULL;
		if (active_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {
		    int w = active_fmt.fmt.pix.width,
			h = active_fmt.fmt.pix.height;
		    int x, y;
		    unsigned char *q = (unsigned char *) p;

		    curpic = new ImageRGBL (w, h);
		    if (curpic == NULL) {
			cerr << "could not allocate ImageRGBL)" << w << "," << h << ")" << endl;
			return;
		    }
		    ImageRGBL &image = *curpic;
		    for (y=0 ; y<h ; y++) for (x=0 ; x<w ; x++) {
			image.r[x][y] = image.g[x][y] = image.b[x][y] = *q++;
		    }
		} else {

		    SDL_RWops *frommem = SDL_RWFromMem(p, len);
		    SDL_Surface *surface = IMG_LoadTyped_RW (frommem, 1, "JPG");
		    
		    curpic = new ImageRGBL (*surface);
		    if (curpic == NULL) {
			cerr << "could not allocate ImageRGBL from surface" << endl;
			return;
		    }
		    SDL_FreeSurface (surface);
		}

		ImageRGBL &image = *curpic;
		image.rendermax (*screen, 0, 0, 640, 480);
		// image.rendernodiff (*screen, 640, 0, 800-640, 600-480);
		if (prepic != NULL) {
		    prepic->substract (image);
		    prepic->rendermax(*screen, 640, 0, 800-640, 600-480);

		    int diff = prepic->averageabsR ();

		    cerr << diff << " ";
// #define DIFFCEIL 15
#define DIFFCEIL 10
		    if (diff > DIFFCEIL) {
static time_t lastcapt = time(NULL);
			time_t difft = time(NULL) - lastcapt;
			if ((pgphotostream != NULL) && (difft > 2)) {
			    cerr << endl << "trig : " << diff << " " << difft << endl;
			    lastcapt = time(NULL);
			    cerr << "outputing" << endl;
			    (*pgphotostream) << "capture-image-and-download" << endl;
			    cerr << endl << "done" << endl;
			}
		    }
		}

		if (prepic != NULL) delete (prepic);
		prepic = curpic;
		curpic = NULL;
	    }
	}

	}
/*        fputc ('.', stdout);
        fflush (stdout);
*/
}

static void mainloop (V4L2Cap &theobject)
{
	unsigned int count;

static int toterror = 0;

//        count = 100;
        count = 10000000;

	int fd = theobject.get_fd();
	if (fd < 0 ) {
	    cerr << "mainloop : theobject (" << theobject.get_dev_name() << ") is unreadable ?" << endl;
	    return;
	}

        while (count-- > 0) {
		toterror = 0;
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO (&fds);
                        FD_SET (fd, &fds);

                        /* Timeout. */
                        // tv.tv_sec = 2;
                        // tv.tv_usec = 0;
                        tv.tv_sec = 0;
                        tv.tv_usec = 10000;

                        r = select (fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;

                                int e = errno;
				cerr << "select returns error " << strerror (e) << endl;
				toterror = 0;
                        }

                        if (0 == r) {
			    toterror ++;
			    if (toterror > 100) {
				cerr << "select timeout ???" << endl;
				toterror = 0;
			    }
			    continue;
                        }

			if (theobject.read_frame ()) {
				toterror = 0;
                    		break;
			}
	
			/* EAGAIN - continue select loop. */
                }
        }
}



static void
usage                           (FILE *                 fp,
                                 int                    argc,
                                 char **                argv)
{
        fprintf (fp,
                 "Usage: %s [options]\n\n"
                 "Options:\n"
                 "-d | --device name   Video device name [/dev/video]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
                 "",
		 argv[0]);
}

static const char short_options [] = "d:hmru";

static const struct option
long_options [] = {
        { "device",     required_argument,      NULL,           'd' },
        { "help",       no_argument,            NULL,           'h' },
        { "mmap",       no_argument,            NULL,           'm' },
        { "read",       no_argument,            NULL,           'r' },
        { "userp",      no_argument,            NULL,           'u' },
        { 0, 0, 0, 0 }
};

} // namespace std

using namespace std;

int main (int argc, char ** argv)
{
	const char * dev_name_default = "/dev/video0";
	const char * dev_name = dev_name_default;
	V4L2Cap::io_method iom = V4L2Cap::IO_METHOD_MMAP;

        for (;;) {
                int index;
                int c;
                
                c = getopt_long (argc, argv,
                                 short_options, long_options,
                                 &index);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'd':
                        dev_name = optarg;
                        break;

                case 'h':
                        usage (stdout, argc, argv);
                        exit (EXIT_SUCCESS);

                case 'm':
                        iom = V4L2Cap::IO_METHOD_MMAP;
			break;

                case 'r':
                        iom = V4L2Cap::IO_METHOD_READ;
			break;

                case 'u':
                        iom = V4L2Cap::IO_METHOD_USERPTR;
			break;

                default:
                        usage (stderr, argc, argv);
                        exit (EXIT_FAILURE);
                }
        }

	string s(dev_name);
	V4L2MonProcess theobject (s);
	theobject.set_io_method (iom);


	SDL_Init(SDL_INIT_VIDEO);
	SDL_WM_SetCaption("show", "show");
	screen = SDL_SetVideoMode(800, 600, 0, 0);

        theobject.open_device ();

        theobject.init_device ();

	theobject.list_controls ();

        theobject.start_capturing ();

        mainloop (theobject);

        theobject.stop_capturing ();

        theobject.uninit_device ();

        theobject.close_device ();

        exit (EXIT_SUCCESS);

	SDL_Quit();
        return 0;
}


