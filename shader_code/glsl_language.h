#pragma once
#include "clike_language.h"

namespace shader_code
{
	struct glsl_language
	{
		template<clike_language::type_class_t Type, int Count>
		struct type_name_t;

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 1>
		{
			static constexpr char *name = "bool";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 2>
		{
			static constexpr char *name = "bvec2";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 3>
		{
			static constexpr char *name = "bvec3";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_bool, 4>
		{
			static constexpr char *name = "bvec4";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 1>
		{
			static constexpr char *name = "int";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 2>
		{
			static constexpr char *name = "ivec2";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 3>
		{
			static constexpr char *name = "ivec3";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_int, 4>
		{
			static constexpr char *name = "ivec4";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 1>
		{
			static constexpr char *name = "float";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 2>
		{
			static constexpr char *name = "vec2";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 3>
		{
			static constexpr char *name = "vec3";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_float, 4>
		{
			static constexpr char *name = "vec4";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_sampler1D, 1>
		{
			static constexpr char *name = "sampler1D";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_sampler2D, 1>
		{
			static constexpr char *name = "sampler2D";
		};

		template<>
		struct type_name_t<clike_language::type_class_t::type_sampler3D, 1>
		{
			static constexpr char *name = "sampler3D";
		};



		template<clike_language::function_class_t Function>
		struct function_name_t;

		template<>
		struct function_name_t<clike_language::function_class_t::function_abs>
		{
			static constexpr char *name = "abs";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_normalize>
		{
			static constexpr char *name = "normalize";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_floor>
		{
			static constexpr char *name = "floor";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_fract>
		{
			static constexpr char *name = "fract";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_pow>
		{
			static constexpr char *name = "pow";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_texture>
		{
			static constexpr char *name = "texture";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_normalize>
		{
			static constexpr char *name = "normalize";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_any>
		{
			static constexpr char *name = "any";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_all>
		{
			static constexpr char *name = "all";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_dot>
		{
			static constexpr char *name = "dot";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_greater>
		{
			static constexpr char *name = "greaterThan";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_less>
		{
			static constexpr char *name = "lessThan";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_equal>
		{
			static constexpr char *name = "equal";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_greater_equal>
		{
			static constexpr char *name = "greaterThanEqual";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_less_equal>
		{
			static constexpr char *name = "lessThanEqual";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_not_equal>
		{
			static constexpr char *name = "notEqual";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_sin>
		{
			static constexpr char *name = "sin";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_cos>
		{
			static constexpr char *name = "cos";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_clamp>
		{
			static constexpr char *name = "clamp";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_min>
		{
			static constexpr char *name = "min";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_max>
		{
			static constexpr char *name = "max";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_sqrt>
		{
			static constexpr char *name = "sqrt";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_rsqrt>
		{
			static constexpr char *name = "inversesqrt";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_exp2>
		{
			static constexpr char *name = "exp2";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_log2>
		{
			static constexpr char *name = "log2";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_ddx>
		{
			static constexpr char *name = "dFdx";
		};

		template<>
		struct function_name_t<clike_language::function_class_t::function_ddy>
		{
			static constexpr char *name = "dFdy";
		};
	};
}