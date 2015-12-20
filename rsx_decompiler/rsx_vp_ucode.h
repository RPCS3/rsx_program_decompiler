#pragma once
#include <cstdint>
#include <string>

namespace rsx
{
	namespace vertex_program
	{
		using u32 = std::uint32_t;
		enum class sca_opcode : u32
		{
			nop = 0x00,
			mov = 0x01,
			rcp = 0x02,
			rcc = 0x03,
			rsq = 0x04,
			exp = 0x05,
			log = 0x06,
			lit = 0x07,
			bra = 0x08,
			bri = 0x09,
			cal = 0x0a,
			cli = 0x0b,
			ret = 0x0c,
			lg2 = 0x0d,
			ex2 = 0x0e,
			sin = 0x0f,
			cos = 0x10,
			brb = 0x11,
			clb = 0x12,
			psh = 0x13,
			pop = 0x14
		};

		enum class vec_opcode : u32
		{
			nop = 0x00,
			mov = 0x01,
			mul = 0x02,
			add = 0x03,
			mad = 0x04,
			dp3 = 0x05,
			dph = 0x06,
			dp4 = 0x07,
			dst = 0x08,
			min = 0x09,
			max = 0x0a,
			slt = 0x0b,
			sge = 0x0c,
			arl = 0x0d,
			frc = 0x0e,
			flr = 0x0f,
			seq = 0x10,
			sfl = 0x11,
			sgt = 0x12,
			sle = 0x13,
			sne = 0x14,
			str = 0x15,
			ssg = 0x16,
			txl = 0x19
		};

		enum class src_register_type : u32
		{
			temporary = 1,
			input = 2,
			constant = 3
		};

		union D0
		{
			u32 _u32;

			struct
			{
				u32 addr_swz : 2;
				u32 mask_w : 2;
				u32 mask_z : 2;
				u32 mask_y : 2;
				u32 mask_x : 2;
				u32 cond : 3;
				u32 cond_test_enable : 1;
				u32 cond_update_enable_0 : 1;
				u32 dst_tmp : 6;
				u32 src0_abs : 1;
				u32 src1_abs : 1;
				u32 src2_abs : 1;
				u32 addr_reg_sel_1 : 1;
				u32 cond_reg_sel_1 : 1;
				u32 staturate : 1;
				u32 index_input : 1;
			u32: 1;
				u32 cond_update_enable_1 : 1;
				u32 vec_result : 1;
			u32: 1;
			};
		};

		union D1
		{
			u32 _u32;

			struct
			{
				u32 src0h : 8;
				u32 input_src : 4;
				u32 const_src : 10;
				vec_opcode vec_opcode : 5;
				sca_opcode sca_opcode : 5;
			};
		};

		union D2
		{
			u32 _u32;

			struct
			{
				u32 src2h : 6;
				u32 src1 : 17;
				u32 src0l : 9;
			};
			struct
			{
				u32 iaddrh : 6;
			u32: 26;
			};
		};

		union D3
		{
			u32 _u32;

			struct
			{
				u32 end : 1;
				u32 index_const : 1;
				u32 dst : 5;
				u32 sca_dst_tmp : 6;
				u32 vec_writemask_w : 1;
				u32 vec_writemask_z : 1;
				u32 vec_writemask_y : 1;
				u32 vec_writemask_x : 1;
				u32 sca_writemask_w : 1;
				u32 sca_writemask_z : 1;
				u32 sca_writemask_y : 1;
				u32 sca_writemask_x : 1;
				u32 src2l : 11;
			};
			struct
			{
			u32: 29;
				u32 iaddrl : 3;
			};
		};

		union SRC
		{
			union
			{
				u32 _u32;

				struct
				{
					u32 src0l : 9;
					u32 src0h : 8;
				};

				struct
				{
					u32 src1 : 17;
				};

				struct
				{
					u32 src2l : 11;
					u32 src2h : 6;
				};
			};

			struct
			{
				src_register_type register_type : 2;
				u32 tmp_src : 6;
				u32 swz_w : 2;
				u32 swz_z : 2;
				u32 swz_y : 2;
				u32 swz_x : 2;
				u32 neg : 1;
			};
		};

		struct ucode_instr
		{
			D0 d0;
			D1 d1;
			D2 d2;
			D3 d3;
		};

		extern const std::string input_registers_table[0x10];
		extern const std::string sca_op_names[0x20];
		extern const std::string vec_op_names[0x20];
	}
}