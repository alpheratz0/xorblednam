#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <png.h>
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <float.h>

#if 0
#define DEBUG
#endif

#define MAX_ITERATIONS (1000)
#define WIDTH          (8192)
#define HEIGHT         (8192)
#define BUFFER_SIZE    (WIDTH*HEIGHT*3)
#define FROMX          (-2.0)
#define TOX            (1.0)
#define FROMY          (-1.5)
#define TOY            (1.5)
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
	puts("usage: xorblednam [-bhmv]");
	exit(0);
}

static void
version(void)
{
	puts("xorblednam version "VERSION);
	exit(0);
}

static double complex
complex_mult(double complex a, double complex b)
{
	double ar, ai, br, bi;

	ar = creal(a); br = creal(b);
	ai = cimag(a); bi = cimag(b);

	return ar * br - ai * bi + (ar * bi + br * ai) * I;
}

static double complex
complex_add(double complex a, double complex b)
{
	double ar, ai, br, bi;

	ar = creal(a); br = creal(b);
	ai = cimag(a); bi = cimag(b);

	return ar + br + (ai + bi) * I;
}

static double
complex_unsqrt_magnitude(double complex c)
{
	double r, i;

	r = creal(c);
	i = cimag(c);

	return r * r + i * i;
}

static void
complex_to_coord(double complex c, int *x, int *y)
{
	*x = (WIDTH * ((creal(c) - FROMX))) / (TOX - FROMX);
	*y = (HEIGHT * ((cimag(c) - FROMY))) / (TOY - FROMY);
}

static bool
belongs_to_mandelbrot_set(double complex c, int *iter)
{
	double complex z;

	z = c;

	for (*iter = 0; *iter < MAX_ITERATIONS; ++*iter) {
		z = complex_add(complex_mult(z, z), c);
		if (complex_unsqrt_magnitude(z) > 4)
			return true;
	}

	return false;
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
	png_destroy_write_struct(&png, NULL);
	fclose(fp);
	free(row);
}

static void
mandelbrot(void)
{
	int iter;
	double x, y;

#if BUFFER_SIZE >= 900000
	uint8_t *buffer, *p;
	buffer = calloc(BUFFER_SIZE, 1);
#else
	uint8_t buffer[BUFFER_SIZE] = {0}, *p;
#endif

	p = &buffer[0];

	for (y = FROMY; TOY - y > DBL_EPSILON; y += STEPY) {
#ifdef DEBUG
		printf("begin row %d\n", (p - &buffer[0]) / (WIDTH * 3));
#endif
		for (x = FROMX; TOX - x > DBL_EPSILON; x += STEPX, p += 3) {
			if (belongs_to_mandelbrot_set(x+y*I, &iter)) {
				p[0] = colors[(iter % NUMCOLORS) * 3];
				p[1] = colors[(iter % NUMCOLORS) * 3 + 1];
				p[2] = colors[(iter % NUMCOLORS) * 3 + 2];
			}
		}
	}

	save_buffer_as_png("mandelbrot.png", buffer, WIDTH, HEIGHT);

#if BUFFER_SIZE >= 900000
	free(buffer);
#endif
}

static void
buddhabrot(void)
{
	double x, y;
	double complex c, z;
	int iter, bx, by;

#if BUFFER_SIZE >= 900000
	uint8_t *buffer;
	int *heatmap;

	buffer = calloc(BUFFER_SIZE, 1);
	heatmap = calloc(WIDTH*HEIGHT, sizeof(int));
#else
	uint8_t buffer[BUFFER_SIZE] = {0};
	int heatmap[WIDTH*HEIGHT] = {0};
#endif

	for (y = FROMY; TOY - y > DBL_EPSILON; y += STEPY) {
		for (x = FROMX; TOX - x > DBL_EPSILON; x += STEPX) {
			c = x+y*I;
			if (belongs_to_mandelbrot_set(c, &iter)) {
				z = x+y*I;

				for (iter = 0; iter < MAX_ITERATIONS; ++iter) {
					z = complex_add(complex_mult(z, z), c);
					complex_to_coord(z, &bx, &by);

					if (bx >= 0 && bx < WIDTH && by >= 0 && by < HEIGHT)
						++heatmap[by*WIDTH+bx];

					if (complex_unsqrt_magnitude(z) > 4)
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

#if BUFFER_SIZE >= 900000
	free(buffer);
	free(heatmap);
#endif
}

int
main(int argc, char **argv)
{
	while (++argv, --argc > 0) {
		if ((*argv)[0] == '-' && (*argv)[1] != '\0' && (*argv)[2] == '\0') {
			switch ((*argv)[1]) {
				case 'b': buddhabrot(); break;
				case 'h': usage(); break;
				case 'm': mandelbrot(); break;
				case 'v': version(); break;
				default: die("invalid option %s", *argv); break;
			}
		} else {
			die("unexpected argument: %s", *argv);
		}
	}

	return 0;
}
