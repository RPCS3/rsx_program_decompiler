#include "pch.h"
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

	void builder::writer_t::next()
	{
		fill_to(++position);
	}
}