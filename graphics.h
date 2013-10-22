#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string>
#include <math.h>
#include <arrayfire.h>

#include <gl/GL.h>

#include "common.h"

const int LUT_SIZE = 65536;
const char* const DELIMETER = " ";
const int MAX_CHARS_PER_LINE = 512;
const int MAX_TOKENS_PER_LINE = 20;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} Pixel;

using namespace af;

// creates a blank RGB image in RGA format ... allocates memory so don't forget to delete
Pixel* MakeRGBImage(const int M, const int N);

// Deal with LUT
void ProcessLUT(const array& phase_mask, const int* lut, Pixel* buffer);
int* LoadLUT(const std::string& fname);

// OpenGL stuff
GLuint MakeTexture(const Pixel* pixmap);
void DrawTexture(const GLuint id, const int width, const int height);
void InitGraphics();
void DisplayMask(const Pixel* buffer, int M, int N);

// Util functions
array MakeDebugPhasemask();

#endif	