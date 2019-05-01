#include "animation.h"

#include <stdio.h>

Animation::Animation() {
	FILE *fp = fopen("frames.txt", "r");
	std::vector<Frame> keyframes;

	int fnum;
	float scale, xoffset, yoffset;

    while (!feof(fp)) {
        fscanf(fp, "%d %f %f %f", &fnum, &scale, &xoffset, &yoffset);
        keyframes.push_back(Frame(fnum, scale, xoffset, yoffset));
    }
    fclose(fp);

    // Interpolate between key frames
    for (int i = 0; i < keyframes.size() - 1; i++) {
    	for (int f = keyframes[i].fnum; f < keyframes[i+1].fnum; f++) {
    		frames.push_back(keyframes[i].lerp(keyframes[i+1], f));
    	}
    }

    frames.push_back(keyframes[keyframes.size() - 1]);

    for (int i = 0; i < frames.size(); i++) {
    	printf("%d - %d %f\n", i, frames[i].fnum, frames[i].scale);
    }
}

Frame Frame::lerp(const Frame&b, int fnum) {
	float t = float(fnum - this->fnum) / float(b.fnum - this->fnum);

	Frame ret;
	ret.fnum = fnum;
	ret.scale = this->scale + (b.scale - this->scale) * t;
	ret.xoffset = this->xoffset + (b.xoffset - this->xoffset) * t;
	ret.yoffset = this->yoffset + (b.yoffset - this->yoffset) * t;
	return ret;
}
