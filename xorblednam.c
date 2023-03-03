/*
	Copyright (C) 2022-2023 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License version 2 as published by the
	Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <png.h>
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <float.h>
#include <math.h>

///////////////////////////////////CONFIGURATION
#define MAX_ITERATIONS (1000)
#define WIDTH          (8192)
#define HEIGHT         (8192)
#define ZOOM_OUT       (3.0)
#define CENTERX        (-0.5)
#define CENTERY        (0.0)
///////////////////////////////////

#define ZOOM_OUTX      (WIDTH > HEIGHT ? ZOOM_OUT : ((ZOOM_OUT * WIDTH) / HEIGHT))
#define ZOOM_OUTY      (WIDTH > HEIGHT ? ((ZOOM_OUT * HEIGHT) / WIDTH) : ZOOM_OUT)
#define FROMX          (CENTERX - ZOOM_OUTX / 2)
#define TOX            (CENTERX + ZOOM_OUTX / 2)
#define FROMY          (- CENTERY - ZOOM_OUTY / 2)
#define TOY            (- CENTERY + ZOOM_OUTY / 2)
#define STEPX          ((TOX - FROMX) / WIDTH)
#define STEPY          ((TOY - FROMY) / HEIGHT)
#define NUMCOLORS      (sizeof(colors) / 3)

static const uint8_t colors[] = {
	 66,  30,  15,
	 25,   7,  26,
	  9,   1,  47,
	  4,   4,  73,
	  0,   7, 100,
	 12,  44, 138,
	 24,  82, 177,
	 57, 125, 209,
	134, 181, 229,
	211, 236, 248,
	241, 233, 191,
	248, 201,  95,
	255, 170,   0,
	204, 128,   0,
	153,  87,   0,
	106,  52,   3
};

static void
die(const char *fmt, ...)
{
	va_list args;

	fputs("xorblednam: ", stderr);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	exit(1);
}

static void
usage(void)
{
	puts("usage: xorblednam [-hv] [-buddhabrot] [-julia] [-mandelbrot] [-burning_ship]");
	exit(0);
}

static void
version(void)
{
	puts("xorblednam version "VERSION);
	exit(0);
}

static long double
complex_magnitude_squared(long double complex c)
{
	long double r, i;

	r = creall(c);
	i = cimagl(c);

	return r * r + i * i;
}

static void
complex_to_coord(long double complex c, int *x, int *y)
{
	*x = (WIDTH * ((creall(c) - FROMX))) / (TOX - FROMX);
	*y = (HEIGHT * ((cimagl(c) - FROMY))) / (TOY - FROMY);
}

static void
save_buffer_as_png(const char *path, uint8_t *buffer, int width, int height)
{
	int x, y;
	FILE *fp;
	png_struct *png;
	png_info *pnginfo;
	png_byte *row;

	if (NULL == (fp = fopen(path, "wb")))
		die("fopen failed: %s", strerror(errno));

	if (NULL == (png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		die("png_create_write_struct failed");

	if (NULL == (pnginfo = png_create_info_struct(png)))
		die("png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png)) != 0)
		die("aborting due to libpng error");

	png_init_io(png, fp);

	png_set_IHDR(
		png, pnginfo, width, height, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
	);

	png_write_info(png, pnginfo);
	png_set_compression_level(png, 3);

	row = malloc(width * 3);

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			row[x*3+0] = buffer[3*(y*width+x)+0];
			row[x*3+1] = buffer[3*(y*width+x)+1];
			row[x*3+2] = buffer[3*(y*width+x)+2];
		}
		png_write_row(png, row);
	}

	png_write_end(png, NULL);
	png_free_data(png, pnginfo, PNG_FREE_ALL, -1);
	png_destroy_info_struct(png, &pnginfo);
	png_destroy_write_struct(&png, NULL);
	fclose(fp);
	free(row);
}

static void
mandelbrot(void)
{
	int iter;
	long double x, y;
	long double complex c, z;
	uint8_t *buffer, *p;

	p = buffer = calloc(WIDTH*HEIGHT*3, 1);

	for (y = FROMY; TOY - y > DBL_EPSILON; y += STEPY) {
		for (x = FROMX; TOX - x > DBL_EPSILON; x += STEPX, p += 3) {
			c = x + y * I;
			z = c;

			for (iter = 0; iter < MAX_ITERATIONS; ++iter) {
				z = z * z + c;
				if (complex_magnitude_squared(z) > 4) {
					p[0] = colors[(iter % NUMCOLORS) * 3];
					p[1] = colors[(iter % NUMCOLORS) * 3 + 1];
					p[2] = colors[(iter % NUMCOLORS) * 3 + 2];
					break;
				}
			}
		}
	}

	save_buffer_as_png("mandelbrot.png", buffer, WIDTH, HEIGHT);

	free(buffer);
}

static void
buddhabrot(void)
{
	long double x, y;
	long double complex c, z;
	int iter, bx, by;
	long double complex orbit[MAX_ITERATIONS];
	uint8_t *buffer;
	int *heatmap;

	buffer = calloc(WIDTH*HEIGHT*3, 1);
	heatmap = calloc(WIDTH*HEIGHT, sizeof(int));

	for (y = FROMY; TOY - y > DBL_EPSILON; y += STEPY) {
		for (x = FROMX; TOX - x > DBL_EPSILON; x += STEPX) {
			c = x + y * I;
			z = c;

			for (iter = 0; iter < MAX_ITERATIONS;) {
				z = z * z + c;
				orbit[iter++] = z;
				if (complex_magnitude_squared(z) > 4) {
					while (iter-- > 0) {
						complex_to_coord(orbit[iter], &bx, &by);
						if (bx >= 0 && bx < WIDTH && by >= 0 && by < HEIGHT)
							++heatmap[by*WIDTH+bx];
					}
					break;
				}
			}
		}
	}

	for (by = 0; by < HEIGHT; ++by) {
		for (bx = 0; bx < WIDTH; ++bx) {
			if (heatmap[by*WIDTH+bx] > 0) {
				buffer[(by*WIDTH+bx)*3 + 0] = colors[(heatmap[by*WIDTH+bx]%NUMCOLORS)*3 + 0];
				buffer[(by*WIDTH+bx)*3 + 1] = colors[(heatmap[by*WIDTH+bx]%NUMCOLORS)*3 + 1];
				buffer[(by*WIDTH+bx)*3 + 2] = colors[(heatmap[by*WIDTH+bx]%NUMCOLORS)*3 + 2];
			}
		}
	}

	save_buffer_as_png("buddhabrot.png", buffer, WIDTH, HEIGHT);

	free(buffer);
	free(heatmap);
}

static void
julia(void)
{
	int iter;
	long double x, y;
	long double complex c, z;
	uint8_t *buffer, *p;

	p = buffer = calloc(WIDTH*HEIGHT*3, 1);

	for (y = FROMY; TOY - y > DBL_EPSILON; y += STEPY) {
		for (x = FROMX; TOX - x > DBL_EPSILON; x += STEPX, p += 3) {
			c = -0.7269 + 0.1889 * I;
			z = x + y * I;

			for (iter = 0; iter < MAX_ITERATIONS; ++iter) {
				z = z * z + c;
				if (complex_magnitude_squared(z) > 4) {
					p[0] = colors[(iter % NUMCOLORS) * 3];
					p[1] = colors[(iter % NUMCOLORS) * 3 + 1];
					p[2] = colors[(iter % NUMCOLORS) * 3 + 2];
					break;
				}
			}
		}
	}

	save_buffer_as_png("julia.png", buffer, WIDTH, HEIGHT);

	free(buffer);
}

static void
burning_ship(void)
{
	int iter;
	long double x, y;
	long double complex c, z;
	uint8_t *buffer, *p;

	p = buffer = calloc(WIDTH*HEIGHT*3, 1);

	for (y = FROMY; TOY - y > DBL_EPSILON; y += STEPY) {
		for (x = FROMX; TOX - x > DBL_EPSILON; x += STEPX, p += 3) {
			c = x + y * I;
			z = c;

			for (iter = 0; iter < MAX_ITERATIONS; ++iter) {
				z = cpow(fabsl(creall(z)) + fabsl(cimagl(z)) * I, 2) + c;
				if (complex_magnitude_squared(z) > 4) {
					p[0] = colors[(iter % NUMCOLORS) * 3];
					p[1] = colors[(iter % NUMCOLORS) * 3 + 1];
					p[2] = colors[(iter % NUMCOLORS) * 3 + 2];
					break;
				}
			}
		}
	}

	save_buffer_as_png("burning_ship.png", buffer, WIDTH, HEIGHT);

	free(buffer);
}

int
main(int argc, char **argv)
{
	if (argc != 2 || !strcmp(argv[1], "-h")) usage();
	else if (!strcmp(argv[1], "-v")) version();
	else if (!strcmp(argv[1], "-buddhabrot")) buddhabrot();
	else if (!strcmp(argv[1], "-julia")) julia();
	else if (!strcmp(argv[1], "-mandelbrot")) mandelbrot();
	else if (!strcmp(argv[1], "-burning_ship")) burning_ship();
	else usage();

	return 0;
}
