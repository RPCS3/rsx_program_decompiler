#pragma once
#include "rsx_fp_ucode.h"
#include "rsx_vp_ucode.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

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

	enum class decompile_language
	{
		glsl
	};

	enum class program_type
	{
		vertex,
		fragment
	};

	struct decompiled_program
	{
		program_type type;
		decompile_language code_language;

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

	extern const std::string index_to_channel[4];
	extern const std::unordered_map<char, int> channel_to_index;
	extern const std::string mask;

	complete_program finalize_program(const decompiled_program& program);


	namespace fragment_program
	{
		decompiled_program decompile(std::size_t offset, ucode_instr* instructions, decompile_language lang);
	}

	namespace vertex_program
	{
		decompiled_program decompile(std::size_t offset, ucode_instr* instructions, decompile_language lang);
	}
}

