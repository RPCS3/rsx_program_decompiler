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

		struct writer_t : expression_base_t
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
					code[position] += expr + "\n";
				}
			}

			void lines(const std::string& string)
			{
				code[position] += string + "\n";
			}

			void next();

			template<typename T>
			writer_t& operator +=(const T& expr)
			{
				lines(expr);
				return *this;
			}

			std::string build()
			{
				for (auto entry : code)
				{
					text += entry.second;
				}

				code.clear();
				return text;
			}

			std::string to_string() const override
			{
				return const_cast<writer_t*>(this)->build();
			}
		};
	};
}