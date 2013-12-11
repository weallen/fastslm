#include "stimulator.h"

void NIDAQBuffer::Reset() {
	if (buffer_ != NULL) {
		delete[] buffer_;
		buffer_ = NULL;
	}
	num_samples_ = -1;
}

void StimPatternBuffer::GenerateUniformFreqSignal(float64 amplitude, float64 stimDuration, float64 duration, float64 frequency) {
	float64 total_pulse_length = 1.0 / frequency;
	float64 intra_pulse_length = stimDuration;
	float64 inter_pulse_length = total_pulse_length - intra_pulse_length;
		
	if (inter_pulse_length < 0) {
		std::cerr << ERRMSG("Using too long stim durations for specified frequency. Cannot create stimulus waveform.") << std::endl;
		return;
	}

	Reset();

	int num_pulses = floor(duration /  total_pulse_length);

	int inter_pulse_samples = floor(inter_pulse_length * sampling_rate_);
	int intra_pulse_samples = floor(intra_pulse_length * sampling_rate_);
	num_samples_ = num_pulses * (inter_pulse_samples + intra_pulse_samples);

	buffer_ = new float64[num_samples_]; 
		
	int z = 0;
	for (int k = 0; k < num_pulses; ++k) {
		for (int i = 0; i < intra_pulse_samples; ++i) {
			buffer_[z] = amplitude;
			z++;
		}
		for (int i = 0; i < inter_pulse_samples; ++i) {
			buffer_[z] = 0.0;
			z++;
		}
		buffer_[num_samples_-1] = 0.0; // ensure that last value is 0
	}

}

void ContinuousSpiralBuffer::LoadGalvoSignals(const char* fnameX, const char* fnameY) {
	float curr_val;
	int i;
	std::string lineX, lineY;
	std::ifstream fX, fY;

	Reset();
		

	fX.open(fnameX, std::ios::in);
	fY.open(fnameY, std::ios::in);
	if (!fX.is_open()) {
		std::cerr << ERRMSG("Can't open file " << fnameX) << std::endl;
		return;
	}
	if (!fY.is_open()) {
		std::cerr << ERRMSG("Can't open file " << fnameY) << std::endl;
		return;
	}
	
	int N = 0;
	while (std::getline(fX, lineX)) {
		++N;
	}
	fX.close();
	fX.open(fnameX, std::ios::in);

	num_samples_ = 2*N; // twice as large to account for both channels
	buffer_ = new float64[num_samples_];

	
	for (i = 0; i < N; ++i) {
		std::getline(fX, lineX);
		std::getline(fY, lineY);
		buffer_[i] = (float64)atof(lineX.c_str());
		buffer_[N+i] = (float64)atof(lineY.c_str());
	}
	fX.close();
	fY.close();
}

void NIDAQTaskRunner::HandleNIDAQError() {
	char error_buffer[2048] = { '\0' };
	if( DAQmxFailed(error_) )
		DAQmxGetExtendedErrorInfo(error_buffer, 2048);
	if( handle_ !=0 ) {
		DAQmxStopTask(handle_);
		DAQmxClearTask(handle_);
	}
	if( DAQmxFailed(error_) )
		printf("DAQmx Error: %s\n",error_buffer);
}

int32 NIDAQTaskRunner::DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	StimPatternRunner* obj = (StimPatternRunner*) callbackData;
	obj->SetIsRunning(false);
	return 0;
}

void ContinuousSpiralRunner::Init() {
	buffer_ = (NIDAQBuffer*) new ContinuousSpiralBuffer(sample_rate_);
}

void ContinuousSpiralRunner::Start() {
	if (!has_loaded_spiral_) {
		std::cout << ERRMSG("Cannot start spirals. Need to load spiral waveform!") << std::endl;
		return;
	}
	if (handle_ != NULL) {
		DAQmxErrChk (DAQmxClearTask(handle_));
	}

	DAQmxErrChk (DAQmxCreateTask("SpiralTask",&handle_));

	DAQmxErrChk (DAQmxCreateAOVoltageChan(handle_, "Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,NULL));
	//DAQmxErrChk (DAQmxSetAODataXferMech(handle_, "Dev2/ao0", DAQmx_Val_Interrupts)); // only for PCI
	
	DAQmxErrChk (DAQmxCreateAOVoltageChan(handle_, "Dev1/ao1","",-10.0,10.0,DAQmx_Val_Volts,NULL));
	//DAQmxErrChk (DAQmxSetAODataXferMech(handle_, "Dev2/ao1", DAQmx_Val_Interrupts)); // only for PCI

	// TODO Make this center the galvos.
	DAQmxErrChk (DAQmxCfgSampClkTiming(handle_,"", buffer_->GetSamplingRate(), DAQmx_Val_Rising, DAQmx_Val_ContSamps, buffer_->GetNumSamples()));
	DAQmxErrChk (DAQmxWriteAnalogF64(handle_, ((ContinuousSpiralBuffer*)buffer_)->GetNumSamplesPerChannel(),0,10.0,DAQmx_Val_GroupByChannel, buffer_->GetBuffer(), NULL, NULL));
	DAQmxErrChk (DAQmxStartTask(handle_));
	is_running_ = true;
}

void ContinuousSpiralRunner::Stop() {
	if (!has_loaded_spiral_) {
		std::cout << ERRMSG("Cannot stop spirals. Need to load spiral waveform!") << std::endl;
		return;
	}

	DAQmxErrChk (DAQmxStopTask(handle_));
	is_running_ = false;
	// TODO Make this return galvos to the corner.
}

void ContinuousSpiralRunner::LoadSpirals(const char* nameX, const char* nameY) {
	((ContinuousSpiralBuffer*)buffer_)->LoadGalvoSignals(nameX, nameY); 
	has_loaded_spiral_ = true;
}


void CenterGalvosRunner::Init() {
	buffer_ = (NIDAQBuffer*) new ContinuousSpiralBuffer(sample_rate_);
}

void CenterGalvosRunner::Start() {
	if (!has_loaded_waveform_) {
		std::cout << ERRMSG("Cannot center galvos. Need to load waveform!") << std::endl;
		return;
	}

	if (handle_ != NULL) {
		DAQmxErrChk (DAQmxClearTask(handle_));
	}
	DAQmxErrChk (DAQmxCreateTask("WaveformTask",&handle_));
	DAQmxErrChk (DAQmxRegisterDoneEvent(handle_, 0, &DoneCallback, (void*)this));

	DAQmxErrChk (DAQmxCreateAOVoltageChan(handle_, "Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,NULL));
	//DAQmxErrChk (DAQmxSetAODataXferMech(handle_, "Dev2/ao0", DAQmx_Val_Interrupts)); // only for PCI
	
	DAQmxErrChk (DAQmxCreateAOVoltageChan(handle_, "Dev1/ao1","",-10.0,10.0,DAQmx_Val_Volts,NULL));
	//DAQmxErrChk (DAQmxSetAODataXferMech(handle_, "Dev2/ao1", DAQmx_Val_Interrupts)); // only for PCI

	// TODO Make this center the galvos.
	DAQmxErrChk (DAQmxCfgSampClkTiming(handle_,"", buffer_->GetSamplingRate(), DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, buffer_->GetNumSamples()));
	DAQmxErrChk (DAQmxSetWriteAttribute (handle_, DAQmx_Write_RegenMode, DAQmx_Val_DoNotAllowRegen));

	DAQmxErrChk (DAQmxWriteAnalogF64(handle_, ((ContinuousSpiralBuffer*)buffer_)->GetNumSamplesPerChannel(),0,10.0,DAQmx_Val_GroupByChannel, buffer_->GetBuffer(), NULL, NULL));
	DAQmxErrChk (DAQmxStartTask(handle_));
	DAQmxErrChk (DAQmxWaitUntilTaskDone(handle_, DAQmx_Val_WaitInfinitely));
	DAQmxErrChk (DAQmxClearTask(handle_));
}

void CenterGalvosRunner::Stop() {
	is_running_ = false;
}

void CenterGalvosRunner::LoadWaveform(const char* nameX, const char* nameY) {
	((ContinuousSpiralBuffer*)buffer_)->LoadGalvoSignals(nameX, nameY); 
	has_loaded_waveform_ = true;
}


void StimPatternRunner::Init() {
	buffer_ = (NIDAQBuffer*) new StimPatternBuffer(sample_rate_);
}


void StimPatternRunner::Start() {
	if (stim_has_changed_) {
		if (handle_ != NULL) {
			DAQmxErrChk (DAQmxClearTask(handle_));
		}
		DAQmxErrChk (DAQmxCreateTask("StimTask",&handle_));
		DAQmxErrChk (DAQmxRegisterDoneEvent(handle_, 0, &DoneCallback, (void*)this));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(handle_, "Dev2/ao0","",-10.0,10.0,DAQmx_Val_Volts,NULL));
		//DAQmxErrChk (DAQmxSetAODataXferMech(handle_, "Dev2/ao2", DAQmx_Val_Interrupts)); // only for PCI
		DAQmxErrChk (DAQmxCfgSampClkTiming(handle_,"", buffer_->GetSamplingRate(), DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, buffer_->GetNumSamples()));
		DAQmxErrChk (DAQmxSetWriteAttribute (handle_, DAQmx_Write_RegenMode, DAQmx_Val_DoNotAllowRegen));
	}

	DAQmxErrChk (DAQmxWriteAnalogF64(handle_, buffer_->GetNumSamples(), 0, 10.0, DAQmx_Val_GroupByChannel, buffer_->GetBuffer(), NULL, NULL));
	DAQmxErrChk (DAQmxStartTask(handle_));
	is_running_ = true;
}

void StimPatternRunner::Stop() {
	float64 zero[] = { 0.0, 0.0 };

	bool32 complete;
	DAQmxErrChk (DAQmxGetTaskComplete(handle_, &complete));
	if (!complete) {
		DAQmxErrChk (DAQmxStopTask(handle_));
	}
	is_running_ = false;
	// TODO Set the voltage to 0 as so not to continuously stim the target upon interrupt.
}

void StimPatternRunner::ChangeStimPattern(float64 amplitude, float64 duration, float64 frequency) {
	((StimPatternBuffer*)buffer_)->GenerateUniformFreqSignal(amplitude, stim_duration_, duration, frequency);
	stim_has_changed_ = true;
}


