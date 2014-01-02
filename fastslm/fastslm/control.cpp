#include "control.h"

void WarmUp(int* lut) {
	Hologram h;

	int M = 512;
	int N = 512;

	Pixel* buffer = MakeRGBImage(M, N); // empty buffer

	// Init GS to pre-JIT compile the arrayfire code
	std::cout << "[DEBUG] Warming up GPU code..." << std::endl;
	array source = af::randn(M, N); 
	timer::start();
	array target = makeRandArray();

	// allocate phase and estimate buffer
	array retrieved_phase(M, N, c32);
	//array estimate(target.dims()); 

	h.GS(target, source, retrieved_phase);
	h.ApplyShift(0, 0, retrieved_phase, retrieved_phase);
	ProcessLUT(retrieved_phase, lut, buffer);

	delete[] buffer;
	std::cout << "[DEBUG] Warmed up in " << timer::stop() << " seconds..." << std::endl;
}

void SLMControl::Initialize(int* lut, concurrency::concurrent_queue<std::string>* q, Calibration calib) {
	lut_ = lut;
	cmd_queue_ = q;
	source_ = af::randn(M_, N_); 
	retrieved_phase_ = array(M_, N_);
	shifted_phase_ = array(M_, N_);
	current_mask_ = MakeRGBImage(M_, N_); // intialize
		
	vignetting_ = af::constant(1.0, M_, N_);

	calib_ = calib;
	td_.SetCalibration(calib_);

	// initialize NIDAQ stuff
	int nidaq_sample_rate = 100000;
	stim_.SetSamplingRate(nidaq_sample_rate);
	stim_.Init();

	int spiral_sample_rate = 250000; // Note this might be vary
	spiral_.SetSamplingRate(spiral_sample_rate);
	spiral_.Init();

	int center_sample_rate = 250000;
	center_.SetSamplingRate(center_sample_rate);
	center_.Init();

	// set up command mapping
	RegisterCommandCallback("BLANK",		&SLMControl::Blank);
	RegisterCommandCallback("SHIFT",		&SLMControl::ApplyShift);
	RegisterCommandCallback("STIM",			&SLMControl::ChangeStim);
	RegisterCommandCallback("RESET",		&SLMControl::Reset);
	RegisterCommandCallback("LOAD",			&SLMControl::LoadCells);
	RegisterCommandCallback("PULSE_SET",	&SLMControl::PulseSet); 
	RegisterCommandCallback("PULSE_START",	&SLMControl::PulseStart);
	RegisterCommandCallback("PULSE_STOP",	&SLMControl::PulseStop);
	RegisterCommandCallback("SPIRAL_START", &SLMControl::SpiralStart);
	RegisterCommandCallback("SPIRAL_STOP",	&SLMControl::SpiralStop);
}

void SLMControl::LoadGalvoWaveforms(const std::string& x_path, const std::string& y_path) {
	spiral_.LoadSpirals(x_path.c_str(), y_path.c_str());
}

void SLMControl::LoadCenterWaveforms(const std::string& x_path, const std::string& y_path) {
	center_.LoadWaveform(x_path.c_str(), y_path.c_str());
}

void SLMControl::LoadVignettingCorrectionMap(const std::string& path) {
	std::ifstream vignetting_file(path);
	if (vignetting_file) {
		af::array map = af::loadimage(path.c_str(), false).asfloat();
		//map /= max_val;
		vignetting_ = map.copy();
	} else {
		std::cout << "Cannot find vignetting correction file at " << path << "..." << std::endl;
	}
}

void SLMControl::RegisterCommandCallback(const char* name, CallbackFnPtr callback) {
	cmds_[std::string(name)] = callback;
}

void SLMControl::Update() {
	// try to get current command to update state
	GetCurrentCommands();

	//DebugGenRandomPattern();

	// Run GS
	if (compute_gs_) {
		h_.GS(target_, source_, retrieved_phase_);
		compute_gs_ = false;
		apply_shift_ = true;
		//ProcessLUT(retrieved_phase_, lut_, current_mask_);
	}

	// apply shift
	if (apply_shift_) {
		h_.ApplyShift(offsetX_, offsetY_, retrieved_phase_, shifted_phase_);
		ProcessLUT(shifted_phase_, lut_, current_mask_);
		apply_shift_ = false;
	}
}

void SLMControl::GetCurrentCommands() {
	std::string currstr;
	std::vector<std::string> tokens;
	while (cmd_queue_->try_pop(currstr)) {
#ifdef _DEBUG
		std::cout << "[DEBUG] " << currstr << "..." << std::endl;
#endif
		// remove trailing NULL character that is the produce of ZMQ
		currstr = currstr.substr(0, currstr.size()-1);

		// tokenize string
		Tokenize(currstr, tokens);

		// apply command
		std::string cmd = tokens[0];

		std::map<std::string, CallbackFnPtr>::iterator it = cmds_.find(cmd);
		if (it != cmds_.end()) {
			CallbackFnPtr callback = cmds_[cmd];
			if (callback != NULL) {
				(this->*callback)(tokens);
			} else {
				std::cout << "[ERROR] Null callback pointer for " << cmd << std::endl;
			}
		} else {
			std::cout << "[ERROR] Command " << cmd << " not found" << std::endl;
		}
	}
}

void SLMControl::DebugCalibPattern() {
	std::vector<std::string> toks;
	toks.push_back("RESET");
	toks.push_back("512");
	toks.push_back("512");
	toks.push_back("10");
	toks.push_back("0.00001");
	toks.push_back("0");
	toks.push_back("9");
	Reset(toks);
	//td_.AddTarget(Position(1.0 - 279.0/512.0, 206.0/512.0, 0));
	//td_.AddTarget(Position(1.0 - 206.0/512.0, 256.0/512.0, 0));
	//td_.AddTarget(Position(1.0 - 231.0/512.0, 288.0/512.0, 0));
	td_.AddTarget(Position(279.0/512.0, 1.0 - 206.0/512.0, 0));
	td_.AddTarget(Position(206.0/512.0, 1.0 - 256.0/512.0, 0));
	td_.AddTarget(Position(231.0/512.0, 1.0 - 288.0/512.0, 0));
	td_.ApplyCalibration();
	cells_loaded_ = true;

	std::vector<int> target_idx;
	target_idx.push_back(0);
	target_idx.push_back(1);
	target_idx.push_back(2);

	target_ = td_.GenerateTargetImage(target_idx);
	compute_gs_ = true;
}


void SLMControl::DebugInitCells() {
	std::vector<std::string> toks;
	toks.push_back("RESET");
	toks.push_back("512");
	toks.push_back("512");
	toks.push_back("50");
	toks.push_back("0.00001");
	toks.push_back("0");
	toks.push_back("49");

	Reset(toks);
	for (int i = 0; i < 1000; ++i) {
		float x = (rand() % 512)/512.0;
		float y = (rand() % 512)/512.0;
		float z = (rand() % 50);
		z = 0;
		td_.AddTarget(Position(x,y,z));
	}
	cells_loaded_ = true;
}

void SLMControl::DebugGenRandomPattern() {
	std::vector<int> target_idx;
	for (int i = 0; i < 25; ++i) {
		int idx = rand() % 1000; // assumes 1000 random cells
		target_idx.push_back(idx);
	}
	target_ = td_.GenerateTargetImage(target_idx);
	compute_gs_ = true;
}

void SLMControl::DebugSingleCell(float z) {
	std::vector<std::string> toks;
	toks.push_back("RESET");
	toks.push_back("512");
	toks.push_back("512");
	toks.push_back("5");
	toks.push_back("0.000001");
	toks.push_back("0");
	toks.push_back("4");
	Reset(toks);

	td_.AddTarget(Position(0.5, 0.5, 0));
	std::vector<int> target_idx;
	target_idx.push_back(0);
	td_.ApplyCalibration();
	target_ = td_.GenerateTargetImage(target_idx);
	cells_loaded_ = true;
	compute_gs_ = true;

}

void SLMControl::LoadCells(const std::vector<std::string>& toks) {

	td_.ResetTargets();
	float x, y, z;
	int N = atoi(toks[1].c_str());

	//std::cout << "Loading " << N << " cells..." << std::endl; 

	for (int i = 0; i < N*3; i += 3) {
		x = atof(toks[2+i].c_str());
		y = 1.0 - atof(toks[2+i+1].c_str()); // !!!! NOTE THAT THIS is y = 1 - y to compensate for image flip
		z = atof(toks[2+i+2].c_str());
		if (x <= 1.0 && y <= 1.0) {
			td_.AddTarget(Position(x, y, z));
		} else {
			std::cout << "[ERROR] Cell x, y coordinates must be within [0, 1]" << std::endl;
		}
	}

	td_.ApplyCalibration();
	cells_loaded_ = true;
}

void SLMControl::ApplyShift(const std::vector<std::string>& toks) {
	if (toks.size() == 3) {
		offsetX_ = atof(toks[1].c_str());
		offsetY_ = atof(toks[2].c_str());

		// apply calibration shift
		float N = (float)M_ - 1;
		float theta = -1 * calib_.dtheta;
		float R11 = cos(theta);
		float R12 = -1 * sin(theta);
		float R21 = sin(theta);
		float R22 = cos(theta);

		float x_temp = offsetX_;

		// rotate
		offsetX_ = R11 * offsetX_ + R12 * offsetY_;
		offsetY_ = R21 * x_temp + R22 * offsetY_;

		apply_shift_ = true;

	}
}

void SLMControl::ChangeStim(const std::vector<std::string>& toks) {
	int cellid;
	int numtargets = td_.GetNumTargets();
	if (cells_loaded_) {
		int N = atoi(toks[1].c_str());
		//std::cout << "Changing stim to " << N << " cells..." << std::endl; 
		std::vector<int> targets;
		for (int i = 0; i < N; ++i) {
			cellid = atoi(toks[2 + i].c_str());
			if (cellid < numtargets) {
				targets.push_back(cellid);
			} else {
				std::cout << "[ERROR] Index of cell " << cellid << " greater than number of targets (" << numtargets << ")" << std::endl;
			}
		}
		target_ = td_.GenerateTargetImage(targets);
		compute_gs_ = true;
	} else {
		std::cout << "[ERROR] No cell positions loaded" << std::endl;
	}
}

// resets phase mask, target database, and recreates source image
void SLMControl::Reset(const std::vector<std::string>& toks) {
	
	if (toks.size() != 7) {
		std::cout << "[ERROR] Reset string has incorrect number of elements!" << std::endl;
		return;
	}

	if (stim_.IsRunning()) {
		stim_.Stop();
	}

	if (spiral_.IsRunning()) {
		spiral_.Stop();
	}

	//std::cout << "Resetting..." << std::endl;
	M_ = atoi(toks[1].c_str());
	N_ = atoi(toks[2].c_str());
	Z_ = atoi(toks[3].c_str());
	Zres_ = atof(toks[4].c_str());
	minZ_ = atof(toks[5].c_str());
	maxZ_ = atof(toks[6].c_str());

	h_ = Hologram(M_, N_, Z_, minZ_, maxZ_, Zres_);
	td_ = TargetDatabase(M_, N_, Z_);
	td_.SetVignettingCorrection(vignetting_);
	td_.SetCalibration(calib_);

	if (current_mask_ != NULL) {
		delete[] current_mask_;
	}

	current_mask_ = MakeRGBImage(M_, N_); // intialize

	offsetX_ = 0;
	offsetY_ = 0;

	compute_gs_ = false;
	cells_loaded_ = false;
	apply_shift_ = false;
}

void SLMControl::PulseSet(const std::vector<std::string>& toks) {
	if (toks.size() != 4) {
		std::cout << "[ERROR] Pulse set string has incorrect number of elements!" << std::endl;
	}
	float64 amplitude = (float64)atof(toks[1].c_str());
	float64 duration = (float64)atof(toks[2].c_str());
	float64 frequency = (float64)atof(toks[3].c_str());

	if (stim_.IsRunning()) {
		stim_.Stop();
	}

	stim_.ChangeStimPattern(amplitude, duration, frequency);
}

void SLMControl::PulseStart(const std::vector<std::string>& toks) {
	if (stim_.IsRunning()) {
		stim_.Stop();
	}
	stim_.Start();
}

void SLMControl::PulseStop(const std::vector<std::string>& toks) {
	stim_.Stop();
}

void SLMControl::SpiralStart(const std::vector<std::string>& toks) {
	center_.Start();

	if (spiral_.IsRunning()) {
		spiral_.Stop();
	}
	spiral_.Start();
}

void SLMControl::SpiralStop(const std::vector<std::string>& toks) {
	if (spiral_.IsRunning()) {
		spiral_.Stop();
	}
}
