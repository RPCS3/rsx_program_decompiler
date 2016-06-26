#pragma once
#include "rsx_fp_ucode.h"
#include "rsx_vp_ucode.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cstring>

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

	template<std::size_t Size = sizeof(std::size_t)>
	struct fnv_1a;

	template<>
	struct fnv_1a<8>
	{
		static const std::size_t offset_basis = size_t(14695981039346656037ULL);
		static const std::size_t prime = size_t(1099511628211ULL);
	};

	template<>
	struct fnv_1a<4>
	{
		static const std::size_t offset_basis = size_t(2166136261);
		static const std::size_t prime = size_t(16777619);
	};

	struct fnv_1a_hasher
	{
		static std::size_t hash(const std::uint8_t* raw, std::size_t size)
		{
			std::size_t result = fnv_1a<>::offset_basis;

			for (std::size_t byte = 0; byte < size; ++byte)
			{
				result ^= (std::size_t)raw[byte];
				result *= fnv_1a<>::prime;
			}

			return result;
		}

		template<typename Type>
		static std::size_t hash(const Type& value)
		{
			return hash((const uint8_t*)&value, sizeof(Type));
		}

		template<typename Type>
		std::size_t operator()(const Type& value) const
		{
			return hash(value);
		}
	};

	enum class texture_target : std::uint8_t
	{
		none,
		_1,
		_2,
		_3,
		cube
	};

	struct alignas(4) program_state
	{
		std::uint32_t input_attributes;
		std::uint32_t output_attributes;
		std::uint32_t ctrl;
		std::uint32_t divider_op;
		std::uint32_t is_int;
		std::uint16_t frequency[16];
		std::uint32_t fog_mode;
		std::uint32_t alpha_func;
		std::uint8_t textures_alpha_kill[16];
		std::uint32_t textures_zfunc[16];
		texture_target textures[16];

		bool operator ==(const program_state &rhs) const
		{
			return std::memcmp(this, &rhs, sizeof(*this)) == 0;
		}

		std::uint64_t hash() const
		{
			return fnv_1a_hasher()(*this);
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

