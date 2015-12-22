#include "rsx_vp_ucode.h"

namespace rsx
{
	namespace vertex_program
	{
		const std::string input_registers_table[0x10] =
		{
			"in_pos", "in_weight", "in_normal",
			"in_diff_color", "in_spec_color",
			"in_fog",
			"in_point_size", "in_7",
			"in_tc0", "in_tc1", "in_tc2", "in_tc3",
			"in_tc4", "in_tc5", "in_tc6", "in_tc7"
		};

		const std::string sca_op_names[0x20] =
		{
			"NOP", "MOV", "RCP", "RCC", "RSQ", "EXP", "LOG",
			"LIT", "BRA", "BRI", "CAL", "CLI", "RET", "LG2",
			"EX2", "SIN", "COS", "BRB", "CLB", "PSH", "POP"
		};

		const std::string vec_op_names[0x20] =
		{
			"NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DPH", "DP4",
			"DST", "MIN", "MAX", "SLT", "SGE", "ARL", "FRC", "FLR",
			"SEQ", "SFL", "SGT", "SLE", "SNE", "STR", "SSG", {},
			{}, "TXL"
		};

		std::uint64_t hash(const ucode_instr *ucode)
		{
			std::uint64_t hash = 0xCBF29CE484222325ULL;

			for (const ucode_instr *ptr = ucode; !ptr->end(); ++ptr)
			{
				hash ^= ptr->d0._u32 | (std::uint64_t(ptr->d1._u32) << 32);
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);

				hash ^= ptr->d2._u32 | (std::uint64_t(ptr->d3._u32) << 32);
				hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) + (hash << 8) + (hash << 40);
			}

			return hash;
		}
	}
}
