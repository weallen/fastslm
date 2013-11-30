#ifndef GS_H_
#define GS_H_

#include <vector>
#include <string>
#include <arrayfire.h>

#include "common.h"
#include "waveoptics.h"

using namespace af;

const float z_fudge_factor = 4.0;


// constant z offset to compensate for improperly aligned optics
// XXX THIS IS HACK -- SHOULD BE A PARAMETER
const float const_z_offset_ = 42E-6; 

/*
 * NOTE: To specify Z-planes, give GS the number of Z planes, min Z, max Z (integers), and Z spacing (float)
 * It will precompute the H wave-propagation matrix, where each slice of the matrix corresponds to one position in Z.
 * Cells will each have an integer index into that matrix specifying their position in space.
 */
class Hologram {
public:
	// default has 10 um spacing
	Hologram() 
		: wavelength_(1064), L_(10.0), num_iter_(1), zres_(1E-6) 
		, minZ_(0), maxZ_(9), M_(512), N_(512), Z_(10)
	{
		Initialize();
	}

	Hologram(int M, int N, int Z, float minZ, float maxZ, float zres) 
		: wavelength_(1064), L_(10.0), num_iter_(1), zres_(zres)
		, minZ_(minZ), maxZ_(maxZ), M_(M), N_(N), Z_(Z)
	{
		Initialize();
	}

	virtual ~Hologram() {}

	// implements the Gerchberg-Saxton algorithm
	void GS(const array& target, const array& source, array& retrieved_phase /*, array& estimate*/);

	// apply shift to phasemask
	// NOTE: Increased offsetY moves to the left in real coordinates
	//		 Increased offsetX moves down in real coordinates
	//		 THIS IS BACKWARDS AND NEGATIVE FROM WHAT YOU WOULD EXPECT!
	void ApplyShift(const float offsetX, const float offsetY, const array& phasemask, array& shifted_phasemask);

private:
	void Initialize() {
		MakeH();
		MakeShiftMatrix();
	}

	array ForwardLensPropagation(const array& incidentField, const float z);
	array BackwardLensPropagation(const array& incidentField, const float z);

	array PropTF(const array& u1, const float z);

	// XXX NOT WORKING RIGHT NOW DO NOT USE
	array PropTFPrecomputed(const array& u1, const int z);

	// precompute matrix for PropTF
	void MakeH();
	void MakeShiftMatrix();

	array H_;

	array shiftX_;
	array shiftY_;

	float zres_;
	float minZ_;
	float maxZ_;

	int M_;
	int N_;
	int Z_;
	int wavelength_;
	float L_;
	int num_iter_;
};


#endif 