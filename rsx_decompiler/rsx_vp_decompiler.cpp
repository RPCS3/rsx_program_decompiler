#include "rsx_decompiler.h"

namespace rsx
{
	namespace vertex_program
	{
		decompiled_program decompile(std::size_t offset, ucode_instr* instructions, decompile_language lang)
		{
			decompiled_program result;

			switch (lang)
			{
			case decompile_language::glsl:
				//result = ...;

				result.code_language = decompile_language::glsl;
				break;

			default:
				throw;
			}

			result.type = program_type::vertex;
			return result;
		}
	}
}