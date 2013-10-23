#ifndef TARGET_H_
#define TARGET_H_

#include <fstream>
#include <string>
#include <vector>
#include <limits>
#include <arrayfire.h>

#include <cmath>

#include "common.h"

#define _USE_MATH_DEFINES
#include <cmath>

struct Calibration {
	Calibration() : dtheta(0.0), shiftX(0.0), shiftY(0.0), scale(1.0), zHeight(0.0) {}
	Calibration(float dT, float sX, float sY, float s, float z) : dtheta(dT), shiftX(sX), shiftY(sY), scale(s), zHeight(z) {}
	virtual ~Calibration() {}

	float dtheta;
	float shiftX;
	float shiftY;
	float scale;
	float zHeight; // in meters
};

struct Position {
	Position() : x(0), y(0), z(0) {}
	Position(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	virtual ~Position() {}

	float x;
	float y;
	float z;
};

af::array makeRandArray();

// Structure containing the locations of all the cells to stimulate
class TargetDatabase {
public:
	TargetDatabase() {}
	TargetDatabase(int w, int h, int d)
		: M_(w), N_(h), Z_(d) {}

	virtual ~TargetDatabase() {}

	void AddTarget(const Position& target) { targets_.push_back(target); }
	Position GetTarget(int i) { return targets_[i]; }
	int GetNumTargets() { return targets_.size(); }
	void ResetTargets() { targets_.clear(); }
	
	void ApplyCalibration();
	void SetCalibration(Calibration calib) { calib_ = calib; }
	Calibration GetCalibration() { return calib_;  }

	static Calibration LoadCalibration(const std::string& fname);

	af::array GenerateTargetImage(const std::vector<int>& curr_targets);

private:
	int M_;
	int N_;
	int Z_;

	Calibration calib_;

	std::vector<Position> targets_;

};


#endif