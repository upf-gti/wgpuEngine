#pragma once

template<unsigned int N>
class Frame {
public:
	float value[N];//value
	float in[N]; //in tangent
	float out[N]; //out tangent
	float time; //frame time
};

typedef Frame<1> ScalarFrame;
typedef Frame<3> VectorFrame;
typedef Frame<4> QuaternionFrame;