#pragma once
#include "builder.h"
#include "clike_language.h"

namespace shader_code
{
	template<typename Language>
	struct clike_builder : clike_language, builder
	{
		using language = Language;

		template<type_class_t Type, int Count>
		struct type_helper_t : clike_language::type_t<Type, Count>
		{
		protected:
			template<type_class_t ExprType = Type, int ExprCount = Count>
			using expression_t = typename clike_language::expression_t<ExprType, ExprCount>;

		public:
			static expression_t<> ctor()
			{
				return invoke();
			}

			static expression_t<> ctor(typename native_type_t<Type>::type value)
			{
				return invoke(expression_t<Type, 1>(native_type_t<Type>::to_string(value)));
			}

			template<type_class_t FromType>
			static expression_t<> ctor(expression_t<FromType> expr)
			{
				return invoke(expr);
			}

			constexpr static const char* name()
			{
				return language::type_name_t<Type, Count>::name;
			}

		protected:
			template<typename... T>
			static expression_t<> invoke(T... exprs)
			{
				return function_t<clike_language::type_t<Type, Count>, language::type_name_t<Type, Count>>::invoke(exprs...);
			}
		};

		template<type_class_t Type, int Count>
		struct type_t;

		template<type_class_t Type>
		struct type_t<Type, 1> : type_helper_t<Type, 1>
		{
			using type_helper_t::ctor;
		};

		template<type_class_t Type>
		struct type_t<Type, 2> : type_helper_t<Type, 2>
		{
			using type_helper_t::ctor;

			template<type_class_t FromType>
			static expression_t<> ctor(expression_t<FromType, 1> expr)
			{
				return invoke(expr);
			}

			static expression_t<> ctor(expression_t<Type, 1> a, expression_t<Type, 1> b)
			{
				return invoke(a, b);
			}
		};

		template<type_class_t Type>
		struct type_t<Type, 3> : type_helper_t<Type, 3>
		{
			using type_helper_t::ctor;

			template<type_class_t FromType>
			static expression_t<> ctor(expression_t<FromType, 1> expr)
			{
				return invoke(expr);
			}

			static expression_t<> ctor(expression_t<Type, 1> a, expression_t<Type, 2> b)
			{
				return invoke(a, b);
			}

			static expression_t<> ctor(expression_t<Type, 2> a, expression_t<Type, 1> b)
			{
				return invoke(a, b);
			}

			static expression_t<> ctor(expression_t<Type, 1> a, expression_t<Type, 1> b, expression_t<Type, 1> c)
			{
				return invoke(a, b, c);
			}
		};

		template<type_class_t Type>
		struct type_t<Type, 4> : type_helper_t<Type, 4>
		{
			using type_helper_t::ctor;

			template<type_class_t FromType>
			static expression_t<> ctor(expression_t<FromType, 1> expr)
			{
				return invoke(expr);
			}

			static expression_t<> ctor(expression_t<Type, 1> a, expression_t<Type, 3> b)
			{
				return invoke(a, b);
			}
			static expression_t<> ctor(expression_t<Type, 3> a, expression_t<Type, 1> b)
			{
				return invoke(a, b);
			}
			static expression_t<> ctor(expression_t<Type, 2> a, expression_t<Type, 2> b)
			{
				return invoke(a, b);
			}
			static expression_t<> ctor(expression_t<Type, 1> a, expression_t<Type, 1> b, expression_t<Type, 1> c, expression_t<Type, 1> d)
			{
				return invoke(a, b, c, d);
			}
			static expression_t<> ctor(expression_t<Type, 2> a, expression_t<Type, 1> b, expression_t<Type, 1> c)
			{
				return invoke(a, b, c);
			}
			static expression_t<> ctor(expression_t<Type, 1> a, expression_t<Type, 2> b, expression_t<Type, 1> c)
			{
				return invoke(a, b, c);
			}
			static expression_t<> ctor(expression_t<Type, 1> a, expression_t<Type, 1> b, expression_t<Type, 2> c)
			{
				return invoke(a, b, c);
			}
		};

		using sampler1D_t = typename type_t<type_class_t::type_sampler1D, 1>;
		using sampler2D_t = typename type_t<type_class_t::type_sampler2D, 1>;
		using sampler3D_t = typename type_t<type_class_t::type_sampler3D, 1>;

		template<size_t Count>
		using boolean_t = typename type_t<type_class_t::type_bool, Count>;

		template<size_t Count>
		using integer_t = typename type_t<type_class_t::type_int, Count>;

		template<size_t Count>
		using float_point_t = typename type_t<type_class_t::type_float, Count>;

		static expression_from<float_point_t<4>> texture(const expression_from<sampler1D_t>& texture, const expression_from<float_point_t<1>>& coord)
		{
			return function_t<float_point_t<4>, language::function_name_t<function_class_t::function_texture>>::invoke(texture, coord);
		}

		static expression_from<float_point_t<4>> texture(const expression_from<sampler2D_t>& texture, const expression_from<float_point_t<2>>& coord)
		{
			return function_t<float_point_t<4>, language::function_name_t<function_class_t::function_texture>>::invoke(texture, coord);
		}

		static expression_from<float_point_t<4>> texture_lod(const expression_from<sampler2D_t>& texture, const expression_from<float_point_t<2>>& coord, const expression_from<float_point_t<1>>& lod)
		{
			return function_t<float_point_t<4>, language::function_name_t<function_class_t::function_texture_lod>>::invoke(texture, coord, lod);
		}

		static expression_from<float_point_t<4>> texture_bias(const expression_from<sampler2D_t>& texture, const expression_from<float_point_t<2>>& coord, const expression_from<float_point_t<1>>& bias)
		{
			return function_t<float_point_t<4>, language::function_name_t<function_class_t::function_texture_bias>>::invoke(texture, coord, bias);
		}

		static expression_from<float_point_t<4>> texture_grad(const expression_from<sampler2D_t>& texture, const expression_from<float_point_t<2>>& coord, const expression_from<float_point_t<2>>& ddx, const expression_from<float_point_t<2>>& ddy)
		{
			return function_t<float_point_t<4>, language::function_name_t<function_class_t::function_texture_grad>>::invoke(texture, coord, ddx, ddy);
		}

		static expression_from<float_point_t<4>> texture(const expression_from<sampler3D_t>& texture, const expression_from<float_point_t<3>>& coord)
		{
			return function_t<float_point_t<4>, language::function_name_t<function_class_t::function_texture>>::invoke(texture, coord);
		}

		template<int Count>
		static expression_from<float_point_t<Count>> normalize(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_normalize>>::invoke(arg);
		}

		template<int Count>
		static expression_from<float_point_t<Count>> abs(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_abs>>::invoke(arg);
		}

		template<int Count>
		static expression_from<integer_t<Count>> abs(const expression_t<type_class_t::type_int, Count>& arg)
		{
			return function_t<integer_t<Count>, language::function_name_t<function_class_t::function_abs>>::invoke(arg);
		}

		template<int Count>
		static expression_from<float_point_t<Count>> normalize(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_normalize>>::invoke(arg);
		}

		template<int Count>
		static expression_from<boolean_t<1>> any(const expression_t<type_class_t::type_bool, Count>& arg)
		{
			return function_t<boolean_t<1>, language::function_name_t<function_class_t::function_any>>::invoke(arg);
		}

		template<int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<boolean_t<1>> all(const expression_t<type_class_t::type_bool, Count>& arg)
		{
			return function_t<boolean_t<1>, language::function_name_t<function_class_t::function_all>>::invoke(arg);
		}

		template<int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<float_point_t<1>> dot(const expression_t<type_class_t::type_float, Count>& a, const expression_t<type_class_t::type_float, Count>& b)
		{
			return function_t<float_point_t<1>, language::function_name_t<function_class_t::function_dot>>::invoke(a, b);
		}

		template<type_class_t Type, int Count>
		static expression_t<Type, Count> min(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<type_t<Type, Count>, language::function_name_t<function_class_t::function_min>>::invoke(a, b);
		}

		template<type_class_t Type, int Count>
		static expression_t<Type, Count> max(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<type_t<Type, Count>, language::function_name_t<function_class_t::function_max>>::invoke(a, b);
		}

		template<type_class_t Type, int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<boolean_t<Count>> greater(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<boolean_t<Count>, language::function_name_t<function_class_t::function_greater>>::invoke(a, b);
		}

		template<type_class_t Type, int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<boolean_t<Count>> less(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<boolean_t<Count>, language::function_name_t<function_class_t::function_less>>::invoke(a, b);
		}

		template<type_class_t Type, int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<boolean_t<Count>> equal(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<boolean_t<Count>, language::function_name_t<function_class_t::function_equal>>::invoke(a, b);
		}

		template<type_class_t Type, int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<boolean_t<Count>> greater_equal(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<boolean_t<Count>, language::function_name_t<function_class_t::function_greater_equal>>::invoke(a, b);
		}

		template<type_class_t Type, int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<boolean_t<Count>> less_equal(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<boolean_t<Count>, language::function_name_t<function_class_t::function_less_equal>>::invoke(a, b);
		}

		template<type_class_t Type, int Count, typename = std::enable_if_t<(Count > 1)>>
		static expression_from<boolean_t<Count>> not_equal(const expression_t<Type, Count>& a, const expression_t<Type, Count>& b)
		{
			return function_t<boolean_t<Count>, language::function_name_t<function_class_t::function_not_equal>>::invoke(a, b);
		}

		template<int Count>
		static expression_from<float_point_t<Count>> fract(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_fract>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> floor(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_floor>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> sin(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_sin>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> cos(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_cos>>::invoke(arg);
		}
		template<type_class_t Type, int Count>
		static expression_from<expression_t<Type, Count>> clamp(const expression_t<Type, Count>& a, const expression_t<Type, 1>& b, const expression_t<Type, 1>& c)
		{
			return function_t<type_t<Type, Count>, language::function_name_t<function_class_t::function_clamp>>::invoke(a, b, c);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> sqrt(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_sqrt>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> rsqrt(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_rsqrt>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> pow(const expression_t<type_class_t::type_float, Count>& a, const expression_t<type_class_t::type_float, Count>& b)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_pow>>::invoke(a, b);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> exp2(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_exp2>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> log2(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_log2>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> ddx(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_ddx>>::invoke(arg);
		}
		template<int Count>
		static expression_from<float_point_t<Count>> ddy(const expression_t<type_class_t::type_float, Count>& arg)
		{
			return function_t<float_point_t<Count>, language::function_name_t<function_class_t::function_ddy>>::invoke(arg);
		}
	};
}