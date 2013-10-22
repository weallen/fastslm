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

using namespace af;

enum SLMCommand { 
	BLANK, // change to blank SLM pattern
	SHIFT, // apply shift to current phase mask
	STIM,  // change active subset of cells
	RESET, // clear all cell positions
	LOAD   // set cell positions
};

// warm up arrayfire to precompile code
void WarmUp(int* lut);

// Design of SLM control:
// - One thread receives data by polling a ZMQ socket, and puts a queue. 
// - The other thread reads data from the queue, computes phase masks if necessary, and displays the phase mask.

class SLMControl {
public:
	SLMControl() 
		: M_(512), N_(512), Z_(10), Zres_(1), offsetX_(0), offsetY_(0), current_mask_(NULL), compute_gs_(false) {}
	SLMControl(int M, int N, int Z, double Zres) 
		: M_(M), N_(N), Z_(Z), Zres_(Zres), offsetX_(0), offsetY_(0), current_mask_(NULL), compute_gs_(false) {}

	virtual ~SLMControl() { if (current_mask_ != NULL) delete[] current_mask_; }

	void Initialize(int* lut, concurrency::concurrent_queue<std::string>* q);

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
	void DebugInitCells();
	void DebugGenRandomPattern();

private:
	// Apply all the commands currently in the queue
	void GetCurrentCommands();

	void Blank(const std::vector<std::string>& toks) {} // NOT IMPLEMENTED YET

	// Command: LOAD N X1 Y1 Z1 X2 Y2 Z2 ... XN YN ZN
	void LoadCells(const std::vector<std::string>& toks);

	// Command: SHIFT X Y
	void ApplyShift(const std::vector<std::string>& toks);

	// Command: STIM N CELL1 CELL2 CELL3 ... CELLN
	void ChangeStim(const std::vector<std::string>& toks);

	// Command: RESET WIDTH HEIGHT DEPTH ZRES
	void Reset(const std::vector<std::string>& toks);

	// split string by whitespace and add to vector
	void Tokenize(const std::string& str, std::vector<std::string>& tokens);


private:
	int M_; // width
	int N_; // height
	int Z_; // depth
	int Zres_; // Z resolution

	// offsets from movement
	int offsetX_;
	int offsetY_;

	array source_; // random source mask used to initialize each call to GS
	array target_; // store a buffer to be used a the target
	Pixel* current_mask_; // mask that will be current displayed
	Hologram h_; // hologram used to compute GS
	TargetDatabase td_; // structure containing cell positions
	array retrieved_phase_;
	array target_z_;

	// variables set externally
	int* lut_;
	concurrency::concurrent_queue<std::string>* cmd_queue_;

	// map to allow switch of different commands
	std::map<std::string, SLMCommand> cmds_;
	bool compute_gs_;
};

#endif