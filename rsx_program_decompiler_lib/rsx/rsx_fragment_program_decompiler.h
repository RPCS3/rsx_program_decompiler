#pragma once
#include "rsx_fragment_program.h"
#include "rsx_program_decompiler.h"
#include <unordered_set>
#include "endianness.h"

namespace rsx
{
	namespace fragment_program
	{
		enum suffix
		{
			suffix_none,
			H = 1 << 0,
			C = 1 << 1
		};

		template<int size> struct dest {};
		template<int index, int size, bool apply_dst_mask = true> struct src {};
		struct texture {};
		struct addr {};
		struct cond {};
		template<typename T> struct arg {};

		union alignas(16) ucode_data
		{
			struct alignas(16)
			{
				OPDEST dst;
				SRC0 src0;
				SRC1 src1;
				SRC2 src2;
			};

			u32 data[4];

			ucode_data unpack()
			{
				ucode_data result;

				result.data[0] = endianness::swap(data[0]); result.data[0] = (result.data[0] << 16) | (result.data[0] >> 16);
				result.data[1] = endianness::swap(data[1]); result.data[1] = (result.data[1] << 16) | (result.data[1] >> 16);
				result.data[2] = endianness::swap(data[2]); result.data[2] = (result.data[2] << 16) | (result.data[2] >> 16);
				result.data[3] = endianness::swap(data[3]); result.data[3] = (result.data[3] << 16) | (result.data[3] >> 16);

				return result;
			}
		};

		static_assert(sizeof(ucode_data) == 4 * 4, "bad ucode_data");

		template<typename decompiler_impl>
		class decompiler;

		template<opcode id, u32 flags, typename... Tuple>
		struct handle_instruction
		{
		};

		template<opcode id, u32 flags, typename... Tuple>
		struct instruction
		{
			template<typename decompiler_impl>
			static void impl(decompiler<decompiler_impl>& decompiler)
			{
				handle_instruction < id, flags, Tuple... >::function(decompiler);
			}
		};

		using MOV = instruction < opcode::MOV, H | C, dest<4>, arg<src<0, 4>> >;
		using MUL = instruction < opcode::MUL, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using ADD = instruction < opcode::ADD, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using MAD = instruction < opcode::MAD, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>>, arg<src<2, 4>> >;
		using DP3 = instruction < opcode::DP3, H | C, dest<3>, arg<src<0, 3, false>>, arg<src<1, 3, false>> >;
		using DP4 = instruction < opcode::DP4, H | C, dest<4>, arg<src<0, 4, false>>, arg<src<1, 4, false>> >;
		using DST = instruction < opcode::DST, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using MIN = instruction < opcode::MIN, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using MAX = instruction < opcode::MAX, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using SLT = instruction < opcode::SLT, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using SGE = instruction < opcode::SGE, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using SLE = instruction < opcode::SLE, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using SGT = instruction < opcode::SGT, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using SNE = instruction < opcode::SNE, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using SEQ = instruction < opcode::SEQ, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using FRC = instruction < opcode::FRC, H | C, dest<4>, arg<src<0, 1>> >;
		using FLR = instruction < opcode::FLR, H | C, dest<4>, arg<src<0, 4>> >;
		using KIL = instruction < opcode::KIL, suffix_none, arg<cond> >;
		using PK4 = instruction < opcode::PK4, H | C, dest<4>, arg<src<0, 4> > >;
		using UP4 = instruction < opcode::UP4, H | C, dest<4>, arg<src<0, 4> > >;
		using DDX = instruction < opcode::DDX, H | C, dest<2>, arg<src<0, 2> > >;
		using DDY = instruction < opcode::DDY, H | C, dest<2>, arg<src<0, 2> > >;
		using TEX = instruction < opcode::TEX, H | C, dest<4>, arg<src<0, 4>>, arg<texture> >;
		using TXP = instruction < opcode::TXP, H | C, dest<4>, arg<src<0, 4>>, arg<texture> >;
		using TXD = instruction < opcode::TXD, H | C, dest<4>, arg<src<0, 4>> >;
		using RCP = instruction < opcode::RCP, H | C, dest<4>, arg<src<0, 4>> >;
		using RSQ = instruction < opcode::RSQ, H | C, dest<4>, arg<src<0, 4>> >;
		using EX2 = instruction < opcode::EX2, H | C, dest<4>, arg<src<0, 4>> >;
		using LG2 = instruction < opcode::LG2, H | C, dest<4>, arg<src<0, 4>> >;
		using LIT = instruction < opcode::LIT, H | C, dest<4>, arg<src<0, 4>> >;
		using LRP = instruction < opcode::LRP, H | C, dest<4>, arg<src<0, 4>> >;
		using STR = instruction < opcode::STR, H | C, dest<4> >;
		using SFL = instruction < opcode::SFL, H | C, dest<4> >;
		using COS = instruction < opcode::COS, H | C, dest<4>, arg<src<0, 1> > >;
		using SIN = instruction < opcode::SIN, H | C, dest<4>, arg<src<0, 1> > >;
		using PK2 = instruction < opcode::PK2, H | C, dest<4>, arg<src<0, 4> > >;
		using UP2 = instruction < opcode::UP2, H | C, dest<4>, arg<src<0, 4> > >;
		using POW = instruction < opcode::POW, H | C, dest<4>, arg<src<0, 4> >, arg<src<1, 4> > >;
		using PKB = instruction < opcode::PKB, H | C, dest<4>, arg<src<0, 4> > >;
		using UPB = instruction < opcode::UPB, H | C, dest<4>, arg<src<0, 4> > >;
		using PK16 = instruction < opcode::PK16, H | C, dest<4>, arg<src<0, 4> > >;
		using UP16 = instruction < opcode::UP16, H | C, dest<4>, arg<src<0, 4> > >;
		using BEM = instruction < opcode::BEM, H | C, dest<4>, arg<src<0, 4>> >;
		using PKG = instruction < opcode::PKG, H | C, dest<4>, arg<src<0, 4>> >;
		using UPG = instruction < opcode::UPG, H | C, dest<4>, arg<src<0, 4>> >;
		using DP2A = instruction < opcode::DP2A, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using TXL = instruction < opcode::TXL, H | C, dest<4>, arg<src<0, 4>>, arg<texture> >;
		using TXB = instruction < opcode::TXB, H | C, dest<4>, arg<src<0, 4>>, arg<texture> >;
		using TEXBEM = instruction < opcode::TEXBEM, H | C, dest<4>, arg<src<0, 4>> >;
		using TXPBEM = instruction < opcode::TXPBEM, H | C, dest<4>, arg<src<0, 4>> >;
		using BEMLUM = instruction < opcode::BEMLUM, H | C, dest<4>, arg<src<0, 4>> >;
		using REFL = instruction < opcode::REFL, H | C, dest<4>, arg<src<0, 4>> >;
		using TIMESWTEX = instruction < opcode::TIMESWTEX, H | C, dest<4>, arg<src<0, 4>> >;
		using DP2 = instruction < opcode::DP2, H | C, dest<2>, arg<src<0, 2, false>>, arg<src<1, 2, false>> >;
		using NRM = instruction < opcode::NRM, H | C, dest<4>, arg<src<0, 4>> >;
		using DIV = instruction < opcode::DIV, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using DIVSQ = instruction < opcode::DIVSQ, H | C, dest<4>, arg<src<0, 4>>, arg<src<1, 4>> >;
		using LIF = instruction < opcode::LIF, H >;
		using FENCT = instruction < opcode::FENCT, suffix_none >;
		using FENCB = instruction < opcode::FENCB, suffix_none >;
		using BRK = instruction < opcode::BRK, suffix_none, arg<addr> >;
		using CAL = instruction < opcode::CAL, suffix_none, arg<addr> >;
		using IFE = instruction < opcode::IFE, H >;
		using LOOP = instruction < opcode::LOOP, H >;
		using REP = instruction < opcode::REP, H >;
		using RET = instruction < opcode::RET, H >;

		template<typename decompiler_impl>
		using instruction_impl_func = void(*)(decompiler<decompiler_impl>&);

		struct info : program_info
		{
			std::vector<u32> constant_offsets;
		};

		template<opcode id, u32 flags, typename... args_type>
		struct handle_instruction < id, flags, arg<args_type>... >
		{
			template<typename decompiler_impl>
			__forceinline static void function(decompiler<decompiler_impl>& decompiler)
			{
				if (id != opcode::FENCT && id != opcode::FENCT)
					throw std::runtime_error("unimplemented instruction: " + instructions_names[(std::size_t)id]);
			}
		};

		template<opcode id, u32 flags, int dest_count, typename... args_type>
		struct handle_instruction < id, flags, dest<dest_count>, arg<args_type>... >
		{
			template<typename decompiler_impl>
			__forceinline static void function(decompiler<decompiler_impl>& decompiler)
			{
				decompiler.set_dst<id, flags, dest_count>(decompiler.arg<args_type>()...);
			}
		};

		template<typename decompiler_impl>
		class decompiler : public program_decompiler_core
		{
			ucode_data* ucode_ptr;
			u32 ucode_size;
			u32 ucode_index;

		public:
			bool is_next_constant = false;

			fragment_program::info info;
			ucode_data ucode;
			u32 ctrl;

			decompiler(void* ucode_ptr, u32 ucode_size, u32 ctrl = 0x40)
				: ucode_ptr((ucode_data*)ucode_ptr)
				, ucode_size(ucode_size)
				, ctrl(ctrl)
			{
			}

		private:
			template<typename T>
			struct expand_arg_t
			{
			};

			template<int index, int count, bool apply_dst_mask>
			struct expand_arg_t<src<index, count, apply_dst_mask>>
			{
				template<typename decompiler_impl>
				__forceinline static program_variable impl(decompiler<decompiler_impl>& decompiler)
				{
					program_variable_type variable_type = program_variable_type::none;
					program_variable variable = {};

					variable.size = count;

					bool is_fp16 = false;
					int type;

					const auto& dst = decompiler.ucode.dst;
					const auto& src0 = decompiler.ucode.src0;
					const auto& src1 = decompiler.ucode.src1;
					const auto& src2 = decompiler.ucode.src2;

					u8 swizzle_x, swizzle_y, swizzle_z, swizzle_w;

					auto get_data_from = [&](auto &src)
					{
						is_fp16 = src.fp16;
						type = src.reg_type;
						variable.index = src.tmp_reg_index;
						swizzle_x = src.swizzle_x;
						swizzle_y = src.swizzle_y;
						swizzle_z = src.swizzle_z;
						swizzle_w = src.swizzle_w;
					};

					switch (index)
					{
					case 0: get_data_from(src0); break;
					case 1: get_data_from(src1); break;
					case 2: get_data_from(src2); break;
					}

					bool need_declare = true;

					switch (type)
					{
					case 0: //temporary register
						variable.name = is_fp16 ? "H" : "R";
						variable.type = program_variable_type::none;
						break;

					case 1: //input register
					{
						variable.type = program_variable_type::input;

						if (0)
						{
							variable.index = dst.src_attr_reg_num;
							variable.name = "fragment_input";
							variable.array_size = 15;
						}
						else
						{
							static const std::string register_table[] =
							{
								"position",
								"diff_color", "spec_color",
								"fogc",
								"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8", "tc9",
								"ssa"
							};

							variable.index = ~0;
							variable.type = program_variable_type::input;
							variable.name = register_table[dst.src_attr_reg_num];
						}
					}
					break;

					case 2: //constant
						need_declare = false;
						decompiler.is_next_constant = true;
						variable.constant.type = program_constant_type::f32;
						{
							ucode_data constant = decompiler.ucode_ptr[decompiler.ucode_index + 1].unpack();
							variable.constant.x.i32_value = constant.data[0];
							variable.constant.y.i32_value = constant.data[1];
							variable.constant.z.i32_value = constant.data[2];
							variable.constant.w.i32_value = constant.data[3];
						}
						break;
					}

					static const std::string mask = "xyzw";

					std::string swizzle;
					swizzle += mask[swizzle_x];
					swizzle += mask[swizzle_y];
					swizzle += mask[swizzle_z];
					swizzle += mask[swizzle_w];

					variable.mask.add(swizzle);

					if (apply_dst_mask)
					{
						variable.mask.add(decompiler.dst_mask<count>().to_string()).symplify();
					}

					if (count != 4)
					{
						std::string mask = variable.mask.to_string();
						if (mask.empty())
							mask = "xyzw";

						variable.mask = mask_t{}.add(mask.substr(0, count));
					}

					if (need_declare)
						variable = decompiler.info.vars.add(variable);

					return variable;
				}
			};

			template<>
			struct expand_arg_t<cond>
			{
				template<typename decompiler_impl>
				__forceinline static program_variable impl(decompiler<decompiler_impl>& decompiler)
				{
					return decompiler.execution_condition();
				}
			};

			template<>
			struct expand_arg_t<texture>
			{
				template<typename decompiler_impl>
				__forceinline static program_variable impl(decompiler<decompiler_impl>& decompiler)
				{
					program_variable result = {};
					result.name = "texture";
					result.index = decompiler.ucode.dst.tex_num;
					result.size = 1;
					result.type = program_variable_type::texture;
					return decompiler.info.vars.add(decompiler_impl::texture_variable(result));
				}
			};

			template<>
			struct expand_arg_t<addr>
			{
				template<typename decompiler_impl>
				__forceinline static program_variable impl(decompiler<decompiler_impl>& decompiler)
				{
					return{ "label",{}, program_variable_type::none, 1, 0 };
				}
			};

		public:
			std::unordered_set<std::string> functions_set;

			template<int count>
			mask_t dst_mask()
			{
				static const std::string mask = "xyzw";

				std::string swizzle;
				if (ucode.dst.mask_x) swizzle += mask[0];
				if (ucode.dst.mask_y) swizzle += mask[1];
				if (ucode.dst.mask_z) swizzle += mask[2];
				if (ucode.dst.mask_w) swizzle += mask[3];

				return mask_t{}.add(swizzle.substr(0, count));
			}

			template<u32 flags, int count>
			program_variable dst()
			{
				if (ucode.dst.no_dest)
					return{};

				program_variable result = {};
				result.index = ucode.dst.dest_reg;
				result.name = ucode.dst.fp16 ? "H" : "R";
				result.size = count;
				result.mask = dst_mask<count>();

				return info.vars.add(result);
			}

			template<typename T>
			program_variable arg()
			{
				return expand_arg_t<T>::impl(*this);
			}

			program_variable update_condition()
			{
				program_variable result = {};

				result.name = "CC";
				result.index = ucode.src0.cond_mod_reg_index;
				result.size = 4;
				result.mask = dst_mask<4>();

				return info.vars.add(result);
			}

			program_variable execution_condition()
			{
				program_variable result = {};

				result.name = "CC";
				result.index = ucode.src0.cond_reg_index;
				result.size = 4;

				static const std::string mask = "xyzw";

				std::string swizzle;
				swizzle += mask[ucode.src0.cond_swizzle_x];
				swizzle += mask[ucode.src0.cond_swizzle_y];
				swizzle += mask[ucode.src0.cond_swizzle_z];
				swizzle += mask[ucode.src0.cond_swizzle_w];

				result.mask.add(swizzle);

				return info.vars.add(result);
			}

			void set_code_line(const std::string &code_line)
			{
				program_decompiler_core::builder.add_code_block(ucode_index, code_line);
			}

			template<opcode id, u32 flags, int count>
			void set_dst(const program_variable& arg0 = {}, const program_variable& arg1 = {}, const program_variable& arg2 = {})
			{
				set_code_line(decompiler_impl::set_dst<id, flags, count>(this, arg0, arg1, arg2));
			}

			void unknown_instruction(opcode op)
			{
				throw std::runtime_error("unimplemented instruction '" + instructions_names[(std::size_t)op] + "' (" + std::to_string((std::size_t)op) + ") #"
					+ std::to_string(ucode_index));
			}

			std::string function_begin(const std::string& name)
			{
				functions_set.insert(name);
				return decompiler_impl::function_begin(name);
			}

			std::string function_end()
			{
				return decompiler_impl::function_end();
			}

		public:
			fragment_program::info& decompile()
			{
				static const instruction_impl_func<decompiler_impl> instructions[0x80] =
				{
					nullptr, MOV::impl, MUL::impl, ADD::impl, MAD::impl, DP3::impl, DP4::impl,
					DST::impl, MIN::impl, MAX::impl, SLT::impl, SGE::impl, SLE::impl, SGT::impl,
					SNE::impl, SEQ::impl, FRC::impl, FLR::impl, KIL::impl, PK4::impl, UP4::impl,
					DDX::impl, DDY::impl, TEX::impl, TXP::impl, TXD::impl, RCP::impl, RSQ::impl,
					EX2::impl, LG2::impl, LIT::impl, LRP::impl, STR::impl, SFL::impl, COS::impl,
					SIN::impl, PK2::impl, UP2::impl, POW::impl, PKB::impl, UPB::impl, PK16::impl,
					UP16::impl, BEM::impl, PKG::impl, UPG::impl, DP2A::impl, TXL::impl, nullptr,
					TXB::impl, nullptr, TEXBEM::impl, TXPBEM::impl, BEMLUM::impl, REFL::impl, TIMESWTEX::impl,
					DP2::impl, NRM::impl, DIV::impl, DIVSQ::impl, LIF::impl, FENCT::impl, FENCB::impl,
					nullptr, BRK::impl, CAL::impl, IFE::impl, LOOP::impl, REP::impl, RET::impl
				};

				for (ucode_index = 0; ucode_index < ucode_size; ++ucode_index)
				{
					if (is_next_constant)
					{
						is_next_constant = false;
						continue;
					}

					ucode = ucode_ptr[ucode_index].unpack();

					const fragment_program::opcode opcode = fragment_program::opcode(ucode.dst.opcode | (ucode.src1.opcode_is_branch << 6));

					if (opcode != fragment_program::opcode::NOP)
					{
						auto function = instructions[(std::size_t)opcode];

						if (function)
						{
							function(*this);
						}
						else
						{
							unknown_instruction(opcode);
						}
					}

					if (ucode.dst.end)
						break;
				}

				builder.add_code_block(0, function_begin("label0"), 0, 1, false);

				std::string end = decompiler_impl::finalyze(this);

				builder.add_code_block(0, decompiler_impl::get_header(this), 0, 0, false);
				builder.add_code_block(ucode_index, function_end(), -1, 0);
				builder.add_code_block(ucode_index, end);

				info.text = builder.build();
				return info;
			}
		};
	}
}