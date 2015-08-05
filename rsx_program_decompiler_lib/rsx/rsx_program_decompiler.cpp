#include "rsx_program_decompiler.h"
#include "fmt.h"

namespace rsx
{
	mask_t& mask_t::add(const std::string& mask)
	{
		if (!mask.empty())
			swizzles.push_back(mask);
		return *this;
	}

	mask_t& mask_t::symplify()
	{
		if (swizzles.size() < 2)
			return *this;

		std::unordered_map<char, char> swizzle;

		static std::unordered_map<int, char> pos_to_swizzle =
		{
			{ 0, 'x' },
			{ 1, 'y' },
			{ 2, 'z' },
			{ 3, 'w' }
		};

		auto it = swizzles.begin();

		const std::string& sw_front = *it;

		for (auto &i : pos_to_swizzle)
		{
			swizzle[i.second] = sw_front.length() > i.first ? sw_front[i.first] : 0;
		}

		for (++it; it != swizzles.end(); ++it)
		{
			std::unordered_map<char, char> new_swizzle;

			for (auto &sw : pos_to_swizzle)
			{
				new_swizzle[sw.second] = swizzle[it->length() <= sw.first ? '\0' : (*it)[sw.first]];
			}

			swizzle = new_swizzle;
		}

		swizzles.clear();
		std::string new_swizzle;

		for (auto &i : pos_to_swizzle)
		{
			if (swizzle[i.second] != '\0')
				new_swizzle += swizzle[i.second];
		}

		swizzles.push_back(new_swizzle);

		return *this;
	}

	std::string mask_t::to_string_impl() const
	{
		return fmt::merge(swizzles, ".");
	}

	std::string mask_t::to_string() const
	{
		return to_string_impl();
	}

	std::string mask_t::to_string()
	{
		return symplify().to_string_impl();
	}

	std::string mask_t::apply_to(const std::string& expr) const
	{
		std::string mask = to_string();

		if (mask.empty())
			return expr;

		return expr + "." + mask;
	}

	std::string program_variable::to_string_impl() const
	{
		if (array_size)
			return name + "[" + std::to_string(index) + "]";

		return index != ~0 ? name + std::to_string(index) : name;
	}

	std::string program_variable::storage_name() const
	{
		return name + (array_size ? "[" + std::to_string(array_size + 1) + "]" : (index != ~0 ? std::to_string(index) : std::string{}));
	}

	std::string program_variable::to_string() const
	{
		return mask.apply_to(to_string_impl());
	}

	std::string program_variable::to_string()
	{
		return mask.symplify().apply_to(to_string_impl());
	}

	bool program_variable::is_null() const
	{
		return name.empty() && constant.type == program_constant_type::none;
	}

	program_variable::operator bool() const
	{
		return !is_null();
	}

	program_variable& program_variables::add(const program_variable& var)
	{
		auto &new_var = m_data[var.storage_name()];
		new_var = var;

		return new_var;
	}

	const program_variable& program_variables::operator[](const std::string& name) const
	{
		auto found = m_data.find(name);
		if (found == m_data.end())
			throw std::logic_error("program_variables: program_variable '" + name + "' not found");

		return found->second;
	}

	program_variable& program_variables::operator[](const std::string& name)
	{
		auto found = m_data.find(name);
		if (found == m_data.end())
			throw std::logic_error("program_variables: program_variable '" + name + "' not found");

		return found->second;
	}

	bool program_variables::exists(const std::string& name) const
	{
		return m_data.find(name) != m_data.end();
	}

	void program_variables::clear()
	{
		m_data.clear();
	}

	std::unordered_map<std::string, program_variable>::iterator program_variables::begin()
	{
		return m_data.begin();
	}

	std::unordered_map<	std::string, program_variable>::iterator program_variables::end()
	{
		return m_data.end();
	}

	void program_info::clear()
	{
		text.clear();
		vars.clear();
	}

	void code_builder::add_code_block(size_t index, const std::string& lines, int tab_before, int tab_after, bool to_end)
	{
		auto& value = m_entries[index];
		std::list<code_line> code_lines;

		const auto blocked_lines = fmt::split(lines, "\n", false);
		if (blocked_lines.size() == 1)
		{
			code_lines.push_back(code_line{ tab_before, tab_after, blocked_lines[0] });
		}
		else if (blocked_lines.size() > 1)
		{
			auto begin = blocked_lines.begin();
			auto end = blocked_lines.end() - 1;

			code_lines.push_back(code_line{ tab_before, 0, *begin++ });
			while (begin != end)
			{
				code_lines.push_back(code_line{ 0, 0, *begin++ });
			}

			code_lines.push_back(code_line{ 0, tab_after, blocked_lines.back() });
		}

		value.code_lines.insert(to_end ? value.code_lines.end() : value.code_lines.begin(), code_lines.begin(), code_lines.end());
	}

	void code_builder::branch(size_t from, size_t to, const std::string& condition)
	{
		if (from < to)
		{
			m_entries[from].branch_to.push_back({ to, condition });
		}
		else
		{
			m_entries[to].branch_from.push_back({ from, condition });
		}
	}

	void code_builder::build_branches()
	{
		std::vector<std::pair<size_t, size_t>> reserved_blocks;

		auto is_reserved = [&](size_t begin, size_t end)
		{
			for (auto &block : reserved_blocks)
			{
				if (begin >= block.first && begin < block.second)
				{
					if (end < block.first || end > block.second)
						return true;
				}
			}

			return false;
		};

		for (auto &entry : m_entries)
		{
			for (auto &binfo : entry.second.branch_to)
			{
				if (is_reserved(entry.first, binfo.index))
				{
					throw std::runtime_error("unhandled branch");
				}

				add_code_block(entry.first, "if (!" + binfo.condition + ")\n{", 0, 1, true);
				add_code_block(binfo.index, "}", -1, 0, false);

				reserved_blocks.push_back(std::make_pair(entry.first, binfo.index));
			}

			for (auto &binfo : entry.second.branch_from)
			{
				if (is_reserved(entry.first, binfo.index))
				{
					throw std::runtime_error("unhandled branch");
				}

				add_code_block(entry.first, "do\n{", 0, 1, true);
				add_code_block(binfo.index, "}\nwhile (" + binfo.condition + ");", -1, 0, false);

				reserved_blocks.push_back(std::make_pair(entry.first, binfo.index));
			}
		}
	}

	std::string code_builder::build_code() const
	{
		std::string result;
		int tab_count = 0;

		for (auto &entry : m_entries)
		{
			for (auto &line : entry.second.code_lines)
			{
				tab_count += line.tab_before;

				if (line.value.empty())
				{
					result += "\n";
				}
				else
				{
					result.append(tab_count, '\t') += line.value + "\n";
				}

				tab_count += line.tab_after;
			}
		}

		return result;
	}

	std::string code_builder::build()
	{
		build_branches();
		return build_code();
	}

	program_info& program_decompiler_core::decompile()
	{
		info.text = builder.build();
		return info;
	}
}