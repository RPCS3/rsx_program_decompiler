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
		template<type_class_t Type, int DstCount, int SrcCount>
		static writer_t move_if(const expression_from<float_point_t<DstCount>> &cond, const std::string& operation, expression_t<Type, DstCount> dst, const expression_t<Type, SrcCount> &src)
		{
			writer_t result;

			int dst_to_cond_swizzle[DstCount];

			for (int i = 0; i < DstCount; ++i)
			{
				dst_to_cond_swizzle[i] = -1;
			}

			for (int i = 0; i < DstCount; ++i)
			{
				int dst_mask = channel_to_index.at(dst.swizzle(i).mask[0]);
				int cond_mask = channel_to_index.at(cond.swizzle(i).mask[0]);

				if (dst_to_cond_swizzle[dst_mask] != -1)
				{
					if (dst_to_cond_swizzle[dst_mask] != cond_mask)
					{
						std::cerr << "Bad conditional move swizzle! (old = " << dst_mask << ", new = " << cond_mask << ")" << std::endl;
					}
				}
				else
				{
					dst_to_cond_swizzle[dst_mask] = cond_mask;
				}
			}

			for (int cond_index = 0; cond_index < 4; ++cond_index)
			{
				std::string dst_swizzle_text;
				std::string src_swizzle_text;

				for (int dst_index = 0; dst_index < DstCount; ++dst_index)
				{
					if (dst_to_cond_swizzle[dst_index] == cond_index)
					{
						if (dst_index >= SrcCount)
						{
							//std::cerr << "Bad conditional move destination swizzle! (dst_index = " << dst_index << ") << std::endl;
							continue;
						}

						dst_swizzle_text += dst.swizzle(dst_index).mask;
						src_swizzle_text += src.swizzle(dst_index).mask;
					}
				}

				if (dst_swizzle_text.empty())
					continue;

				if (dst_swizzle_text == "xyzw")
					dst_swizzle_text.clear();

				if (src_swizzle_text == "xyzw")
					src_swizzle_text.clear();

				expression_from<float_point_t<1>> cond_expr(cond.text, index_to_channel[cond_index], false, cond.base_count);
				expression_from<float_point_t<1>> dst_expr(dst.text, dst_swizzle_text, false, dst.base_count);
				expression_from<float_point_t<1>> src_expr(src.text, src_swizzle_text, src.is_single, src.base_count);
				static const expression_from<float_point_t<1>> condition_value{ 0.0 };

				result += if_(cond_expr.call_operator<boolean_t<1>>(operation, condition_value), dst_expr = src_expr);
			}

			return result;
		}

		template<type_class_t Type, int DstCount, int SrcCount>
		static writer_t move(expression_t<Type, DstCount> dst, const expression_t<Type, SrcCount> &src)
		{
			writer_t result;
			std::string src_swizzle_text;

			for (int i = 0; i < DstCount; ++i)
			{
				int dst_index = channel_to_index.at(dst.swizzle(i).mask[0]);
				src_swizzle_text += src.swizzle(dst_index).mask[0];
			}

			if (src_swizzle_text == "xyzw")
				src_swizzle_text.clear();

			expression_from<float_point_t<1>> dst_expr(dst.text, dst_swizzle_text, false, false);
			expression_from<float_point_t<1>> src_expr(src.text, src_swizzle_text, src.is_single, false);
			static const expression_from<float_point_t<1>> condition_value{ 0.0 };

			result += if_(cond_expr.call_operator<boolean_t<1>>(operation, condition_value), dst_expr = src_expr);
			return result;
		}
	};
}

#include "glsl_language.h"
#include "rsx_fp_ucode.h"

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

				expression_from<float_point_t<4>> temp(int index)
				{
					return variable("tmp" + std::to_string(index));
				}

				expression_from<float_point_t<4>> input(int index)
				{
					return variable(input_attrib_map[index]);
				}
			};

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
						case src_reg_type_t::temporary: return context.temp(src.tmp_index);
						case src_reg_type_t::input: return context.input(data.dst.src_attr_reg_num);
						case src_reg_type_t::constant: return context.constant();
						}
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
				}

				expression_from<float_point_t<4>> dst(context_t& context) const
				{
				}
			};

			static_assert(sizeof(instruction_t) == 16, "Bad instruction_t implementation");

		public:
			instruction_t* instruction;
			context_t context;
			writer_t writer;

			expression_from<float_point_t<4>> src(int index)
			{
				return instruction->src(context, index);
			}

			expression_from<boolean_t<4>> modify_condition()
			{
				return{ "" };
			}

			expression_from<boolean_t<4>> execution_condition()
			{
				return{ "" };
			}

			expression_from<sampler2D_t> tex()
			{
				return{ "" };
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

			//template<type_class_t Type, int Count>
			writer_t set_dst(const expression_t<type_class_t::type_float, 4>& arg)
			{
				writer_t result;

				auto modify_cond = modify_condition();
				auto dest = instruction->dst(context);

				if (!instruction->data.src0.exec_if_eq || !instruction->data.src0.exec_if_gr || !instruction->data.src0.exec_if_lt)
				{
					std::string cond_mask;
					cond_mask += index_to_channel[instruction->data.src0.cond_swizzle_x];
					cond_mask += index_to_channel[instruction->data.src0.cond_swizzle_y];
					cond_mask += index_to_channel[instruction->data.src0.cond_swizzle_z];
					cond_mask += index_to_channel[instruction->data.src0.cond_swizzle_w];

					auto cond = execution_condition();

					std::string operation;

					if (instruction->data.src0.exec_if_gr && instruction->data.src0.exec_if_lt)
					{
						operation = "!=";
					}
					else if (!instruction->data.src0.exec_if_gr && !instruction->data.src0.exec_if_lt)
					{
						operation = "==";
					}
					else
					{
						if (instruction->data.src0.exec_if_gr)
							operation += ">";
						else if (instruction->data.src0.exec_if_lt)
							operation += "<";

						if (instruction->data.src0.exec_if_eq)
							operation += "=";
					}

					auto set_channel = [&](int index)
					{
						if (instruction->data.dst.set_cond)
						{
							if (instruction->data.dst.no_dest)
							{
								result += if_(cond.swizzle(index).call_operator<boolean_t<1>>(operation, 0.0f), modify_cond.swizzle(index) = boolean_t<1>::ctor(arg.swizzle(index)));
							}
							else
							{
								result += if_(cond.swizzle(index).call_operator(operation, 0.0f), modify_cond.swizzle(index) = boolean_t<1>::ctor(dest.swizzle(index) = arg.swizzle(index)));
							}
						}
						else
						{
							if (instruction->data.dst.no_dest)
							{
								result += if_(cond.swizzle(index).call_operator(operation, 0.0f), arg.swizzle(index));
							}
							else
							{
								result += if_(cond.swizzle(index).call_operator(operation, 0.0f), dest.swizzle(index) = arg.swizzle(index));
							}
						}
					};

					if (instruction->data.dst.mask_x) set_channel(0);
					if (instruction->data.dst.mask_y) set_channel(1);
					if (instruction->data.dst.mask_z) set_channel(2);
					if (instruction->data.dst.mask_w) set_channel(3);
				}

				return result;
			}

			writer_t comment(const std::string& lines)
			{
				writer_t result;

				result += "//" + lines;

				return result;
			}

			writer_t unimplemented(const std::string& lines)
			{
				return comment(lines);
			}

			expression_base_t decode_instruction()
			{
				switch (instruction->data.dst.opcode)
				{
				case opcode::NOP: return comment("nop");
				case opcode::MOV: return set_dst(src(0));
				case opcode::MUL: return set_dst(src(0) * src(1));
				case opcode::ADD: return set_dst(src(0) + src(1));
				case opcode::MAD: return set_dst(src(0) * src(1) + src(2));
				case opcode::DP3: return set_dst(float_point_t<4>::ctor(dot(src(0).xyz(), src(1).xyz())));
				case opcode::DP4: return set_dst(float_point_t<4>::ctor(dot(src(0), src(1))));
				case opcode::DST:
				{
					auto src_0 = src(0);
					auto src_1 = src(1);

					return set_dst(float_point_t<4>::ctor(1.0f, src_0.y() * src_1.y(), src_0.z(), src_1.w()));
				}
				case opcode::MIN: return set_dst(min(src(0), src(1)));
				case opcode::MAX: return set_dst(max(src(0), src(1)));
				case opcode::SLT: return set_dst(float_point_t<4>::ctor(less(src(0), src(1))));
				case opcode::SGE: return set_dst(float_point_t<4>::ctor(greater_equal(src(0), src(1))));
				case opcode::SLE: return set_dst(float_point_t<4>::ctor(less_equal(src(0), src(1))));
				case opcode::SGT: return set_dst(float_point_t<4>::ctor(greater(src(0), src(1))));
				case opcode::SNE: return set_dst(float_point_t<4>::ctor(not_equal(src(0), src(1))));
				case opcode::SEQ: return set_dst(float_point_t<4>::ctor(equal(src(0), src(1))));
				case opcode::FRC: return set_dst(fract(src(0)));
				case opcode::FLR: return set_dst(floor(src(0)));
				case opcode::KIL: return conditional(expression_from<void_t>("discard"));
				case opcode::PK4: return unimplemented("PK4");
				case opcode::UP4: return unimplemented("UP4");
				case opcode::DDX: return set_dst(ddx(src(0)));
				case opcode::DDY: return set_dst(ddy(src(0)));
				case opcode::TEX: return set_dst(texture(tex(), src(0).xy()));
				case opcode::TXP: return set_dst(texture(tex(), src(0).xy() / src(0).w()));
				case opcode::TXD: return set_dst(texture_grad(tex(), src(0).xy(), src(1).xy(), src(2).xy()));
				case opcode::RCP: return set_dst(float_point_t<1>::ctor(1.0f) / src(0));
				case opcode::RSQ: return set_dst(rsqrt(src(0)));
				case opcode::EX2: return set_dst(exp2(src(0)));
				case opcode::LG2: return set_dst(log2(src(0)));
				case opcode::LIT: return unimplemented("LIT");
				case opcode::LRP: return unimplemented("LRP");
				case opcode::STR: return set_dst(float_point_t<4>::ctor(1.0f));
				case opcode::SFL: return set_dst(float_point_t<4>::ctor(0.0f));
				case opcode::COS: return set_dst(cos(src(0)));
				case opcode::SIN: return set_dst(sin(src(0)));
				case opcode::PK2: return unimplemented("PK2");
				case opcode::UP2: return unimplemented("UP2");
				case opcode::POW: return set_dst(pow(src(0), src(1)));
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
				case opcode::DIV: return set_dst(src(0) / src(1));
				case opcode::DIVSQ: return set_dst(src(0) / sqrt(src(1)));
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
			}

			void decompile(std::size_t offset, instruction_t* instructions)
			{
				for (std::size_t index = offset; index < 512; ++index, writer.next())
				{
					instruction = instructions + index;

					writer += decode_instruction();

					if (instruction->data.dst.end)
						break;
				}
			}

			void test()
			{
				expression_from<float_point_t<4>> C{ "C", "wwww" };
				expression_from<float_point_t<4>> A{ "A" };
				expression_from<float_point_t<4>> B{ "B" };

				expression_from<sampler2D_t> tex = { "tex0" };

				std::cout << move_if(C.xyz(), "!=", -A.xyz(), texture(tex, A.xy() / B.zw() + C.xw()).xyz()).build();
			}
		};
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
#include <iostream>

namespace vs
{
	int main()
	{
		//load_shader_cache("SUPER12321");
		//vp_shader a{};

		//for (auto& line : a.writer.code)
		//	std::cout << line.second << std::endl;

		rsx::fragment_program::decompiler{}.test();
		std::cin.get();
		return 0;
	}
}