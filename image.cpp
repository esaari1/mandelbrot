#include <stdlib.h>
#include <png.h>

#include "color.h"

void abort_(const char * s) {
	fprintf(stderr, "%s\n", s);
	exit(1);
}

void saveImage(const char *filename, int width, int height, unsigned int *data, unsigned int maxIter) {
	HSVMap colorMap;

	int bit_depth = 8;
	int color_type = PNG_COLOR_TYPE_RGB;

	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		abort_("Error opening file");
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose(fp);
		abort_("Error png_create_write_struct");
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose(fp);
		abort_("png_create_info_struct");
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		abort_("setjmp");
	}

	png_init_io(png_ptr, fp);

	/* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[write_png_file] Error during writing header");

	png_set_IHDR(png_ptr, info_ptr, width, height,
		bit_depth, color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[write_png_file] Error during writing bytes");

    png_bytep row = (png_bytep) malloc(3 * width * sizeof(png_byte));

	// Write image data
	int x, y;
	for (y = 0 ; y < height ; y++) {
		for (x = 0 ; x < width ; x++) {
			HSV h = colorMap.lerp((float) data[y * width + x] / (float) maxIter);
			RGB rgb = h.ToRGB();

			row[x * 3] = rgb.r; // data[y * width + x];
			row[x * 3 + 1] = rgb.g; // data[y * width + x];
			row[x * 3 + 2] = rgb.b; // data[y * width + x];
		}
		png_write_row(png_ptr, row);
	}

	free(row);

    /* end write */
    if (setjmp(png_jmpbuf(png_ptr))) {
		abort_("[write_png_file] Error during end of write");
    }

    png_write_end(png_ptr, NULL);

	fclose(fp);
}
