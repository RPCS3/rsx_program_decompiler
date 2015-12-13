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
			std::unordered_map<std::size_t, std::string> code;
			std::size_t position = 0;

			struct writer_to
			{
				std::string *code;
			};

			writer_to operator()(std::size_t position);

			template<typename... T>
			void lines(const T&... exprs)
			{
				for (auto& expr : { exprs.finalize(true)... })
				{
					code[position] += !expr.empty() && expr.back() != '\n' ? expr + "\n" : expr;
				}
			}

			void lines(const std::string& string)
			{
				code[position] += string;
			}

			void lines(const writer_t& writer)
			{
				lines(writer.build());
			}

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

			std::string build() const
			{
				std::string result;

				for (auto entry : code)
				{
					result += entry.second;
				}

				return result;
			}

			void clear()
			{
				code.clear();
			}
		};
	};
}