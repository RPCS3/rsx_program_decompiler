#pragma once

#include "rsx_decompiler.h"
#include <clike_builder.h>

namespace rsx
{
	template<typename Language>
	struct decompiler_base : shader_code::clike_builder<Language>
	{
		enum class compare_function
		{
			less,
			greater,
			equal,
			less_equal,
			greater_equal,
			not_equal
		};

		template<type_class_t Type, int Count>
		static expression_from<boolean_t<Count>> single_compare_function(compare_function function, expression_t<Type, Count> a, expression_t<Type, Count> b)
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

		template<type_class_t Type, int Count>
		static expression_from<boolean_t<Count>> vector_compare_function(compare_function function, expression_t<Type, Count> a, expression_t<Type, Count> b)
		{
			switch (function)
			{
			case compare_function::less: return less(a, b);
			case compare_function::greater: return greater(a, b);
			case compare_function::equal: return equal(a, b);
			case compare_function::less_equal: return less_equal(a, b);
			case compare_function::greater_equal: return greater_equal(a, b);
			case compare_function::not_equal: return not_equal(a, b);
			}

			throw;
		}

		template<type_class_t Type, int Count>
		static expression_from<boolean_t<Count>> custom_compare(compare_function function, int channel_count, expression_t<Type, Count> a, expression_t<Type, Count> b)
		{
			if (channel_count == 1)
			{
				return  single_compare_function(function, a, b);
			}

			return vector_compare_function(function, a, b);
		}
	};
}