#pragma once
#include <string>

namespace rsx
{
	namespace fragment_program
	{
		using u32 = std::uint32_t;

		enum class opcode : u32
		{
			nop = 0x00, // No-Operation
			mov = 0x01, // Move
			mul = 0x02, // Multiply
			add = 0x03, // Add
			mad = 0x04, // Multiply-Add
			dp3 = 0x05, // 3-component Dot Product
			dp4 = 0x06, // 4-component Dot Product
			dst = 0x07, // Distance
			min = 0x08, // Minimum
			max = 0x09, // Maximum
			slt = 0x0A, // Set-If-LessThan
			sge = 0x0B, // Set-If-GreaterEqual
			sle = 0x0C, // Set-If-LessEqual
			sgt = 0x0D, // Set-If-GreaterThan
			sne = 0x0E, // Set-If-NotEqual
			seq = 0x0F, // Set-If-Equal
			frc = 0x10, // Fraction (fract)
			flr = 0x11, // Floor
			kil = 0x12, // Kill fragment
			pk4 = 0x13, // Pack four signed 8-bit values
			up4 = 0x14, // Unpack four signed 8-bit values
			ddx = 0x15, // Partial-derivative in x (Screen space derivative w.r.t. x)
			ddy = 0x16, // Partial-derivative in y (Screen space derivative w.r.t. y)
			tex = 0x17, // Texture lookup
			txp = 0x18, // Texture sample with projection (Projective texture lookup)
			txd = 0x19, // Texture sample with partial differentiation (Texture lookup with derivatives)
			rcp = 0x1A, // Reciprocal
			rsq = 0x1B, // Reciprocal Square Root
			ex2 = 0x1C, // Exponentiation base 2
			lg2 = 0x1D, // Log base 2
			lit = 0x1E, // Lighting coefficients
			lrp = 0x1F, // Linear Interpolation
			str = 0x20, // Set-If-True
			sfl = 0x21, // Set-If-False
			cos = 0x22, // Cosine
			sin = 0x23, // Sine
			pk2 = 0x24, // Pack two 16-bit floats
			up2 = 0x25, // Unpack two 16-bit floats
			pow = 0x26, // Power
			pkb = 0x27, // Pack bytes
			upb = 0x28, // Unpack bytes
			pk16 = 0x29, // Pack 16 bits
			up16 = 0x2A, // Unpack 16
			bem = 0x2B, // Bump-environment map (a.k.a. 2D coordinate transform)
			pkg = 0x2C, // Pack with sRGB transformation
			upg = 0x2D, // Unpack gamma
			dp2a = 0x2E, // 2-component dot product with scalar addition
			txl= 0x2F, // Texture sample with explicit LOD
			txb = 0x31, // Texture sample with bias
			texbem = 0x33,
			txpbem = 0x34,
			bemlum = 0x35,
			refl = 0x36, // Reflection vector
			timeswtex = 0x37,
			dp2 = 0x38, // 2-component dot product
			nrm = 0x39, // Normalize
			div = 0x3A, // Division
			divsq = 0x3B, // Divide by Square Root
			lif = 0x3C, // Final part of LIT
			fenct = 0x3D, // Fence T?
			fencb = 0x3E, // Fence B?

			brk = 0x40, // Break
			cal = 0x41, // Subroutine call
			ife = 0x42, // If
			loop = 0x43, // Loop
			rep = 0x44, // Repeat
			ret = 0x45  // Return
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
