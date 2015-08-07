#pragma once
#include "rsx_program_decompiler.h"
#include "rsx_fragment_program_decompiler.h"
#include <cassert>

namespace rsx
{
	namespace fragment_program
	{
		struct glsl_decompiler_impl
		{
			using decompiler = fragment_program::decompiler < glsl_decompiler_impl >;

			__forceinline static std::string get_header(decompiler* dec)
			{
				std::string result = "#version 420\n\n";
				for (auto &var : dec->info.vars)
				{
					switch (var.second.type)
					{
					case program_variable_type::input: result += "in "; break;
					case program_variable_type::output: result += "layout(location = " + std::to_string(var.second.index) + ") out "; break;
					case program_variable_type::constant: result += "uniform "; break;
					case program_variable_type::texture: result += "layout(binding = " + std::to_string(var.second.index) + ") uniform "; break;
					}

					if (var.second.storage_type.empty())
					{
						if (var.second.size == 1)
						{
							var.second.storage_type = "float";
						}
						else
						{
							var.second.storage_type = "vec" + std::to_string(var.second.size);
						}
					}

					result += var.second.storage_type + " " + var.second.name +
						(var.second.array_size ? ("[" + std::to_string(var.second.array_size + 1) + "]") :
							(var.second.index != ~0 ? std::to_string(var.second.index) : std::string{})) + ";\n";
				}

				for (auto &func : dec->functions_set)
				{
					result += "void " + func + "();\n";
				}

				return result;
			}

			__forceinline static program_variable texture_variable(program_variable arg)
			{
				arg.storage_type = "sampler2D";
				return arg;
			}

			__forceinline static std::string variable_to_string(const program_variable& arg)
			{
				fmt::string result;

				if (arg.constant.type == program_constant_type::none)
				{
					result = arg;
				}
				else
				{
					assert(arg.constant.type == program_constant_type::f32);

					std::string mask = arg.mask.to_string();
					std::unordered_map<char, float> constant_map =
					{
						{ 'x', arg.constant.x.f32_value },
						{ 'y', arg.constant.y.f32_value },
						{ 'z', arg.constant.z.f32_value },
						{ 'w', arg.constant.w.f32_value },
					};

					if (mask.empty())
					{
						result = fmt::format("vec4(%g, %g, %g, %g)",
							arg.constant.x.f32_value,
							arg.constant.y.f32_value,
							arg.constant.z.f32_value,
							arg.constant.w.f32_value);
					}
					else if (mask.size() == 1)
					{
						result = fmt::format("%g", constant_map[mask[0]]);
					}
					else
					{
						result = fmt::format("vec%d(", mask.size());

						for (size_t i = 0; i < mask.size(); ++i)
						{
							if (i)
								result += ", ";
							result += fmt::format("%g", constant_map[mask[i]]);
						}

						result += ")";
					}
				}

				if (arg.is_abs)
					result = "abs(" + result + ")";

				if (arg.is_neg)
					result = "-" + result;

				return result;
			}

			__forceinline static std::string function_begin(const std::string& name)
			{
				return "\nvoid " + name + "()\n{";
			}

			__forceinline static std::string function_end()
			{
				return "}";
			}

			__forceinline static std::string execution_condition(decompiler* dec)
			{
				if (dec->ucode.src0.exec_if_gr &&
					dec->ucode.src0.exec_if_lt &&
					dec->ucode.src0.exec_if_eq)
				{
					return{};
				}

				if (!dec->ucode.src0.exec_if_gr &&
					!dec->ucode.src0.exec_if_lt &&
					!dec->ucode.src0.exec_if_eq)
				{
					return "if (false) ";
				}

				program_variable execution_condition = dec->execution_condition_register(true);

				if (execution_condition.mask.to_string().size() == 1)
				{
					std::string execution_condition_operation;

					if (dec->ucode.src0.exec_if_gr && dec->ucode.src0.exec_if_eq)
						execution_condition_operation = ">=";
					else if (dec->ucode.src0.exec_if_lt && dec->ucode.src0.exec_if_eq)
						execution_condition_operation = "<=";
					else if (dec->ucode.src0.exec_if_gr && dec->ucode.src0.exec_if_lt)
						execution_condition_operation = "!=";
					else if (dec->ucode.src0.exec_if_gr)
						execution_condition_operation = ">";
					else if (dec->ucode.src0.exec_if_lt)
						execution_condition_operation = "<";
					else //if(dec->ucode.src0.exec_if_eq)
						execution_condition_operation = "==";

					return "if (" + execution_condition.to_string() + " " + execution_condition_operation + " 0.0f) ";
				}

				std::string execution_condition_function;

				if (dec->ucode.src0.exec_if_gr && dec->ucode.src0.exec_if_eq)
					execution_condition_function = "greaterThanEquals";
				else if (dec->ucode.src0.exec_if_lt && dec->ucode.src0.exec_if_eq)
					execution_condition_function = "lessThanEquals";
				else if (dec->ucode.src0.exec_if_gr && dec->ucode.src0.exec_if_lt)
					execution_condition_function = "notEquals";
				else if (dec->ucode.src0.exec_if_gr)
					execution_condition_function = "greaterThan";
				else if (dec->ucode.src0.exec_if_lt)
					execution_condition_function = "lessThan";
				else //if(dec->ucode.src0.exec_if_eq)
					execution_condition_function = "equals";

				return "if (any(" + execution_condition_function + "(" + execution_condition.to_string() + ", " + " vec4(0.0f)))) ";
			}

			template<opcode id, u32 flags, int count>
			__forceinline static std::string set_dst(decompiler* dec, const program_variable& arg0, const program_variable& arg1, const program_variable& arg2)
			{
				opcode _id = id;

				static const char operators[] =
				{
					'?', '?', '*', '+', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?',
					'?', '?', '/', '?', '?', '?', '?',
					'?', '?', '?', '?', '?', '?', '?'
				};

				static const std::string functions[] =
				{
					"?", "?", "?", "?", "fma", "dot", "dot",
					"distantion", "min", "max", "lessThan", "greaterThanEquals", "lessThanEqual", "greaterThan",
					"notEqual", "equal", "fract", "floor", "?", "pk4", "up4",
					"dFdx", "dFdy", "texture", "txp", "txd", "rcp", "rsq",
					"exp2", "log2", "lit", "lrp", "str", "sfl", "cos",
					"sin", "pk2", "up2", "pow", "pkb", "upb", "pk16",
					"up16", "bem", "pkg", "upg", "dpa2", "txl", "?",
					"txb", "?", "texbem", "txpbem", "bemlum", "refl", "timeswtex",
					"dot", "normalize", "?", "divsq", "lif", "fenct", "fencb",
					"?", "break", "cal", "ife", "loop", "rep", "return"
				};

				fmt::string value;

				switch (id)
				{
				case opcode::MOV:
				case opcode::MUL:
				case opcode::DIV:
					//operators

					if (!arg0.is_null())
					{
						value = variable_to_string(arg0);

						if (!arg1.is_null())
						{
							value += " " + std::string(1, operators[(std::size_t)id]) + " " + variable_to_string(arg1);
						}
					}
					break;

				case opcode::ADD:
				{
					value = variable_to_string(arg0);
					std::string arg1_string = variable_to_string(arg1);
					if (arg1_string[0] == '-')
					{
						value += " - " + arg1_string.substr(1);
					}
					else
					{
						value += " + " + arg1_string;
					}
				}
					break;

				case opcode::DIVSQ:
					value = variable_to_string(arg0) + " / sqrt(" + variable_to_string(arg1) + ")";
					break;

				case opcode::LIF:
				{
					program_variable arg0_y = arg0; arg0_y.mask.add("y");
					program_variable arg0_w = arg0; arg0_w.mask.add("w");
					std::string arg0_y_string = variable_to_string(arg0_y);
					std::string arg0_w_string = variable_to_string(arg0_w);

					//vec4(1.0f, $0.y, ($0.y > 0 ? pow(2.0f, $0.w) : 0.0f), 1.0f)
					value += "vec4(1.0f, " + arg0_y_string + ", (" + arg0_y_string + " > 0.0f ? pow(2.0f, " + arg0_w_string + ") : 0.0f), 1.0f)";

					//value += variable_to_string(arg0) + " / sqrt(" + variable_to_string(arg1) + ")";
				}
				break;

				default:
					//functions

					value += functions[(std::size_t)id] + "(";

					if (!arg0.is_null())
					{
						value += variable_to_string(arg0);

						if (!arg1.is_null())
						{
							value += ", " + variable_to_string(arg1);

							if (!arg2.is_null())
							{
								value += ", " + variable_to_string(arg2);
							}
						}
					}

					value += ")";
					break;
				}

				program_variable dst = dec->dst<flags, count>();

				if (id == opcode::SIN || id == opcode::COS)
				{
					std::string dst_mask = dst.mask.to_string();

					if (dst_mask.size() != 1)
					{
						size_t size = 4;

						if (!dst_mask.empty())
							size = dst_mask.size();

						value = "vec" + std::to_string(size) + "(" + value + ")";
					}
				}

				if (flags & H)
				{
					switch (dec->ucode.dst.prec)
					{
					case 0: //fp32, do nothing
						break;

					case 1: //fp16, clamping
						value = "clamp(" + value + ", -65536, 65536)";
						break;

					case 2: //fixed point 12?
						value = "clamp(" + value + ", -1, 1)";
						//throw std::runtime_error("fragment program decompiler: unimplemented precision.");
						break;
					}

					switch (dec->ucode.src1.scale)
					{
					case 0: break;
					case 1: value = mask_t::append_brackets_if_needed(value) + " * 2.0"; break;
					case 2: value = mask_t::append_brackets_if_needed(value) + " * 4.0"; break;
					case 3: value = mask_t::append_brackets_if_needed(value) + " * 8.0"; break;
					case 5: value = mask_t::append_brackets_if_needed(value) + " / 2.0"; break;
					case 6: value = mask_t::append_brackets_if_needed(value) + " / 4.0"; break;
					case 7: value = mask_t::append_brackets_if_needed(value) + " / 8.0"; break;

					default:
						throw std::runtime_error("fragment program decompiler: unimplemented scale (" + std::to_string(dec->ucode.src1.scale) + "). ");
					}

					if (dec->ucode.dst.saturate)
					{
						value = "clamp(" + value + ", 0, 1)";
					}
				}

				program_variable update_condition;
				std::string result;

				bool do_update_condition = false;
				if ((flags & C) && dec->ucode.dst.set_cond)
				{
					update_condition = dec->update_condition_register();
					do_update_condition = !update_condition.is_null();
				}

				if (dec->ucode.src0.exec_if_eq && dec->ucode.src0.exec_if_gr && dec->ucode.src0.exec_if_lt)
				{
					std::string dst_string;

					if (dst)
					{
						dst_string = dst.to_string();
						result += dst_string + " = " + value + ";\n";
					}

					if (do_update_condition)
					{
						result += update_condition.to_string() + " = " + (dst ? dst_string : value) + ";\n";
					}
				}
				else
				{
					program_variable execution_condition = dec->execution_condition_register();
					mask_t update_mask;
					update_mask
						.add(execution_condition.mask.to_string())
						.add(dst.mask.to_string());

					update_mask = mask_t{}.add(update_mask.to_string().substr(0, count));

					fmt::string execution_condition_string = execution_condition;
					std::string execution_condition_operation;

					if (dec->ucode.src0.exec_if_gr && dec->ucode.src0.exec_if_eq)
						execution_condition_operation = ">=";
					else if (dec->ucode.src0.exec_if_lt && dec->ucode.src0.exec_if_eq)
						execution_condition_operation = "<=";
					else if (dec->ucode.src0.exec_if_gr && dec->ucode.src0.exec_if_lt)
						execution_condition_operation = "!=";
					else if (dec->ucode.src0.exec_if_gr)
						execution_condition_operation = ">";
					else if (dec->ucode.src0.exec_if_lt)
						execution_condition_operation = "<";
					else //if(dec->ucode.src0.exec_if_eq)
						execution_condition_operation = "==";

					std::string update_mask_string = update_mask.to_string();
					if (update_mask_string.empty())
						update_mask_string = "xyzw";

					std::string dst_mask = dst.mask.to_string();
					if (dst_mask.empty())
						dst_mask = "xyzw";

					std::string last_condition_group;
					std::string last_line;

					for (char _mask : std::string("xyzw").substr(0, update_mask_string.size()))
					{
						const mask_t mask = mask_t{}.add(std::string(1, _mask));

						auto channel_execution_condition = execution_condition;
						std::string channel_execution_condition_mask = channel_execution_condition.mask.add(mask.to_string()).to_string();

						if (channel_execution_condition_mask != last_condition_group)
						{
							if (!last_condition_group.empty())
							{
								result += "}\n";
							}

							result += "if (" + channel_execution_condition.to_string() + " " + execution_condition_operation + " 0.0f)\n{\n";
							last_condition_group = channel_execution_condition_mask;
							last_line.clear();
						}

						std::string channel_dst_string;

						if (dst)
						{
							auto channel_dst = dst;
							channel_dst.mask = mask_t{}.add(dst_mask).add(mask.to_string());
							channel_dst_string = channel_dst.to_string();
							std::string line = "\t" + channel_dst_string + " = " + mask.apply_to(value) + ";\n";

							if (last_line == line)
							{
								continue;
							}

							result += line;
							last_line = line;
						}

						if (do_update_condition)
						{
							std::string cond_value = mask.apply_to(dst ? channel_dst_string : value);

							result += "\t" + mask.apply_to(update_condition.to_string()) + " = " + cond_value + ";\n";
						}
					}

					if (!last_condition_group.empty())
						result += "}\n";
				}
				return result;
			}

			template<opcode id, u32 flags>
			static void nodest_instruction(decompiler* dec, const program_variable& arg0, const program_variable& arg1, const program_variable& arg2)
			{
				switch (id)
				{
				case opcode::LOOP:
					dec->set_code_line(fmt::format("for (int i = %d; i < %d; i += %d)\n{",
						dec->ucode.src1.init_counter, dec->ucode.src1.end_counter, dec->ucode.src1.increment), 0, 1);

					dec->builder.add_code_block(dec->ucode.src2.end_offset >> 2, "}", -1, 0);
					break;

				case opcode::IFE:
					dec->set_code_line(execution_condition(dec) + "\n{", 0, 1);

					if (dec->ucode.src1.else_offset != dec->ucode.src2.end_offset)
						dec->builder.add_code_block(dec->ucode.src1.else_offset >> 2, "}\nelse\n{", -1, 1);

					dec->builder.add_code_block(dec->ucode.src2.end_offset >> 2, "}", -1, 0);
					break;

				case opcode::BRK:
				{
					dec->set_code_line(execution_condition(dec) + "break;");
				}
				break;

				default:
					//break;
					throw std::runtime_error("unimplemented nodest instruction: " + instructions_names[(std::size_t)id]);
				}
			}

			static std::string finalyze(decompiler *dec)
			{
				std::string result = "\nvoid main()\n{\n";
				result += "\tlabel0();\n";

				struct destination_info
				{
					std::string name;
					bool is_need_declare;
				};

				const std::size_t color_source_registers[] =
				{
					{ 0 },
					{ 2 },
					{ 3 },
					{ 4 }
				};
				program_variable color_variable{};
				color_variable.type = program_variable_type::output;
				color_variable.size = 4;
				color_variable.name = "ocolor";

				for (u32 i = 0; i < (u32)std::size(color_source_registers); ++i)
				{
					std::size_t color_register_index = dec->ctrl & 0x40 ? color_source_registers[i] : color_source_registers[i] * 2;
					std::string color_register = (dec->ctrl & 0x40 ? "R" : "H") + std::to_string(color_register_index);
					if (dec->info.vars.exists(color_register))
					{
						color_variable.index = i;
						result += "\t" + dec->info.vars.add(color_variable).to_string() + " = " + color_register + ";\n";
					}
				}

				if (dec->ctrl & 0xe)
				{
					if (dec->ctrl & 0x40)
					{
						if (dec->info.vars.exists("R1"))
						{
							result += "\tgl_FragDepth = R1.z;\n";
						}
					}
					else
					{
						if (dec->info.vars.exists("H2"))
						{
							result += "\tgl_FragDepth = H2.z;\n";
						}
					}
				}

				return result + "}";
			}
		};

		using glsl_decompiler = decompiler < glsl_decompiler_impl >;
	}
}