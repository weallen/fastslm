#include "gs.h"

// fx = source (f(x))
// Fu = target (Fourier transform of f(x))
void Hologram::GS(const array& Fu, const array& fx, const array& z_planes, array& retrieved_phase /*, array& estimate*/) {
	/*array A(target.dims(), c32);
	array B(target.dims(), c32);
	
	array absS(source.dims());
	array absT(target.dims());

	int Z = target.dims(2);
	gfor(array z, Z)  {
		A(span, span, z) = ForwardLensPropagation(target(span, span, z), 10);
		//A(span, span, z) = ifft2(target(span, span, z));
		absS(span, span, z) = abs(source(span, span, z));
		absT(span, span, z) = abs(target(span, span, z));
	}

	A.eval();
	for (int k = 0; k < num_iter_; ++k) {
		gfor(array z, Z)  {
			B(span, span, z) = absS(span, span, z) * exp(af::i * arg(A(span, span, z)));
			B(span, span, z) = ForwardLensPropagation(B(span, span, z), 10);
			A(span, span, z) = absT(span, span, z) * exp(af::i * arg(B(span, span, z)));
			A(span, span, z) = BackwardLensPropagation(A(span, span, z), 10);
		}
	}
	gfor(array z, Z)   {
		B(span, span, z) = absS(span, span, z) * exp(af::i * arg(A(span, span, z)));
		B(span, span, z) = ForwardLensPropagation(B(span, span, z), 10);
		estimate(span, span, z) = abs(B(span, span, z));
		retrieved_phase(span, span, z) = arg(A(span, span, z)) + af::Pi;
	}*/

	float target_z = z_planes(0).scalar<float>();
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
			G = ForwardLensPropagation(g, target_z);
			Gp = abs(Fu(span, span, z)) * exp(af::i * arg(G));
			temp(span, span, z) = BackwardLensPropagation(Gp, target_z);
		}
		gp = sum(temp, 2);
	}
	g = abs(fx) * exp(af::i * arg(gp));

	// Don't compute estimate
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
	return PropTF(propagatedField, z);
}

array Hologram::PropTF(const array& u1, const int z) {
	array U1 = fft2(fftshift(u1));
	return ifftshift(ifft2(H_(span, span, z) * U1));
}

array Hologram::BackwardLensPropagation(const array& incidentField, const float z) {
	array propagatedField = PropTF(incidentField, -z);
#ifdef USE_SHIFT
	return ifftshift(ifft2(ifftshift(propagatedField)));
#else
	return ifft2(propagatedField);
#endif
}

void Hologram::MakeH() {
	float dx = L_/(float)M_;
	float k = 2*af::Pi/wavelength_;

	float fs = 1.0/dx;
	float df = fs / (float)M_;
	
	// equivalent of linspace
	array fx = linspace(0, fs-df, M_) - (fs - (M_ % 2)*df)/2;
	// equivalent of meshgrid
	int n = fx.elements();
	array FX = tile(fx, 1, n);
	array FY = tile(fx.T(), n, 1);
	//array H = fftshift(exp( af::i * complex(- af::Pi * z * (pow2(FX) + pow2(FY)))));
	H_ = constant(0, M_, N_, Z_, c32);
	for (int z = 0; z < Z_; ++z) {
		H_(span, span, z) = (exp( af::i * complex(- af::Pi * z * (pow2(FX) + pow2(FY)))));
	}
}