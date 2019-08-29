#include "animation.h"

#include <stdio.h>
#include "color.h"

Animation::Animation(const char *afile) {
	FILE *fp = fopen(afile, "r");
	std::vector<Frame> keyframes;

	int fnum, maxIter;
	float xoffset, yoffset;

    while (!feof(fp)) {
        fscanf(fp, "%d %f %f %d", &fnum, &xoffset, &yoffset, &maxIter);
        keyframes.push_back(Frame(fnum, xoffset, yoffset, maxIter));
    }
    fclose(fp);

    // Interpolate between key frames
    for (int i = 0; i < keyframes.size() - 1; i++) {
    	for (int f = keyframes[i].fnum; f < keyframes[i+1].fnum; f++) {
    		frames.push_back(keyframes[i].lerp(keyframes[i+1], f));
    	}
    }

    frames.push_back(keyframes[keyframes.size() - 1]);
}

Frame Frame::lerp(const Frame&b, int fnum) {
	float t = float(fnum - this->fnum) / float(b.fnum - this->fnum);

	Frame ret;
	ret.fnum = fnum;
	// ret.xoffset = cosineInterpolate(this->xoffset, b.xoffset, t); // this->xoffset + (b.xoffset - this->xoffset) * t;
	// ret.yoffset = cosineInterpolate(this->yoffset, b.yoffset, t); // this->yoffset + (b.yoffset - this->yoffset) * t;
    ret.xoffset = this->xoffset + (b.xoffset - this->xoffset) * t;
    ret.yoffset = this->yoffset + (b.yoffset - this->yoffset) * t;
    ret.maxIter = int(float(this->maxIter) + (float(b.maxIter - this->maxIter) * t));
	return ret;
}
