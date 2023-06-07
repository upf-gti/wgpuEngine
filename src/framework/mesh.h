#pragma once

#include <vector>

class Mesh {

public:

	void createQuad();

	void* data();
	size_t getSize();

private:

	std::vector<float> vertices;

};