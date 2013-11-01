#include "gs.h"

//th = -1*dtheta/180*pi;
//R = [cos(th) - 1 * sin(th); sin(th) cos(th)];
//p1res = (p1-N/2)*s+N/2;
//p1r=N/2+(R*(p1res-N/2)')'
//c = mean(p2) - mean(p1r);
//p1t = p1r + repmat(c, [3 1])

// fx = source (f(x))
// Fu = target (Fourier transform of f(x))
void Hologram::GS(const array& Fu, const array& fx, array& retrieved_phase /*, array& estimate*/) {
	array z_planes = seq(Z_);
	float target_z = z_planes(0).scalar<float>();
	//float target_z = 0;
	int Z = Fu.dims(2);
	array target_arr = Fu(span, span, 0);
	array gp = exp(fx);
	array g = abs(fx) * exp(af::i * fx);
	array G = ForwardLensPropagation(g, target_z);
	array Gp;
	array temp(Fu.dims(), c32);
	for (int i = 0; i < num_iter_; ++i) {
		g = abs(fx) * exp(af::i * arg(gp));
		gfor(array z, Z) {
			target_z = z_planes(z).scalar<float>();
			//target_z = z(0).scalar<float>();
			G = ForwardLensPropagation(g, target_z);
			Gp = abs(Fu(span, span, z)) * exp(af::i * arg(G));
			temp(span, span, z) = BackwardLensPropagation(Gp, target_z);
		}
		gp = sum(temp, 2);
	}
	g = abs(fx) * exp(af::i * arg(gp));

	// compute estimate
	//gfor(array z, Z)   {
	//	target_z = z_planes(z).scalar<float>();
	//	G = ForwardLensPropagation(g, target_z);
	//	estimate(span, span, z) = abs(G);
	//}
	retrieved_phase = arg(g) + af::Pi;
}

array Hologram::ForwardLensPropagation(const array& incidentField, const float z) {
#ifdef USE_SHIFT
	array propagatedField = fftshift(fft2(fftshift(incidentField)));
#else
	array propagatedField = fft2(incidentField);
#endif
	return PropTF(propagatedField, (int)z);
}

array Hologram::BackwardLensPropagation(const array& incidentField, const float z) {
	array propagatedField = PropTF(incidentField, -z);
#ifdef USE_SHIFT
	return ifftshift(ifft2(ifftshift(propagatedField)));
#else
	return ifft2(propagatedField);
#endif
}

array Hologram::PropTF(const array& u1, const int z) {
#ifdef USE_SHIFT
	array U1 = fft2(fftshift(u1));
	return ifftshift(ifft2(H_(span, span, z) * U1));
#else
	array U1 = fft2(u1);
	return ifft2(H_(span, span, z) * U1);
#endif
}


void Hologram::MakeH() {
	float dx = L_/(float)M_;
	float k = 2*af::Pi/wavelength_;

	float fs = 1.0/dx;
	float df = fs / (float)M_;
	
	array zpos = linspace(minZ_, maxZ_, Z_); // position in z space relative to focal plane

	// equivalent of linspace
	array fx = linspace(0, fs-df, M_) - (fs - (M_ % 2)*df)/2;
	// equivalent of meshgrid
	int n = fx.elements();
	array FX = tile(fx, 1, n);
	array FY = tile(fx.T(), n, 1);

	//array H = fftshift(exp( af::i * complex(- af::Pi * z * (pow2(FX) + pow2(FY)))));
	H_ = constant(0, M_, N_, Z_, c32);
	for (int z = 0; z < Z_; ++z) {
		H_(span, span, z) = exp( af::i * complex(- af::Pi * wavelength_ * zpos(z) * zres_ * (pow2(FX) + pow2(FY))));
		//H_(span, span, z) = exp(af::i * complex(-af::Pi  * z * (pow2(FX) + pow2(FY))));
#ifdef USE_SHIFT
		H_(span, span, z) = fftshift(H_(span, span, z));
#endif
	}
}

void Hologram::MakeShiftMatrix() {
	//array fx = seq(0, M_-1);
	array fx = linspace(-1, 1, M_);
	float c = 2 * 3.14159 * 50 * .81203 / 110.9;
	int n = fx.elements();
	shiftX_ = tile(fx, 1, n);
	shiftY_ = tile(fx.T(), n, 1);
}

void Hologram::ApplyShift(const float offsetX, const float offsetY, const array& phasemask, array& shifted_phasemask) {
	//if (offsetX == 0 && offsetY == 0) {
	//	shifted_phasemask = phasemask.copy();
	//}
	//else {
	float c = 2 * 3.14159 * 50 * .81203 / 110.9; 
	float modfact = 2 * 3.14159;
	shifted_phasemask = phasemask + (shiftX_ * offsetX + shiftY_ * offsetY) * c;
	shifted_phasemask = shifted_phasemask - af::floor(shifted_phasemask / modfact) * modfact; // modulus modfact
	//}
}