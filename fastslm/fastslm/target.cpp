#include "target.h"

af::array TargetDatabase::GenerateTargetImage(const std::vector<int>& curr_targets) {
	af::array target_image = af::constant(0, M_, N_, Z_);
	int xpos, ypos, idx;

	for (int i = 0; i < curr_targets.size(); ++i) {
		idx = curr_targets[i];
		xpos = std::min<int>(floor(targets_[idx].x * (M_ - 1)), M_ - 1);
		ypos = std::min<int>(floor(targets_[idx].y * (N_ - 1)), N_ - 1);
		target_image(xpos, ypos, floor(targets_[idx].z)) = 255.0;//std::numeric_limits<float>::max();
	}
	
	for (int z = 0; z < Z_; ++z) {
		target_image(af::span, af::span, z) /= vignetting_;
		target_image(af::span, af::span, z) = (target_image(af::span, af::span, z) * af::max(af::max(target_image(af::span, af::span, z)))) * 255.0;
	}

	return target_image;
}


Calibration TargetDatabase::LoadCalibration(const std::string& fname) {
	std::string line;
	std::vector<std::string> toks;
	std::ifstream input(fname.c_str());
	std::getline(input, line);
	Tokenize(line, toks);
	Calibration calib;

	calib.dtheta = atof(toks[0].c_str());
	calib.shiftX = atof(toks[1].c_str());
	calib.shiftY = atof(toks[2].c_str());
	calib.scale = atof(toks[3].c_str());
	calib.zHeight = atof(toks[4].c_str());

	return calib;
}

//th = -1*dtheta/180*pi;
//R = [cos(th) - 1 * sin(th); sin(th) cos(th)];
//p1res = (p1-N/2)*s+N/2;
//p1r=N/2+(R*(p1res-N/2)')'
//c = mean(p2) - mean(p1r);
//p1t = p1r + repmat(c, [3 1])
void TargetDatabase::ApplyCalibration() {
	float x, y, z, x_temp;

	float N = (float) M_-1;
	float theta = -1 * calib_.dtheta;
	float R11 = cos(theta);
	float R12 = -1 * sin(theta);
	float R21 = sin(theta);
	float R22 = cos(theta);

	for (int i = 0; i < targets_.size(); ++i) {
		
		x = targets_[i].x;
		y = targets_[i].y;
		z = targets_[i].z;

		// scale to [0...N]
		x *= M_-1;
		y *= N_-1;

		// scale
		x = (x - N / 2)*calib_.scale + N / 2;
		y = (y - N / 2)*calib_.scale + N / 2;

		x_temp = x;

		// rotate
		x = N / 2 + (R11 * (x - N / 2) + R12 * (y - N / 2));
		y = N / 2 + (R21 * (x_temp - N / 2) + R22 * (y - N / 2));

		// translate
		x += calib_.shiftY; // !!! NOTE THAT THESE ARE BACKWARDS
		y += -1*calib_.shiftX; // !!! NOTE MULTIPLICATION BY -1 TO COMPENSATE FOR IMAGE FLIP

		// scale back to [0..1]
		x /= M_-1;
		y /= N_-1;

		targets_[i] = Position(x, y, z);
	}
}

af::array makeRandArray() {
		const int xSize = 512;
		const int ySize = 512;
		const int zSize = 10;

		af::array target = af::constant(0, xSize, ySize, zSize);

		af::array xs = af::randu(100,1) * xSize;
		af::array ys = af::randu(100,1) * ySize;
		int N = 100;
		
		for (int i = 0; i < N; ++i) {
			for (int z = 0; z < zSize; ++z) {
				target(xs(i), ys(i), z) = 1;
			}
		}
		return target;
}
