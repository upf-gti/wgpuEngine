#pragma once

#include <vector>

class Mesh {

	std::vector<float> vertices;

public:

	void create_quad();

	void* data();
	size_t get_size();
	void destroy();
};