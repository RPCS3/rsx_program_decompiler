#pragma once
#include "rsx_fp_ucode.h"
#include <vector>
#include <unordered_set>

namespace rsx
{
	enum class sampler_type
	{
		sampler_1d,
		sampler_2d,
		sampler_3d
		//TODO
	};

	enum class register_type
	{
		half_float_point,
		single_float_point,
		integer
	};

	struct texture_info
	{
		int id;
		sampler_type type;
		std::string name;

		bool operator==(const texture_info& rhs) const
		{
			return rhs.name == name;
		}

		std::size_t hash() const
		{
			return std::hash<std::string>{}(name);
		}
	};

	struct constant_info
	{
		int id;
		std::string name;

		bool operator==(const constant_info& rhs) const
		{
			return rhs.name == name;
		}

		std::size_t hash() const
		{
			return std::hash<std::string>{}(name);
		}
	};

	struct register_info
	{
		int id;
		register_type type;
		std::string name;

		bool operator==(const register_info& rhs) const
		{
			return rhs.name == name;
		}

		std::size_t hash() const
		{
			return std::hash<std::string>{}(name);
		}
	};

	struct hasher
	{
		template<typename Type>
		std::size_t operator()(const Type &obj) const
		{
			return obj.hash();
		}
	};

	struct decompiled_program
	{
		std::unordered_set<constant_info, hasher> constants;
		std::unordered_set<texture_info, hasher> textures;
		std::unordered_set<register_info, hasher> temporary_registers;
		unsigned int input_attributes = 0;
		unsigned int output_attributes = 0;

		std::string entry_function;
		std::string code;
	};

	struct complete_program
	{
		std::string code;
	};

	namespace fragment_program
	{
		decompiled_program decompile(std::size_t offset, ucode_instr* instructions);
	}

	inline complete_program finalize_program(const decompiled_program& program)
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
