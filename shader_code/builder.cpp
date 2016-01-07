#include "builder.h"

namespace shader_code
{
	builder::expression_base_t::expression_base_t(const std::string& text)
		: text(text)
	{
	}

	std::string builder::expression_base_t::to_string() const
	{
		return text;
	}

	std::string builder::expression_base_t::finalize(bool put_end) const
	{
		return to_string();
	}


	builder::writer_t::writer_t()
	{
		fill_to(position);
	}

	builder::writer_t::writer_t(const std::string &string) : writer_t()
	{
		code[position] += string;
	}

	void builder::writer_t::fill_to(std::size_t position)
	{
		if (code.size() <= position)
		{
			code.resize(position + 1);
		}
	}

	void builder::writer_t::before(std::size_t position, const std::string& string)
	{
		fill_to(position);
		code[position] = string + code[position];
	}

	void builder::writer_t::after(std::size_t position, const std::string& string)
	{
		fill_to(position);
		code[position] += string;
	}

	void builder::writer_t::lines(const writer_t& writer)
	{
		code[position] += writer.build();
	}

	void builder::writer_t::next()
	{
		fill_to(++position);
	}

	static std::vector<std::string> get_lines(const std::string& source)
	{
		std::vector<std::string> result;

		size_t cursor_begin = 0;

		for (size_t cursor_end = 0; cursor_end < source.length(); ++cursor_end)
		{
			if (source[cursor_end] == '\n')
			{
				result.emplace_back(source.cbegin() + cursor_begin, source.cbegin() + cursor_end);

				cursor_begin = cursor_end + 1;
			}
		}

		if (cursor_begin != source.length())
		{
			result.emplace_back(source.cbegin() + cursor_begin, source.cend());
		}

		return result;
	}

	std::string builder::writer_t::finalize() const
	{
		std::string result;
		int lvl = 0;

		for (const std::string &line : get_lines(build()))
		{
			if (line.empty())
			{
				result += "\n";
			}
			else
			{
				std::size_t line_pos = std::string::npos;

				line_pos = std::string::npos;
				while ((line_pos = line.find('}', line_pos + 1)) != std::string::npos)
				{
					--lvl;
				}

				result += std::string(lvl, '\t') + line + "\n";

				while ((line_pos = line.find('{', line_pos + 1)) != std::string::npos)
				{
					++lvl;
				}
			}
		}

		return result;
	}

	std::string builder::writer_t::build() const
	{
		std::string result;

		for (const std::string &entry : code)
		{
			result += entry;
		}

		return result;
	}

	void builder::writer_t::clear()
	{
		code.clear();
	}
}
