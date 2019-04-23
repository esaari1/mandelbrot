#include <stdio.h>
#include <OpenCL/cl.h>
#include <time.h>
#include <unistd.h>

#include "color.h"

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

// int main(int argc, char **argv) {
//     int maxIter = 100;// 100000;
//     int width = 1000;
//     int height = 1000;
//     const char *filename = "test.png";

//     int c;

//     while ((c = getopt(argc, argv, "m:f:")) != -1) {
//         switch(c) {
//             case 'm':
//                 maxIter = atoi(optarg);
//                 break;
//             case 'f':
//                 filename = optarg;
//                 break;
//         }
//     }

void runGPU(int width, int height, int maxIter, const char *filename) {
	cl_uint numPlatforms = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);

	cl_platform_id *platforms = (cl_platform_id *) malloc(numPlatforms * sizeof(cl_platform_id));
	clGetPlatformIDs(numPlatforms, platforms, NULL);

	cl_uint numDevices;
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);

	cl_device_id *devices = (cl_device_id *) malloc(numDevices * sizeof(cl_device_id));
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);

	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);

    // Load the kernel source code into the array source_str
    size_t source_size;
    char *source_str = readKernel(source_size);

    const int LIST_SIZE = width * height * sizeof(unsigned int);

    unsigned int *data = (unsigned int *)malloc(LIST_SIZE);

    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, 1, &devices[1], NULL, NULL, &status);
 
    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, devices[1], 0, &status);
 
    // Create memory buffers on the device for each vector 
    cl_mem data_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, LIST_SIZE, NULL, &status);

    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &status);
    printf("Create status = %d\n", status);
    if (status != CL_SUCCESS) {
    	exit(1);
    }

    // Build the program
    status = clBuildProgram(program, 1, &devices[1], NULL, NULL, NULL);
    printf("Build status = %d\n", status);
    if (status != CL_SUCCESS) {
		size_t length;
		clGetProgramBuildInfo(program, devices[1], CL_PROGRAM_BUILD_LOG, 0, NULL, &length);
        char *buffer = (char *)malloc(length * sizeof(char));

        clGetProgramBuildInfo(program, devices[1], CL_PROGRAM_BUILD_LOG, length, buffer, NULL);
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
    clGetKernelWorkGroupInfo(kernel, devices[1], CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &kernelSize, NULL);
    printf("Kernel work group size = %lu\n", kernelSize);
 
    // Set the arguments of the kernel
    status = clSetKernelArg(kernel, 0, sizeof(int), (void *)&maxIter);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&data_mem);
    printf("clSetKernelArg status = %d\n", status);
    if (status != CL_SUCCESS) {
        exit(1);
    }

    fflush(stdout);
 
    // Execute the OpenCL kernel on the list
    size_t global_item_size[3] = {100, 100, 100};
    size_t local_item_size[3] = {5, 5, 5};
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