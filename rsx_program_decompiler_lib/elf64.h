#pragma once
#include "endianness.h"

namespace elf
{
	enum class elf_class : u8
	{
		elf32 = 1,
		elf64 = 2
	};

	enum class elf_encoding : u8
	{
		little_endian = 1,
		big_endian = 2
	};

	enum class osabi : u8
	{
		sysv = 0,
		hpux = 1,
		standalone = 255
	};

	struct alignas(16) ident_t
	{
		u8 magic[4];
		elf_class file_class;
		elf_encoding data_encoding;
		u8 file_version;
		osabi osabi;
		u8 abi_version;
		u8 pad[6];
		u8 ident_size;
	};

	static_assert(sizeof(ident_t) == 16, "bad ident_t implementation");
}

namespace elf64
{
	using endianness::ue;

	using addr_t = ue<u64>;
	using off_t = ue<u64>;
	using half_t = ue<u16>;
	using word_t = ue<u32>;
	using sword_t = ue<s32>;
	using xword_t = ue<u64>;
	using sxword_t = ue<s64>;

	enum class elf_type : half_t::type
	{
		none = 0,
		rel = 1,
		exec = 2,
		dyn = 3,
		core = 4,
		loos = 0xfe00,
		hios = 0xfeff,
		loproc = 0xff00,
		hiproc = 0xffff
	};

	enum class sh_type : word_t::type
	{
		null,
		progbits,
		symtab,
		strtab,
		rela,
		hash,
		dynamic,
		note,
		shlib,
		dynsym,
		loos = 0x6000'0000,
		hios = 0x6fff'ffff,
		loproc = 0x7000'0000,
		hiproc = 0x7fff'ffff
	};

	enum class sh_flags : xword_t::type
	{
		write = 1,
		alloc = 2,
		execinstr = 4,
		maskos = 0x0f00'0000,
		maskproc = 0xf000'0000,
	};

	enum class shndx
	{
		undef = 0,
		loproc = 0xff00,
		hiproc = 0xff1f,
		loos = 0xff20,
		hioc = 0xff3f,
		abs = 0xfff1,
		common = 0xffff2
	};

	enum class st_binding : u8
	{
		local,
		global,
		weak,
	};

	static std::string to_string(st_binding value)
	{
		switch (value)
		{
		case st_binding::local: return "local";
		case st_binding::global: return "global";
		case st_binding::weak: return "weak";
		}

		return "unknown" + std::to_string((u8)value);
	}

	enum class st_type : u8
	{
		notype,
		object,
		func,
		section,
		file
	};

	struct ehdr
	{
		elf::ident_t e_ident;
		ue<elf_type> e_type;
		half_t e_machine;
		word_t e_version;
		addr_t e_entry;
		off_t e_phoff;
		off_t e_shoff;
		word_t e_flags;
		half_t e_ehsize;
		half_t e_phentsize;
		half_t e_phnum;
		half_t e_shentsize;
		half_t e_shnum;
		half_t e_shstrndx;
	};

	struct shdr
	{
		word_t sh_name;
		ue<sh_type> sh_type;
		ue<sh_flags> sh_flags;
		addr_t sh_addr;
		off_t sh_offset;
		xword_t sh_size;
		word_t sh_link;
		word_t sh_info;
		xword_t sh_addralign;
		xword_t sh_entsize;
	};

	struct sym
	{
		word_t st_name;
		struct alignas(1)
		{
			st_binding binding : 4;
			st_type type : 4;
		} st_info;
		u8 st_other;
		half_t st_shndx;
		addr_t st_value;
		xword_t st_size;
	};
}