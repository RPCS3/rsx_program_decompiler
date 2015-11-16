/*
#include <rsx/rsx.h>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <functional>
#include "CgBinaryProgram.h"
#include "elf64.h"
#include <cctype>


template<typename DecompilerType>
int process_ucode(const std::string& ipath, const std::string& opath)
{
	if (auto &ifile_stream = std::ifstream{ ipath, std::ios::binary | std::ios::ate })
	{
		if (auto &ofile_stream = std::ofstream{ opath })
		{
			std::vector<char> buffer(ifile_stream.tellg());
			ifile_stream.seekg(0);

			ifile_stream.read(buffer.data(), buffer.size());
			auto info = DecompilerType{ buffer.data(), (u32)buffer.size() }.decompile();
			ofile_stream << info.text;

			return 0;
		}

		return -4;
	}

	return -3;
}

int extract_ucode(const std::string& ipath, const std::string& opath)
{
	if (auto &ifile_stream = std::ifstream{ ipath, std::ios::binary | std::ios::ate })
	{
		std::size_t input_size = ifile_stream.tellg();
		ifile_stream.seekg(0, ifile_stream.beg);

		if (auto &ofile_stream = std::ofstream{ opath, std::ios::binary })
		{
			std::vector<u8> buffer(input_size);
			ifile_stream.read((char*)buffer.data(), buffer.size());
			cg::CgBinaryProgram* program = (cg::CgBinaryProgram*)buffer.data();
			ofile_stream.write((char*)(buffer.data() + program->ucode), program->ucodeSize);

			return 0;
		}

		return -4;
	}

	return -3;
}

int extract_objects_from_elf(const std::string& elf_path, const std::string& output_path, std::vector<std::string> objects_names)
{
	if (std::ifstream& ifile = std::ifstream(elf_path, std::ios::binary))
	{
		using namespace endianness;

		elf64::ehdr ehdr;
		ifile.read((char*)&ehdr, sizeof(elf64::ehdr));

		endian data_endian = ehdr.e_ident.data_encoding == elf::elf_encoding::big_endian ? endian::big : endian::little;

		u64 symoffset = 0;
		u64 symcount = 0;
		u64 symstroffset = 0;
		u64 symstrsize = 0;

		std::vector<elf64::shdr> sections(unpack(ehdr.e_shnum, data_endian));

		{
			ifile.seekg(unpack(ehdr.e_shoff, data_endian));
			ifile.read((char*)sections.data(), sections.size() * sizeof(elf64::shdr));

			for (auto &section : sections)
			{
				if (unpack(section.sh_type, data_endian) == elf64::sh_type::symtab)
				{
					symoffset = unpack(section.sh_offset, data_endian);
					symcount = unpack(section.sh_size, data_endian) / sizeof(elf64::sym);
					auto &symstr = sections[unpack(section.sh_link, data_endian)];
					symstroffset = unpack(symstr.sh_offset, data_endian);
					symstrsize = unpack(symstr.sh_size, data_endian);
					break;
				}
			}
		}

		if (symoffset == 0 || symcount == 0)
			return -3;

		std::vector<char> syms_names(symstrsize);
		ifile.seekg(symstroffset);
		ifile.read(syms_names.data(), symstrsize);

		std::vector<elf64::sym> syms(symcount);
		ifile.seekg(symoffset);
		ifile.read((char*)syms.data(), symcount * sizeof(elf64::sym));

		if (std::ofstream& ofile = std::ofstream(output_path, std::ios::binary))
		{
			auto find_symbol = [&](const std::string& obj_name)
			{
				for (auto &sym : syms)
				{
					std::string name = syms_names.data() + unpack(sym.st_name, data_endian);

					if (obj_name == name)
					{
						u16 shndx_ = unpack(sym.st_shndx, data_endian);

						if (shndx_ == (u16)elf64::shndx::abs)
						{
							return unpack(sym.st_value, data_endian);
						}
						else
						{
							return
								unpack(sections[shndx_].sh_offset, data_endian) - unpack(sections[shndx_].sh_addr, data_endian)
								+ unpack(sym.st_value, data_endian);
						}

						break;
					}
				}

				throw std::runtime_error("symbol '" + obj_name + "' not found.");
			};

			std::vector<u64> symbols;
			for (auto str : objects_names)
				symbols.push_back(find_symbol(str));

			assert(symbols.size() == 2);
			std::vector<char> buffer(symbols[1] - symbols[0]);
			ifile.seekg(symbols[0]);
			ifile.read(buffer.data(), buffer.size());
			ofile.write(buffer.data(), buffer.size());
		}
	}

	return 0;
}

const std::unordered_map<std::string, std::function<int(const std::string&, const std::string&)>> g_profiles =
{
	{ "fp_glsl", process_ucode<rsx::fragment_program::glsl_decompiler> },
	//{ "vp_glsl", process_ucode<rsx::vertex_program::glsl_decompiler> },
	{ "cgbin_extract_ucode", extract_ucode },
	{ "elf_extract_fp_cgbin", [](const std::string& inp, const std::string& outp) { return extract_objects_from_elf(inp, outp, { "_binary_fp_shader_fpo_start", "_binary_fp_shader_fpo_end" }); } },
	{ "elf_extract_vp_cgbin", [](const std::string& inp, const std::string& outp) { return extract_objects_from_elf(inp, outp, { "_binary_vp_shader_vpo_start", "_binary_vp_shader_vpo_end" }); } }
};

void help()
{
	std::cout << "usage: [profile] <path to ucode file> <path to output file>" << std::endl;
	std::cout << "supported profiles: ";
	for (auto &profile : g_profiles)
	{
		std::cout << profile.first << " ";
	}
	std::cout << std::endl;
}*/

namespace vs
{
	int main();
}

int main(int argc, char** argv)
{
	return vs::main();
	/*
	if (argc != 4)
	{
		help();
		return -1;
	}

	auto found = g_profiles.find(std::string(argv[1]).substr(1));

	if (found == g_profiles.end())
	{
		help();
		return -2;
	}

	try
	{
		return found->second(argv[2], argv[3]);
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		return -5;
	}
	*/
}
