#pragma once

#include "glm/vec4.hpp"

typedef glm::vec4 Color;

namespace colors {
	const Color WHITE	= Color(1.f,	1.f,	1.f,	1.0f);
	const Color BLACK	= Color(0.f,    0.f,    0.f,	1.0f);
	const Color RED		= Color(1.f,	0.f,	0.f,	1.0f);
	const Color GREEN	= Color(0.f,	1.f,	0.f,	1.0f);
	const Color BLUE	= Color(0.f,	0.f,	1.f,	1.0f);
	const Color PURPLE	= Color(1.f,	0.f,	1.f,	1.0f);
	const Color YELLOW	= Color(1.f,	1.f,	0.f,	1.0f);
	const Color CYAN	= Color(0.f,	1.f,	1.f,	1.0f);
	const Color GRAY	= Color(0.4f,	0.4f,	0.4f,	1.0f);
	const Color RUST	= Color(0.82f,  0.35f,  0.15f,  1.0f);
}
