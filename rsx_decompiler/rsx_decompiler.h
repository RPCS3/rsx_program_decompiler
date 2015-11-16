#pragma once
#include <vector>

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

struct decompiled_rsx_shader
{
	std::vector<std::size_t> constant_offsets;
	std::vector<std::string> uniforms;
	int input_attributes;
	int output_attributes;

	std::string code;
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
};