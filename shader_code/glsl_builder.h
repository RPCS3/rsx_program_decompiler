#pragma once
#include "clike_builder.h"
#include "glsl_language.h"

namespace shader_code
{
	struct glsl_builder : clike_builder<glsl_language>
	{
		using bool_t = boolean_t<1>;
		using bvec2_t = boolean_t<2>;
		using bvec3_t = boolean_t<3>;
		using bvec4_t = boolean_t<4>;

		using int_t = integer_t<1>;
		using ivec2_t = integer_t<2>;
		using ivec3_t = integer_t<3>;
		using ivec4_t = integer_t<4>;

		using float_t = float_point_t<1>;
		using vec2_t = float_point_t<2>;
		using vec3_t = float_point_t<3>;
		using vec4_t = float_point_t<4>;
	};
}
