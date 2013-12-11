// fastslm.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <vector>
#include <stdio.h>
#include <thread>

#include <arrayfire.h>

#include <glfw3.h>
#include <gl/GLU.h>

#include "gs.h"
#include "graphics.h"
#include "waveoptics.h"
#include "target.h"
#include "network.h"
#include "control.h"


using namespace af;

const int SLM_res = 512;
const int sim_res_fact = 1;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}


int main(int argc, char** argv) {
	
	try {
		af::deviceset(0);
		af::info();
		
		NetworkHandler nh; // object to receive inputs asynchronously over ZMQ
		SLMControl controller; // object to read 
		Pixel* buffer;
		SLMDisplay display;
		int* lut;

		// Image dimensions (Fixed for now)
		int M = 512;
		int N = 512;
		int Z = 10;
		
		// Load lookup table and make buffer for image
		std::string lutpath = std::string("C:\\Users\\tardigrade\\SLM\\SLM\\SLM2047.lut");
		//std::string lutpath = std::string("C:\\Users\\Admin\\Desktop\\slmscope\\SLM\\SLM2047.lut");

		std::ifstream lutfile(lutpath);
		if (lutfile) {
			std::cout << "[DEBUG] Loading LUT from " << lutpath << "..." << std::endl;
			// TODO Add error handling
			lut = LoadLUT(lutpath);
		} else {
			std::cout << "[ERROR] LUT file at " << lutpath << " not found!" << std::endl;
			exit(-1);
		}

		std::string calibpath = std::string("C:\\Users\\tardigrade\\SLM\\SLM\\cal.txt");
		//std::string calibpath = std::string("C:\\Users\\Admin\\Desktop\\slmscope\\SLM\\cal.txt");
		std::ifstream calibfile(calibpath);
		
		Calibration calib;
		if (calibfile) {
			std::cout << "[DEBUG] Loading calibration from " << calibpath << "..." << std::endl;
			calib = TargetDatabase::LoadCalibration(calibpath);
		} else {
			std::cout << "[ERROR] Calibration file at " << calibpath << " not found!" << std::endl;
			std::cout << "[ERROR] Using no calibration" << std::endl;
		}
		
		// debug calibration
		//calib.dtheta = 0;
		//calib.shiftX = 0;
		//calib.shiftY = 0;
		//calib.scale = 1;

		// warm up arrayfire
		WarmUp(lut);
		
		// Initialize controller
		concurrency::concurrent_queue<std::string>* queue = nh.GetQueue();
		std::cout << "[DEBUG] Initializing controller..." << std::endl;
		controller.Initialize(lut, queue, calib);

		std::string x_galvo_path = std::string("C:\\Users\\tardigrade\\SLM\\x_galvo.txt");
		std::string y_galvo_path = std::string("C:\\Users\\tardigrade\\SLM\\y_galvo.txt");
		int Nsamples = 321; // this should definitely not be hardcoded
		controller.LoadGalvoWaveforms(x_galvo_path, y_galvo_path, Nsamples);

		// Initialize OpenGL display
		std::cout << "[DEBUG] Initializing graphics..." << std::endl;
		GLFWwindow* window;
		if (!glfwInit()) {
			exit(EXIT_FAILURE);
		}

		//window = glfwCreateWindow(M, N, "Display", NULL, NULL);
		// make fullscreen
		int count;
		GLFWmonitor** monitors = glfwGetMonitors(&count);
		int monitor_width = 1024;
		int monitor_height = 768;
		
		/*
		int widthMM, heightMM;
		glfwGetMonitorPhysicalSize(monitors[0], &widthMM, &heightMM);

		if (count > 1) {
			if (widthMM == monitor_width && heightMM == monitor_height) {
				window = glfwCreateWindow(widthMM, heightMM, "Display", monitors[0], NULL);
			}
			else {
				window = glfwCreateWindow(widthMM, heightMM, "Display", monitors[1], NULL);
			}
		} else {
		}*/

		//window = glfwCreateWindow(monitor_width, monitor_height, "Display", glfwGetPrimaryMonitor(), NULL);
		window = glfwCreateWindow(monitor_width, monitor_height, "Display", NULL, NULL);

		if (!window) {
			glfwTerminate();
			std::cout << "[ERROR] Couldn't initialize display." << std::endl;
			exit(EXIT_FAILURE);
		}

		glfwMakeContextCurrent(window);
		glfwSetKeyCallback(window, key_callback);

		display.InitGraphics(M, N, monitor_width, monitor_height);

		// debug stuff
		//controller.DebugInitCells();
		//controller.DebugGenRandomPattern();
		//controller.DebugSingleCell(0.0f);
		//controller.DebugCalibPattern();

		// initialize asych IO
		nh.Connect("127.0.0.1", 9091);
		nh.Connect("127.0.0.1", 9089);

		nh.StartListen();

		timer::start();

		// Main loop
		int frame = 0;
		while (!glfwWindowShouldClose(window)) {		

			controller.Update();
			buffer = controller.CurrentMask();

			display.DisplayMask(buffer, M, N);

			glfwSwapBuffers(window);
			glfwPollEvents();
			++frame;
		}

		delete[] lut;

		double elapsed = timer::stop();
		printf("[DEBUG] Ran at %g (Hz)\n", 1/(elapsed/frame));

		printf("hit [enter]...");
		getchar();

		nh.StopListen();

	} catch (af::exception& e) {
		fprintf(stderr, "%s\n", e.what());
		throw;
	}

	glfwTerminate();


	return 0;
}
