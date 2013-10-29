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

		std::string calibpath = std::string("C:\\Users\\Admin\\Desktop\\slmscope\\SLM\\calib.txt");
		Calibration calib;
		//calib = TargetDatabase::LoadCalibration(calibpath);

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
			return 0;
		}

		window = glfwCreateWindow(M, N, "Display", NULL, NULL);

		if (!window) {
			glfwTerminate();
			std::cout << "Couldn't initialize display." << std::endl;
			return 0;
		}

		glfwMakeContextCurrent(window);
		display.InitGraphics(M, N);

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
