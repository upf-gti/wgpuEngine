#include <glm/matrix.hpp>

// from: https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644
class Frustum
{
public:
	Frustum() {}

	// m = ProjectionMatrix * ViewMatrix 
	Frustum(const glm::mat4& view_projection);

    void set_view_projection(const glm::mat4& view_projection);

	// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
	bool is_box_visible(const glm::vec3& minp, const glm::vec3& maxp) const;

private:
	enum Planes
	{
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far,
		Count,
		Combinations = Count * (Count - 1) / 2
	};

	template<Planes i, Planes j>
	struct ij2k
	{
		enum { k = i * (9 - i) / 2 + j - 1 };
	};

	template<Planes a, Planes b, Planes c>
	glm::vec3 intersection(const glm::vec3* crosses) const;
	
    glm::vec4   m_planes[Count] = {};
    glm::vec3   m_points[8] = {};
};
