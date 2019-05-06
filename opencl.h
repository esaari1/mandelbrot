#ifndef _OPENCL_H
#define _OPENCL_H

#include <OpenCL/cl.h>

class OpenCL {

public:
	OpenCL(int w, int h);
	~OpenCL();

	void runGPU(int maxIter, bool animate);

private:
	void initialize();
	void doRun(int maxIter, float scale, float xoffset, float yoffset, const char *filename);

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
