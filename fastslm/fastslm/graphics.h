#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string>
#include <math.h>
#include <arrayfire.h>


#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/GLU.h>
#include <glfw3.h>

#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>

#include "helper_cuda.h"
#include "common.h"
#include "cudalut.h"

const int LUT_SIZE = 65536;
const char* const DELIMETER = " ";
const int MAX_CHARS_PER_LINE = 512;
const int MAX_TOKENS_PER_LINE = 20;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} Pixel;

static const char *glsl_shader =
    "#version 130\n"
    "out uvec4 FragColor;\n"
    "void main()\n"
    "{"
    "  FragColor = uvec4(gl_Color.xyz * 255.0, 255.0);\n"
    "}\n";


using namespace af;

static void window_focus_callback(GLFWwindow* window, int focused);
static void window_iconify_callback(GLFWwindow* window, int iconified);

GLFWwindow* InitializeMonitor(int monitor_width, int monitor_height, bool fullscreen);

// creates a blank RGB image in RGA format ... allocates memory so don't forget to delete
Pixel* MakeRGBImage(const int M, const int N);

// Deal with LUT
void ProcessLUT(const array& phase_mask, const int* lut, Pixel* buffer);
int* LoadLUT(const std::string& fname);

class SLMDisplay_CUDA 
{
public:
	SLMDisplay_CUDA() : texture_(NULL) {}
	virtual ~SLMDisplay_CUDA() { 
		if (texture_ != NULL) glDeleteTextures(1, &texture_);
	}

	void InitGraphics(int M, int N, int width, int height);
	void DisplayMask(af::array& mask);

private:
	void MakeTexture(int M, int N); 
	void DrawTexture(const int width, const int height);
	void CompileShader();

	GLuint texture_;
	GLuint buffer_;
	GLuint shader_;
	cudaGraphicsResource_t cuda_resource_buffer_;

};

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