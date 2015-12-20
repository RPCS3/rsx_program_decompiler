#include "rsx_decompiler.h"
#include <glsl_language.h>
#include "rsx_decompiler_base.h"
#include <map>

namespace rsx
{
	namespace vertex_program
	{
		template<typename Language>
		class decompiler : public decompiler_base<Language>
		{
			using base = decompiler_base<Language>;

			template<int Count>
			using boolean_expr = typename base::boolean_expr<Count>;

			template<int Count>
			using float_point_expr = typename base::float_point_expr<Count>;

			template<int Count>
			using integer_expr = typename base::integer_expr<Count>;

			template<int Count>
			using float_point_t = typename base::float_point_t<Count>;

			template<int Count>
			using boolean_t = typename base::boolean_t<Count>;

			template<int Count>
			using integer_t = typename base::integer_t<Count>;

			struct context_t
			{
				decompiled_shader program;

				std::string address_register(u32 address_register)
				{
					register_info info;
					info.id = address_register;
					info.name = "a" + std::to_string(info.id);
					info.type = register_type::integer;
					program.temporary_registers.insert(info);

					return info.name;
				}

				std::string address_mask(u32 swizzle)
				{
					return std::string(1, mask[swizzle]);
				}

				float_point_expr<4> constant(u32 id, u32 index_constant, u32 address_register_, u32 swizzle)
				{
					constant_info info;
					info.id = 468;
					info.name = "vc[468]";
					program.constants.insert(info);

					return "vc[" + std::to_string(id) + 
						(index_constant ? " + " + address_register(address_register_) + "." + address_mask(swizzle) : "")
						+ "]";
				}

				float_point_expr<4> output(u32 index)
				{
					program.output_attributes |= (1 << index);
					register_info info;
					info.id = index;
					info.type = register_type::single_float_point;
					info.name = rsx::fragment_program::input_attrib_map[index];//"o" + std::to_string(index);
					program.temporary_registers.insert(info);
					return info.name;
				}

				float_point_expr<4> temporary(u32 index)
				{
					register_info info;
					info.id = index;
					info.type = register_type::single_float_point;
					info.name = "r" + std::to_string(index);
					program.temporary_registers.insert(info);
					return info.name;
				}

				float_point_expr<4> condition(int index)
				{
					register_info info;
					info.id = index;
					info.type = register_type::single_float_point;
					info.name = "rc" + std::to_string(index);
					program.temporary_registers.insert(info);

					return info.name;
				}

				float_point_expr<4> input(int index)
				{
					program.input_attributes |= (1 << index);
					return input_registers_table[index];
				}
			};

		public:
			struct instruction_t
			{
				ucode_instr data;

				SRC unpack_src(int index) const
				{
					SRC result;

					switch (index)
					{
					case 0:
						result.src0l = data.d2.src0l;
						result.src0h = data.d1.src0h;
						break;

					case 1:
						result.src1 = data.d2.src1;
						break;

					case 2:
						result.src2l = data.d3.src2l;
						result.src2h = data.d2.src2h;
						break;

					default:
						throw;
					}

					return result;
				}

				const instruction_t& unpack() const
				{
					return *this;
				}
			};

			static_assert(sizeof(instruction_t) == 16, "Bad instruction_t implementation");

		private:
			instruction_t instruction;
			context_t context;
			bool is_vec;

			u32 temporary_destination_register_index() const
			{
				if (is_vec)
				{
					return instruction.data.d0.dst_tmp;
				}

				return instruction.data.d3.sca_dst_tmp;
			}

			u32 address_value() const
			{
				return (instruction.data.d2.iaddrh << 3) | instruction.data.d3.iaddrl;
			}

			integer_expr<1> address_register()
			{
				return{ context.address_register(instruction.data.d0.addr_reg_sel_1), context.address_mask(instruction.data.d0.addr_swz), true, 4 };
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

			float_point_expr<4> src(int index, bool is_swizzle_as_dst = false)
			{
				SRC src = instruction.unpack_src(index);

				u32 is_abs;

				switch (index)
				{
				case 0: is_abs = instruction.data.d0.src0_abs; break;
				case 1: is_abs = instruction.data.d0.src1_abs; break;
				case 2: is_abs = instruction.data.d0.src2_abs; break;
				}

				auto get_register = [&]()
				{
					switch (src.register_type)
					{
					case src_register_type::temporary:
						return context.temporary(src.tmp_src);

					case src_register_type::input:
						return context.input(instruction.data.d1.input_src);

					case src_register_type::constant:
						return context.constant(instruction.data.d1.const_src,
							instruction.data.d3.index_const,
							instruction.data.d0.addr_reg_sel_1,
							instruction.data.d0.addr_swz);
					}

					throw;
				};

				float_point_expr<4> result = get_register();

				result.assign(result.swizzle(src.swz_x, src.swz_y, src.swz_z, src.swz_w));

				if (is_swizzle_as_dst)
				{
					result.assign(swizzle_as_dst(result));
				}

				if (is_abs)
				{
					result.assign(base::abs(result));
				}

				if (src.neg)
				{
					result.assign(-result);
				}

				return result;
			}

			float_point_expr<4> src_swizzled_as_dst(int index)
			{
				return src(index, true);
			}

			std::string destination_swizzle() const
			{
				std::string result;

				if (is_vec)
				{
					if (instruction.data.d3.vec_writemask_x) result += "x";
					if (instruction.data.d3.vec_writemask_y) result += "y";
					if (instruction.data.d3.vec_writemask_z) result += "z";
					if (instruction.data.d3.vec_writemask_w) result += "w";
				}
				else
				{
					if (instruction.data.d3.sca_writemask_x) result += "x";
					if (instruction.data.d3.sca_writemask_y) result += "y";
					if (instruction.data.d3.sca_writemask_z) result += "z";
					if (instruction.data.d3.sca_writemask_w) result += "w";
				}

				return result;
			}

			std::pair<bool, float_point_expr<4>> destination_register()
			{
				if (is_vec && instruction.data.d0.vec_result)
				{
					return{ true, { context.output(instruction.data.d3.dst).text, destination_swizzle() } };
				}

				u32 tmp = temporary_destination_register_index();
				bool no_dest = tmp == 63;

				if (no_dest)
				{
					return{ false, {} };
				}

				return{ true,{ context.temporary(tmp).text, destination_swizzle() } };
			}

			float_point_expr<4> update_condition_register()
			{
				return{ context.condition(instruction.data.d0.cond_reg_sel_1).text, destination_swizzle() };
			}
			float_point_expr<4> execution_condition_register()
			{
				std::string swizzle;

				swizzle += mask[instruction.data.d0.mask_x];
				swizzle += mask[instruction.data.d0.mask_y];
				swizzle += mask[instruction.data.d0.mask_z];
				swizzle += mask[instruction.data.d0.mask_w];

				return{ context.condition(instruction.data.d0.cond_reg_sel_1).text, swizzle };
			}

			boolean_expr<4> compare(typename base::compare_function function, float_point_expr<4> a, float_point_expr<4> b)
			{
				return base::custom_compare(function, (int)destination_swizzle().size(), a, b);
			}

			float_point_expr<4> apply_instruction_modifiers(float_point_expr<4> arg)
			{
				if (instruction.data.d0.staturate)
				{
					return base::clamp(arg, -1.0f, 1.0f);
				}

				return arg;
			}

			enum
			{
				lt = 0x1,
				eq = 0x2,
				gt = 0x4,

				always = (lt | eq | gt),
				never = 0
			};

			typename base::writer_t set_dst(float_point_expr<4> arg)
			{
				auto dst_pair = destination_register();
				bool has_dst = dst_pair.first;
				bool has_result = has_dst || (instruction.data.d0.cond_update_enable_0 && instruction.data.d0.cond_update_enable_1);
				float_point_expr<4> dest{ dst_pair.second };

				typename base::writer_t result;

				if (instruction.data.d0.cond_test_enable && instruction.data.d0.cond != always)
				{
					if (!has_result)
					{
						result += base::warning("Extra condition skipped");
						result += arg;
					}
					else
					{
						static const std::string operations[0x8] =
						{
							"error",
							"<",
							"==",
							"<=",
							">",
							"!=",
							">=",
							"error"
						};

						auto cond = execution_condition_register();
						std::string operation = operations[instruction.data.d0.cond];
						static const float_point_expr<1> zero(0.0f);

						std::map<char, std::vector<std::pair<int, int>>> condition_map;

						int channel_index = 0;

						if (is_vec)
						{
							if (instruction.data.d3.vec_writemask_x) condition_map[cond.mask[0]].push_back({ 0, channel_index++ });
							if (instruction.data.d3.vec_writemask_y) condition_map[cond.mask[0]].push_back({ 1, channel_index++ });
							if (instruction.data.d3.vec_writemask_z) condition_map[cond.mask[0]].push_back({ 2, channel_index++ });
							if (instruction.data.d3.vec_writemask_w) condition_map[cond.mask[0]].push_back({ 3, channel_index });
						}
						else
						{
							if (instruction.data.d3.sca_writemask_x) condition_map[cond.mask[0]].push_back({ 0, channel_index++ });
							if (instruction.data.d3.sca_writemask_y) condition_map[cond.mask[0]].push_back({ 1, channel_index++ });
							if (instruction.data.d3.sca_writemask_z) condition_map[cond.mask[0]].push_back({ 2, channel_index++ });
							if (instruction.data.d3.sca_writemask_w) condition_map[cond.mask[0]].push_back({ 3, channel_index });
						}

						auto src = apply_instruction_modifiers(arg);

						src.assign(float_point_expr<4>(arg.text, arg.mask, true, 4));

						for (auto &entry : condition_map)
						{
							std::string src_swizzle;
							std::string dst_swizzle;

							for (std::pair<int, int> channels : entry.second)
							{
								src_swizzle += src.swizzle(channels.second).mask[0];
								dst_swizzle += dest.swizzle(channels.second).mask[0];
							}

							float_point_expr<4> expression{ src.with_mask(src_swizzle) };

							if (has_dst)
							{
								expression.assign(dest.with_mask(dst_swizzle) = expression);
							}

							if (instruction.data.d0.cond_update_enable_0 && instruction.data.d0.cond_update_enable_1)
							{
								expression.assign(cond.with_mask(dst_swizzle) = expression);
							}

							result += base::if_(cond.swizzle(channel_to_index.at(entry.first)).call_operator<boolean_t<1>>(operation, zero), expression);
						}
					}
				}
				else
				{
					float_point_expr<4> expression{ apply_instruction_modifiers(arg.without_scope()) };

					if (has_dst)
					{
						expression.assign(dest = expression);
					}

					if (instruction.data.d0.cond_update_enable_0 && instruction.data.d0.cond_update_enable_1)
					{
						expression.assign(update_condition_register() = expression);
					}

					result += expression;
				}

				return result;
			}

			typename base::writer_t set_dst(float_point_expr<1> arg)
			{
				if (destination_swizzle().size() != 1)
				{
					return set_dst(float_point_t<4>::ctor(arg));
				}

				return set_dst(float_point_expr<4>{ arg.to_string() });
			}

			typename base::writer_t set_dst(boolean_expr<4> arg)
			{
				std::string arg_string;

				bool is_single = true;

				switch (destination_swizzle().size())
				{
				case 1: arg_string = arg.to_string() + " ? 1.0 : 0.0"; is_single = false; break;
				case 2: arg_string = float_point_t<2>::ctor(boolean_expr<2>{ arg.to_string() }).to_string(); break;
				case 3: arg_string = float_point_t<3>::ctor(boolean_expr<3>{ arg.to_string() }).to_string(); break;
				case 4: arg_string = float_point_t<4>::ctor(boolean_expr<4>{ arg.to_string() }).to_string(); break;

				default:
					throw;
				}

				return set_dst(float_point_expr<4>{ arg_string, std::string("xyzw"), is_single, 4 });
			}

			enum class condition_operation
			{
				all,
				any
			};

			typename base::compare_function execution_condition_function() const
			{
				switch (instruction.data.d0.cond)
				{
				case gt | eq: return base::compare_function::greater_equal;
				case lt | eq: return base::compare_function::less_equal;
				case lt | gt: return base::compare_function::not_equal;
				case gt: return base::compare_function::greater;
				case lt: return base::compare_function::less;
				case eq: return base::compare_function::equal;
				}

				throw;
			}

			boolean_expr<1> execution_condition(condition_operation operation)
			{
				if (instruction.data.d0.cond == always)
				{
					return true;
				}

				if (instruction.data.d0.cond == never)
				{
					return false;
				}

				auto cond = execution_condition_register();

				if (instruction.data.d0.mask_x == instruction.data.d0.mask_y &&
					instruction.data.d0.mask_y == instruction.data.d0.mask_z &&
					instruction.data.d0.mask_z == instruction.data.d0.mask_w)
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

			typename base::expression_base_t decode_sca_instruction()
			{
				is_vec = false;

				switch (instruction.data.d1.sca_opcode)
				{
				case sca_opcode::mov: return set_dst(src_swizzled_as_dst(2));
				case sca_opcode::rcp: return set_dst(float_point_t<1>::ctor(1.0f) / src_swizzled_as_dst(2));
				case sca_opcode::rcc: return set_dst(base::clamp(float_point_t<1>::ctor(1.0f) / src_swizzled_as_dst(2), 5.42101e-20f, 1.884467e19f));
				case sca_opcode::rsq: return set_dst(base::rsqrt(base::abs(src_swizzled_as_dst(2))));
				case sca_opcode::exp: return set_dst(base::exp(src_swizzled_as_dst(2)));
				case sca_opcode::log: return set_dst(base::log(src_swizzled_as_dst(2)));
				case sca_opcode::lit:
				{
					auto t = src(2);

					float_point_expr<1> z_value{ (t.x() > 0.0f).text };
					z_value.assign("(" + z_value.text + " ? " + base::exp2(t.w() * base::log2(t.y())).text + " : 0.0)");

					return set_dst(swizzle_as_dst(float_point_t<4>::ctor(1.0f, t.x(), z_value, 1.0f)));
				}
				case sca_opcode::bra: break;
				case sca_opcode::bri:
				{
					std::size_t from = base::writer.position;
					std::size_t to = address_value();

					boolean_expr<1> condition{ execution_condition(condition_operation::all) };

					if (to > from)
					{
						base::writer.before(from, "if (!(" + condition.to_string() + "))\n{\n");
						base::writer.before(to, "}\n");
					}
					else
					{
						base::writer.before(from, "}\nwhile (" + condition.to_string() + ");\n");
						base::writer.before(to, "do\n{\n");
					}
				}
				return{ "" };

				case sca_opcode::cal: break;
				case sca_opcode::cli: break;
				case sca_opcode::ret: return conditional(typename base::void_expr{ "return" });
				case sca_opcode::lg2: return set_dst(base::log2(src_swizzled_as_dst(2)));
				case sca_opcode::ex2: return set_dst(base::exp2(src_swizzled_as_dst(2)));
				case sca_opcode::sin: return set_dst(base::sin(src_swizzled_as_dst(2)));
				case sca_opcode::cos: return set_dst(base::cos(src_swizzled_as_dst(2)));
				case sca_opcode::brb: break;
				case sca_opcode::clb: break;
				case sca_opcode::psh: break;
				case sca_opcode::pop: break;
				default:
					throw;
				}

				return base::unimplemented("sca " + sca_op_names[(int)instruction.data.d1.sca_opcode]);
			}

			typename base::expression_base_t decode_vec_instruction()
			{
				is_vec = true;

				switch (instruction.data.d1.vec_opcode)
				{
				case vec_opcode::mov: return set_dst(src_swizzled_as_dst(0));
				case vec_opcode::mul: return set_dst(src_swizzled_as_dst(0) * src_swizzled_as_dst(1));
				case vec_opcode::add: return set_dst(src_swizzled_as_dst(0) + src_swizzled_as_dst(2));
				case vec_opcode::mad: return set_dst(src_swizzled_as_dst(0) * src_swizzled_as_dst(1).without_scope() + src_swizzled_as_dst(2));
				case vec_opcode::dp3: return set_dst(base::dot(src(0).xyz(), src(1).xyz()));
				case vec_opcode::dph: break;
				case vec_opcode::dp4: return set_dst(base::dot(src(0), src(1)));
				case vec_opcode::dst: break;
				case vec_opcode::min: return set_dst(base::min(src_swizzled_as_dst(0), src_swizzled_as_dst(1)));
				case vec_opcode::max: return set_dst(base::max(src_swizzled_as_dst(0), src_swizzled_as_dst(1)));
				case vec_opcode::slt: return set_dst(compare(base::compare_function::less, src_swizzled_as_dst(0), src_swizzled_as_dst(1)));
				case vec_opcode::sge: return set_dst(compare(base::compare_function::greater_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)));
				case vec_opcode::arl: return typename base::writer_t{} += address_register() = integer_t<1>::ctor(src(0).x());
				case vec_opcode::frc: return set_dst(base::fract(src_swizzled_as_dst(0)));
				case vec_opcode::flr: return set_dst(base::floor(src_swizzled_as_dst(0)));;
				case vec_opcode::seq: return set_dst(compare(base::compare_function::equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)));
				case vec_opcode::sfl: return set_dst(0.0f);
				case vec_opcode::sgt: return set_dst(compare(base::compare_function::greater, src_swizzled_as_dst(0), src_swizzled_as_dst(1)));;
				case vec_opcode::sle: return set_dst(compare(base::compare_function::less_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)));
				case vec_opcode::sne: return set_dst(compare(base::compare_function::not_equal, src_swizzled_as_dst(0), src_swizzled_as_dst(1)));
				case vec_opcode::str: return set_dst(1.0f);
				case vec_opcode::ssg: break;
				case vec_opcode::txl: break;

				default:
					throw;
				}

				return base::unimplemented("vec " + vec_op_names[(int)instruction.data.d1.vec_opcode]);
			}

			typename base::expression_base_t decode_instruction()
			{
				if (instruction.data.d1.sca_opcode == sca_opcode::nop && instruction.data.d1.vec_opcode == vec_opcode::nop)
				{
					return base::comment("NOP");
				}

				typename base::writer_t result;

				if (instruction.data.d1.sca_opcode != sca_opcode::nop)
				{
					result += decode_sca_instruction();
				}

				if (instruction.data.d1.vec_opcode != vec_opcode::nop)
				{
					result += decode_vec_instruction();
				}

				return result;
			}

		public:
			decompiled_shader decompile(std::size_t offset, instruction_t *instructions)
			{
				for (std::size_t i = offset; i < 512; ++i, base::writer.next())
				{
					instruction = instructions[i].unpack();

					base::writer += decode_instruction();

					if (instruction.data.d3.end)
						break;
				}

				context.program.entry_function = "func0";
				base::writer.before(0, "void func0()\n{\n");
				base::writer.after(base::writer.position, "}\n");
				context.program.code = base::writer.finalize();

				return context.program;
			}
		};

		template<typename Language>
		decompiled_shader decompile(std::size_t offset, ucode_instr *instructions)
		{
			return decompiler<Language>{}.decompile(offset, (typename decompiler<Language>::instruction_t*)instructions);
		}

		decompiled_shader decompile(std::size_t offset, ucode_instr* instructions, decompile_language lang)
		{
			decompiled_shader result;

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
