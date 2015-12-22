#pragma once
#include "clike_language.h"

namespace shader_code
{
	namespace glsl_language_impl
	{
		template<clike_language::type_class_t Type, int Count>
		struct type_name_t;

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 1>
		{
			static constexpr auto name = "bool";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 2>
		{
			static constexpr auto name = "bvec2";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 3>
		{
			static constexpr auto name = "bvec3";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 4>
		{
			static constexpr auto name = "bvec4";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 1>
		{
			static constexpr auto name = "int";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 2>
		{
			static constexpr auto name = "ivec2";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 3>
		{
			static constexpr auto name = "ivec3";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 4>
		{
			static constexpr auto name = "ivec4";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 1>
		{
			static constexpr auto name = "float";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 2>
		{
			static constexpr auto name = "vec2";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 3>
		{
			static constexpr auto name = "vec3";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 4>
		{
			static constexpr auto name = "vec4";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_sampler1D, 1>
		{
			static constexpr auto name = "sampler1D";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_sampler2D, 1>
		{
			static constexpr auto name = "sampler2D";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_sampler3D, 1>
		{
			static constexpr auto name = "sampler3D";
		};



		template<clike_language::function_class_t Function>
		struct function_name_t;

		template<>
		struct function_name_t<clike_language::function_class_t::function_abs>
		{
			static constexpr auto name = "abs";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_normalize>
		{
			static constexpr auto name = "normalize";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_floor>
		{
			static constexpr auto name = "floor";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_fract>
		{
			static constexpr auto name = "fract";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_pow>
		{
			static constexpr auto name = "pow";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_texture>
		{
			static constexpr auto name = "texture";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_texture_grad>
		{
			static constexpr auto name = "textureGrad";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_texture_bias>
		{
			static constexpr auto name = "textureBias";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_texture_lod>
		{
			static constexpr auto name = "textureLod";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_any>
		{
			static constexpr auto name = "any";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_all>
		{
			static constexpr auto name = "all";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_dot>
		{
			static constexpr auto name = "dot";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_greater>
		{
			static constexpr auto name = "greaterThan";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_less>
		{
			static constexpr auto name = "lessThan";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_equal>
		{
			static constexpr auto name = "equal";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_greater_equal>
		{
			static constexpr auto name = "greaterThanEqual";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_less_equal>
		{
			static constexpr auto name = "lessThanEqual";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_not_equal>
		{
			static constexpr auto name = "notEqual";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_sin>
		{
			static constexpr auto name = "sin";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_cos>
		{
			static constexpr auto name = "cos";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_clamp>
		{
			static constexpr auto name = "clamp";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_min>
		{
			static constexpr auto name = "min";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_max>
		{
			static constexpr auto name = "max";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_sqrt>
		{
			static constexpr auto name = "sqrt";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_rsqrt>
		{
			static constexpr auto name = "inversesqrt";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_exp>
		{
			static constexpr auto name = "exp";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_log>
		{
			static constexpr auto name = "log";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_exp2>
		{
			static constexpr auto name = "exp2";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_log2>
		{
			static constexpr auto name = "log2";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_ddx>
		{
			static constexpr auto name = "dFdx";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_ddy>
		{
			static constexpr auto name = "dFdy";
		};
	}

	struct glsl_language
	{
		template<clike_language::type_class_t Type, int Count>
		using type_name_t = glsl_language_impl::type_name_t<Type, Count>;

		template<clike_language::function_class_t Function>
		using function_name_t = glsl_language_impl::function_name_t<Function>;
	};
}
