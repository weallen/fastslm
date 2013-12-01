#include "stimulator.h"


GalvosController::GalvosController() {
	/*
	int32 error = 0;
	char err_buffer[2048] = {'\0'};

	const char out_chan[256] = "Dev/ao0";
	const float64 min_val = -10.0;
	const float64 max_val = 10.0;
	const float64 rate = 1000.0; // output data at 1 kHz
	const uInt64 num_samps = 1000; // number of samples in waveform

	DAQmxErrChk(DAQmxCreateTask("", &task_handle_));
	DAQmxErrChk(DAQmxCreateAOVoltageChan(task_handle_, out_chan, "", min_val, max_val, DAQmx_Val_Volts, NULL));
	DAQmxErrChk(DAQmxCfgSampClkTiming(task_handle_, "", rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, num_samps));
	DAQmxErrChk(DAQmxSetWriteRegenMode(task_handle_, DAQmx_Val_AllowRegen));

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(err_buffer, 2048);
	}*/
}

GalvosController::~GalvosController() {
	if (task_handle_ != 0) {
		//DAQmxClearTask(task_handle_);
	}
}

void GalvosController::StartSpiral() {
	//DAQmxStartTask(task_handle_);
}

void GalvosController::StopSpiral() {
	//DAQmxStopTask(task_handle_);
}

void GalvosController::LoadSpiral(const char* fname) {
}
