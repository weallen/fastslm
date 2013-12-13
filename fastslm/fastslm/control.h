#ifndef CONTROL_H_
#define CONTROL_H_

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <map>

#include <ppl.h>
#include <concurrent_queue.h>

#include <arrayfire.h>

#include "graphics.h"
#include "gs.h"
#include "target.h"
#include "stimulator.h"

using namespace af;

enum SLMCommand { 
	// basic commands 
	BLANK, // change to blank SLM pattern
	SHIFT, // apply shift to current phase mask
	STIM,  // change active subset of cells
	RESET, // clear all cell positions
	LOAD,   // set cell positions

	// commands for automated control of stimulus
	PULSE_SET, // set current stimulation pulse parameters
	PULSE_START, // start current pulse 
	PULSE_STOP, // force immediate stop current pulse

	// commands for automated control of spiral galvos
	SPIRAL_START, // start continuous spiralling
	SPIRAL_STOP, // stop continuous spiralling
};

// warm up arrayfire to precompile code
void WarmUp(int* lut);

// Design of SLM control:
// - One thread receives data by polling a ZMQ socket, and puts a queue. 
// - The other thread reads data from the queue, computes phase masks if necessary, and displays the phase mask.

class SLMControl {
public:
	SLMControl() 
		: M_(512), N_(512), Z_(10), Zres_(1)
		, offsetX_(0), offsetY_(0), current_mask_(NULL)
		, compute_gs_(false), cells_loaded_(false), apply_shift_(false)
		, z_fudge_factor_(4.0) {}

	SLMControl(int M, int N, int Z, double Zres) 
		: M_(M), N_(N), Z_(Z), Zres_(Zres)
		, offsetX_(0), offsetY_(0), current_mask_(NULL)
		, compute_gs_(false), cells_loaded_(false), apply_shift_(false)
		, z_fudge_factor_(4.0) {}

	virtual ~SLMControl() { if (current_mask_ != NULL) delete[] current_mask_; }

	void Initialize(int* lut, concurrency::concurrent_queue<std::string>* q, Calibration calib);

	// NOTE This must be called after initialize
	void LoadGalvoWaveforms(const std::string& x_path, const std::string& y_path); 
	void LoadCenterWaveforms(const std::string& x_path, const std::string& y_path);
	void LoadVignettingCorrectionMap(const std::string& path);

	// main function to update SLM display
	void Update();

	Pixel* CurrentMask() { return current_mask_; }

	// accessors
	void SetWidth(int M) { M_ = M; }
	void SetHeight(int N) { N_ = N; }
	void SetDepth(int Z) { Z_ = Z; }
	void SetZRes(int Zres) { Zres_ = Zres;}

	int GetWidth() { return M_; }
	int GetHeight() { return N_; }
	int GetDepth() { return Z_; }
	int GetZRes() { return Zres_; }

	// debugging functions that generates random patterns
	void DebugInitCells(); // create cell locations for random pattern (1000 cells)
	void DebugGenRandomPattern(); // generate random pattern
	void DebugSingleCell(float z); // Single cell at [0.5, 0.5] with specified z
	void DebugCalibPattern(); // generate triangular calibration pattern

private:
	// Apply all the commands currently in the queue
	void GetCurrentCommands();

	void Blank(const std::vector<std::string>& toks) {} // XXX NOT IMPLEMENTED 

	// Command: LOAD N X1 Y1 Z1 X2 Y2 Z2 ... XN YN ZN
	// REMEMBER THAT XN, YN are in [0...1] and ZN is in [0...Z]
	void LoadCells(const std::vector<std::string>& toks);

	// Command: SHIFT X Y
	void ApplyShift(const std::vector<std::string>& toks);

	// Command: STIM N CELL1 CELL2 CELL3 ... CELLN
	void ChangeStim(const std::vector<std::string>& toks);

	// Command: RESET WIDTH HEIGHT DEPTH ZRES ZMIN ZMAX
	void Reset(const std::vector<std::string>& toks);

	// Command: PULSE_SET AMPLITUDE DURATION FREQUENCY
	void PulseSet(const std::vector<std::string>& toks);

	// Command: PULSE_START
	void PulseStart(const std::vector<std::string>& toks);

	// Command: PULSE_STOP
	void PulseStop(const std::vector<std::string>& toks);

	// Command: SPIRAL_START
	void SpiralStart(const std::vector<std::string>& toks);
	
	// Command: SPIRAL_STOP
	void SpiralStop(const std::vector<std::string>& toks);

private:
	typedef void (SLMControl::*CallbackFnPtr)(const std::vector<std::string>&);

	void RegisterCommandCallback(const char* name, CallbackFnPtr callback);

	int M_; // width
	int N_; // height
	int Z_; // depth
	float Zres_; // Z resolution
	float minZ_;
	float maxZ_;

	// offsets from movement
	float offsetX_;
	float offsetY_;

	// NIDAQ stuff
	ContinuousSpiralRunner spiral_; // object to continuously produce spirals
	StimPatternRunner stim_; // object to produce stimulation Pockel's cell signals
	CenterGalvosRunner center_;

	// GS stuff
	array source_; // random source mask used to initialize each call to GS
	array target_; // store a buffer to be used a the target
	Pixel* current_mask_; // mask that will be current displayed
	Hologram h_; // hologram used to compute GS
	TargetDatabase td_; // structure containing cell positions
	array retrieved_phase_; // phasemask after computing GS
	array shifted_phase_; // phasemask after applying motion correction
	array target_z_; // ???

	// variables set externally
	int* lut_;
	concurrency::concurrent_queue<std::string>* cmd_queue_;

	// map to allow switch of different commands
	std::map<std::string, CallbackFnPtr> cmds_;

	// state flags
	bool compute_gs_;
	bool cells_loaded_; 
	bool apply_shift_;
	
	// calbiration stuff
	Calibration calib_;
	float z_fudge_factor_;
	af::array vignetting_;

};

#endif