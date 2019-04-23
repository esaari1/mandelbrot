#include <stdio.h>
#include <string.h>
#include <OpenCL/cl.h>

char *getDeviceParamString(cl_device_id deviceID, int paramID) {
	size_t paramSize;
	clGetDeviceInfo(deviceID, paramID, 0, NULL, &paramSize);

	char *value = (char *) malloc((paramSize + 1) * sizeof(char));
	clGetDeviceInfo(deviceID, paramID, paramSize, value, NULL);
	return value;
}

void getDeviceInfo(cl_device_id deviceID) {
	char *value = getDeviceParamString(deviceID, CL_DEVICE_NAME);
	printf("Name = %s\n", value);
	free(value);

	cl_uint uival;
	cl_bool flag;
	size_t sval;

	clGetDeviceInfo(deviceID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &uival, NULL);
	printf("Compute Units = %d\n", uival);

	clGetDeviceInfo(deviceID, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &flag, NULL);
	printf("Image support = %d\n", flag);

	clGetDeviceInfo(deviceID, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &uival, NULL);
	printf("Max clock = %d Mhz\n", uival);

	clGetDeviceInfo(deviceID, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &sval, NULL);
	printf("Max work group size = %lu\n", sval);

	clGetDeviceInfo(deviceID, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &uival, NULL);

	size_t *dims = (size_t *) malloc(sizeof(size_t) * uival);
	clGetDeviceInfo(deviceID, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * uival, dims, NULL);

	size_t totalDim = 1;
	printf("Max work item sizes = (");
	for (int i = 0; i < uival; i++) {
		printf("%lu ", dims[i]);
		totalDim *= dims[i];
	}
	printf(") = %lu\n", totalDim);
	free(dims);

	value = getDeviceParamString(deviceID, CL_DEVICE_EXTENSIONS);
	char *exts = strtok(value, " ");
	printf("Extensions:\n");

	while (exts != NULL) {
		printf("\t%s\n", exts);
		exts = strtok(NULL, " ");
	}
	free(value);
}

int main() {
	cl_uint numPlatforms = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
	printf("%d platforms\n", numPlatforms);

	cl_platform_id *platforms = (cl_platform_id *) malloc(numPlatforms * sizeof(cl_platform_id));
	clGetPlatformIDs(numPlatforms, platforms, NULL);

	cl_uint numDevices;
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
	printf("%d devices\n", numDevices);

	cl_device_id *devices = (cl_device_id *) malloc(numDevices * sizeof(cl_device_id));
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);

	for (int i = 0; i < numDevices; i++) {
		printf("\n");
		getDeviceInfo(devices[i]);
	}

	return 0;
}
