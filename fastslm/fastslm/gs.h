#ifndef GS_H_
#define GS_H_

#include <vector>
#include <string>
#include <arrayfire.h>

#include "common.h"
#include "waveoptics.h"

using namespace af;

class Hologram {
public:
	Hologram() 
		: wavelength_(1064)
		, L_(10.0)
		, num_iter_(1)
		, M_(512)
		, N_(512)
		, Z_(10)
	{
		Initialize();
	}

	Hologram(int M, int N, int Z) 
		: wavelength_(1064)
		, L_(10.0)
		, num_iter_(1)
		, M_(M)
		, N_(N)
		, Z_(Z)
	{
		Initialize();
	}

	virtual ~Hologram() {}

	// implements the Gerchberg-Saxton algorithm
	void GS(const array& target, const array& source, const array& target_z, array& retrieved_phase /*, array& estimate*/);

	// apply shift to phasemask
	void ApplyShift(const int offsetX, const int offsetY, const array& phasemask, array& shifted_phasemask);

private:
	void Initialize() {
		MakeH();
		MakeShiftMatrix();
	}

	array ForwardLensPropagation(const array& incidentField, const float z);
	array BackwardLensPropagation(const array& incidentField, const float z);

	array PropTF(const array& u1, const int z);

	// precompute matrix for PropTF
	void MakeH();
	void MakeShiftMatrix();

	array H_;
	array shiftX_;
	array shiftY_;

	int M_;
	int N_;
	int Z_;
	int wavelength_;
	float L_;
	int num_iter_;
};


#endif 