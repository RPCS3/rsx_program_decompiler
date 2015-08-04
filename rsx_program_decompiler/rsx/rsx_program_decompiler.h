#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include "fmt.h"

namespace rsx
{
	enum class program_variable_type
	{
		none,
		input,
		output,
		constant
	};

	struct mask_t
	{
		std::vector<std::string> swizzles;

		mask_t& add(const std::string& mask);
		mask_t& symplify();

	private:
		std::string to_string_impl() const;

	public:
		std::string to_string() const;
		std::string to_string();
	};

	struct program_variable
	{
		std::string name;
		mask_t mask;
		program_variable_type type;
		u32 index;
		u8 size;
		std::string storage_type;
		u32 array_size;
		bool is_neg;
		bool is_abs;

	private:
		std::string to_string_impl() const;
		std::string append_dot_if_not_empty(const std::string& string) const;

	public:
		std::string storage_name() const;
		std::string to_string() const;
		std::string to_string();
		bool is_null() const;
		explicit operator bool() const;
	};

	class program_variables
	{
		std::unordered_map<std::string, program_variable> m_data;

	public:
		program_variable& add(const program_variable& var);
		const program_variable& operator[](const std::string& name) const;
		program_variable& operator[](const std::string& name);
		bool exists(const std::string& name) const;

		void clear();

		std::unordered_map<std::string, program_variable>::iterator begin();
		std::unordered_map<std::string, program_variable>::iterator end();
	};

	struct program_info
	{
		std::string text;
		program_variables vars;

		void clear();
	};

	class code_builder
	{
		struct code_line
		{
			int tab_before;
			int tab_after;
			std::string value;
		};

		struct branch_info
		{
			size_t index;
			std::string condition;
		};

		struct entry
		{
			std::list<branch_info> branch_to;
			std::list<branch_info> branch_from;
			std::list<code_line> code_lines;
		};

		std::map<size_t, entry> m_entries;
		size_t next_index = 0;

	public:
		void add_code_block(size_t index, const std::string& lines, int tab_before = 0, int tab_after = 0, bool to_end = true);
		void branch(size_t from, size_t to, const std::string& condition = {});
		void build_branches();
		std::string build_code() const;
		std::string build();
	};

	struct program_decompiler_core
	{
		code_builder builder;
		program_info info;

		program_info& decompile();
	};
}