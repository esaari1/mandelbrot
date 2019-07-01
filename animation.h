#ifndef _ANIMATION_H
#define _ANIMATION_H

#include <vector>

class Frame {
public:
	int fnum;
	float xoffset;
	float yoffset;

	Frame() {
		Frame(0, 0, 0);
	}

	Frame(int f, float x, float y) {
		this->fnum = f;
		this->xoffset = x;
		this->yoffset = y;
	}

	Frame lerp(const Frame&b, int fnum);
};

class Animation {
public:
	Animation();
	std::vector<Frame> frames;
};

#endif
