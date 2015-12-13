#include "pch.h"
#include "clike_builder.h"
#include <unordered_set>
#include <functional>
#include <memory>
#include <iostream>
#include <map>

namespace rsx
{
	static const std::string index_to_channel[4] = { "x", "y", "z", "w" };
	static const std::unordered_map<char, int> channel_to_index = { { 'x', 0 },{ 'y', 1 },{ 'z', 2 },{ 'w', 3 } };
	static const std::string mask = "xyzw";

	template<typename Language>
	struct decompiler_base : shader_code::clike_builder<Language>
	{
		enum class compare_function
		{
			less,
			greater,
			equal,
			less_equal,
			greater_equal,
			not_equal
		};

		template<type_class_t Type, int Count>
		static expression_from<boolean_t<Count>> single_compare_function(compare_function function, expression_t<Type, Count> a, expression_t<Type, Count> b)
		{
			std::string operator_string;

			switch (function)
			{
			case compare_function::less: operator_string = "<"; break;
			case compare_function::greater: operator_string = ">"; break;
			case compare_function::equal: operator_string = "=="; break;
			case compare_function::less_equal: operator_string = "<="; break;
			case compare_function::greater_equal: operator_string = ">="; break;
			case compare_function::not_equal: operator_string = "!="; break;

			default:
				throw;
			}

			return a.to_string() + " " + operator_string + " " + b.to_string();
		}

		template<type_class_t Type, int Count>
		static expression_from<boolean_t<Count>> vector_compare_function(compare_function function, expression_t<Type, Count> a, expression_t<Type, Count> b)
		{
			switch (function)
			{
			case compare_function::less: return less(a, b);
			case compare_function::greater: return greater(a, b);
			case compare_function::equal: return equal(a, b);
			case compare_function::less_equal: return less_equal(a, b);
			case compare_function::greater_equal: return greater_equal(a, b);
			case compare_function::not_equal: return not_equal(a, b);
			}

			throw;
		}

		template<type_class_t Type, int Count>
		static expression_from<boolean_t<Count>> custom_compare(compare_function function, int channel_count, expression_t<Type, Count> a, expression_t<Type, Count> b)
		{
			if (channel_count == 1)
			{
				return  single_compare_function(function, a, b);
			}

			return vector_compare_function(function, a, b);
		}
	};
}


#include "glsl_language.h"
#include "rsx_fp_ucode.h"
#include "rsx_decompiler.h"

namespace rsx
{
	namespace fragment_program
	{
		class decompiler : public decompiler_base<shader_code::glsl_language>
		{
			enum class variable_modifier
			{
				none,
				in,
				out,
				constant
			};

			struct context_t
			{
				struct variable_info
				{
					std::string type;
					std::string name;
				};

				u32 offset;
				bool is_next_is_constant;

				std::vector<u32> constants_offsets;
				std::unordered_map<variable_modifier, variable_info> variables;

				template<typename Type = float_point_t<4>>
				expression_from<Type> variable(const std::string& name, variable_modifier modifier = variable_modifier::none)
				{
					variables[modifier] = { Type::name(), name };

					return{ name };
				}

				expression_from<float_point_t<4>> constant()
				{
					return variable("fc" + std::to_string(offset + sizeof(instruction_t)));
				}

				expression_from<float_point_t<4>> temporary(bool is_fp16, int index)
				{
					return variable((is_fp16 ? "h" : "r") + std::to_string(index));
				}

				expression_from<float_point_t<4>> input(int index)
				{
					return variable(input_attrib_map[index]);
				}
			};

		public:
			struct instruction_t
			{
				ucode_instr data;

				struct src_t
				{
					src_reg_type_t reg_type;
					u32 tmp_index;

					u32 swizzle_x;
					u32 swizzle_y;
					u32 swizzle_z;
					u32 swizzle_w;

					bool fp16;
					bool neg;
					bool abs;
				};

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

					result.data.dst._u32  = swap_endianess((data.dst._u32 << 16)  | (data.dst._u32 >> 16));
					result.data.src0._u32 = swap_endianess((data.src0._u32 << 16) | (data.src0._u32 >> 16));
					result.data.src1._u32 = swap_endianess((data.src1._u32 << 16) | (data.src1._u32 >> 16));
					result.data.src2._u32 = swap_endianess((data.src2._u32 << 16) | (data.src2._u32 >> 16));

					return result;
				}

				template<typename SrcType>
				static src_t unpack_src(const SrcType& src)
				{
					src_t result;

					result.reg_type = src.reg_type;
					result.tmp_index = src.tmp_reg_index;

					result.swizzle_x = src.swizzle_x;
					result.swizzle_y = src.swizzle_y;
					result.swizzle_z = src.swizzle_z;
					result.swizzle_w = src.swizzle_w;

					result.fp16 = src.fp16;
					result.abs = src.abs;
					result.neg = src.neg;

					return result;
				}

				expression_from<float_point_t<4>> swizzle_as_dst(expression_from<float_point_t<4>> arg) const
				{
					std::string arg_mask;

					for (char channel : destination_swizzle())
					{
						arg_mask += arg.mask[channel_to_index.at(channel)];
					}

					return expression_from<float_point_t<4>>(arg.text, arg_mask, arg.is_single, arg.base_count);
				}

				expression_from<float_point_t<4>> src(context_t& context, int index, bool is_swizzle_as_dst = false) const
				{
					src_t src;

					switch (index)
					{
					case 0: src = unpack_src(data.src0); break;
					case 1: src = unpack_src(data.src1); break;
					case 2: src = unpack_src(data.src2); break;
					}

					auto get_variable = [&](const src_t& src)
					{
						switch (src.reg_type)
						{
						case src_reg_type_t::temporary: return context.temporary(src.fp16, src.tmp_index);
						case src_reg_type_t::input: return context.input(data.dst.src_attr_reg_num);
						case src_reg_type_t::constant:
							context.is_next_is_constant = true;
							return context.constant();
						}

						throw;
					};

					expression_from<float_point_t<4>> result = get_variable(src);

					result.assign(result.swizzle(src.swizzle_x, src.swizzle_y, src.swizzle_z, src.swizzle_w));

					if (is_swizzle_as_dst)
					{
						result.assign(swizzle_as_dst(result));
					}

					if (src.abs)
					{
						result.assign(abs(result));
					}

					if (src.neg)
					{
						result.assign(-result);
					}

					return result;
				}

				std::string destination_swizzle() const
				{
					std::string swizzle;

					if (data.dst.mask_x) swizzle += mask[0];
					if (data.dst.mask_y) swizzle += mask[1];
					if (data.dst.mask_z) swizzle += mask[2];
					if (data.dst.mask_w) swizzle += mask[3];

					return swizzle;
				}

				expression_from<float_point_t<4>> destination(context_t& context) const
				{
					if (data.dst.no_dest)
					{
						return{ "", destination_swizzle() };
					}

					return{ context.temporary(data.dst.fp16, data.dst.dest_reg).to_string(), destination_swizzle() };
				}
			};

			static_assert(sizeof(instruction_t) == 16, "Bad instruction_t implementation");

			instruction_t instruction;
			context_t context;
			writer_t writer;

			expression_from<float_point_t<4>> src(int index, bool is_swizzle_as_dst = false)
			{
				return instruction.src(context, index, is_swizzle_as_dst);
			}

			expression_from<float_point_t<4>> src_swizzled_as_dst(int index)
			{
				return src(index, instruction.data.dst.set_cond || !instruction.data.dst.no_dest);
			}

			expression_from<float_point_t<4>> modify_condition_register() const
			{
				return{ "cc" + std::to_string(instruction.data.src0.cond_mod_reg_index) };
			}

			expression_from<float_point_t<4>> execution_condition_register() const
			{
				std::string swizzle;

				swizzle += mask[instruction.data.src0.cond_swizzle_x];
				swizzle += mask[instruction.data.src0.cond_swizzle_y];
				swizzle += mask[instruction.data.src0.cond_swizzle_z];
				swizzle += mask[instruction.data.src0.cond_swizzle_w];

				return{ "cc" + std::to_string(instruction.data.src0.cond_reg_index), swizzle };
			}

			enum class condition_operation
			{
				all,
				any
			};

			compare_function execution_condition_function() const
			{
				if (instruction.data.src0.exec_if_gr && instruction.data.src0.exec_if_eq)
				{
					return compare_function::greater_equal;
				}
				if (instruction.data.src0.exec_if_lt && instruction.data.src0.exec_if_eq)
				{
					return compare_function::less_equal;
				}
				if (instruction.data.src0.exec_if_gr && instruction.data.src0.exec_if_lt)
				{
					return compare_function::not_equal;
				}
				if (instruction.data.src0.exec_if_gr)
				{
					return compare_function::greater;
				}
				if (instruction.data.src0.exec_if_lt)
				{
					return compare_function::less;
				}

				if (instruction.data.src0.exec_if_eq)
				{
					return compare_function::equal;
				}

				throw;
			}

			expression_from<boolean_t<1>> execution_condition(condition_operation operation) const
			{
				auto cond = execution_condition_register();

				if (instruction.data.src0.exec_if_gr && instruction.data.src0.exec_if_eq && instruction.data.src0.exec_if_gr)
				{
					return true;
				}

				if (!instruction.data.src0.exec_if_gr && !instruction.data.src0.exec_if_eq && !instruction.data.src0.exec_if_gr)
				{
					return false;
				}

				if (instruction.data.src0.cond_swizzle_x == instruction.data.src0.cond_swizzle_y &&
					instruction.data.src0.cond_swizzle_y == instruction.data.src0.cond_swizzle_z &&
					instruction.data.src0.cond_swizzle_z == instruction.data.src0.cond_swizzle_w)
				{
					return custom_compare(execution_condition_function(), 1, cond.x(), expression_from<float_point_t<1>>(0.0f));
				}

				auto result = custom_compare(execution_condition_function(), 4, cond, float_point_t<4>::ctor(0.0f));

				switch (operation)
				{
				case condition_operation::all: return all(result);
				case condition_operation::any: return any(result);
				}

				throw;
			}

			expression_from<boolean_t<4>> compare(compare_function function, expression_from<float_point_t<4>> a, expression_from<float_point_t<4>> b)
			{
				return custom_compare(function, (int)instruction.destination_swizzle().size(), a, b);
			}

			expression_from<sampler2D_t> tex()
			{
				return{ "unk_tex" };
			}

			template<typename ExprType>
			expression_from<void_t> conditional(const ExprType& expr)
			{
				bool need_condition = false;

				if (need_condition)
				{
					return if_(any(execution_condition(condition_operation::any)), expr);
				}

				return expr;
			}

			enum set_dst_flags
			{
				none,
				disable_swizzle_as_dst = 1,
			};

			template<typename Type>
			Type apply_instruction_modifiers(Type arg)
			{
				using float_t = expression_from<float_point_t<1>>;

				switch (instruction.data.src1.scale)
				{
				case 0: break;
				case 1: arg.assign(arg * (float_t)2.0f); break;
				case 2: arg.assign(arg * (float_t)4.0f); break;
				case 3: arg.assign(arg * (float_t)8.0f); break;
				case 5: arg.assign(arg / (float_t)2.0f); break;
				case 6: arg.assign(arg / (float_t)4.0f); break;
				case 7: arg.assign(arg / (float_t)8.0f); break;

				default:
					throw std::runtime_error("fragment program decompiler: unimplemented scale (" + std::to_string(instruction.data.src1.scale) + "). ");
				}

				if (instruction.data.dst.saturate)
				{
					arg.assign(clamp(arg, 0.0f, 1.0f));
				}
				else
				{
					switch (instruction.data.dst.prec)
					{
					case 0: //fp32
						if (!instruction.data.dst.fp16)
						{
							break;
						}
						//result is fp16, clamp to fp16

					case 1: //fp16
						arg.assign(clamp(arg, -65536.0f, 65536.0f));
						break;

					case 2: //fixed point 12
						arg.assign(clamp(arg, -1.0f, 1.0f));
						break;

					default:
						throw std::runtime_error("fragment program decompiler: unimplemented precision.");
					}
				}

				return arg;
			}

			writer_t set_dst(const expression_from<float_point_t<4>>& arg, u32 flags = none)
			{
				writer_t result;

				auto modify_cond = modify_condition_register();
				auto dest = instruction.destination(context);

				if (!instruction.data.src0.exec_if_eq || !instruction.data.src0.exec_if_gr || !instruction.data.src0.exec_if_lt)
				{
					std::string cond_mask;
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_x];
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_y];
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_z];
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_w];

					auto cond = execution_condition_register();

					std::string operation;

					if (instruction.data.src0.exec_if_gr && instruction.data.src0.exec_if_lt)
					{
						operation = "!=";
					}
					else if (!instruction.data.src0.exec_if_gr && !instruction.data.src0.exec_if_lt)
					{
						operation = "==";
					}
					else
					{
						if (instruction.data.src0.exec_if_gr)
							operation += ">";
						else if (instruction.data.src0.exec_if_lt)
							operation += "<";

						if (instruction.data.src0.exec_if_eq)
							operation += "=";
					}

					if (!instruction.data.dst.set_cond && instruction.data.dst.no_dest)
					{
						//condition must be already handled in instruction semantic (IFE, LOOP, etc)
						result += comment("WARNING: extra condition test skipped");
						result += arg;
					}
					else
					{
						static const expression_from<float_point_t<1>> zero(0.0f);

						std::map<char, std::vector<std::pair<int, int>>> condition_map;

						int channel_index = 0;
						if (instruction.data.dst.mask_x) condition_map[cond.mask[0]].push_back({ 0, channel_index++ });
						if (instruction.data.dst.mask_y) condition_map[cond.mask[1]].push_back({ 1, channel_index++ });
						if (instruction.data.dst.mask_z) condition_map[cond.mask[2]].push_back({ 2, channel_index++ });
						if (instruction.data.dst.mask_w) condition_map[cond.mask[3]].push_back({ 3, channel_index });

						auto src = arg;

						if (flags & disable_swizzle_as_dst)
						{
							src.assign(expression_from<float_point_t<4>>(arg.text, true, dest.mask.size()));
						}

						for (auto &entry : condition_map)
						{
							std::string src_swizzle;
							std::string dst_swizzle;

							for (std::pair<int, int> channels : entry.second)
							{
								src_swizzle += src.swizzle(flags & disable_swizzle_as_dst ? channels.second : channels.first).mask[0];
								dst_swizzle += dest.swizzle(channels.second).mask[0];
							}

							expression_from<float_point_t<4>> expression{ src.with_mask(src_swizzle) };

							if (!instruction.data.dst.no_dest)
							{
								expression.assign(dest.with_mask(dst_swizzle) = expression);
							}

							if (instruction.data.dst.set_cond)
							{
								expression.assign(cond.with_mask(dst_swizzle) = expression);
							}

							result += if_(cond.swizzle(channel_to_index.at(entry.first)).call_operator<boolean_t<1>>(operation, zero), expression);
						}
					}
				}
				else
				{
					expression_from<float_point_t<4>> src = arg;

					if (instruction.data.dst.set_cond || !instruction.data.dst.no_dest)
					{
						if ((flags & disable_swizzle_as_dst) == 0)
						{
							src.assign(instruction.swizzle_as_dst(arg));
						}

						src.assign(apply_instruction_modifiers(src).without_scope());

						if (!instruction.data.dst.no_dest)
						{
							src.assign(dest = src);
						}

						if (instruction.data.dst.set_cond)
						{
							src.assign(expression_from<float_point_t<4>>(modify_cond.text, dest.mask) = src);
						}
					}

					result += src;
				}

				return result;
			}

			writer_t set_dst(const expression_from<float_point_t<1>>& arg, u32 flags = none)
			{
				if (instruction.destination_swizzle().size() != 1)
				{
					return set_dst(float_point_t<4>::ctor(arg), flags);
				}

				return set_dst(expression_from<float_point_t<4>>{ arg.to_string() }, flags | disable_swizzle_as_dst);
			}

			writer_t set_dst(const expression_from<boolean_t<4>>& arg, u32 flags = none)
			{
				std::string arg_string;

				bool is_single = true;

				switch (instruction.destination_swizzle().size())
				{
				case 1: arg_string = arg.to_string() + " ? 1.0 : 0.0"; is_single = false; break;
				case 2: arg_string = float_point_t<2>::ctor(expression_from<boolean_t<2>>{ arg.to_string() }).to_string(); break;
				case 3: arg_string = float_point_t<3>::ctor(expression_from<boolean_t<3>>{ arg.to_string() }).to_string(); break;
				case 4: arg_string = float_point_t<4>::ctor(expression_from<boolean_t<4>>{ arg.to_string() }).to_string(); break;

				default:
					throw;
				}

				return set_dst(expression_from<float_point_t<4>>{ arg_string, std::string("xyzw"), is_single, 4 }, flags);
			}

			writer_t comment(const std::string& lines)
			{
				writer_t result;

				result += "//" + lines + "\n";

				return result;
			}

			writer_t warning(const std::string& lines)
			{
				return comment("WARNING: " + lines);
			}

			writer_t unimplemented(const std::string& lines)
			{
				return comment("TODO: " + lines);
			}

			expression_base_t decode_instruction()
			{
				switch (instruction.data.dst.opcode | (instruction.data.src1.opcode_is_branch << 6))
				{
				case opcode::NOP: return comment("nop");
				case opcode::MOV: return set_dst(src_swizzled_as_dst(0), disable_swizzle_as_dst);
				case opcode::MUL: return set_dst(src_swizzled_as_dst(0) * src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case opcode::ADD: return set_dst(src_swizzled_as_dst(0) + src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case opcode::MAD: return set_dst((src_swizzled_as_dst(0) * src_swizzled_as_dst(1)).without_scope() + src_swizzled_as_dst(2), disable_swizzle_as_dst);
				case opcode::DP3: return set_dst(dot(src(0).xyz(), src(1).xyz()));
				case opcode::DP4: return set_dst(dot(src(0), src(1)));
				case opcode::DST:
				{
					auto src_0 = src(0);
					auto src_1 = src(1);

					return set_dst(float_point_t<4>::ctor(1.0f, src_0.y() * src_1.y(), src_0.z(), src_1.w()));
				}
				case opcode::MIN: return set_dst(min(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::MAX: return set_dst(max(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::SLT: return set_dst(compare(compare_function::less, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::SGE: return set_dst(compare(compare_function::greater_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::SLE: return set_dst(compare(compare_function::less_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::SGT: return set_dst(compare(compare_function::greater, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::SNE: return set_dst(compare(compare_function::not_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::SEQ: return set_dst(compare(compare_function::equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::FRC: return set_dst(fract(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::FLR: return set_dst(floor(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::KIL: return conditional(expression_from<void_t>("discard"));
				case opcode::PK4: return unimplemented("PK4");
				case opcode::UP4: return unimplemented("UP4");
				case opcode::DDX: return set_dst(ddx(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::DDY: return set_dst(ddy(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::TEX: return set_dst(texture(tex(), src(0).xy()));
				case opcode::TXP: return set_dst(texture(tex(), src(0).xy() / src(0).w()));
				case opcode::TXD: return set_dst(texture_grad(tex(), src(0).xy(), src(1).xy(), src(2).xy()));
				case opcode::RCP: return set_dst(float_point_t<1>::ctor(1.0f) / src_swizzled_as_dst(0), disable_swizzle_as_dst);
				case opcode::RSQ: return set_dst(rsqrt(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::EX2: return set_dst(exp2(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::LG2: return set_dst(log2(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::LIT: return unimplemented("LIT");
				case opcode::LRP: return unimplemented("LRP");
				case opcode::STR: return set_dst(1.0f);
				case opcode::SFL: return set_dst(0.0f);
				case opcode::COS: return set_dst(cos(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::SIN: return set_dst(sin(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case opcode::PK2: return unimplemented("PK2");
				case opcode::UP2: return unimplemented("UP2");
				case opcode::POW: return set_dst(pow(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::PKB: return unimplemented("PKB");
				case opcode::UPB: return unimplemented("UPB");
				case opcode::PK16: return unimplemented("PK16");
				case opcode::UP16: return unimplemented("UP16");
				case opcode::BEM: return unimplemented("BEM");
				case opcode::PKG: return unimplemented("PKG");
				case opcode::UPG: return unimplemented("UPG");
				case opcode::DP2A:
				{
					auto src_0 = src(0);
					auto src_1 = src(1);

					return set_dst(float_point_t<4>::ctor(src_0.x() * src_1.x() + src_0.y() * src_1.y() + src(2).z()));
				}
				case opcode::TXL: return set_dst(texture_lod(tex(), src(0).xy(), src(1).x()));
				case opcode::TXB: return set_dst(texture_bias(tex(), src(0).xy(), src(1).x()));
				case opcode::TEXBEM: return unimplemented("TEXBEM");
				case opcode::TXPBEM: return unimplemented("TXPBEM");
				case opcode::BEMLUM: return unimplemented("BEMLUM");
				case opcode::REFL: return unimplemented("REFL");
				case opcode::TIMESWTEX: return unimplemented("TIMESWTEX");
				case opcode::DP2: return set_dst(dot(src(0).xy(), src(1).xy()));
				case opcode::NRM: return set_dst(normalize(src(0).xyz()).xyzx());
				case opcode::DIV: return set_dst(src_swizzled_as_dst(0) / src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case opcode::DIVSQ: return set_dst(src_swizzled_as_dst(0) / sqrt(src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::LIF: return unimplemented("LIF");
				case opcode::FENCT: return comment("fenct");
				case opcode::FENCB: return comment("fencb");
				case opcode::BRK: return conditional(expression_from<void_t>("break"));
				case opcode::CAL: return unimplemented("CAL");
				case opcode::IFE:
					writer += writer_t{ "if (" + execution_condition(condition_operation::all).to_string() + ")\n{\n" };

					if (instruction.data.src2.end_offset != instruction.data.src1.else_offset)
						writer.before(instruction.data.src1.else_offset >> 2, "}\nelse\n{\n");

					writer.after(instruction.data.src2.end_offset >> 2, "}\n");

					return "";

				case opcode::LOOP:
					writer += writer_t
					{
						"for ("
						"int i = " + std::to_string(instruction.data.src1.init_counter) + "; " +
						"i < " + std::to_string(instruction.data.src1.end_counter) + "; " +
						(instruction.data.src1.increment == 1 ? "i++" : "i += " + std::to_string(instruction.data.src1.increment)) +
						")\n{\n"
					};

					writer.after(instruction.data.src2.end_offset >> 2, "}\n");
					return "";
				case opcode::REP: return unimplemented("REP");
				case opcode::RET: return conditional(expression_from<void_t>("return"));
				}

				throw;
			}

			void decompile(std::size_t offset, instruction_t* instructions)
			{
				context.offset = 0;
				context.is_next_is_constant = false;

				for (std::size_t index = offset; true; ++index, writer.next(), context.offset += sizeof(instruction_t))
				{
					if (context.is_next_is_constant)
					{
						context.is_next_is_constant = false;
						continue;
					}

					instruction = instructions[index].unpack();

					writer += decode_instruction();

					if (instruction.data.dst.end)
						break;
				}
			}
		};

		decompiled_program decompile(std::size_t offset, ucode_instr *instructions)
		{
			decompiler dec;
			dec.decompile(offset, (decompiler::instruction_t*)instructions);

			decompiled_program result{};

			//result.constant_offsets = ...;
			//result.uniforms = ...;
			//result.textures = ...;
			//result.temporary_registers = ...;
			//result.input_attributes = ...;
			//result.output_attributes = ...;

			result.code = dec.writer.build();

			return result;
		}
	}
}
