#include "rsx_vp_ucode.h"

namespace rsx
{
	namespace vertex_program
	{
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
	}
}