#include <math.h>

#include "color.h"

Spline::Spline() {
	std::vector<double> x(6), r(6), g(6), b(6);

	x[0] = 0; x[1] = 0.16; x[2] = 0.42; x[3] = 0.6425; x[4] = 0.8575; x[5] = 1;
	r[0] = 0; r[1] = 32; r[2] = 237; r[3] = 255; r[4] = 0; r[5] = 0;
	g[0] = 7; g[1] = 107; g[2] = 255; g[3] = 170; g[4] = 2; g[5] = 0;
	b[0] = 100; b[1] = 203; b[2] = 255; b[3] = 0; b[4] = 0; b[5] = 0;

	redSpline.set_points(x, r);
	greenSpline.set_points(x, g);
	blueSpline.set_points(x, b);
}

RGB Spline::spline(float t) {
	RGB rgb;
	rgb.r = fmin(redSpline(t), 255);
	rgb.g = fmin(greenSpline(t), 255);
	rgb.b = fmin(blueSpline(t), 255);
	return rgb;
}

// int main() {
// 	double t = 0.1;

// 	Spline spline;
// 	RGB rgb = spline.spline(t);

// 	printf("spline at %f is %d, %d, %d\n", t, rgb.r, rgb.g, rgb.b);
// 	return 0;
// }
