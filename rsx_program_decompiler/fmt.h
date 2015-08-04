#pragma once
#include <string>
#include <vector>
#include <cstddef>
#include <functional>

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;
using u64 = std::uint64_t;
using s64 = std::int64_t;

#if defined(_MSC_VER)
	#define snprintf _snprintf
	#define force_inline __forceinline
#else
	#define force_inline inline
#endif

#define safe_buffers

namespace fmt
{
	struct empty_t{};

	//small wrapper used to deal with bitfields
	template<typename T>
	T by_value(T x) { return x; }

	//wrapper to deal with advance sprintf formating options with automatic length finding
	template<typename... Args> std::string Format(const char* fmt, Args... parameters)
	{
		std::size_t length = 256;
		std::string str;

		for (;;)
		{
			std::vector<char> buffptr(length);
#if !defined(_MSC_VER)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
			std::size_t printlen = snprintf(buffptr.data(), length, fmt, std::forward<Args>(parameters)...);
#pragma clang diagnostic pop
#else
			std::size_t printlen = _snprintf_s(buffptr.data(), length, length - 1, fmt, std::forward<Args>(parameters)...);
#endif
			if (printlen < length)
			{
				str = std::string(buffptr.data(), printlen);
				break;
			}
			length *= 2;
		}
		return str;
	}

	std::string replace_first(const std::string& src, const std::string& from, const std::string& to);
	std::string replace_all(const std::string &src, const std::string& from, const std::string& to);

	template<std::size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::string>(&list)[list_size])
	{
		for (std::size_t pos = 0; pos < src.length(); ++pos)
		{
			for (std::size_t i = 0; i < list_size; ++i)
			{
				const std::size_t comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second : list[i].second) + src.substr(pos + comp_length);
					pos += list[i].second.length() - 1;
					break;
				}
			}
		}

		return src;
	}

	template<std::size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::function<std::string()>>(&list)[list_size])
	{
		for (std::size_t pos = 0; pos < src.length(); ++pos)
		{
			for (std::size_t i = 0; i < list_size; ++i)
			{
				const std::size_t comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second() : list[i].second()) + src.substr(pos + comp_length);
					pos += list[i].second().length() - 1;
					break;
				}
			}
		}

		return src;
	}

	template<typename T, bool is_enum = std::is_enum<T>::value>
	struct unveil
	{
		typedef T result_type;

		force_inline static result_type get_value(const T& arg)
		{
			return arg;
		}
	};

	template<>
	struct unveil<char*, false>
	{
		typedef const char* result_type;

		force_inline static result_type get_value(const char* arg)
		{
			return arg;
		}
	};

	template<std::size_t N>
	struct unveil<const char[N], false>
	{
		typedef const char* result_type;

		force_inline static result_type get_value(const char(&arg)[N])
		{
			return arg;
		}
	};

	template<>
	struct unveil<std::string, false>
	{
		typedef const char* result_type;

		force_inline static result_type get_value(const std::string& arg)
		{
			return arg.c_str();
		}
	};

	template<typename T>
	struct unveil<T, true>
	{
		typedef typename std::underlying_type<T>::type result_type;

		force_inline static result_type get_value(const T& arg)
		{
			return static_cast<result_type>(arg);
		}
	};
	/*
	template<typename T, typename T2>
	struct unveil<be_t<T, T2>, false>
	{
		typedef typename unveil<T>::result_type result_type;

		force_inline static result_type get_value(const be_t<T, T2>& arg)
		{
			return unveil<T>::get_value(arg.value());
		}
	};
	*/
	template<typename T>
	force_inline typename unveil<T>::result_type do_unveil(const T& arg)
	{
		return unveil<T>::get_value(arg);
	}

	/*
	fmt::format(const char* fmt, args...)

	Formatting function with special functionality:

	std::string forced to .c_str()
	be_t<> forced to .value() (fmt::unveil reverts byte order automatically)

	External specializations for fmt::unveil (can be found in another headers):
	vm::ps3::ptr (fmt::unveil) (vm_ptr.h) (with appropriate address type, using .addr() can be avoided)
	vm::ps3::bptr (fmt::unveil) (vm_ptr.h)
	vm::psv::ptr (fmt::unveil) (vm_ptr.h)
	vm::ps3::ref (fmt::unveil) (vm_ref.h)
	vm::ps3::bref (fmt::unveil) (vm_ref.h)
	vm::psv::ref (fmt::unveil) (vm_ref.h)
	
	*/
	template<typename... Args> force_inline safe_buffers std::string format(const char* fmt, Args... args)
	{
		return Format(fmt, do_unveil(args)...);
	}

	std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators = { " ", "\t" }, bool is_skip_empty = true);
	std::vector<std::string> split(const std::string& source, const std::string& separator, bool is_skip_empty = true);

	template<typename T>
	std::string merge(const T& source, const std::string& separator)
	{
		if (!source.size())
		{
			return{};
		}

		std::string result;

		auto it = source.begin();
		auto end = source.end();
		for (--end; it != end; ++it)
		{
			result += *it + separator;
		}

		return result + source.back();
	}

	template<typename T>
	std::string merge(std::initializer_list<T> sources, const std::string& separator)
	{
		if (!sources.size())
		{
			return{};
		}

		std::string result;
		bool first = true;

		for (auto &v : sources)
		{
			if (first)
			{
				result = fmt::merge(v, separator);
				first = false;
			}
			else
			{
				result += separator + fmt::merge(v, separator);
			}
		}

		return result;
	}

	std::string tolower(std::string source);
	std::string toupper(std::string source);
	std::string escape(std::string source);

	template<typename T, bool use_std_to_string>
	struct to_string_impl
	{
		static std::string func(const T& value)
		{
			return value.to_string();
		}
	};

	template<typename T, bool use_std_to_string = std::is_arithmetic<T>::value>
	std::string to_string(const T& value)
	{
		return to_string_impl<std::remove_cv_t<T>, use_std_to_string>::func(value);
	}

	class string : public std::string
	{
	public:
		//using std::string;
		string() = default;

		template<typename T>
		string(const T& value) : std::string(to_string(value))
		{
		}

		string(std::size_t count, char ch) : std::string(count, ch)
		{
		}

		template<typename T>
		string& operator = (const T& rhs)
		{
			std::string::operator=(to_string(rhs));
			return *this;
		}

		template<typename T>
		string operator + (const T& rhs) const
		{
			return to_string(*this) + to_string(rhs);
		}

		template<typename T>
		string& operator += (const T& rhs)
		{
			std::string::operator+=(to_string(rhs));
			return *this;
		}

		std::vector<std::string> split(std::initializer_list<std::string> separators = { " ", "\t" }, bool is_skip_empty = true) const;

		string& to_lower();
		string as_lower() const;
		string& to_upper();
		string as_upper() const;
	};

	template<typename T>
	struct to_string_impl<T, true>
	{
		static std::string func(const T& value)
		{
			return std::to_string(value);
		}
	};

	template<bool use_std_to_string>
	struct to_string_impl<std::string, use_std_to_string>
	{
		static const std::string& func(const std::string& value)
		{
			return value;
		}
	};

	template<bool use_std_to_string>
	struct to_string_impl<string, use_std_to_string>
	{
		static const std::string& func(const string& value)
		{
			return value;
		}
	};

	template<bool use_std_to_string>
	struct to_string_impl<char*, use_std_to_string>
	{
		static std::string func(const char* value)
		{
			return value;
		}
	};

	template<bool use_std_to_string, std::size_t N>
	struct to_string_impl<char[N], use_std_to_string>
	{
		static std::string func(const char(&value)[N])
		{
			return value;
		}
	};
}
