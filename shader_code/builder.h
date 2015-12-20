#pragma once
#include <string>
#include <unordered_map>

namespace shader_code
{
	struct builder
	{
		template<typename TypeType, TypeType Type, int Count>
		struct type_t
		{
			static constexpr TypeType type = Type;
			static constexpr int count = Count;
		};

		struct expression_base_t
		{
			std::string text;

			expression_base_t() = default;
			expression_base_t(const std::string& text);
			virtual std::string to_string() const;
			virtual std::string finalize(bool put_end) const;
		};

		struct writer_t
		{
			std::vector<std::string> code;
			std::size_t position = 0;

			writer_t();
			writer_t(const std::string &string);

			void fill_to(std::size_t position);

			template<typename... T>
			void lines(const T&... exprs)
			{
				for (auto& expr : { exprs.finalize(true)... })
				{
					code[position] += !expr.empty() && expr.back() != '\n' ? expr + "\n" : expr;
				}
			}

			void before(std::size_t position, const std::string& string);
			void after(std::size_t position, const std::string& string);
			void lines(const writer_t& writer);
			void next();

			template<typename T>
			writer_t& operator +=(const T& expr)
			{
				lines(expr);
				return *this;
			}

			operator const expression_base_t() const
			{
				return{ build() };
			}

			std::string finalize() const;
			std::string build() const;

			void clear();
		};
	};
}