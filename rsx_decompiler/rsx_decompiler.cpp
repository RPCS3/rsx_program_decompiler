#include "rsx_decompiler.h"
#include <unordered_map>
#include <string>
#include <iterator>

namespace rsx
{
	const std::string index_to_channel[4] = { "x", "y", "z", "w" };
	const std::unordered_map<char, int> channel_to_index = { { 'x', 0 },{ 'y', 1 },{ 'z', 2 },{ 'w', 3 } };
	const std::string mask = "xyzw";

	bool raw_shader::operator ==(const raw_shader &rhs) const
	{
		if (ucode.size() != rhs.ucode.size())
		{
			return false;
		}

		if (rhs.type == program_type::vertex)
		{
			return std::memcmp(ucode.data(), rhs.ucode.data(), ucode.size()) == 0;
		}

		const auto *src0 = (const fragment_program::ucode_instr*)rhs.ucode.data();
		const auto *src1 = (const fragment_program::ucode_instr*)ucode.data();

		while (true)
		{
			if (memcmp(src0, src1, sizeof(fragment_program::ucode_instr)) != 0)
			{
				return false;
			}

			if (src0->end())
			{
				break;
			}

			int step = src0->has_constant() ? 2 : 1;

			src0 += step;
			src1 += step;
		}

		return true;
	}

	void analyze_raw_shader(raw_shader &shader)
	{
		std::uint64_t hash = 0xCBF29CE484222325ULL;
		std::size_t size = 0;

		if (shader.type == program_type::vertex)
		{
			using namespace vertex_program;

			const ucode_instr *ptr = (const ucode_instr*)shader.ucode_ptr;

			while (true)
			{
				hash ^= ptr->d0._u32 | (std::uint64_t(ptr->d1._u32) << 32);
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);

				hash ^= ptr->d2._u32 | (std::uint64_t(ptr->d3._u32) << 32);
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);

				size += sizeof(u32) * 4;

				if (ptr->end())
					break;

				++ptr;
			}
		}
		else
		{
			using namespace fragment_program;

			const ucode_instr *ptr = (const ucode_instr*)shader.ucode_ptr;

			while (true)
			{
				hash ^= ptr->dst._u32 | (std::uint64_t(ptr->src0._u32) << 32);
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);

				hash ^= ptr->src1._u32 | (std::uint64_t(ptr->src2._u32) << 32);
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);

				int step = ptr->has_constant() ? 2 : 1;

				size += sizeof(u32) * 4 * step;

				if (ptr->end())
				{
					break;
				}

				ptr += step;
			}
		}

		shader.ucode.resize(size);
		memcpy(shader.ucode.data(), shader.ucode_ptr, size);
		shader.ucode_hash = hash;
	}

	namespace fragment_program
	{
		decompiled_shader decompile(const raw_shader &shader, decompile_language lang);
	}

	namespace vertex_program
	{
		decompiled_shader decompile(const raw_shader &shader, decompile_language lang);
	}

	decompiled_shader decompile(const rsx::raw_shader& shader, decompile_language lang)
	{
		switch (shader.type)
		{
		case program_type::vertex: return vertex_program::decompile(shader, lang);
		case program_type::fragment: return fragment_program::decompile(shader, lang);
		}

		throw std::logic_error("");
	}
}
