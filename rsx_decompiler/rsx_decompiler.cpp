#include "pch.h"
#include "clike_builder.h"
#include <unordered_set>
#include <functional>
#include <memory>
#include <iostream>

namespace rsx
{
	static const std::string index_to_channel[4] = { "x", "y", "z", "w" };
	static const std::unordered_map<char, int> channel_to_index = { { 'x', 0 },{ 'y', 1 },{ 'z', 2 },{ 'w', 3 } };

	template<typename Language>
	struct decompiler_base : shader_code::clike_builder<Language>
	{
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

				expression_from<float_point_t<4>> src(context_t& context, int index) const
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

					if (src.abs)
					{
						result.assign(abs(result));
					}

					if (src.neg)
					{
						result.assign(-result);
					}

					return result.swizzle(src.swizzle_x, src.swizzle_y, src.swizzle_z, src.swizzle_w);
				}


				expression_from<float_point_t<4>> output(context_t& context) const
				{
					return{ "unk_output" };
				}

				std::string destination_swizzle() const
				{
					std::string swizzle;
					static const std::string mask = "xyzw";

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

			expression_from<float_point_t<4>> src(int index)
			{
				return instruction.src(context, index);
			}

			expression_from<float_point_t<4>> swizzle_as_dst(expression_from<float_point_t<4>> arg) const
			{
				std::string arg_mask;

				for (char channel : instruction.destination_swizzle())
				{
					arg_mask += arg.mask[channel_to_index.at(channel)];
				}

				return expression_from<float_point_t<4>>(arg.text, arg_mask, arg.is_single);
			}

			expression_from<float_point_t<4>> src_swizzled_as_dst(int index)
			{
				if (instruction.data.dst.set_cond || !instruction.data.dst.no_dest)
				{
					return swizzle_as_dst(src(index));
				}

				return src(index);
			}

			expression_from<boolean_t<4>> modify_condition()
			{
				return{ "unk_modify_condition" };
			}

			expression_from<boolean_t<4>> execution_condition()
			{
				return{ "unk_execution_condition" };
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
					return if_(any(execution_condition()), expr);
				}

				return expr;
			}

			enum set_dst_flags
			{
				none,
				disable_swizzle_as_dst = 1,
			};

			template<typename Type>
			Type apply_dst_flags(Type arg)
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

				switch (instruction.data.dst.prec)
				{
				case 0: //fp32, do nothing
					break;

				case 1: //fp16
					arg.assign(clamp(arg, -65536.0f, 65536.0f));
					break;

				case 2: //fixed point 12
					arg.assign(clamp(arg, -1.0f, 1.0f));
					break;

				default:
					throw std::runtime_error("fragment program decompiler: unimplemented precision.");
				}

				if (instruction.data.dst.saturate)
				{
					arg.assign(clamp(arg, 0.0f, 1.0f));
				}

				return arg;
			}

			writer_t set_dst(const expression_from<float_point_t<4>>& arg, set_dst_flags flags = none)
			{
				writer_t result;

				auto modify_cond = modify_condition();
				auto dest = instruction.destination(context);

				if (!instruction.data.src0.exec_if_eq || !instruction.data.src0.exec_if_gr || !instruction.data.src0.exec_if_lt)
				{
					std::string cond_mask;
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_x];
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_y];
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_z];
					cond_mask += index_to_channel[instruction.data.src0.cond_swizzle_w];

					auto cond = execution_condition();

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

					static const expression_from<float_point_t<1>> zero(0.0f);

					auto set_channel = [&](int index)
					{
						auto src = apply_dst_flags(arg.swizzle(index));

						if (instruction.data.dst.set_cond)
						{
							if (instruction.data.dst.no_dest)
							{
								result += if_(cond.swizzle(index).call_operator<boolean_t<1>>(operation, zero),
									modify_cond.swizzle(index) = boolean_t<1>::ctor(src));
							}
							else
							{
								result += if_(cond.swizzle(index).call_operator(operation, zero),
									modify_cond.swizzle(index) = boolean_t<1>::ctor(dest.swizzle(index) = src));
							}
						}
						else
						{
							result += if_(cond.swizzle(index).call_operator(operation, zero), dest.swizzle(index) = src);
						}
					};

					if (!instruction.data.dst.set_cond && instruction.data.dst.no_dest)
					{
						//condition must be already handled in instruction semantic (IFE, LOOP, etc)
						result += comment("extra condition test skipped");
						result += arg;
					}
					else
					{
						if (instruction.data.dst.mask_x) set_channel(0);
						if (instruction.data.dst.mask_y) set_channel(1);
						if (instruction.data.dst.mask_z) set_channel(2);
						if (instruction.data.dst.mask_w) set_channel(3);
					}
				}
				else
				{
					expression_from<float_point_t<4>> src = arg;

					if (instruction.data.dst.set_cond || !instruction.data.dst.no_dest)
					{
						if ((flags & disable_swizzle_as_dst) == 0)
						{
							src.assign(swizzle_as_dst(arg));
						}

						src.assign(apply_dst_flags(src).without_scope());

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

			writer_t comment(const std::string& lines)
			{
				writer_t result;

				result += "//" + lines + "\n";

				return result;
			}

			writer_t unimplemented(const std::string& lines)
			{
				return comment(lines);
			}

			expression_base_t decode_instruction()
			{
				switch (instruction.data.dst.opcode)
				{
				case opcode::NOP: return comment("nop");
				case opcode::MOV: return set_dst(src(0));
				case opcode::MUL: return set_dst(src_swizzled_as_dst(0) * src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case opcode::ADD: return set_dst(src_swizzled_as_dst(0) + src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case opcode::MAD: return set_dst((src_swizzled_as_dst(0) * src_swizzled_as_dst(1)).without_scope() + src_swizzled_as_dst(2), disable_swizzle_as_dst);
				case opcode::DP3: return set_dst(float_point_t<4>::ctor(dot(src(0).xyz(), src(1).xyz())));
				case opcode::DP4: return set_dst(float_point_t<4>::ctor(dot(src(0), src(1))));
				case opcode::DST:
				{
					auto src_0 = src(0);
					auto src_1 = src(1);

					return set_dst(float_point_t<4>::ctor(1.0f, src_0.y() * src_1.y(), src_0.z(), src_1.w()));
				}
				case opcode::MIN: return set_dst(min(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::MAX: return set_dst(max(src_swizzled_as_dst(0), src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::SLT: return set_dst(float_point_t<4>::ctor(less(src(0), src(1))));
				case opcode::SGE: return set_dst(float_point_t<4>::ctor(greater_equal(src(0), src(1))));
				case opcode::SLE: return set_dst(float_point_t<4>::ctor(less_equal(src(0), src(1))));
				case opcode::SGT: return set_dst(float_point_t<4>::ctor(greater(src(0), src(1))));
				case opcode::SNE: return set_dst(float_point_t<4>::ctor(not_equal(src(0), src(1))));
				case opcode::SEQ: return set_dst(float_point_t<4>::ctor(equal(src(0), src(1))));
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
				case opcode::STR: return set_dst(float_point_t<4>::ctor(1.0f));
				case opcode::SFL: return set_dst(float_point_t<4>::ctor(0.0f));
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
				case opcode::DP2: return set_dst(float_point_t<4>::ctor(dot(src(0).xy(), src(1).xy())));
				case opcode::NRM: return set_dst(normalize(src(0).xyz()).xyzx());
				case opcode::DIV: return set_dst(src_swizzled_as_dst(0) / src_swizzled_as_dst(1), disable_swizzle_as_dst);
				case opcode::DIVSQ: return set_dst(src_swizzled_as_dst(0) / sqrt(src_swizzled_as_dst(1)), disable_swizzle_as_dst);
				case opcode::LIF: return unimplemented("LIF");
				case opcode::FENCT: return comment("fenct");
				case opcode::FENCB: return comment("fencb");
				case opcode::BRK: return conditional(expression_from<void_t>("break"));
				case opcode::CAL: return unimplemented("CAL");
				case opcode::IFE: return unimplemented("IFE");
				case opcode::LOOP: return unimplemented("LOOP");
				case opcode::REP: return unimplemented("REP");
				case opcode::RET: return conditional(expression_from<void_t>("return"));
				}

				throw;
			}

			void decompile(std::size_t offset, instruction_t* instructions)
			{
				context.offset = 0;
				context.is_next_is_constant = false;

				for (std::size_t index = offset; index < 512; ++index, writer.next(), context.offset += sizeof(instruction_t))
				{
					if (context.is_next_is_constant)
					{
						context.is_next_is_constant = false;
						continue;
					}

					instruction = (instructions + index)->unpack();

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

/*
struct vp_shader : shader_code::clike_builder<shader_code::glsl_language>
{
	writer_t writer;

	struct variable_info
	{
		std::string type;
		std::string name;

		bool operator ==(const variable_info& rhs) const
		{
			return type == rhs.type && name == rhs.name;
		}

		struct hash
		{
			std::size_t operator()(const variable_info& arg) const
			{
				return
					std::hash<std::string>()(arg.type) ^
					(std::hash<std::string>()(arg.name) - 1);
			}
		};
	};

	std::unordered_set<variable_info, variable_info::hash> uniforms;

	template<typename Type>
	expression_from<Type> make_uniform(const std::string& name, expression_from<Type> initializer)
	{
		return{ name };
	}

	template<typename Type>
	expression_from<Type> make_uniform(const std::string& name)
	{
		return{ name };
	}

	struct else_t : expression_base_t
	{

	};

	struct if_t : expression_from<void_t>
	{
		if_t(expression_from<boolean_t<1>> condition, std::function<void()> body = nullptr)
			: expression_t("if (" + condition.to_string() + ")")
		{
		}

		std::string finalize() const override
		{
			return to_string();
		}
	};

	vp_shader()
	{
		auto A = make_uniform<boolean_t<4>>("A", boolean_t<4>::ctor(true));
		auto B = make_uniform<float_point_t<4>>("B", float_point_t<4>::ctor(0.f));
		auto C = make_uniform<float_point_t<4>>("C");
		auto K = make_uniform<float_point_t<2>>("K");
		auto D = make_uniform<sampler2D_t>("D");

		writer.lines(
			texture(D).xyzw(),
			float_point_t<4>::ctor(1),
			float_point_t<1>::ctor(1.f) / float_point_t<1>::ctor(),
			if_t(A.x()),
			A.x() = !(K.xy().y() != C.x()),
			K.xy(),
			A.y() = B.x() != C.x()
		);
	}
};

#include <unordered_map>

struct rsx_fragment_shader
{
	std::vector<unsigned int> data;

	struct hash_t
	{
		std::size_t operator ()(const rsx_fragment_shader& arg) const
		{
			return 0;
			//return arg.hash;
		}
	};

	bool operator ==(const rsx_fragment_shader& rhs) const
	{
		if (data.size() != rhs.data.size())
			return false;

		for (std::size_t i = 0; i < data.size(); ++i)
		{
			if (data[i] != rhs.data[i])
				return false;
		}

		return true;
	}
};

struct decompiled_rsx_fragment_shader
{
	std::vector<std::size_t> constant_offsets;
	std::vector<std::string> uniforms;
	int input_attributes;
	int output_attributes;

	std::string code;
};

struct glsl_shader_t
{
	int id;
};

template<typename CompiledType>
struct finalized_rsx_fragment_shader
{
	int input_attributes;
	int output_attributes;
	int control;

	std::string code;
	CompiledType shader;
};

template<typename Type>
struct rsx_program
{
	std::shared_ptr<finalized_rsx_fragment_shader<glsl_shader_t>> fragment_shader;
	std::shared_ptr<finalized_rsx_fragment_shader<glsl_shader_t>> vertex_shader;
};

/*

namespace fmt
{
	bool mask_test(const std::string &source, const std::string &mask, std::size_t source_offset = 0, std::size_t mask_offset = 0)
	{
		while (char sym = mask[mask_offset])
		{
			if (source[source_offset] == '\0')
				return false;

			switch (sym)
			{
			case '*':
				while (source[source_offset])
				{
					if (mask_test(source, mask, source_offset, mask_offset + 1))
						return true;

					++source_offset;
				}

				return false;

			case '\\':
				sym = mask[++mask_offset];

			default:
				if (sym != source[source_offset])
					return false;

			case '?':
				++source_offset; ++mask_offset;
				break;
			}
		}

		return source[source_offset] == '\0';
	}
}

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::tr2::sys;

void _load_fragment_cache_file(const fs::path& data_path, const fs::path &source_path)
{
	rsx_fragment_shader shader;
	{
		std::ifstream data_f(data_path, std::ios::ate | std::ios::binary);
		if (!data_f)
			return;

		shader.data.resize(data_f.tellg() / sizeof(rsx_fragment_shader::data[0]));
		data_f.seekg(0, data_f.beg);

		data_f.read((char*)shader.data.data(), shader.data.size() * sizeof(rsx_fragment_shader::data[0]));
	}

	cache_t<rsx_fragment_shader, decompiled_rsx_fragment_shader> cache;

	decompiled_rsx_fragment_shader &decompiled_shader = cache.entry(shader);

	{
		std::ifstream data_f(data_path, std::ios::ate);
		if (!data_f)
			return;

		data_f
			>> decompiled_shader.input_attributes
			>> decompiled_shader.output_attributes;


		int constants_count;
		data_f >> constants_count;

		if (constants_count)
		{
			decompiled_shader.constant_offsets.resize(constants_count);

			for (auto &offset : decompiled_shader.constant_offsets)
			{
				data_f >> offset;
			}
		}

		int uniforms_count;
		data_f >> uniforms_count;

		if (uniforms_count)
		{
			decompiled_shader.uniforms.resize(uniforms_count);

			for (auto &uniform : decompiled_shader.uniforms)
			{
				data_f >> uniform;
			}
		}

		auto from_pos = data_f.tellg();
		data_f.seekg(0, data_f.end);
		auto end_pos = data_f.tellg();

		decompiled_shader.code.resize(end_pos - from_pos);
		data_f.seekg(from_pos);
		data_f.read((char*)decompiled_shader.code.data(), decompiled_shader.code.size());
	}
}

void _save_fragment_cache_file(const fs::path& data_path, const fs::path &source_path, std::pair<const rsx_fragment_shader&, decompiled_rsx_fragment_shader&> data)
{
	rsx_fragment_shader shader;
	{
		std::ifstream data_f(data_path, std::ios::ate | std::ios::binary);
		if (!data_f)
			return;

		shader.data.resize(data_f.tellg() / sizeof(rsx_fragment_shader::data[0]));
		data_f.seekg(0, data_f.beg);

		data_f.read((char*)shader.data.data(), shader.data.size() * sizeof(rsx_fragment_shader::data[0]));
	}

	cache_t<rsx_fragment_shader, decompiled_rsx_fragment_shader> cache;

	decompiled_rsx_fragment_shader &decompiled_shader = cache.entry(shader);

	{
		std::ifstream data_f(data_path, std::ios::ate);
		if (!data_f)
			return;

		data_f
			>> decompiled_shader.input_attributes
			>> decompiled_shader.output_attributes;


		int constants_count;
		data_f >> constants_count;

		if (constants_count)
		{
			decompiled_shader.constant_offsets.resize(constants_count);

			for (auto &offset : decompiled_shader.constant_offsets)
			{
				data_f >> offset;
			}
		}

		int uniforms_count;
		data_f >> uniforms_count;

		if (uniforms_count)
		{
			decompiled_shader.uniforms.resize(uniforms_count);

			for (auto &uniform : decompiled_shader.uniforms)
			{
				data_f >> uniform;
			}
		}

		auto from_pos = data_f.tellg();
		data_f.seekg(0, data_f.end);
		auto end_pos = data_f.tellg();

		decompiled_shader.code.resize(end_pos - from_pos);
		data_f.seekg(from_pos);
		data_f.read((char*)decompiled_shader.code.data(), decompiled_shader.code.size());
	}
}


void _load_shader_cache(const std::string &cache_path)
{
	if (!fs::exists(cache_path))
	{
		if (!fs::create_directories(cache_path))
			return;
	}

	for (auto entry : fs::directory_iterator{ fs::path(cache_path).parent_path() })
	{
		if (entry.status().type() != fs::file_type::regular)
		{
			continue;
		}

		if (fmt::mask_test(entry.path().filename().string(), "*.vp.data"))
		{
			fs::path(entry.path()).replace_extension("glsl");
		}

		if (fmt::mask_test(entry.path().filename().string(), "*.fp.data"))
		{
			_load_fragment_cache_file(entry.path(), fs::path(entry.path()).replace_extension("glsl").string());
		}
	}
}

void load_shader_cache(const std::string &game_id)
{
	_load_shader_cache("./data/cache/rsx/");

	if (!game_id.empty())
		_load_shader_cache("./data/" + game_id + "/cache/rsx/");

	cache_t<rsx_fragment_shader, decompiled_rsx_fragment_shader> cache;
	cache.each([](std::pair<const rsx_fragment_shader&, decompiled_rsx_fragment_shader&> elem)
	{
		elem.first.data;
	});
}
*/
