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
#include <gl/GLU.h>

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

class SLMDisplay
{
public:
	SLMDisplay() : blank_(NULL) {}
	virtual ~SLMDisplay() { if (blank_ != NULL) { delete[] blank_; } }

	// M and N are the size of the phasemask
	// width and height are the size of the screen
	void InitGraphics(int M, int N, int width, int height);
	void DisplayMask(const Pixel* buffer, int M, int N);

private:
	// OpenGL stuff
	void MakeTexture(int M, int N);
	void DrawTexture(const int width, const int height);
	GLuint id_;
	Pixel* blank_;
};
// Util functions
array DebugMakePhasemask();

#endif	