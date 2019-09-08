#include <math.h>
#include <stdio.h>

#include "opencl.h"
#include "color.h"
#include "animation.h"

#define MAX_SOURCE_SIZE (0x100000)
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

char *readKernel(size_t &source_size) {
    FILE *fp;
    char *source_str;

    fp = fopen("mandelbrotd_kernel.txt", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*) malloc(MAX_SOURCE_SIZE);
    source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose( fp );

    return source_str;
}

OpenCL::OpenCL(int w, int h) {
    width = w;
    height = h;
}

OpenCL::~OpenCL() {
    // Clean up
    clFlush(command_queue);
    clFinish(command_queue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(data_mem);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);

    free(devices);
    free(platforms);
}

void OpenCL::runGPU(int maxIter, bool animate, const char *afile, int frame, const char *outputDir, double x, double y) {
    initialize();

    if (!animate) {
        double scale = 1.0;
        if (frame > 1) {
            scale = pow(0.9349, frame-1);
        }

        doRun(maxIter, scale, x, y, "test.png");
    } else {
        Animation a(afile);
        char filename[32];
        double scale = 1.0;

        for (int i = 0; i < a.frames.size(); i++) {
//            if (a.frames[i].maxIter > 0) {
//                maxIter = a.frames[i].maxIter;
//            }

			if (frame == 0 || i >= frame) {
	            sprintf(filename, "./%s/frame-%d.png", outputDir, (i+1));
	            printf("Frame %d of %lu %d\n", (i+1), a.frames.size(), maxIter);
	            doRun(maxIter, scale, a.frames[i].xoffset, a.frames[i].yoffset, filename);
			}

            scale *= 0.9349;
            maxIter *= 1.017;
//			scale *= 0.95;
//            maxIter *= 1.015;
        }
    }
}

void OpenCL::initialize() {
    cl_uint numPlatforms = 0;
    cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);

    platforms = (cl_platform_id *) malloc(numPlatforms * sizeof(cl_platform_id));
    clGetPlatformIDs(numPlatforms, platforms, NULL);

    cl_uint numDevices;
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);

    devices = (cl_device_id *) malloc(numDevices * sizeof(cl_device_id));
    clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);

    cl_uint workItemDims;
    clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &workItemDims, NULL);

    size_t *dims = (size_t *) malloc(sizeof(size_t) * workItemDims);
    clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * workItemDims, dims, NULL);

    width = ceil((double) width / dims[0]) * dims[0];
    height = ceil((double) height / dims[1]) * dims[1];
    const int LIST_SIZE = width * height * sizeof(double);

    // Create an OpenCL context
    context = clCreateContext( NULL, numDevices, devices, NULL, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("clCreateContext failed = %d\n", status);
        exit(1);
    }

    // Create a command queue
    command_queue = clCreateCommandQueue(context, devices[0], 0, &status);
    if (status != CL_SUCCESS) {
        printf("clCreateCommandQueue failed = %d\n", status);
        exit(1);
    }

    // Create memory buffers on the device for data
    data_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, LIST_SIZE, NULL, &status);
    if (status != CL_SUCCESS) {
        printf("clCreateBuffer failed = %d\n", status);
        exit(1);
    }

    // Load the kernel source code into the array source_str
    size_t source_size;
    char *source_str = readKernel(source_size);

    // Create a program from the kernel source
    program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &status);
    if (status != CL_SUCCESS) {
        printf("Create program failed = %d\n", status);
        exit(1);
    }

    // Build the program
    status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
    if (status != CL_SUCCESS) {
        size_t length;
        clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &length);
        char *buffer = (char *)malloc(length * sizeof(char));

        printf("Build failure = %d\n", status);
        clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, length, buffer, NULL);
        printf("LOG = %s\n", buffer);
        free(buffer);
        exit(1);
    }

    // Create the OpenCL kernel
    kernel = clCreateKernel(program, "mandelbrotd", &status);
    if (status != CL_SUCCESS) {
        printf("Create kernel failed = %d\n", status);
        exit(1);
    }

    free(source_str);
}

void OpenCL::doRun(int maxIter, double scale, double xoffset, double yoffset, const char *filename) {
    double ratio = (double) height / width;
    double minX = -2.f * scale;
    double minY = minX * ratio;
    minX += xoffset;
    minY -= yoffset;

    double ratioX = 4.f / width * scale;
    double ratioY = 4.f * ratio / height * scale;
    const int LIST_SIZE = width * height * sizeof(double);

    double *data = (double *)malloc(LIST_SIZE);

     // Set the arguments of the kernel
    int argID = 0;
    cl_uint status = clSetKernelArg(kernel, argID++, sizeof(int), (void *)&width);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(double), (void *)&ratioX);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(double), (void *)&ratioY);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(double), (void *)&minX);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(double), (void *)&minY);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(int), (void *)&maxIter);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(cl_mem), (void *)&data_mem);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    // Execute the OpenCL kernel on the list
    size_t global_item_size[3] = {1024, 1024, 2};
    size_t local_item_size[3] = {128, 1, 1};
    cl_event event;

    status = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, &event);
    if (status != CL_SUCCESS) {
        printf("clEnqueueNDRangeKernel failed = %d\n", status);
    	exit(1);
    }

    // Read the memory buffer C on the device to the local variable C
	status = clEnqueueReadBuffer(command_queue, data_mem, CL_TRUE, 0, LIST_SIZE, data, 1, &event, NULL);
    if (status != CL_SUCCESS) {
    	printf("clEnqueueReadBuffer failed = %d\n", status);
        exit(1);
    }

    saveImage(filename, width, height, data, maxIter);
    free(data);
 }
