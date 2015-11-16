#include "pch.h"
#include "rsx_fp_ucode.h"

namespace rsx
{
	namespace fragment_program
	{
		const std::string input_attrib_map[16] =
		{
			"in_wpos", //0
			"in_col0", //1
			"in_col1", //2
			"in_fogc", //3
			"in_tex0", //4
			"in_tex1", //5
			"in_tex2", //6
			"in_tex3", //7
			"in_tex4", //8
			"in_tex5", //9
			"in_tex6", //10
			"in_tex7", //11
			"in_tex8", //12
			"in_tex9", //13
			"in_ssa", //14
			"in_unk"  //15
		};

		const std::string input_attr_regs[16] =
		{
			"WPOS", "COL0", "COL1", "FOGC", "TEX0",
			"TEX1", "TEX2", "TEX3", "TEX4", "TEX5",
			"TEX6", "TEX7", "TEX8", "TEX9", "SSA"
		};

		const std::string instructions_names[128] =
		{
			"NOP", "MOV", "MUL", "ADD", "MAD", "DP3", "DP4",
			"DST", "MIN", "MAX", "SLT", "SGE", "SLE", "SGT",
			"SNE", "SEQ", "FRC", "FLR", "KIL", "PK4", "UP4",
			"DDX", "DDY", "TEX", "TXP", "TXD", "RCP", "RSQ",
			"EX2", "LG2", "LIT", "LRP", "STR", "SFL", "COS",
			"SIN", "PK2", "UP2", "POW", "PKB", "UPB", "PK16",
			"UP16", "BEM", "PKG", "UPG", "DP2A", "TXL", "NULL",
			"TXB", "NULL", "TEXBEM", "TXPBEM", "BEMLUM", "REFL", "TIMESWTEX",
			"DP2", "NRM", "DIV", "DIVSQ", "LIF", "FENCT", "FENCB",
			"NULL", "BRK", "CAL", "IFE", "LOOP", "REP", "RET"
		};
	}
}