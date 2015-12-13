#pragma once
#include "rsx_fp_ucode.h"
#include <vector>

/*
struct rsx_vertex_shader
{
	std::vector<unsigned int> data;

	struct hash_t
	{
		std::size_t operator ()(const rsx_vertex_shader& arg) const
		{
			return 0;
			//return arg.hash;
		}
	};

	bool operator ==(const rsx_vertex_shader& rhs) const
	{
		if (data.size() != rhs.data.size())
			return false;

		for (std::size_t i = 0; i < data.size(); ++i)
		{
			if (data[i] != rhs.data[i])
				return false;
		}

		return true;
	}
};

struct rsx_fragment_shader
{
	std::vector<unsigned int> data;

	struct hash_t
	{
		std::size_t operator ()(const rsx_fragment_shader& arg) const
		{
			return 0;
			//return arg.hash;
		}
	};

	bool operator ==(const rsx_fragment_shader& rhs) const
	{
		if (data.size() != rhs.data.size())
			return false;

		for (std::size_t i = 0; i < data.size(); ++i)
		{
			if (data[i] != rhs.data[i])
				return false;
		}

		return true;
	}
};



struct finalized_rsx_vertex_shader
{
	int input_attributes;

	std::string code;

	struct hash_t
	{
		std::size_t operator ()(const finalized_rsx_vertex_shader& arg) const
		{
			return 0;
			//return arg.hash;
		}
	};

	bool operator ==(const finalized_rsx_vertex_shader& rhs) const
	{
		return
			input_attributes == rhs.input_attributes &&
			code == rhs.code;
	}
};


struct finalized_rsx_fragment_shader
{
	int output_attributes;
	int control;

	std::string code;

	struct hash_t
	{
		std::size_t operator ()(const finalized_rsx_fragment_shader& arg) const
		{
			return 0;
			//return arg.hash;
		}
	};

	bool operator ==(const finalized_rsx_fragment_shader& rhs) const
	{
		return
			output_attributes == rhs.output_attributes &&
			control == rhs.control &&
			code == rhs.code;
	}
};*/

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
	};

	struct register_info
	{
		int id;
		register_type type;
	};

	struct decompiled_program
	{
		std::vector<std::size_t> constant_offsets;
		std::vector<std::string> uniforms;
		std::vector<texture_info> textures;
		std::vector<register_info> temporary_registers;
		unsigned int input_attributes;
		unsigned int output_attributes;

		std::string code;
	};

	namespace fragment_program
	{
		decompiled_program decompile(std::size_t offset, ucode_instr* instructions);
	}
}
