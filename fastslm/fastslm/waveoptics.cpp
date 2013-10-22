#include "waveoptics.h"

array WOPropTF(const array& u1, const float L, const float wavelength, const int z) {
	int M = u1.dims(0);
	int N = u1.dims(1);
	float dx = L/(float)M;
	float k = 2*af::Pi/wavelength;

	float fs = 1.0/dx;
	float df = fs / (float)M;
	
	// equivalent of linspace
	array fx = linspace(0, fs-df, M) - (fs - (M % 2)*df)/2;
	// equivalent of meshgrid
	int n = fx.elements();
	array FX = tile(fx, 1, n);
	array FY = tile(fx.T(), n, 1);
	//array H = fftshift(exp( af::i * complex(- af::Pi * z * (pow2(FX) + pow2(FY)))));
	array H = (exp( af::i * complex(- af::Pi * z * (pow2(FX) + pow2(FY)))));
#ifdef USE_SHIFT
	array U1 = fft2(fftshift(u1));
	return ifftshift(ifft2(H * U1));
#else
	array U1 = fft2(u1);
	return ifft2(H * U1);
#endif
}
