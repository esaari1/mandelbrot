#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "color.h"
#include "opencl.h"

#define NUM_THREADS 8

int width = 1000;
int height = 1000;
int maxIter = 100;
int yidx = 0;
float scale = 1.0;
float xoffset = 0;
float yoffset = 0;

void *mandelbrot(void *args) {
	float *data = (float *) args;

	float ratio = (float) height / width;
    float minX = -2.f * scale;
    float minY = minX * ratio;
    minX += xoffset;
    minY -= yoffset;

	// float fourw = 4.0 / width;
	// float fourh = 4.0 / height;

	float ratioX = 4.f / width * scale;
    float ratioY = 4.f * ratio / height * scale;

	while (1) {
		int y = __atomic_fetch_add(&yidx, 1, __ATOMIC_SEQ_CST);
		if (y >= height) {
			break;
		}

		//float im = fourh * y - 2.0;
		float im = ratioY * y + minY;

		for (int x = 0; x < width; x++) {
			// float re = fourw * x - 2.0;
			float re = ratioX * x + minX;

			float iter = 0;
			float x2 = 0;
			float y2 = 0;
			float xNew = 0;

			while (((x2*x2 + y2*y2) <= 4) && (iter < maxIter)) {
				xNew = x2*x2 - y2*y2 + re;
				y2 = 2*x2*y2 + im;
				x2 = xNew;
				iter++;
			}

			 if ( iter < maxIter ) {
				float log_zn = log( x2*x2 + y2*y2 ) / 2.0;
				float nu = log( log_zn / log(2.0) ) / log(2.0);
				iter = iter + 1.0 - nu;
			}

			data[y * width + x] = iter;
		}
	}

	return NULL;
}

void runCPU(int maxIter) {
	float *data = (float *)malloc(width * height *sizeof(float));
	pthread_t threads[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, mandelbrot, data);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	saveImage("test.png", width, height, data, maxIter);
	free(data);
}

int main(int argc, char **argv) {
    bool cpu = false;
    bool animate = false;
    int frame;
    float x, y;

    int c;

    while ((c = getopt(argc, argv, "m:cw:h:af:x:y:")) != -1) {
        switch(c) {
            case 'm':
                maxIter = atoi(optarg);
                break;
            case 'c':
            	cpu = true;
            	break;
            case 'w':
            	width = atoi(optarg);
            	break;
            case 'h':
            	height = atoi(optarg);
            	break;
            case 'a':
            	animate = true;
            	break;
            case 'f':
            	frame = atoi(optarg);
            	break;
            case 'x':
            	x = atof(optarg);
            	break;
            case 'y':
            	y = atof(optarg);
            	break;
        }
    }

	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);

    if (cpu) {
    	xoffset = x;
    	yoffset = y;
		if (frame > 1) {
			scale = pow(0.9349, frame-1);
		}

    	runCPU(maxIter);
    } else {
    	OpenCL ocl(width, height);
    	ocl.runGPU(maxIter, animate, frame, x, y);
    }

	clock_gettime(CLOCK_MONOTONIC, &finish);
	double elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

	printf("RUNTIME = %f seconds\n", elapsed);

    return 0;
}
