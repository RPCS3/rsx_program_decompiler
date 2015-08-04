#include "fmt.h"
#include <algorithm>

namespace fmt
{
	std::string replace_first(const std::string& src, const std::string& from, const std::string& to)
	{
		auto pos = src.find(from);

		if (pos == std::string::npos)
		{
			return src;
		}

		return (pos ? src.substr(0, pos) + to : to) + std::string(src.c_str() + pos + from.length());
	}

	std::string replace_all(const std::string &src, const std::string& from, const std::string& to)
	{
		std::string target = src;
		for (auto pos = target.find(from); pos != std::string::npos; pos = target.find(from, pos + 1))
		{
			target = (pos ? target.substr(0, pos) + to : to) + std::string(target.c_str() + pos + from.length());
			pos += to.length();
		}

		return target;
	}

	std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty)
	{
		std::vector<std::string> result;

		std::size_t cursor_begin = 0;

		for (std::size_t cursor_end = 0; cursor_end < source.length(); ++cursor_end)
		{
			for (auto &separator : separators)
			{
				if (strncmp(source.c_str() + cursor_end, separator.c_str(), separator.length()) == 0)
				{
					std::string candidate = source.substr(cursor_begin, cursor_end - cursor_begin);
					if (!is_skip_empty || !candidate.empty())
						result.push_back(candidate);

					cursor_begin = cursor_end + separator.length();
					cursor_end = cursor_begin - 1;
					break;
				}
			}
		}

		if (cursor_begin != source.length())
		{
			result.push_back(source.substr(cursor_begin));
		}

		return std::move(result);
	}

	std::vector<std::string> split(const std::string& source, const std::string& separator, bool is_skip_empty)
	{
		return split(source, { separator }, is_skip_empty);
	}

	std::string tolower(std::string source)
	{
		std::transform(source.begin(), source.end(), source.begin(), ::tolower);

		return source;
	}

	std::string toupper(std::string source)
	{
		std::transform(source.begin(), source.end(), source.begin(), ::toupper);

		return source;
	}

	std::string escape(std::string source)
	{
		const std::pair<std::string, std::string> escape_list[] =
		{
			{ "\\", "\\\\" },
			{ "\a", "\\a" },
			{ "\b", "\\b" },
			{ "\f", "\\f" },
			{ "\n", "\\n\n" },
			{ "\r", "\\r" },
			{ "\t", "\\t" },
			{ "\v", "\\v" },
		};

		source = replace_all(source, escape_list);

		for (char c = 0; c < 32; c++)
		{
			if (c != '\n')
				source = replace_all(source, std::string(1, c), Format("\\x%02X", c));
		}

		return source;
	}

	std::vector<std::string> string::split(std::initializer_list<std::string> separators, bool is_skip_empty) const
	{
		return fmt::split(*this, separators, is_skip_empty);
	}

	string& string::to_lower()
	{
		return *this = tolower(*this);
	}

	string string::as_lower() const
	{
		return tolower(*this);
	}

	string& string::to_upper()
	{
		return *this = toupper(*this);
	}

	string string::as_upper() const
	{
		return toupper(*this);
	}
}