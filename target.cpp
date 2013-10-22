#include "target.h"

af::array TargetDatabase::GenerateTargetImage(const std::vector<int>& curr_targets) {
	af::array target_image = af::constant(0, M_, N_, Z_);
	for (int i = 0; i < curr_targets.size(); ++i) {
			target_image(floor(targets_[i].x * M_), floor(targets_[i].y * N_), floor(targets_[i].z)) = 255;//std::numeric_limits<float>::max();
	}

	return target_image;
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