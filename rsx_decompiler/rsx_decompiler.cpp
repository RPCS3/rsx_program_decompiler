#include "rsx_decompiler.h"
#include <unordered_map>
#include <string>

namespace rsx
{
	const std::string index_to_channel[4] = { "x", "y", "z", "w" };
	const std::unordered_map<char, int> channel_to_index = { { 'x', 0 },{ 'y', 1 },{ 'z', 2 },{ 'w', 3 } };
	const std::string mask = "xyzw";

	complete_program finalize_program(const decompiled_program& program)
	{
		complete_program result{ "#version 420\n\n" };

		for (const constant_info& constant : program.constants)
		{
			result.code += "uniform vec4 " + constant.name + ";\n";
		}

		result.code += "\n";

		for (const register_info& temporary : program.temporary_registers)
		{
			result.code += "vec4 " + temporary.name + " = vec4(0.0);\n";
		}

		result.code += "\n";

		for (const texture_info& texture : program.textures)
		{
			result.code += "uniform sampler2D " + texture.name + ";\n";
		}

		for (std::size_t index = 0; index < std::size(rsx::fragment_program::input_attrib_map); ++index)
		{
			if (program.input_attributes & (1 << index))
			{
				result.code += "in vec4 " + rsx::fragment_program::input_attrib_map[index] + ";\n";
			}
		}

		result.code += "\n";
		result.code += program.code;

		result.code +=
			R"(
void main()
{
	)" + program.entry_function + R"(();
}
)";
		return result;
	}
}
