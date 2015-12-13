#pragma once
#include <string>

namespace rsx
{
	using u32 = unsigned int;

	namespace fragment_program
	{
		enum class opcode : u32
		{
			NOP = 0x00, // No-Operation
			MOV = 0x01, // Move
			MUL = 0x02, // Multiply
			ADD = 0x03, // Add
			MAD = 0x04, // Multiply-Add
			DP3 = 0x05, // 3-component Dot Product
			DP4 = 0x06, // 4-component Dot Product
			DST = 0x07, // Distance
			MIN = 0x08, // Minimum
			MAX = 0x09, // Maximum
			SLT = 0x0A, // Set-If-LessThan
			SGE = 0x0B, // Set-If-GreaterEqual
			SLE = 0x0C, // Set-If-LessEqual
			SGT = 0x0D, // Set-If-GreaterThan
			SNE = 0x0E, // Set-If-NotEqual
			SEQ = 0x0F, // Set-If-Equal
			FRC = 0x10, // Fraction (fract)
			FLR = 0x11, // Floor
			KIL = 0x12, // Kill fragment
			PK4 = 0x13, // Pack four signed 8-bit values
			UP4 = 0x14, // Unpack four signed 8-bit values
			DDX = 0x15, // Partial-derivative in x (Screen space derivative w.r.t. x)
			DDY = 0x16, // Partial-derivative in y (Screen space derivative w.r.t. y)
			TEX = 0x17, // Texture lookup
			TXP = 0x18, // Texture sample with projection (Projective texture lookup)
			TXD = 0x19, // Texture sample with partial differentiation (Texture lookup with derivatives)
			RCP = 0x1A, // Reciprocal
			RSQ = 0x1B, // Reciprocal Square Root
			EX2 = 0x1C, // Exponentiation base 2
			LG2 = 0x1D, // Log base 2
			LIT = 0x1E, // Lighting coefficients
			LRP = 0x1F, // Linear Interpolation
			STR = 0x20, // Set-If-True
			SFL = 0x21, // Set-If-False
			COS = 0x22, // Cosine
			SIN = 0x23, // Sine
			PK2 = 0x24, // Pack two 16-bit floats
			UP2 = 0x25, // Unpack two 16-bit floats
			POW = 0x26, // Power
			PKB = 0x27, // Pack bytes
			UPB = 0x28, // Unpack bytes
			PK16 = 0x29, // Pack 16 bits
			UP16 = 0x2A, // Unpack 16
			BEM = 0x2B, // Bump-environment map (a.k.a. 2D coordinate transform)
			PKG = 0x2C, // Pack with sRGB transformation
			UPG = 0x2D, // Unpack gamma
			DP2A = 0x2E, // 2-component dot product with scalar addition
			TXL = 0x2F, // Texture sample with explicit LOD
			TXB = 0x31, // Texture sample with bias
			TEXBEM = 0x33,
			TXPBEM = 0x34,
			BEMLUM = 0x35,
			REFL = 0x36, // Reflection vector
			TIMESWTEX = 0x37,
			DP2 = 0x38, // 2-component dot product
			NRM = 0x39, // Normalize
			DIV = 0x3A, // Division
			DIVSQ = 0x3B, // Divide by Square Root
			LIF = 0x3C, // Final part of LIT
			FENCT = 0x3D, // Fence T?
			FENCB = 0x3E, // Fence B?

			BRK = 0x40, // Break
			CAL = 0x41, // Subroutine call
			IFE = 0x42, // If
			LOOP = 0x43, // Loop
			REP = 0x44, // Repeat
			RET = 0x45  // Return
		};

		inline opcode operator |(opcode lhs, u32 rhs)
		{
			return opcode(u32(lhs) | rhs);
		}
		inline opcode operator |(u32 lhs, opcode rhs)
		{
			return opcode(lhs | u32(rhs));
		}

		enum class src_reg_type_t : u32
		{
			temporary,
			input,
			constant
		};

		union alignas(4) OPDEST
		{
			u32 _u32;

			struct
			{
				u32 end : 1; // Set to 1 if this is the last instruction
				u32 dest_reg : 6; // Destination register index
				u32 fp16 : 1; // Destination is a half register (H0 to H47)
				u32 set_cond : 1; // Condition Code Registers (CC0 and CC1) are updated
				u32 mask_x : 1;
				u32 mask_y : 1;
				u32 mask_z : 1;
				u32 mask_w : 1;
				u32 src_attr_reg_num : 4;
				u32 tex_num : 4;
				u32 exp_tex : 1; // _bx2
				u32 prec : 2;
				opcode opcode : 6;
				u32 no_dest : 1;
				u32 saturate : 1; // _sat
			};
		};

		union alignas(4) SRC0
		{
			u32 _u32;

			struct
			{
				src_reg_type_t reg_type : 2;
				u32 tmp_reg_index : 6;
				u32 fp16 : 1;
				u32 swizzle_x : 2;
				u32 swizzle_y : 2;
				u32 swizzle_z : 2;
				u32 swizzle_w : 2;
				u32 neg : 1;
				u32 exec_if_lt : 1;
				u32 exec_if_eq : 1;
				u32 exec_if_gr : 1;
				u32 cond_swizzle_x : 2;
				u32 cond_swizzle_y : 2;
				u32 cond_swizzle_z : 2;
				u32 cond_swizzle_w : 2;
				u32 abs : 1;
				u32 cond_mod_reg_index : 1;
				u32 cond_reg_index : 1;
			};
		};

		union alignas(4) SRC1
		{
			u32 _u32;

			struct
			{
				src_reg_type_t reg_type : 2;
				u32 tmp_reg_index : 6;
				u32 fp16 : 1;
				u32 swizzle_x : 2;
				u32 swizzle_y : 2;
				u32 swizzle_z : 2;
				u32 swizzle_w : 2;
				u32 neg : 1;
				u32 abs : 1;
				u32 input_mod_src0 : 3;
			u32: 6;
				u32 scale : 3;
				u32 opcode_is_branch : 1;
			};

			struct
			{
				u32 else_offset : 31;
			u32: 1;
			};

			// LOOP, REP
			struct
			{
			u32: 2;
				u32 end_counter : 8; // End counter value for LOOP or rep count for REP
				u32 init_counter : 8; // Initial counter value for LOOP
			u32: 1;
				u32 increment : 8; // Increment value for LOOP
			};
		};

		union alignas(4) SRC2
		{
			u32 _u32;

			u32 end_offset;

			struct
			{
				src_reg_type_t reg_type : 2;
				u32 tmp_reg_index : 6;
				u32 fp16 : 1;
				u32 swizzle_x : 2;
				u32 swizzle_y : 2;
				u32 swizzle_z : 2;
				u32 swizzle_w : 2;
				u32 neg : 1;
				u32 abs : 1;
				u32 addr_reg : 11;
				u32 use_index_reg : 1;
				u32 perspective_corr : 1;
			};
		};

		struct alignas(16) ucode_instr
		{
			OPDEST dst;
			SRC0 src0;
			SRC1 src1;
			SRC2 src2;
		};

		extern const std::string input_attr_regs[16];
		extern const std::string instructions_names[128];
		extern const std::string input_attrib_map[16];
	}
}
