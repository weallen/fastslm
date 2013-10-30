#include "control.h"

void WarmUp(int* lut) {
	Hologram h;

	int M = 512;
	int N = 512;

	Pixel* buffer = MakeRGBImage(M, N); // empty buffer

	// Init GS to pre-JIT compile the arrayfire code
	std::cout << "Warming up GPU code..." << std::endl;
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
	std::cout << "Warmed up in " << timer::stop() << " seconds..." << std::endl;
}

void SLMControl::Initialize(int* lut, concurrency::concurrent_queue<std::string>* q, Calibration calib) {
	lut_ = lut;
	cmd_queue_ = q;
	source_ = af::randn(M_, N_); 
	retrieved_phase_ = array(M_, N_);
	shifted_phase_ = array(M_, N_);
	current_mask_ = MakeRGBImage(M_, N_); // intialize
	td_.SetCalibration(calib);

	// set up command mapping
	cmds_["BLANK"] = BLANK;
	cmds_["SHIFT"] = SHIFT;
	cmds_["STIM"] = STIM;
	cmds_["RESET"] = RESET;
	cmds_["LOAD"] = LOAD;
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
		ProcessLUT(retrieved_phase_, lut_, current_mask_);
	}

	// apply shift
	
	if (apply_shift_) {
		//h_.ApplyShift(offsetX_, offsetY_, retrieved_phase_, shifted_phase_);
		//ProcessLUT(shifted_phase_, lut_, current_mask_);
		apply_shift_ = false;
	}
}

void SLMControl::GetCurrentCommands() {
	std::string currstr;
	std::vector<std::string> tokens;

	while (cmd_queue_->try_pop(currstr)) {
#ifdef _DEBUG
		std::cout << currstr << std::endl;
#endif

		// tokenize string
		Tokenize(currstr, tokens);

		// apply command
		switch(cmds_[tokens[0]]) {
		case BLANK:
			Blank(tokens);
			continue;
		case SHIFT:
			ApplyShift(tokens);
			continue;
		case STIM:
			ChangeStim(tokens);
			continue;
		case RESET:
			Reset(tokens);
			continue;
		case LOAD:
			LoadCells(tokens);
			continue;
		}
	}
}

void SLMControl::DebugInitCells() {
	std::vector<std::string> toks;
	toks.push_back("RESET");
	toks.push_back("512");
	toks.push_back("512");
	toks.push_back("10");
	toks.push_back("1");
	toks.push_back("0.00001");
	toks.push_back("0");
	toks.push_back("9");

	Reset(toks);
	for (int i = 0; i < 1000; ++i) {
		float x = (rand() % 512)/512.0;
		float y = (rand() % 512)/512.0;
		float z = (rand() % 10);
		td_.AddTarget(Position(x,y,z));
	}
	cells_loaded_ = true;
}

void SLMControl::DebugGenRandomPattern() {
	std::vector<int> target_idx;
	for (int i = 0; i < 10; ++i) {
		int idx = rand() % 1000;
		target_idx.push_back(idx);
	}
	target_ = td_.GenerateTargetImage(target_idx);
	compute_gs_ = true;
}

void SLMControl::LoadCells(const std::vector<std::string>& toks) {

	td_.ResetTargets();
	float x, y, z;
	int N = atoi(toks[1].c_str());

	//std::cout << "Loading " << N << " cells..." << std::endl; 

	for (int i = 0; i < N*3; i += 3) {
		x = atof(toks[2+i].c_str());
		y = atof(toks[2+i+1].c_str());
		z = atof(toks[2+i+2].c_str());
		if (x <= 1.0 && y <= 1.0) {
			td_.AddTarget(Position(x, y, z));
		}
		else {
			std::cout << "ERROR: Cell x, y coordinates must be within [0, 1]" << std::endl;
		}
	}

	td_.ApplyCalibration();
	cells_loaded_ = true;
}

void SLMControl::ApplyShift(const std::vector<std::string>& toks) {
	offsetX_ = atoi(toks[1].c_str());
	offsetY_ = atoi(toks[2].c_str());
	apply_shift_ = true;
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
				std::cout << "ERROR: Index of cell " << cellid << " greater than number of targets (" << numtargets << ")" << std::endl;
			}
		}
		target_ = td_.GenerateTargetImage(targets);
		compute_gs_ = true;
	} else {
		std::cout << "ERROR: No cell positions loaded" << std::endl;
	}
}

// resets phase mask, target database, and recreates source image
void SLMControl::Reset(const std::vector<std::string>& toks) {
	
	//std::cout << "Resetting..." << std::endl;
	M_ = atoi(toks[1].c_str());
	N_ = atoi(toks[2].c_str());
	Z_ = atoi(toks[3].c_str());
	Zres_ = atof(toks[4].c_str())/z_fudge_factor_;
	minZ_ = atof(toks[5].c_str());
	maxZ_ = atof(toks[6].c_str());

	//Zres_ = 0.00001;
	//minZ_ = 0;
	//maxZ_ = 9;
	h_ = Hologram(M_, N_, Z_, minZ_, maxZ_, Zres_);
	td_ = TargetDatabase(M_, N_, Z_);
	
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
