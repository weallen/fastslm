#include "graphics.h"

static void window_focus_callback(GLFWwindow* window, int focused) {
}

static void window_iconify_callback(GLFWwindow* window, int iconified)
{
}

GLFWwindow* InitializeMonitor(int monitor_width, int monitor_height, bool fullscreen) {

	// Initialize OpenGL display
	std::cout << "[DEBUG] Initializing graphics..." << std::endl;
	GLFWwindow* window;
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	// try to find the SLM monitor
	
	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	int slm_monitor = -1; // by default, -1
	for (int i = 0; i < count; ++i) {
		const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
		if (mode->width == monitor_width && mode->height == monitor_height) {
			slm_monitor = i;
		}
	}

	// if found the SLM monitor
	if (slm_monitor > 0) {
		// this is kind of a hack because GLFW doesn't support fullscreen on multiple monitors
		// in the current version of GLFW, taking the focus off the fullscreen window minimizes it
		// instead, we're creating a window without decorations and positioning it over the 1024x768 SLM monitor 
		// that we're assuming is to the right of the primary monitor
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		window = glfwCreateWindow(monitor_width, monitor_height, "Display", NULL, NULL);
		// put window on other screen (assuming smaller monitor is to right of larger monitor)
		glfwSetWindowPos(window, mode->width, 0);

	} else { // otherwise create a window
		window = glfwCreateWindow(monitor_width, monitor_height, "Display", NULL, NULL);
	}
	glfwSetWindowFocusCallback(window, &window_focus_callback);
	glfwSetWindowIconifyCallback(window, &window_iconify_callback);
	
	return window;
}

void DisplayMask(const array& mask) {
}


Pixel* MakeRGBImage(const int M, const int N) {
	Pixel* image = new Pixel[M*N];
	for (int i = 0; i < M; ++i) {
		for (int j = 0; j < N; ++j) {
			image[i*M + j].r = 0;
			image[i*M + j].g = 0;
			image[i*M + j].b = 0;
		}
	}
	return image;
}

void ProcessLUT(const array& phase_mask, const int* lut, Pixel* output) {
	int M = phase_mask.dims(0);
	int N = phase_mask.dims(1);
	array total_bit = round(phase_mask * pow(2,16)/(2*af::Pi));
	float* bits = total_bit.host<float>();
	af::sync();
	for (int i = 0; i < M; ++i) {
		for (int j = 0; j < N; ++j) {
			int a = (int)bits[i*N + j];
			uint16_t x = (uint16_t) lut[a];
			//uint16_t x = (uint16_t)a;
			output[(i*N+j)].g = (uint8_t)(x >> 8); // set red pixel
			output[(i*N+j)].r = (uint8_t)(x & 0x00ff); // set green pixel
		}
	}
	array::free(bits);
}


int* LoadLUT(const std::string& fname) {
	// assumes there are 2**16 (65536)
	std::ifstream fin;
	fin.open(fname.c_str());
	std::string line;
	int* lut = new int[LUT_SIZE];
	int k = 0;
	while(std::getline(fin, line)) {
		std::stringstream lineStream(line);
		std::string cell;
		int z = 0;
		while(std::getline(lineStream, cell, ' ')) {
			if (z = 1) {
				lut[k] = atoi(cell.c_str());
			}
			z++;
		}
		k++;
	}
	return lut;
}

void SLMDisplay::MakeTexture(int M, int N) {
	glGenTextures(1, &id_);
	glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	blank_ = MakeRGBImage(M, N);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, M, N, 0, GL_RGB, GL_UNSIGNED_BYTE, blank_);
}

void SLMDisplay::DrawTexture(const int M, const int N) {
	glBindTexture(GL_TEXTURE_2D, id_);

	float width = M;
	float height = N;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	const GLfloat texture_coordinates[] = {0, 1,
                                       0, 0,
                                       1, 1,
                                       1, 0};
	glTexCoordPointer(2, GL_FLOAT, 0, texture_coordinates);

	const GLfloat vertices[] = {0, height,
								0, 0,
								width, height,
								width, 0};
	glVertexPointer(2, GL_FLOAT, 0, vertices);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void SLMDisplay::InitGraphics(int M, int N, int width, int height) {
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	MakeTexture(M, N);
}

void SLMDisplay::DisplayMask(const Pixel* buffer, int M, int N) {
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, id_);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, M,
		N, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	//GLuint id = MakeTexture(buffer);
	DrawTexture(M, N);
	// alternatively...
	//glDrawPixels(M, N, GL_RGB, GL_UNSIGNED_BYTE, buffer);
}

// makes a debug phasemask from 0...2pi
array DebugMakePhasemask() {
	array range = linspace(0, (float) 2*af::Pi, 512);
	array temp = constant(1, 512, 512);
	for (int i = 0; i < 512; ++i) {
		temp.row(i) = range;
	}
	return temp;
}
