#include "rsx_decompiler.h"
#include <unordered_map>
#include <string>
#include <iterator>

namespace rsx
{
	const std::string index_to_channel[4] = { "x", "y", "z", "w" };
	const std::unordered_map<char, int> channel_to_index = { { 'x', 0 },{ 'y', 1 },{ 'z', 2 },{ 'w', 3 } };
	const std::string mask = "xyzw";

	complete_shader finalize_program(const decompiled_shader& program)
	{
		complete_shader result{ "#version 420\n\n" };

		for (const constant_info& constant : program.constants)
		{
			result.code += "uniform vec4 " + constant.name + ";\n";
		}

		result.code += "\n";

		for (const register_info& temporary : program.temporary_registers)
		{
			std::string type;
			switch (temporary.type)
			{
			case register_type::half_float_point:
			case register_type::single_float_point:
				type += "vec4";
				break;

			case register_type::integer:
				type += "ivec4";
				break;

			default:
				throw;
			}

			result.code += type + " " + temporary.name + " = " + type + "(0);\n";
		}

		result.code += "\n";

		for (const texture_info& texture : program.textures)
		{
			result.code += "uniform sampler2D " + texture.name + ";\n";
		}

		switch (program.type)
		{
		case program_type::fragment:
			for (std::size_t index = 0; index < 16; ++index)
			{
				if (program.input_attributes & (1 << index))
				{
					result.code += "in vec4 " + rsx::fragment_program::input_attrib_names[index] + ";\n";
				}
			}
			break;

		case program_type::vertex:
			for (std::size_t index = 0; index < 16; ++index)
			{
				if (program.input_attributes & (1 << index))
				{
					result.code += "in vec4 " + rsx::vertex_program::input_attrib_names[index] + ";\n";
				}
			}
			break;

		default:
			throw;
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
