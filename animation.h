#ifndef _ANIMATION_H
#define _ANIMATION_H

#include <vector>

class Frame {
public:
	int fnum;
	double xoffset;
	double yoffset;
	int maxIter;

	Frame() {
		Frame(0, 0, 0, 0);
	}

	Frame(int f, double x, double y, int maxIter) {
		this->fnum = f;
		this->xoffset = x;
		this->yoffset = y;
		this->maxIter = maxIter;
	}

	Frame lerp(const Frame&b, int fnum);
};

class Animation {
public:
	Animation(const char *afile = "frames.txt");
	std::vector<Frame> frames;
};

#endif
