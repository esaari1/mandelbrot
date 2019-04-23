#include "color.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

RGB HSV::ToRGB() {
    float c = this->v * this->s;
    float x = c * (1.0 - abs(fmod(this->h / 60.0, 2) - 1));
    float m = this->v - c;

    RGB rgb;

    int region = this->h / 60;
    switch (region) {
        case 0:
            rgb.r = (c + m) * 255; rgb.g = (x + m) * 255; rgb.b = m * 255;
            break;
        case 1:
            rgb.r = (x + m) * 255; rgb.g = (c + m) * 255; rgb.b = m * 255;
            break;
        case 2:
            rgb.r = m * 255; rgb.g = (c + m) * 255; rgb.b = (x + m) * 255;
            break;
        case 3:
            rgb.r = m * 255; rgb.g = (x + m) * 255; rgb.b = (c + m) * 255;
            break;
        case 4:
            rgb.r = (x + m) * 255; rgb.g = m * 255; rgb.b = (c + m) * 255;
            break;
        default:
            rgb.r = (c + m) * 255; rgb.g = m * 255; rgb.b = (x + m) * 255;
            break;
    }
    return rgb;
}

HSV HSV::lerp(const HSV& b, float t) {
    HSV ret;
    ret.h = int(this->h + float(b.h - this->h) * t);
    ret.s = this->s + (b.s - this->s) * t;
    ret.v = this->v + (b.v - this->v) * t;
    return ret;
}

HSVMap::HSVMap() {
    FILE *fp = fopen("colors.txt", "r");
    int h;
    float s, v;
    while (!feof(fp)) {
        fscanf(fp, "%d %f %f", &h, &s, &v);
        this->colors.push_back(HSV(h, s, v));
    }
    fclose(fp);
}

HSV HSVMap::lerp(float t) {
    int idx = (this->colors.size() - 1) * t;
    if (idx == this->colors.size()-1) {
        return this->colors[this->colors.size() - 1];
    }

    return this->colors[idx].lerp(this->colors[idx + 1], t);
}
