#include "graphics.h"

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
			uint16_t x = lut[a];
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

GLuint MakeTexture(const Pixel* pixmap) {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512,
                 512, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 pixmap);
    return id;

}

void DrawTexture(const GLuint id, const int width, const int height) {
	glBindTexture(GL_TEXTURE_2D, id);
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

void InitGraphics() {
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 512, 512, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}

void DisplayMask(const Pixel* buffer, int M, int N) {
	glClear(GL_COLOR_BUFFER_BIT);
	GLuint id = MakeTexture(buffer);
	DrawTexture(id, M, N);
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
