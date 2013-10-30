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

		// Image dimensions (Fixed for now)
		int M = 512;
		int N = 512;
		int Z = 10;

		//TargetDatabase td(M, N, Z); // object to store set of cell positions

		// load some test targets
		//for (int i = 0; i < 1000; ++i) {
		//	float x = (rand() % 512)/512.0;
		//	float y = (rand() % 512)/512.0;
		//	float z = (rand() % 10);
		//	td.AddTarget(Position(x,y,z));
		//}
		
		// Load lookup table and make buffer for image
		std::string lutpath = std::string("C:\\Users\\tardigrade\\SLM\\SLM\\SLM2047.lut");
		std::cout << "Loading LUT from " << lutpath << "..." << std::endl;
		// TODO Add error handling
		int* lut = LoadLUT(lutpath);

		std::string calibpath = std::string("C:\\Users\\tardigrade\\Dropbox\\SLM_shared\\2013-10-16\\calib_new.txt");
		Calibration calib;
		calib = TargetDatabase::LoadCalibration(calibpath);
		
		// debug calibration
		//calib.dtheta = 3.14159;
		//calib.shiftX = 0;
		//calib.shiftY = 0;

		// warm up arrayfire
		WarmUp(lut);
		
		// Initialize controller
		concurrency::concurrent_queue<std::string>* queue = nh.GetQueue();
		std::cout << "Initializing controller..." << std::endl;
		controller.Initialize(lut, queue, calib);

		// Initialize OpenGL display
		std::cout << "Initializing graphics..." << std::endl;
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
			std::cout << "Couldn't initialize display." << std::endl;
			exit(EXIT_FAILURE);
		}

		glfwMakeContextCurrent(window);
		glfwSetKeyCallback(window, key_callback);

		display.InitGraphics(M, N, monitor_width, monitor_height);

		// debug stuff
		//controller.DebugInitCells();

		// initialize asych IO
		nh.Connect("127.0.0.1", 9091);
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
		printf("Ran at %g (Hz)\n", 1/(elapsed/frame));

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
