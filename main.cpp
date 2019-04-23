#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "color.h"

void runGPU(int width, int height, int maxIter, const char *filename);

#define NUM_THREADS 1

int width = 1000;
int height = 1000;
int maxIter = 0;
int yidx = 0;

void *mandelbrot(void *args) {
	unsigned int *data = (unsigned int *) args;

	float fourw = 4.0 / width;
	float fourh = 4.0 / height;

	while (1) {
		int y = __atomic_fetch_add(&yidx, 1, __ATOMIC_SEQ_CST);
		if (y >= height) {
			break;
		}

		float im = fourh * y - 2.0;

		for (int x = 0; x < width; x++) {
			float re = fourw * x - 2.0;

			int iter = 0;
			float x2 = 0;
			float y2 = 0;
			float xNew = 0;

			while (((x2*x2 + y2*y2) <= 4) && (iter < maxIter)) {
				xNew = x2*x2 - y2*y2 + re;
				y2 = 2*x2*y2 + im;
				x2 = xNew;
				iter++;
			}

			data[y * width + x] = iter;
		}
	}

	return NULL;
}

void runCPU(int maxIter, const char *filename) {
	unsigned int *data = (unsigned int *)malloc(width * height *sizeof(unsigned int));
	pthread_t threads[NUM_THREADS];

	struct timespec start, finish;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, mandelbrot, data);
	}
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &finish);
	double elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

	printf("%f\n", elapsed);

	saveImage(filename, width, height, data, maxIter);
	free(data);
}

int main(int argc, char **argv) {
    maxIter = 100;
    const char *filename = "test.png";
    bool cpu = false;

    int c;

    while ((c = getopt(argc, argv, "m:f:c")) != -1) {
        switch(c) {
            case 'm':
                maxIter = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'c':
            	cpu = true;
            	break;
        }
    }

    if (cpu) {
    	runCPU(maxIter, filename);
    } else {
    	runGPU(width, height, maxIter, filename);
    }

    return 0;
}