#ifndef STIMULATOR_H_
#define STIMULATOR_H_

#include <NIDAQmx.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

#define PI 3.14159

class PockelsController 
{
public:
	PockelsController() {}
	virtual ~PockelsController() {}

private:
};

class GalvosController
{
public:
	GalvosController();
	virtual ~GalvosController();

	// move galvo from corner and start making a spiral
	void StartSpiral();
	
	// stop making a spiral and move galvo back to corner
	void StopSpiral();

	// Load spiral waveform from file
	void LoadSpiral(const char* fname);

private:
	unsigned short* x_waveform_;
	unsigned short* y_waveform_;
	
	TaskHandle task_handle_;
};

// triggers pockels cell with some frequency of pulses
// for some duration
class PulseStimulator 
{
public:
	PulseStimulator() {}
	virtual ~PulseStimulator() {}

private:
};

#endif STIMULATOR_H_