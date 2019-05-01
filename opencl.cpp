#include <stdio.h>
#include <OpenCL/cl.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "color.h"
#include "animation.h"

#define MAX_SOURCE_SIZE (0x100000)

void doRun(int width, int height, int maxIter, float scale, const char *filename);

char *readKernel(size_t &source_size) {
    FILE *fp;
    char *source_str;
 
    fp = fopen("mandelbrot_kernel.txt", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*) malloc(MAX_SOURCE_SIZE);
    source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose( fp );

    return source_str;
}

void runGPU(int width, int height, int maxIter, bool animate) {
    if (!animate) {
        doRun(width, height, maxIter, 1, "test.png");
    } else {
        Animation a;
        char filename[32];
        for (int i = 0; i < a.frames.size(); i++) {
            sprintf(filename, "./output/frame-%d.png", i);
            printf("%d %f\n", i, a.frames[i].scale);
            doRun(width, height, maxIter, a.frames[i].scale, filename);
        }
    }
}

void doRun(int width, int height, int maxIter, float scale, const char *filename) {
    cl_uint numPlatforms = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);

	cl_platform_id *platforms = (cl_platform_id *) malloc(numPlatforms * sizeof(cl_platform_id));
	clGetPlatformIDs(numPlatforms, platforms, NULL);

	cl_uint numDevices;
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);

	cl_device_id *devices = (cl_device_id *) malloc(numDevices * sizeof(cl_device_id));
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);

    cl_uint workItemDims;
    clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &workItemDims, NULL);

    size_t *dims = (size_t *) malloc(sizeof(size_t) * workItemDims);
    clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * workItemDims, dims, NULL);

    width = ceil((float) width / dims[0]) * dims[0];
    printf("new width = %d\n", width);

    height = ceil((float) height / dims[1]) * dims[1];
    printf("new height = %d\n", height);

    float ratio = (float) height / width;
    float minX = -2.f * scale;
    float minY = minX * ratio;
    //minX -= 1;

    float ratioX = 4.f / width * scale;
    float ratioY = 4.f * ratio / height * scale;

	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);

    // Load the kernel source code into the array source_str
    size_t source_size;
    char *source_str = readKernel(source_size);

    const int LIST_SIZE = width * height * sizeof(unsigned int);

    unsigned int *data = (unsigned int *)malloc(LIST_SIZE);

    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, numDevices, devices, NULL, NULL, &status);
 
    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, devices[0], 0, &status);
 
    // Create memory buffers on the device for each vector 
    cl_mem data_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, LIST_SIZE, NULL, &status);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &status);
    printf("Create status = %d\n", status);
    if (status != CL_SUCCESS) {
    	exit(1);
    }

    // Build the program
    status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
    printf("Build status = %d\n", status);
    if (status != CL_SUCCESS) {
		size_t length;
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &length);
        char *buffer = (char *)malloc(length * sizeof(char));

        clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, length, buffer, NULL);
        printf("%lu\n", length);
		printf("LOG = %s\n", buffer);
        free(buffer);
		exit(1);
    }
 
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "mandelbrot", &status);
    printf("Create kernel status = %d\n", status);
    if (status != CL_SUCCESS) {
    	exit(1);
    }

    size_t kernelSize;
    clGetKernelWorkGroupInfo(kernel, devices[0], CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &kernelSize, NULL);
    printf("Kernel work group size = %lu\n", kernelSize);
 
    // Set the arguments of the kernel
    int argID = 0;
    status = clSetKernelArg(kernel, argID++, sizeof(int), (void *)&width);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&ratioX);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&ratioY);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&minX);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&minY);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(int), (void *)&maxIter);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(cl_mem), (void *)&data_mem);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    fflush(stdout);
 
    // Execute the OpenCL kernel on the list
    size_t global_item_size[3] = {256, 256, 16};
    size_t local_item_size[3] = {128, 1, 1};
    status = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, NULL);
    printf("clEnqueueNDRangeKernel status = %d\n", status);
    if (status != CL_SUCCESS) {
    	exit(1);
    }

    // Read the memory buffer C on the device to the local variable C
	status = clEnqueueReadBuffer(command_queue, data_mem, CL_TRUE, 0, LIST_SIZE, data, 0, NULL, NULL);
	printf("clEnqueueReadBuffer status = %d\n", status);

    saveImage(filename, width, height, data, maxIter);
    free(data);
 
    // Clean up
    status = clFlush(command_queue);
    status = clFinish(command_queue);
    status = clReleaseKernel(kernel);
    status = clReleaseProgram(program);
    status = clReleaseMemObject(data_mem);
    status = clReleaseCommandQueue(command_queue);
    status = clReleaseContext(context);

    free(source_str);
    free(devices);
    free(platforms);

	clock_gettime(CLOCK_MONOTONIC, &finish);
	double elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

	printf("%f\n", elapsed);
}