#include "rsx_decompiler.h"
#include <glsl_language.h>
#include "rsx_decompiler_base.h"

namespace rsx
{
	namespace vertex_program
	{
		class decompiler : public decompiler_base<shader_code::glsl_language>
		{
			struct context_t
			{
				decompiled_program program;
			};

		public:
			struct instruction_t
			{
				ucode_instr data;

				static u32 swap_endianess(u32 data)
				{
					return
						((data >> 24) & 0x000000ff) |
						((data >> 8) & 0x0000ff00) |
						((data << 8) & 0x00ff0000) |
						((data << 24) & 0xff000000);
				}

				instruction_t unpack()
				{
					instruction_t result;

					result.data.d0._u32 = swap_endianess(data.d0._u32);
					result.data.d1._u32 = swap_endianess(data.d1._u32);
					result.data.d2._u32 = swap_endianess(data.d2._u32);
					result.data.d3._u32 = swap_endianess(data.d3._u32);

					return result;
				}
			};

			static_assert(sizeof(instruction_t) == 16, "Bad instruction_t implementation");

			instruction_t instruction;
			context_t context;

			decompiled_program decompile(std::size_t offset, instruction_t *instructions)
			{
				for (std::size_t i = offset; i < 512; ++i, writer.next())
				{
					instruction = instructions[i].unpack();

					if (instruction.data.d1.sca_opcode == sca_opcode::nop && instruction.data.d1.vec_opcode == vec_opcode::nop)
					{
						writer += comment("NOP");
					}
					else
					{
						if (instruction.data.d1.sca_opcode != sca_opcode::nop)
						{
							writer += comment(sca_op_names[(int)instruction.data.d1.sca_opcode]);
						}

						if (instruction.data.d1.vec_opcode != vec_opcode::nop)
						{
							writer += comment(vec_op_names[(int)instruction.data.d1.vec_opcode]);
						}
					}

					if (instruction.data.d3.end)
						break;
				}

				context.program.entry_function = "func0";
				context.program.code = "void func0()\n{\n" + writer.build() + "}\n";

				return context.program;
			}
		};

		template<typename Language>
		decompiled_program decompile(std::size_t offset, ucode_instr *instructions)
		{
			return decompiler{}.decompile(offset, (decompiler::instruction_t*)instructions);
		}

		decompiled_program decompile(std::size_t offset, ucode_instr* instructions, decompile_language lang)
		{
			decompiled_program result;

			switch (lang)
			{
			case decompile_language::glsl:
				result = decompile<shader_code::glsl_language>(offset, instructions);

				result.code_language = decompile_language::glsl;
				break;

			default:
				throw;
			}

			result.type = program_type::vertex;
			return result;
		}
	}
}