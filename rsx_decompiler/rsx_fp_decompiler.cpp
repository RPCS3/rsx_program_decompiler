#include "rsx_decompiler_base.h"
#include "rsx_fp_ucode.h"
#include <glsl_language.h>
#include <map>
#include "../rsx_program_decompiler/endianness.h"

using namespace endianness;

namespace rsx
{
	namespace fragment_program
	{
		template<typename Language>
		class decompiler : public decompiler_base<Language>
		{
			using base = decompiler_base<Language>;

			template<int Count>
			using boolean_expr = typename base::template boolean_expr<Count>;

			template<int Count>
			using float_point_expr = typename base::template float_point_expr<Count>;

			using sampler2D_expr = typename base::template expression_from<typename base::sampler2D_t>;

			template<int Count>
			using float_point_t = typename base::template float_point_t<Count>;

			template<int Count>
			using boolean_t = typename base::template boolean_t<Count>;

			struct context_t
			{
				decompiled_shader program;
				bool is_next_is_constant;
				u32 offset;

				float_point_expr<4> constant()
				{
					constant_info info;
					info.id = offset + sizeof(instruction_t);
					info.name = "fc" + std::to_string(info.id);
					program.constants.insert(info);
					is_next_is_constant = true;

					return info.name;
				}

				float_point_expr<4> temporary(bool is_fp16, int index)
				{
					register_info info;
					info.id = index;
					info.type = is_fp16 ? register_type::half_float_point : register_type::single_float_point;
					info.name = (is_fp16 ? "h" : "r") + std::to_string(index);
					program.temporary_registers.insert(info);
					return info.name;
				}

				float_point_expr<4> condition(int index)
				{
					register_info info;
					info.id = index;
					info.type = register_type::single_float_point;
					info.name = "cc" + std::to_string(index);
					program.temporary_registers.insert(info);

					return info.name;
				}

				sampler2D_expr texture(int index)
				{
					texture_info info;
					info.id = index;
					info.name = "texture" + std::to_string(index);
					program.textures.insert(info);

					return info.name;
				}

				float_point_expr<4> texture_coords_modifier(int index)
				{
					return{ "texture" + std::to_string(index) + "_cm" };
				}

				float_point_expr<4> input(int index)
				{
					program.input_attributes |= (1 << index);
					return input_attrib_names[index];
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

				instruction_t unpack() const
				{
					instruction_t result;

					(be_t<u32>&)result.data.dst._u32 = (data.dst._u32 << 16) | (data.dst._u32 >> 16);
					(be_t<u32>&)result.data.src0._u32 = (data.src0._u32 << 16) | (data.src0._u32 >> 16);
					(be_t<u32>&)result.data.src1._u32 = (data.src1._u32 << 16) | (data.src1._u32 >> 16);
					(be_t<u32>&)result.data.src2._u32 = (data.src2._u32 << 16) | (data.src2._u32 >> 16);

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

				float_point_expr<4> swizzle_as_dst(float_point_expr<4> arg) const
				{
					std::string arg_mask;

					for (char channel : destination_swizzle())
					{
						arg_mask += arg.mask[channel_to_index.at(channel)];
					}

					return float_point_expr<4>(arg.text, arg_mask, arg.is_single, arg.base_count);
				}

				float_point_expr<4> src(context_t& context, int index, bool is_swizzle_as_dst = false) const
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
						case src_reg_type_t::constant: return context.constant();
						}

						throw std::runtime_error("bad instruction argument (#" + std::to_string((u32)src.reg_type) + ") type.");
					};

					float_point_expr<4> result = get_variable(src);

					result.assign(result.swizzle(src.swizzle_x, src.swizzle_y, src.swizzle_z, src.swizzle_w));

					if (is_swizzle_as_dst)
					{
						result.assign(swizzle_as_dst(result));
					}

					if (src.abs)
					{
						result.assign(base::abs(result));
					}

					if (src.neg)
					{
						result.assign(-result);
					}

					return result;
				}

				sampler2D_expr texture(context_t& context) const
				{
					return context.texture(data.dst.tex_num);
				}

				float_point_expr<4> texture_coords_modifier(context_t& context) const
				{
					return context.texture_coords_modifier(data.dst.tex_num);
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

				float_point_expr<4> destination(context_t& context) const
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

			float_point_expr<4> src(int index, bool is_swizzle_as_dst = false)
			{
				return instruction.src(context, index, is_swizzle_as_dst);
			}

			float_point_expr<4> src_swizzled_as_dst(int index)
			{
				return src(index, instruction.data.dst.set_cond || !instruction.data.dst.no_dest);
			}

			float_point_expr<4> modify_condition_register()
			{
				return context.condition(instruction.data.src0.cond_mod_reg_index);
			}

			float_point_expr<4> execution_condition_register()
			{
				std::string swizzle;

				swizzle += mask[instruction.data.src0.cond_swizzle_x];
				swizzle += mask[instruction.data.src0.cond_swizzle_y];
				swizzle += mask[instruction.data.src0.cond_swizzle_z];
				swizzle += mask[instruction.data.src0.cond_swizzle_w];

				return{ context.condition(instruction.data.src0.cond_reg_index).text, swizzle };
			}

			enum class condition_operation
			{
				all,
				any
			};

			typename base::compare_function execution_condition_function() const
			{
				if (instruction.data.src0.exec_if_gr && instruction.data.src0.exec_if_eq)
				{
					return base::compare_function::greater_equal;
				}
				if (instruction.data.src0.exec_if_lt && instruction.data.src0.exec_if_eq)
				{
					return base::compare_function::less_equal;
				}
				if (instruction.data.src0.exec_if_gr && instruction.data.src0.exec_if_lt)
				{
					return base::compare_function::not_equal;
				}
				if (instruction.data.src0.exec_if_gr)
				{
					return base::compare_function::greater;
				}
				if (instruction.data.src0.exec_if_lt)
				{
					return base::compare_function::less;
				}

				if (instruction.data.src0.exec_if_eq)
				{
					return base::compare_function::equal;
				}

				throw std::logic_error("");
			}

			boolean_expr<1> execution_condition(condition_operation operation)
			{
				if (instruction.data.src0.exec_if_gr && instruction.data.src0.exec_if_eq && instruction.data.src0.exec_if_gr)
				{
					return true;
				}

				if (!instruction.data.src0.exec_if_gr && !instruction.data.src0.exec_if_eq && !instruction.data.src0.exec_if_gr)
				{
					return false;
				}

				auto cond = execution_condition_register();

				if (instruction.data.src0.cond_swizzle_x == instruction.data.src0.cond_swizzle_y &&
					instruction.data.src0.cond_swizzle_y == instruction.data.src0.cond_swizzle_z &&
					instruction.data.src0.cond_swizzle_z == instruction.data.src0.cond_swizzle_w)
				{
					return base::custom_compare(execution_condition_function(), 1, cond.x(), float_point_expr<1>(0.0f));
				}

				auto result = base::custom_compare(execution_condition_function(), 4, cond, float_point_t<4>::ctor(0.0f));

				switch (operation)
				{
				case condition_operation::all: return base::all(result);
				case condition_operation::any: return base::any(result);
				}

				throw;
			}

			boolean_expr<4> compare(typename base::compare_function function, float_point_expr<4> a, float_point_expr<4> b)
			{
				return base::custom_compare(function, (int)instruction.destination_swizzle().size(), a, b);
			}

			sampler2D_expr texture()
			{
				return instruction.texture(context);
			}

			float_point_expr<4> texture_coords(int src_index = 0)
			{
				return src(src_index) * instruction.texture_coords_modifier(context);
			}

			template<typename ExprType>
			typename base::void_expr conditional(const ExprType& expr)
			{
				bool need_condition = false;

				if (need_condition)
				{
					return base::if_(base::any(execution_condition(condition_operation::any)), expr);
				}

				return expr;
			}

			enum set_dst_flags
			{
				none,
				disable_swizzle_as_dst = 1,
				allow_bx2
			};

			template<typename Type>
			Type apply_instruction_modifiers(Type arg)
			{
				using float_expr = float_point_expr<1>;

				switch (instruction.data.src1.scale)
				{
				case 0: break;
				case 1: arg.assign(arg * (float_expr)2.0f); break;
				case 2: arg.assign(arg * (float_expr)4.0f); break;
				case 3: arg.assign(arg * (float_expr)8.0f); break;
				case 5: arg.assign(arg / (float_expr)2.0f); break;
				case 6: arg.assign(arg / (float_expr)4.0f); break;
				case 7: arg.assign(arg / (float_expr)8.0f); break;

				default:
					throw std::runtime_error("fragment program decompiler: unimplemented scale (" + std::to_string(instruction.data.src1.scale) + "). ");
				}

				if (instruction.data.dst.saturate)
				{
					arg.assign(base::clamp(arg, 0.0f, 1.0f));
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
						arg.assign(base::clamp(arg, -65536.0f, 65536.0f));
						break;

					case 2: //fixed point 12
						arg.assign(base::clamp(arg, -1.0f, 1.0f));
						break;

					default:
						throw std::runtime_error("fragment program decompiler: unimplemented precision. (" + std::to_string(instruction.data.dst.prec) +").");
					}
				}

				return arg;
			}

			builder::writer_t set_dst(const float_point_expr<4>& arg, u32 flags = none)
			{
				builder::writer_t result;

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
						result += base::warning("extra condition test skipped");
						result += arg;
					}
					else
					{
						static const float_point_expr<1> zero(0.0f);

						std::map<char, std::vector<std::pair<int, int>>> condition_map;

						int channel_index = 0;
						if (instruction.data.dst.mask_x) condition_map[cond.mask[0]].push_back({ 0, channel_index++ });
						if (instruction.data.dst.mask_y) condition_map[cond.mask[1]].push_back({ 1, channel_index++ });
						if (instruction.data.dst.mask_z) condition_map[cond.mask[2]].push_back({ 2, channel_index++ });
						if (instruction.data.dst.mask_w) condition_map[cond.mask[3]].push_back({ 3, channel_index });

						auto src = apply_instruction_modifiers(arg);

						if (flags & disable_swizzle_as_dst)
						{
							if (src.mask.size() != dest.mask.size())
								src.assign(float_point_expr<4>(arg.text, arg.mask, true, (int)dest.mask.size()));
							else
								src.assign(src.without_scope());
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

							float_point_expr<4> expression{ src.with_mask(src_swizzle) };

							if (!instruction.data.dst.no_dest)
							{
								expression.assign(dest.with_mask(dst_swizzle) = expression);
							}

							if (instruction.data.dst.set_cond)
							{
								expression.assign(modify_cond.with_mask(dst_swizzle) = expression);
							}

							result += base::if_(cond.swizzle(channel_to_index.at(entry.first)).template call_operator<boolean_t<1>>(operation, zero), expression);
						}
					}
				}
				else
				{
					float_point_expr<4> src = arg;

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
							src.assign(modify_cond.with_mask(dest.mask) = src);
						}
					}

					result += src;
				}

				return result;
			}

			builder::writer_t set_dst(const float_point_expr<1>& arg, u32 flags = none)
			{
				switch (instruction.destination_swizzle().size())
				{
				case 1: return set_dst(float_point_expr<4>{ arg.to_string() }, flags | disable_swizzle_as_dst);
				case 2: return set_dst(float_point_expr<4>{ float_point_t<2>::ctor(arg).to_string() }, flags);
				case 3: return set_dst(float_point_expr<4>{ float_point_t<3>::ctor(arg).to_string() }, flags);
				case 4: return set_dst(float_point_t<4>::ctor(arg), flags);

				default:
					throw std::logic_error("bad destination swizzle.");
				}
			}

			builder::writer_t set_dst(const boolean_expr<4>& arg, u32 flags = none)
			{
				std::string arg_string;

				bool is_single = true;

				switch (instruction.destination_swizzle().size())
				{
				case 1: arg_string = arg.to_string() + " ? 1.0 : 0.0"; is_single = false; break;
				case 2: arg_string = float_point_t<2>::ctor(boolean_expr<2>{ arg.to_string() }).to_string(); break;
				case 3: arg_string = float_point_t<3>::ctor(boolean_expr<3>{ arg.to_string() }).to_string(); break;
				case 4: arg_string = float_point_t<4>::ctor(boolean_expr<4>{ arg.to_string() }).to_string(); break;

				default:
					throw std::logic_error("bad destination swizzle.");
				}

				return set_dst(float_point_expr<4>{ arg_string, std::string("xyzw"), is_single, 4 }, flags);
			}

			builder::expression_base_t decode_instruction()
			{
				switch (u32(instruction.data.dst.opcode) | (u32(instruction.data.src1.opcode_is_branch) << 6))
				{
				case (u32)opcode_t::nop: return base::comment("nop");
				case (u32)opcode_t::mov: return set_dst(src_swizzled_as_dst(0), disable_swizzle_as_dst);
				case (u32)opcode_t::mul: return set_dst(src_swizzled_as_dst(0) * src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case (u32)opcode_t::add: return set_dst(src_swizzled_as_dst(0) + src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case (u32)opcode_t::mad: return set_dst((src_swizzled_as_dst(0) * src_swizzled_as_dst(1)).without_scope() + src_swizzled_as_dst(2), disable_swizzle_as_dst);
				case (u32)opcode_t::dp3: return set_dst(base::dot(src(0).xyz(), src(1).xyz()));
				case (u32)opcode_t::dp4: return set_dst(base::dot(src(0), src(1)));
				case (u32)opcode_t::dst:
				{
					auto src_0 = src(0);
					auto src_1 = src(1);

					return set_dst(float_point_t<4>::ctor(1.0f, src_0.y() * src_1.y(), src_0.z(), src_1.w()));
				}
				case (u32)opcode_t::min: return set_dst(base::min(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::max: return set_dst(base::max(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::slt: return set_dst(compare(base::compare_function::less, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::sge: return set_dst(compare(base::compare_function::greater_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::sle: return set_dst(compare(base::compare_function::less_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::sgt: return set_dst(compare(base::compare_function::greater, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::sne: return set_dst(compare(base::compare_function::not_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::seq: return set_dst(compare(base::compare_function::equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::frc: return set_dst(base::fract(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case (u32)opcode_t::flr: return set_dst(base::floor(src_swizzled_as_dst(0)), disable_swizzle_as_dst);
				case (u32)opcode_t::kil: return conditional(typename base::void_expr{ "discard;" });
				case (u32)opcode_t::pk4: return base::unimplemented("PK4");
				case (u32)opcode_t::up4: return base::unimplemented("UP4");
				//case (u32)opcode_t::ddx: return set_dst(base::ddx(src(0).xy()), disable_swizzle_as_dst);
				//case (u32)opcode_t::ddy: return set_dst(base::ddy(src(0).xy()), disable_swizzle_as_dst);
				case (u32)opcode_t::tex: return set_dst(base::texture(texture(), texture_coords().xy()), allow_bx2);
				case (u32)opcode_t::txp: return set_dst(base::texture(texture(), texture_coords().xy() / src(0).w()), allow_bx2);
				case (u32)opcode_t::txd: return set_dst(base::texture_grad(texture(), texture_coords().xy(), src(1).xy(), src(2).xy()), allow_bx2);
				case (u32)opcode_t::rcp: return set_dst(float_point_t<1>::ctor(1.0f) / src(0).x(), disable_swizzle_as_dst);
				case (u32)opcode_t::rsq: return set_dst(base::rsqrt(src(0).x()), disable_swizzle_as_dst);
				case (u32)opcode_t::ex2: return set_dst(base::exp2(src(0).x()), disable_swizzle_as_dst);
				case (u32)opcode_t::lg2: return set_dst(base::log2(src(0).x()), disable_swizzle_as_dst);
				case (u32)opcode_t::lit: return base::unimplemented("LIT");
				case (u32)opcode_t::lrp: return base::unimplemented("LRP");
				case (u32)opcode_t::str: return set_dst(1.0f);
				case (u32)opcode_t::sfl: return set_dst(0.0f);
				case (u32)opcode_t::cos: return set_dst(base::cos(src(0).x()), disable_swizzle_as_dst);
				case (u32)opcode_t::sin: return set_dst(base::sin(src(0).x()), disable_swizzle_as_dst);
				case (u32)opcode_t::pk2: return base::unimplemented("PK2");
				case (u32)opcode_t::up2: return base::unimplemented("UP2");
				case (u32)opcode_t::pow: return set_dst(base::pow(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case (u32)opcode_t::pkb: return base::unimplemented("PKB");
				case (u32)opcode_t::upb: return base::unimplemented("UPB");
				case (u32)opcode_t::pk16: return base::unimplemented("PK16");
				case (u32)opcode_t::up16: return base::unimplemented("UP16");
				case (u32)opcode_t::bem: return base::unimplemented("BEM");
				case (u32)opcode_t::pkg: return base::unimplemented("PKG");
				case (u32)opcode_t::upg: return base::unimplemented("UPG");
				case (u32)opcode_t::dp2a:
				{
					auto src_0 = src(0);
					auto src_1 = src(1);

					return set_dst(float_point_t<4>::ctor(src_0.x() * src_1.x() + src_0.y() * src_1.y() + src(2).z()));
				}
				case (u32)opcode_t::txl: return set_dst(base::texture_lod(texture(), texture_coords().xy(), src(1).x()), allow_bx2);
				case (u32)opcode_t::txb: return set_dst(base::texture_bias(texture(), texture_coords().xy(), src(1).x()), allow_bx2);
				case (u32)opcode_t::texbem: return base::unimplemented("TEXBEM");
				case (u32)opcode_t::txpbem: return base::unimplemented("TXPBEM");
				case (u32)opcode_t::bemlum: return base::unimplemented("BEMLUM");
				case (u32)opcode_t::refl: return base::unimplemented("REFL");
				case (u32)opcode_t::timeswtex: return base::unimplemented("TIMESWTEX");
				case (u32)opcode_t::dp2: return set_dst(base::dot(src(0).xy(), src(1).xy()));
				case (u32)opcode_t::nrm: return set_dst(base::normalize(src(0).xyz()).xyzx());
				case (u32)opcode_t::div: return set_dst(src_swizzled_as_dst(0) / src(1).x(), disable_swizzle_as_dst);
				case (u32)opcode_t::divsq: return set_dst(src_swizzled_as_dst(0) / base::sqrt(src(1).x()), disable_swizzle_as_dst);
				case (u32)opcode_t::lif: return base::unimplemented("LIF");
				case (u32)opcode_t::fenct: return base::comment("fenct");
				case (u32)opcode_t::fencb: return base::comment("fencb");
				case (u32)opcode_t::brk: return conditional(typename base::void_expr("break;"));
				case (u32)opcode_t::cal: return base::unimplemented("CAL");
				case (u32)opcode_t::ife:
					base::writer += typename base::writer_t{ "if (" + execution_condition(condition_operation::all).to_string() + ")\n{\n" };

					if (instruction.data.src2.end_offset != instruction.data.src1.else_offset)
						base::writer.before(instruction.data.src1.else_offset >> 2, "}\nelse\n{\n");

					base::writer.after(instruction.data.src2.end_offset >> 2, "}\n");

					return{ "" };

				case (u32)opcode_t::loop:
					base::writer += typename base::writer_t
					{
						"for ("
						"int i = " + std::to_string(instruction.data.src1.init_counter) + "; " +
						"i < " + std::to_string(instruction.data.src1.end_counter) + "; " +
						(instruction.data.src1.increment == 1 ? "i++" : "i += " + std::to_string(instruction.data.src1.increment)) +
						")\n{\n"
					};

					base::writer.after(instruction.data.src2.end_offset >> 2, "}\n");
					return{ "" };
				case (u32)opcode_t::rep: return base::unimplemented("REP");
				case (u32)opcode_t::ret: return conditional(typename base::void_expr("return;"));
				}

				throw std::runtime_error("bad instruction. (" + std::to_string(u32(instruction.data.dst.opcode) | (u32(instruction.data.src1.opcode_is_branch) << 6)) + ")");
			}

			decompiled_shader decompile(std::size_t offset, const instruction_t* instructions)
			{
				context.offset = 0;
				context.is_next_is_constant = false;

				for (std::size_t index = offset; true; ++index, base::writer.next(), context.offset += sizeof(instruction_t))
				{
					if (context.is_next_is_constant)
					{
						context.is_next_is_constant = false;
						continue;
					}

					instruction = instructions[index].unpack();

					try
					{
						base::writer += decode_instruction();
					}
					catch (const std::runtime_error &ex)
					{
						base::writer += base::comment("exception: " + std::string(ex.what()));
					}

					if (instruction.data.dst.end)
					{
						context.offset += context.is_next_is_constant ? sizeof(instruction_t) * 2 : sizeof(instruction_t);
						break;
					}
				}

				context.program.entry_function = "func0";
				base::writer.before(0, "void func0()\n{\n");
				base::writer.after(base::writer.position, "}\n");
				context.program.code = base::writer.finalize();

				return context.program;
			}
		};

		template<typename Language>
		decompiled_shader decompile(std::size_t offset, const void *instructions)
		{
			return decompiler<Language>{}.decompile(offset, (const typename decompiler<Language>::instruction_t*)instructions);
		}

		decompiled_shader decompile(const raw_shader &shader, decompile_language lang)
		{
			decompiled_shader result;
			switch (lang)
			{
			case decompile_language::glsl:
				result = decompile<shader_code::glsl_language>(shader.offset, shader.ucode.data());
				result.code_language = decompile_language::glsl;
				break;

			default:
				throw;
			}

			return result;
		}
	}
}
