#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "color.h"
#include "opencl.h"
#include "animation.h"

#define NUM_THREADS 8

int width = 1000;
int height = 1000;
double maxIter = 100;
int yidx = 0;
double scale = 1.0;
double xoffset = 0;
double yoffset = 0;

void *mandelbrot(void *args) {
	double *data = (double *) args;

	double ratio = (double) height / width;
    double minX = -2.f * scale;
    double minY = minX * ratio;
    minX += xoffset;
    minY -= yoffset;

	double ratioX = 4.0 / width * scale;
    double ratioY = 4.0 * ratio / height * scale;

	while (1) {
		int y = __atomic_fetch_add(&yidx, 1, __ATOMIC_SEQ_CST);
		if (y >= height) {
			break;
		}

		double im = ratioY * y + minY;

		for (int x = 0; x < width; x++) {
			double re = ratioX * x + minX;

			unsigned int iter = 0;
			double x2 = 0;
			double y2 = 0;
			double xNew = 0;

			while (((x2*x2 + y2*y2) <= 4) && (iter < maxIter)) {
				xNew = x2*x2 - y2*y2 + re;
				y2 = 2*x2*y2 + im;
				x2 = xNew;
				iter++;
			}

			 if ( iter < maxIter ) {
				double log_zn = log( x2*x2 + y2*y2 ) / 2.0;
				double nu = log( log_zn / log(2.0) ) / log(2.0);
				data[y * width + x] = (double) iter + 1.0 - nu;
			} else {
				data[y * width + x] = iter;
			}
		}
	}

	return NULL;
}

void cpuImage(const char *fname, double *data) {
	pthread_t threads[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, mandelbrot, data);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	saveImage(fname, width, height, data, maxIter);
}

void runCPU(bool animate, const char *afile, int frame, float x, float y) {
	double *data = (double *)malloc(width * height *sizeof(double));

    if (!animate) {
        if (frame > 1) {
            scale = pow(0.9349, frame-1);
        }
        xoffset = x;
        yoffset = y;
        cpuImage("test.png", data);
    } else {
        Animation a(afile);
        char filename[32];

        for (int i = 0; i < a.frames.size(); i++) {
            sprintf(filename, "./output/frame-%d.png", (i+1));
            printf("Frame %d of %lu %lf\n", (i+1), a.frames.size(), maxIter);
            xoffset = a.frames[i].xoffset;
            yoffset = a.frames[i].yoffset;
            cpuImage(filename, data);

            scale *= 0.9349;
            yidx = 0;
            maxIter *= 1.0137;
        }
    }

    free(data);
}


int main(int argc, char **argv) {
    bool cpu = true;
    bool animate = false;
    int frame;
    float x, y;
    const char *afile;

    int c;

    while ((c = getopt(argc, argv, "m:gw:h:a:f:x:y:")) != -1) {
        switch(c) {
            case 'm':
                maxIter = atoi(optarg);
                break;
            case 'g':
            	cpu = false;
            	break;
            case 'w':
            	width = atoi(optarg);
            	break;
            case 'h':
            	height = atoi(optarg);
            	break;
            case 'a':
            	animate = true;
                afile = optarg;
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
    	runCPU(animate, afile, frame, x, y);
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

// ./mandelbrot -c -x -0.77568377 -y 0.13646737 -m 3000 -f 320