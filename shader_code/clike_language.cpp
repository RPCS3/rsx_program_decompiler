#include "pch.h"
#include "clike_language.h"

namespace shader_code
{
	clike_language::expression_from<clike_language::void_t> clike_language::begin_block()
	{
		return expression<void_t>("\n{\n");
	}

	clike_language::expression_from<clike_language::void_t> clike_language::end_block()
	{
		return expression<void_t>("\n}\n");
	}
}