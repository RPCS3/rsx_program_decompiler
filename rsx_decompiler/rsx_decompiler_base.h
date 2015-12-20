#pragma once

#include "rsx_decompiler.h"
#include <clike_builder.h>

namespace rsx
{
	using namespace shader_code;

	template<typename Language>
	struct decompiler_base : clike_builder<Language>
	{
		using base = clike_builder<Language>;

		builder::writer_t writer;

		enum class compare_function
		{
			less,
			greater,
			equal,
			less_equal,
			greater_equal,
			not_equal
		};

		template<clike_language::type_class_t Type, int Count>
		static typename base::boolean_expr<Count> single_compare_function(compare_function function,
			typename base::expression_t<Type, Count> a, typename base::expression_t<Type, Count> b)
		{
			std::string operator_string;

			switch (function)
			{
			case compare_function::less: operator_string = "<"; break;
			case compare_function::greater: operator_string = ">"; break;
			case compare_function::equal: operator_string = "=="; break;
			case compare_function::less_equal: operator_string = "<="; break;
			case compare_function::greater_equal: operator_string = ">="; break;
			case compare_function::not_equal: operator_string = "!="; break;

			default:
				throw;
			}

			return a.to_string() + " " + operator_string + " " + b.to_string();
		}

		template<clike_language::type_class_t Type, int Count>
		static typename base::boolean_expr<Count> vector_compare_function(compare_function function,
			typename base::expression_t<Type, Count> a, typename base::expression_t<Type, Count> b)
		{
			switch (function)
			{
			case compare_function::less: return base::less(a, b);
			case compare_function::greater: return base::greater(a, b);
			case compare_function::equal: return base::equal(a, b);
			case compare_function::less_equal: return base::less_equal(a, b);
			case compare_function::greater_equal: return base::greater_equal(a, b);
			case compare_function::not_equal: return base::not_equal(a, b);
			}

			throw;
		}

		template<clike_language::type_class_t Type, int Count>
		static typename base::boolean_expr<Count> custom_compare(compare_function function, int channel_count,
			typename base::expression_t<Type, Count> a, typename base::expression_t<Type, Count> b)
		{
			if (channel_count == 1)
			{
				return  single_compare_function(function, a, b);
			}

			return vector_compare_function(function, a, b);
		}

		builder::writer_t comment(const std::string& lines)
		{
			return{ "//" + lines + "\n" };
		}

		builder::writer_t warning(const std::string& lines)
		{
			return comment("WARNING: " + lines);
		}

		builder::writer_t unimplemented(const std::string& lines)
		{
			return comment("TODO: " + lines);
		}
	};
}