#pragma once
#include "builder.h"

namespace shader_code
{
	struct clike_language
	{
		enum class type_class_t
		{
			type_void,
			type_bool,
			type_int,
			type_float,
			type_sampler1D,
			type_sampler2D,
			type_sampler3D
		};

		template<typename Type>
		struct native_type_base_t
		{
			using type = Type;

			static std::string to_string(Type value)
			{
				return std::to_string(value);
			}
		};

		template<type_class_t Type>
		struct native_type_t;

		template<>
		struct native_type_t<type_class_t::type_bool>
		{
			using type = bool;

			static std::string to_string(type value)
			{
				return value ? "true" : "false";
			}
		};

		template<>
		struct native_type_t<type_class_t::type_int> : native_type_base_t<int>
		{
		};

		template<>
		struct native_type_t<type_class_t::type_float> : native_type_base_t<float>
		{
		};

		template<>
		struct native_type_t<type_class_t::type_sampler1D> : native_type_base_t<int>
		{
		};

		template<>
		struct native_type_t<type_class_t::type_sampler2D> : native_type_base_t<int>
		{
		};

		template<>
		struct native_type_t<type_class_t::type_sampler3D> : native_type_base_t<int>
		{
		};

		template<type_class_t Type, int Count>
		using type_t = builder::type_t<type_class_t, Type, Count>;

		template<size_t Count>
		using boolean_t = typename type_t<type_class_t::type_bool, Count>;

		template<size_t Count>
		using integer_t = typename type_t<type_class_t::type_int, Count>;

		template<size_t Count>
		using float_point_t = typename type_t<type_class_t::type_float, Count>;

		using sampler1D_t = type_t<type_class_t::type_sampler1D, 1>;
		using sampler2D_t = type_t<type_class_t::type_sampler2D, 1>;
		using sampler3D_t = type_t<type_class_t::type_sampler3D, 1>;
		using void_t = type_t<type_class_t::type_void, 0>;

		enum function_class_t
		{
			function_abs,
			function_fract,
			function_floor,
			function_exp,
			function_log,
			function_exp2,
			function_log2,
			function_pow,
			function_texture,
			function_texture_lod,
			function_texture_bias,
			function_texture_grad,
			function_normalize,
			function_any,
			function_all,
			function_dot,
			function_min,
			function_max,
			function_greater,
			function_less,
			function_equal,
			function_greater_equal,
			function_less_equal,
			function_not_equal,
			function_sin,
			function_cos,
			function_clamp,
			function_sqrt,
			function_rsqrt,
			function_ddx,
			function_ddy
		};

		template<type_class_t Type, int Count>
		struct expression_t;

		template<typename Type>
		using expression_from = expression_t<Type::type, Type::count>;

		template<type_class_t Type, int Count>
		struct expression_helper_t : builder::expression_base_t
		{
			using type = type_t<Type, Count>;

			std::string mask;
			bool is_single;
			int base_count = Count;

			expression_helper_t() = default;

			expression_helper_t(const std::string& text, bool is_single = true, int base_count = Count)
				: expression_base_t{ text }
				, is_single(is_single)
				, mask{ std::string("xyzw").substr(0, base_count) }
				, base_count(base_count)
			{
			}

			expression_helper_t(const std::string& text, const std::string& mask, bool is_single = true, int base_count = Count)
				: expression_base_t{ text }
				, is_single(is_single)
				, mask(mask)
				, base_count(base_count)
			{
			}

			template<size_t N>
			expression_helper_t(const std::string& text, const char (&mask)[N], bool is_single = true, int base_count = Count)
				: expression_helper_t{ text, std::string(mask), is_single, base_count }
			{
				static_assert(N == Count + 1, "Bad swizzle!");
			}

			void assign(const expression_helper_t& rhs)
			{
				text = rhs.text;
				mask = rhs.mask;
				is_single = rhs.is_single;
				base_count = rhs.base_count;
			}

			template<typename Type>
			expression_t<Type::type, Type::count> as() const
			{
				return{ text, mask, is_single, base_count };
			}

			expression_t<Type, Count> with_mask(const std::string &mask) const
			{
				return{ text, mask, is_single, base_count };
			}

			template<typename... Channels>
			auto swizzle(Channels... channels) const -> expression_t<Type, sizeof...(channels)>
			{
				static_assert(sizeof...(channels) <= 4 && sizeof...(channels) > 0, "bad swizzle");

				std::string new_mask;

				using sw = std::string[]; sw{ (new_mask += mask.substr(channels, 1))... };
				
				return{ !is_single ? "(" + text + ")" : text, new_mask, is_single, base_count };
			}

			std::string to_string() const override
			{
				if (mask.empty() || mask == std::string("xyzw").substr(0, base_count))
				{
					if (!is_single)
					{
						return "(" + text + ")";
					}

					return text;
				}

				if (!is_single)
				{
					return "(" + text + ")." + mask;
				}

				return text + "." + mask;
			}

			std::string finalize(bool put_end) const override
			{
				if (mask.empty() || mask == std::string("xyzw").substr(0, base_count))
				{
					return text + (put_end ? ";" : "");
				}

				return text + "." + mask + (put_end ? ";" : "");
			}

			expression_t<Type, Count> scope() const
			{
				return{ to_string() };
			}

			expression_t<Type, Count> without_scope() const
			{
				return{ text, mask, true, base_count };
			}

		//protected:
			template<typename RetType = type, typename ArgType>
			expression_t<RetType::type, RetType::count> call_operator(const std::string& opname, const ArgType& rhs) const
			{
				return{ to_string() + " " + opname + " " + rhs.to_string(), false };
			}

			template<typename RetType = type>
			expression_t<RetType::type, RetType::count> call_operator(const std::string& opname) const
			{
				return{ opname + to_string(), true };
			}
		};

		template<type_class_t Type, int Count>
		struct expression_ctors_t : expression_helper_t<Type, Count>
		{
			using expression_helper_t::expression_helper_t;
		};

		template<type_class_t Type>
		struct expression_ctors_t<Type, 1> : expression_helper_t<Type, 1>
		{
			expression_ctors_t(typename native_type_t<Type>::type value)
				: expression_helper_t(native_type_t<Type>::to_string(value))
			{
			}

			using expression_helper_t::expression_helper_t;
		};

#define _SWIZZLE_MASK(pos) mask.substr(pos, 1)
#define _SWIZZLE_FUNC(Count, Name, ...) expression_t<Type, Count> Name() const { return swizzle(__VA_ARGS__); }

#define SWIZZLE1(Xn, Xv) _SWIZZLE_FUNC(1, Xn, (Xv))
#define SWIZZLE2(Xn, Xv, Yn, Yv) _SWIZZLE_FUNC(2, Xn##Yn, Xv, Yv)
#define SWIZZLE3(Xn, Xv, Yn, Yv, Zn, Zv) _SWIZZLE_FUNC(3, Xn##Yn##Zn, Xv, Yv, Zv)
#define SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, Wn, Wv) _SWIZZLE_FUNC(4, Xn##Yn##Zn##Wn, Xv, Yv, Zv, Wv)


#define SWIZZLE4_2_1(Xn, Xv) \
		SWIZZLE2(Xn, Xv, x, 0) \
		SWIZZLE2(Xn, Xv, y, 1) \
		SWIZZLE2(Xn, Xv, z, 2) \
		SWIZZLE2(Xn, Xv, w, 3) \

#define SWIZZLE3_2_1(Xn, Xv) \
		SWIZZLE2(Xn, Xv, x, 0) \
		SWIZZLE2(Xn, Xv, y, 1) \
		SWIZZLE2(Xn, Xv, z, 2) \

#define SWIZZLE2_2_1(Xn, Xv) \
		SWIZZLE2(Xn, Xv, x, 0) \
		SWIZZLE2(Xn, Xv, y, 1) \

#define SWIZZLE4_3_2(Xn, Xv, Yn, Yv) \
		SWIZZLE3(Xn, Xv, Yn, Yv, x, 0) \
		SWIZZLE3(Xn, Xv, Yn, Yv, y, 1) \
		SWIZZLE3(Xn, Xv, Yn, Yv, z, 2) \
		SWIZZLE3(Xn, Xv, Yn, Yv, w, 3) \

#define SWIZZLE3_3_2(Xn, Xv, Yn, Yv) \
		SWIZZLE3(Xn, Xv, Yn, Yv, x, 0) \
		SWIZZLE3(Xn, Xv, Yn, Yv, y, 1) \
		SWIZZLE3(Xn, Xv, Yn, Yv, z, 2) \

#define SWIZZLE2_3_2(Xn, Xv, Yn, Yv) \
		SWIZZLE3(Xn, Xv, Yn, Yv, x, 0) \
		SWIZZLE3(Xn, Xv, Yn, Yv, y, 1) \

#define SWIZZLE4_3_1(Xn, Xv) \
		SWIZZLE4_3_2(Xn, Xv, x, 0) \
		SWIZZLE4_3_2(Xn, Xv, y, 1) \
		SWIZZLE4_3_2(Xn, Xv, z, 2) \
		SWIZZLE4_3_2(Xn, Xv, w, 3) \

#define SWIZZLE3_3_1(Xn, Xv) \
		SWIZZLE3_3_2(Xn, Xv, x, 0) \
		SWIZZLE3_3_2(Xn, Xv, y, 1) \
		SWIZZLE3_3_2(Xn, Xv, z, 2) \

#define SWIZZLE2_3_1(Xn, Xv) \
		SWIZZLE2_3_2(Xn, Xv, x, 0) \
		SWIZZLE2_3_2(Xn, Xv, y, 1) \


#define SWIZZLE4_4_3(Xn, Xv, Yn, Yv, Zn, Zv) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, x, 0) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, y, 1) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, z, 2) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, w, 3) \

#define SWIZZLE3_4_3(Xn, Xv, Yn, Yv, Zn, Zv) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, x, 0) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, y, 1) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, z, 2) \

#define SWIZZLE2_4_3(Xn, Xv, Yn, Yv, Zn, Zv) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, x, 0) \
		SWIZZLE4(Xn, Xv, Yn, Yv, Zn, Zv, y, 1) \

#define SWIZZLE4_4_2(Xn, Xv, Yn, Yv) \
		SWIZZLE4_4_3(Xn, Xv, Yn, Yv, x, 0) \
		SWIZZLE4_4_3(Xn, Xv, Yn, Yv, y, 1) \
		SWIZZLE4_4_3(Xn, Xv, Yn, Yv, z, 2) \
		SWIZZLE4_4_3(Xn, Xv, Yn, Yv, w, 3) \

#define SWIZZLE3_4_2(Xn, Xv, Yn, Yv) \
		SWIZZLE3_4_3(Xn, Xv, Yn, Yv, x, 0) \
		SWIZZLE3_4_3(Xn, Xv, Yn, Yv, y, 1) \
		SWIZZLE3_4_3(Xn, Xv, Yn, Yv, z, 2) \

#define SWIZZLE2_4_2(Xn, Xv, Yn, Yv) \
		SWIZZLE2_4_3(Xn, Xv, Yn, Yv, x, 0) \
		SWIZZLE2_4_3(Xn, Xv, Yn, Yv, y, 1) \

#define SWIZZLE4_4_1(Xn, Xv) \
		SWIZZLE4_4_2(Xn, Xv, x, 0) \
		SWIZZLE4_4_2(Xn, Xv, y, 1) \
		SWIZZLE4_4_2(Xn, Xv, z, 2) \
		SWIZZLE4_4_2(Xn, Xv, w, 3) \

#define SWIZZLE3_4_1(Xn, Xv) \
		SWIZZLE3_4_2(Xn, Xv, x, 0) \
		SWIZZLE3_4_2(Xn, Xv, y, 1) \
		SWIZZLE3_4_2(Xn, Xv, z, 2) \

#define SWIZZLE2_4_1(Xn, Xv) \
		SWIZZLE2_4_2(Xn, Xv, x, 0) \
		SWIZZLE2_4_2(Xn, Xv, y, 1) \


		template<type_class_t Type, int Count>
		struct expression_swizzle_t : expression_ctors_t<Type, Count>
		{
			using expression_ctors_t::expression_ctors_t;
		};

		template<type_class_t Type>
		struct expression_swizzle_t<Type, 2> : expression_ctors_t<Type, 2>
		{
			using expression_ctors_t::expression_ctors_t;

			SWIZZLE1(x, 0)
				SWIZZLE1(y, 1)

				SWIZZLE2_2_1(x, 0)
				SWIZZLE2_2_1(y, 1)

				SWIZZLE2_3_1(x, 0)
				SWIZZLE2_3_1(y, 1)

				SWIZZLE2_4_1(x, 0)
				SWIZZLE2_4_1(y, 1)
		};

		template<type_class_t Type>
		struct expression_swizzle_t<Type, 3> : expression_ctors_t<Type, 3>
		{
			using expression_ctors_t::expression_ctors_t;

			SWIZZLE1(x, 0)
				SWIZZLE1(y, 1)
				SWIZZLE1(z, 2)

				SWIZZLE3_2_1(x, 0)
				SWIZZLE3_2_1(y, 1)
				SWIZZLE3_2_1(z, 2)

				SWIZZLE3_3_1(x, 0)
				SWIZZLE3_3_1(y, 1)
				SWIZZLE3_3_1(z, 2)

				SWIZZLE3_4_1(x, 0)
				SWIZZLE3_4_1(y, 1)
				SWIZZLE3_4_1(z, 2)
		};

		template<type_class_t Type>
		struct expression_swizzle_t<Type, 4> : expression_ctors_t<Type, 4>
		{
			using expression_ctors_t::expression_ctors_t;

			SWIZZLE1(x, 0)
				SWIZZLE1(y, 1)
				SWIZZLE1(z, 2)
				SWIZZLE1(w, 3)

				SWIZZLE4_2_1(x, 0)
				SWIZZLE4_2_1(y, 1)
				SWIZZLE4_2_1(z, 2)
				SWIZZLE4_2_1(w, 3)

				SWIZZLE4_3_1(x, 0)
				SWIZZLE4_3_1(y, 1)
				SWIZZLE4_3_1(z, 2)
				SWIZZLE4_3_1(w, 3)

				SWIZZLE4_4_1(x, 0)
				SWIZZLE4_4_1(y, 1)
				SWIZZLE4_4_1(z, 2)
				SWIZZLE4_4_1(w, 3)
		};

		template<type_class_t Type, int Count>
		struct expression_t : expression_swizzle_t<Type, Count>
		{
			using expression_swizzle_t::expression_swizzle_t;

			const expression_t operator -()
			{
				if (is_single && text[0] == '-')
					return expression_t{ text.substr(1), mask };

				return call_operator("-");
			}

			const expression_t operator-(const expression_t& rhs) const
			{
				if (rhs.is_single && rhs.text[0] == '-')
					return call_operator("+", expression_t{ rhs.text.substr(1), rhs.mask });

				return call_operator("-", rhs);
			}
			const expression_t operator+(const expression_t& rhs) const
			{
				if (rhs.is_single && rhs.text[0] == '-')
					return call_operator("-", expression_t{ rhs.text.substr(1), rhs.mask });

				return call_operator("+", rhs);
			}
			const expression_t operator/(const expression_t& rhs) const { return call_operator("/", rhs); }
			const expression_t operator*(const expression_t& rhs) const { return call_operator("*", rhs); }

			expression_t operator-=(const expression_t& rhs) { return call_operator("-=", rhs); }
			expression_t operator+=(const expression_t& rhs) { return call_operator("+=", rhs); }
			expression_t operator/=(const expression_t& rhs) { return call_operator("/=", rhs); }
			expression_t operator*=(const expression_t& rhs) { return call_operator("*=", rhs); }

			expression_t operator=(const expression_t& rhs) { return call_operator("=", rhs); }

			const expression_t operator-(const expression_t<Type, 1>& rhs) const { return call_operator("-", rhs); }
			const expression_t operator+(const expression_t<Type, 1>& rhs) const { return call_operator("+", rhs); }
			const expression_t operator/(const expression_t<Type, 1>& rhs) const { return call_operator("/", rhs); }
			const expression_t operator*(const expression_t<Type, 1>& rhs) const { return call_operator("*", rhs); }

			expression_t operator-=(const expression_t<Type, 1>& rhs) { return call_operator("-=", rhs); }
			expression_t operator+=(const expression_t<Type, 1>& rhs) { return call_operator("+=", rhs); }
			expression_t operator/=(const expression_t<Type, 1>& rhs) { return call_operator("/=", rhs); }
			expression_t operator*=(const expression_t<Type, 1>& rhs) { return call_operator("*=", rhs); }
		};

		template<>
		struct expression_t<type_class_t::type_bool, 1> : expression_swizzle_t<type_class_t::type_bool, 1>
		{
			using expression_swizzle_t::expression_swizzle_t;

			expression_t operator=(const expression_t& rhs) { return call_operator("=", rhs); }

			const expression_from<boolean_t<1>> operator!() const { return call_operator<boolean_t<1>>("!"); }
			const expression_from<boolean_t<1>> operator==(const expression_t& rhs) const { return call_operator<boolean_t<1>>("==", rhs); }
			const expression_from<boolean_t<1>> operator!=(const expression_t& rhs) const { return call_operator<boolean_t<1>>("!=", rhs); }
		};

		template<int Count>
		struct expression_t<type_class_t::type_bool, Count> : expression_swizzle_t<type_class_t::type_bool, Count>
		{
			using expression_swizzle_t::expression_swizzle_t;

			expression_t operator=(const expression_t& rhs) { return call_operator("=", rhs); }
		};

		template<type_class_t Type>
		struct expression_t<Type, 1> : expression_swizzle_t<Type, 1>
		{
			using expression_swizzle_t::expression_swizzle_t;

			const expression_t operator -()
			{
				if (is_single && text[0] == '-')
					return expression_t{ text.substr(1), mask };

				return call_operator("-");
			}

			const expression_t operator-(const expression_t& rhs) const
			{
				if (rhs.is_single && rhs.text[0] == '-')
					return call_operator("+", expression_t{ rhs.text.substr(1), rhs.mask });

				return call_operator("-", rhs);
			}
			const expression_t operator+(const expression_t& rhs) const
			{
				if (rhs.is_single && rhs.text[0] == '-')
					return call_operator("-", expression_t{ rhs.text.substr(1), rhs.mask });

				return call_operator("+", rhs);
			}
			template<int Count> const expression_t<Type, Count> operator/(const expression_t<Type, Count>& rhs) const { return call_operator<type_t<Type, Count>>("/", rhs); }
			template<int Count> const expression_t<Type, Count> operator*(const expression_t<Type, Count>& rhs) const { return call_operator<type_t<Type, Count>>("*", rhs); }

			expression_t operator=(const expression_t& rhs) { return call_operator("=", rhs); }

			expression_t operator-=(const expression_t& rhs) { return call_operator("-=", rhs); }
			expression_t operator+=(const expression_t& rhs) { return call_operator("+=", rhs); }
			expression_t operator/=(const expression_t& rhs) { return call_operator("/=", rhs); }
			expression_t operator*=(const expression_t& rhs) { return call_operator("*=", rhs); }

			const expression_from<boolean_t<1>> operator >(const expression_t& rhs) const { return call_operator<boolean_t<1>>(">", rhs); }
			const expression_from<boolean_t<1>> operator >=(const expression_t& rhs) const { return call_operator<boolean_t<1>>(">=", rhs); }
			const expression_from<boolean_t<1>> operator <(const expression_t& rhs) const { return call_operator<boolean_t<1>>("<", rhs); }
			const expression_from<boolean_t<1>> operator <=(const expression_t& rhs) const { return call_operator<boolean_t<1>>("<=", rhs); }
			const expression_from<boolean_t<1>> operator==(const expression_t& rhs) const { return call_operator<boolean_t<1>>("==", rhs); }
			const expression_from<boolean_t<1>> operator!=(const expression_t& rhs) const { return call_operator<boolean_t<1>>("!=", rhs); }
		};

		template<typename Type>
		static expression_t<(type_class_t)Type::type, Type::count> expression(const std::string& text, bool is_single = true)
		{
			return{ text, is_single };
		}

		template<typename Type, typename NameType>
		struct function_t
		{
			using return_type = Type;

			template<typename... ArgsType>
			static expression_t<(type_class_t)return_type::type, return_type::count> invoke(const ArgsType&... args)
			{
				std::string result;

				using sw = std::string[]; sw{ (result += (result.empty() ? "" : ", ") + args.finalize(false))..., "" };

				return{ std::string(NameType::name) + "(" + result + ")" };
			}
		};

		template<typename ReturnType, typename... ArgsType, typename NameType>
		struct function_t<ReturnType(ArgsType...), NameType>
		{
			using return_type = ReturnType;

			static expression_from<return_type> invoke(const expression_from<ArgsType>&... args)
			{
				std::string result = std::string(NameType::name) + "(";

				using sw = std::string[]; sw{ (result += args.to_string())..., "" };

				return{ result + ")" };
			}
		};

		template<type_class_t Type, int Count>
		static expression_from<void_t> if_(expression_from<boolean_t<1>> condition, expression_t<Type, Count> then)
		{
			return expression<void_t>("if (" + condition.finalize(false) + ") " + then.finalize(false));
		}

		template<type_class_t ThenType, int ThenCount, type_class_t ElseType, int ElseCount>
		static expression_from<void_t> if_(expression_from<boolean_t<1>> condition, expression_t<ThenType, ThenCount> then, expression_t<ElseType, ElseCount> else_)
		{
			return expression<void_t>("if (" + condition.finalize(false) + ") " + then.finalize() + " else " + else_.finalize(false));
		}

		static expression_from<void_t> begin_block();
		static expression_from<void_t> end_block();
	};
}