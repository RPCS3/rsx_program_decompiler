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
		std::string name;

		int id;
		register_type type;

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

	struct raw_shader
	{
		const void *ucode_ptr;
		std::size_t offset;

		std::vector<uint8_t> ucode;
		program_type type;
		std::uint64_t ucode_hash;

		std::uint64_t hash() const
		{
			return ucode_hash;
		}

		bool operator ==(const raw_shader &rhs) const;
	};

	struct program_state
	{
		union
		{
			struct
			{
				std::uint32_t output_attributes;
				std::uint32_t ctrl;
			};

			std::uint64_t _u64;
		};

		bool operator ==(const program_state &rhs) const
		{
			return _u64 == rhs._u64;
		}

		std::uint64_t hash() const
		{
			return std::hash<std::uint64_t>()(_u64);
		}
	};

	struct raw_program
	{
		raw_shader fragment_shader;
		raw_shader vertex_shader;
		program_state state;

		std::uint64_t hash() const
		{
			return
				fragment_shader.hash() ^
				(vertex_shader.hash() << 1) ^
				(state.hash() << 1);
		}

		bool operator == (const raw_program &rhs) const
		{
			return
				state == rhs.state &&
				vertex_shader == rhs.vertex_shader &&
				fragment_shader == rhs.fragment_shader;
		}
	};

	struct decompiled_shader
	{
		const raw_shader *raw;
		decompile_language code_language;

		std::unordered_set<constant_info, hasher> constants;
		std::unordered_set<texture_info, hasher> textures;
		std::unordered_set<register_info, hasher> temporary_registers;
		std::uint32_t input_attributes = 0;
		std::uint32_t output_attributes = 0;

		std::string entry_function;
		std::string code;
	};

	struct complete_shader
	{
		const decompiled_shader *decompiled;
		program_state state;
		std::string code;

		void *user_data;
	};

	extern const std::string index_to_channel[4];
	extern const std::unordered_map<char, int> channel_to_index;
	extern const std::string mask;

	void analyze_raw_shader(raw_shader &shader);

	decompiled_shader decompile(const rsx::raw_shader& shader, decompile_language lang);
}

