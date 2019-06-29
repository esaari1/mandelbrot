#include <math.h>

#include "opencl.h"
#include "color.h"
#include "animation.h"

#define MAX_SOURCE_SIZE (0x100000)

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

void OpenCL::runGPU(int maxIter, bool animate, int frame, float x, float y) {
    initialize();

    if (!animate) {
        float scale = 1.0;
        if (frame > 1) {
            scale = pow(0.9349, frame-1);
        }
        doRun(maxIter, scale, x, y, "test.png");
    } else {
        Animation a;
        char filename[32];
        float factor = 1.0;
        for (int i = 0; i < a.frames.size(); i++) {
            sprintf(filename, "./output/frame-%d.png", (i+1));
            printf("Frame %d of %lu\n", (i+1), a.frames.size());
            doRun(maxIter, factor, a.frames[i].xoffset, a.frames[i].yoffset, filename);

            factor *= 0.9349;
            maxIter *= 2;
        }
    }
}

void OpenCL::initialize() {
    cl_uint numPlatforms = 0;
    cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);

    platforms = (cl_platform_id *) malloc(numPlatforms * sizeof(cl_platform_id));
    clGetPlatformIDs(numPlatforms, platforms, NULL);

    cl_uint numDevices;
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);

    devices = (cl_device_id *) malloc(numDevices * sizeof(cl_device_id));
    clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);

    cl_uint workItemDims;
    clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &workItemDims, NULL);

    size_t *dims = (size_t *) malloc(sizeof(size_t) * workItemDims);
    clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * workItemDims, dims, NULL);

    width = ceil((float) width / dims[0]) * dims[0];
    height = ceil((float) height / dims[1]) * dims[1];
    const int LIST_SIZE = width * height * sizeof(float);

    // Create an OpenCL context
    context = clCreateContext( NULL, numDevices, devices, NULL, NULL, &status);
 
    // Create a command queue
    command_queue = clCreateCommandQueue(context, devices[0], 0, &status);
 
    // Create memory buffers on the device for data
    data_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, LIST_SIZE, NULL, &status);

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
    kernel = clCreateKernel(program, "mandelbrot", &status);
    if (status != CL_SUCCESS) {
        printf("Create kernel failed = %d\n", status);
        exit(1);
    }

    free(source_str);
}

void OpenCL::doRun(int maxIter, float scale, float xoffset, float yoffset, const char *filename) {
    float ratio = (float) height / width;
    float minX = -2.f * scale;
    float minY = minX * ratio;
    minX += xoffset;
    minY -= yoffset;

    float ratioX = 4.f / width * scale;
    float ratioY = 4.f * ratio / height * scale;
    const int LIST_SIZE = width * height * sizeof(float);

    float *data = (float *)malloc(LIST_SIZE);

     // Set the arguments of the kernel
    int argID = 0;
    cl_uint status = clSetKernelArg(kernel, argID++, sizeof(int), (void *)&width);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&ratioX);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&ratioY);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&minX);
    if (status != CL_SUCCESS) {
        printf("clSetKernelArg failed = %d\n", status);
        exit(1);
    }

    status = clSetKernelArg(kernel, argID++, sizeof(float), (void *)&minY);
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
    size_t global_item_size[3] = {256, 256, 16};
    size_t local_item_size[3] = {128, 1, 1};
    status = clEnqueueNDRangeKernel(command_queue, kernel, 3, NULL, global_item_size, local_item_size, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
        printf("clEnqueueNDRangeKernel failed = %d\n", status);
    	exit(1);
    }

    // Read the memory buffer C on the device to the local variable C
	status = clEnqueueReadBuffer(command_queue, data_mem, CL_TRUE, 0, LIST_SIZE, data, 0, NULL, NULL);
    if (status != CL_SUCCESS) {
    	printf("clEnqueueReadBuffer failed = %d\n", status);
        exit(1);
    }

    saveImage(filename, width, height, data, maxIter);
    free(data);
 }