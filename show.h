
#include <string>
#include <iostream>

namespace std {

class V4L2Cap {
    public:

    typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
    } io_method;

    struct buffer {
	void *                  start;
	size_t                  length;
    };

    private:
	bool debug;
	string dev_name;
	int fd;
	struct buffer *buffers;
	bool state;

	io_method io;


	bool init_mmap (void);
	bool init_userp (size_t buffer_size);
	bool init_read (size_t buffer_size);
    public:
	struct v4l2_format active_fmt;

	bool enumerate_menu (struct v4l2_queryctrl &queryctrl, struct v4l2_querymenu &querymenu);
	void list_controls (void);

	bool init_device (void);
	bool uninit_device (void);
	bool open_device (void);
	bool close_device (void);
	bool start_capturing (void);
	bool stop_capturing (void);

// a supprimer ????
	bool errno_exit (const string & s);

	void set_io_method (io_method io) { V4L2Cap::io = io; }
	int read_frame (void);
	virtual void process_image (void *p) = 0;


    public:
	V4L2Cap (string dev_name);
	friend bool operator ! (V4L2Cap const & v);
	inline operator void * (void) const {
	    if (state)
		return (void *) this;
	    else
		return NULL;
	};

	inline int get_fd(void) { return fd; }
	const string & get_dev_name (void) { return dev_name; }

};

bool operator ! (V4L2Cap const & v);

} // namespace std
