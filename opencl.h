#ifndef _OPENCL_H
#define _OPENCL_H

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

class OpenCL {

public:
	OpenCL(int w, int h);
	~OpenCL();

	void runGPU(int maxIter, bool animate, const char *afile, int frame = 0, double x = 0, double y = 0);

private:
	void initialize();
	void doRun(int maxIter, double scale, double xoffset, double yoffset, const char *filename);

	int width, height;
	cl_platform_id *platforms;
	cl_device_id *devices;
	cl_kernel kernel;
	cl_context context;
	cl_command_queue command_queue;
	cl_mem data_mem;
	cl_program program;
};

#endif
