#ifndef _COLOR_H
#define _COLOR_H

#include <vector>
#include "spline.h"

typedef struct RGB {
	int r;
	int g;
	int b;
} RGB;

class HSV {
public:
	int h;
	float s;
	float v;

public:
	HSV() {
		HSV(0, 0, 0);
	}

	HSV(int h, float s, float v) {
		set(h, s, v);
	}

	void set(int h, float s, float v) {
		this->h = h;
		this->s = s;
		this->v = v;
	}

	HSV lerp(const HSV& b, float t);
	RGB ToRGB();
};

class HSVMap {
	std::vector<HSV> colors;

public:
	HSVMap();

	HSV lerp(float t);
};

void saveImage(const char *filename, int width, int height, double *data, double maxIter);
float cosineInterpolate(float y1, float y2, float mu);

class Spline {
	tk::spline redSpline;
	tk::spline greenSpline;
	tk::spline blueSpline;

public:
	Spline();
	RGB spline(double t);
};

#endif
