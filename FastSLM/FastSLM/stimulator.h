#ifndef STIMULATOR_H_
#define STIMULATOR_H_

#include <NIDAQmx.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>

#include "common.h"

#define ERRMSG(e) "[ERROR] (" << __FUNCDNAME__ << ") " << e 

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error_=(functionCall)) ) HandleNIDAQError(); else

#define PI	3.1415926535

enum AnalogOutputChannel {
	kAO0, kAO1,
	kAO2, kAO3
};

class NIDAQBuffer {
public:
	NIDAQBuffer() : buffer_(NULL), num_samples_(-1), sampling_rate_(10000) {}
	NIDAQBuffer(int samp_rate) : buffer_(NULL), num_samples_(-1), sampling_rate_(samp_rate) {}

	virtual ~NIDAQBuffer() { if (buffer_ != NULL) delete[] buffer_; }

	float64* GetBuffer() { return buffer_; } // buffer with waveform
	int GetNumSamples() { return num_samples_; } // number of samples in buffer
	int GetSamplingRate() { return sampling_rate_; } // sampling rate of buffer

	void SetSamplingRate(int sampling_rate) { sampling_rate_ = sampling_rate; }

protected:
	void Reset();

	float64* buffer_;
	int num_samples_;
	int sampling_rate_;
};

class StimPatternBuffer : public NIDAQBuffer {
public:
	StimPatternBuffer() : NIDAQBuffer() {}
	StimPatternBuffer(int samp_rate) : NIDAQBuffer(samp_rate) { }

	// Generates a pockels cell signal with some uniform frequency of stimulation for some fixed duration.
	// samplingRate is in Hz
	// amplitude is in Volts
	// stimDuration is in seconds -- ON time (e.g. 0.004 for 4 msec)
	// duration is in seconds
	// frequency is in Hertz
	void GenerateUniformFreqSignal(float64 amplitude, float64 stimDuration, float64 duration, float64 frequency);
};

class ContinuousSpiralBuffer : public NIDAQBuffer {
public:
	ContinuousSpiralBuffer() : NIDAQBuffer() {}
	ContinuousSpiralBuffer(int samp_rate) : NIDAQBuffer(samp_rate) {}

	int GetNumSamplesPerChannel() { return num_samples_ / 2; }

	// fnameX is file with X galvo signal
	// fnameY is file with Y galvo signal
	// N is number of samples to read from file (must be less than length of file)
	void LoadGalvoSignals(const char* fnameX, const char* fnameY);
};

class NIDAQTaskRunner {
public:
	NIDAQTaskRunner() : handle_(NULL), error_(0), sample_rate_(10000), is_running_(false) {}
	virtual ~NIDAQTaskRunner() { delete buffer_; }

	virtual void Init() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;

	int GetSamplingRate() { return sample_rate_; }
	void SetSamplingRate(int sample_rate) { sample_rate_ = sample_rate; }
	bool IsRunning() { return is_running_; }
	void SetIsRunning(bool is_running) { is_running_ = is_running; }

protected:
	void HandleNIDAQError();
	static int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

	NIDAQBuffer* buffer_;
	TaskHandle handle_;
	int32 error_; // error signal from NIDAQmx functions
	int sample_rate_;
	bool is_running_;
};

class ContinuousSpiralRunner : public NIDAQTaskRunner {
public:
	ContinuousSpiralRunner() : NIDAQTaskRunner(), has_loaded_spiral_(false) {}
	virtual ~ContinuousSpiralRunner() {  }

	// MUST CALL INIT BEFORE ANY OTHER FUNCTIONS OR WILL CRASH
	virtual void Init();
	virtual void Start();
	virtual void Stop();
	
	void LoadSpirals(const char* nameX, const char* nameY);

private:
	bool has_loaded_spiral_;
	// no variables
};

class CenterGalvosRunner : public NIDAQTaskRunner {
public:
	CenterGalvosRunner() : NIDAQTaskRunner(), has_loaded_waveform_(false) {}
	virtual ~CenterGalvosRunner() {}

	virtual void Init();
	virtual void Start();
	virtual void Stop();

	void LoadWaveform(const char* nameX, const char* nameY);

private:
	bool has_loaded_waveform_;
};

class StimPatternRunner : public NIDAQTaskRunner {
public:
	StimPatternRunner() : NIDAQTaskRunner(), stim_duration_(0.004), stim_has_changed_(false) {}

	virtual void Init();
	virtual void Start();
	virtual void Stop();

	void ChangeStimPattern(float64 amplitude, float64 duration, float64 frequency);

private:
	float64 stim_duration_;
	bool stim_has_changed_;
};


#endif STIMULATOR_H_