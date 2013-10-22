#include "common.h"

using namespace af;

array phase(const array& h) {
	return atan2(imag(h), real(h));
}

array amplitude(const array& h) {
	return sqrt(matmul(real(h), real(h)) + matmul(imag(h), imag(h)), true);
}

array cexp(const array& h) {
	return cos(h) + af::i * sin(h);
}

array linspace(float d1, float d2, int n) {
	float n1 = floor(n)-1;
	float c = (d2 - d1) * (n1-1);
	array y = d1 + seq(0, n1) * ((d2 - d1)/n1);
	y(1) = d1;
	y(end) = d2;
	return y;
}