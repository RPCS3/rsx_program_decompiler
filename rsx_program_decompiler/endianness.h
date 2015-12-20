#pragma once
#include "fmt.h"

namespace endianness
{
	template<std::size_t size>
	struct swap_impl;

	template<> struct swap_impl<1>
	{
		template<typename T>
		constexpr static T swap(T value)
		{
			return value;
		}
	};

	template<> struct swap_impl<2>
	{
		template<typename T>
		constexpr static T swap(T value)
		{
			return ((value >> 8) & 0xff) | ((value << 8) & 0xff00);
		}
	};

	template<> struct swap_impl<4>
	{
		template<typename T>
		constexpr static T swap(T value)
		{
			return
				((value >> 24) & 0x000000ff) |
				((value >> 8) & 0x0000ff00) |
				((value << 8) & 0x00ff0000) |
				((value << 24) & 0xff000000);
		}
	};

	template<> struct swap_impl<8>
	{
		template<typename T>
		constexpr static T swap(T value)
		{
			return
				((value >> 56) & 0x00000000000000ff) |
				((value >> 40) & 0x000000000000ff00) |
				((value >> 24) & 0x0000000000ff0000) |
				((value >> 8) & 0x00000000ff000000) |
				((value << 8) & 0x000000ff00000000) |
				((value << 24) & 0x0000ff0000000000) |
				((value << 40) & 0x00ff000000000000) |
				((value << 56) & 0xff00000000000000);
		}
	};

	template<typename T>
	constexpr T swap(T value)
	{
		return swap_impl<sizeof(T)>::swap(value);
	}

	template<std::size_t size>
	struct simple_type;

	template<> struct simple_type<1> { using type = u8; };
	template<> struct simple_type<2> { using type = u16; };
	template<> struct simple_type<4> { using type = u32; };
	template<> struct simple_type<8> { using type = u64; };

	template<std::size_t size>
	using simple_type_t = typename simple_type<size>::type;

	template<typename T>
	using simple_type_for_t = typename simple_type<sizeof(T)>::type;

	template<typename Type, typename EndiannessImpl>
	class alignas(sizeof(Type)) endianness_t
	{
	public:
		using type = std::remove_cv_t<Type>;
		using storage_type = simple_type_for_t<type>;

	private:
		storage_type m_packed_value;

		constexpr type unpack() const
		{
			return EndiannessImpl::unpack(m_packed_value);
		}

		void pack(type value)
		{
			m_packed_value = EndiannessImpl::pack<storage_type>(value);
		}

	public:
		constexpr endianness_t(type value) : m_packed_value(EndiannessImpl::pack<storage_type>(value))
		{
		}

		constexpr endianness_t() : m_packed_value{}
		{
		}

		constexpr operator type() const
		{
			return EndiannessImpl::unpack(m_packed_value);
		}

		endianness_t& operator +=(const endianness_t& rhs) { pack(unpack() + rhs.unpack()); return *this; }
		endianness_t& operator -=(const endianness_t& rhs) { pack(unpack() - rhs.unpack()); return *this; }
		endianness_t& operator *=(const endianness_t& rhs) { pack(unpack() * rhs.unpack()); return *this; }
		endianness_t& operator /=(const endianness_t& rhs) { pack(unpack() / rhs.unpack()); return *this; }
		endianness_t& operator %=(const endianness_t& rhs) { pack(unpack() % rhs.unpack()); return *this; }
		endianness_t& operator &=(const endianness_t& rhs) { pack(unpack() & rhs.unpack()); return *this; }
		endianness_t& operator |=(const endianness_t& rhs) { pack(unpack() | rhs.unpack()); return *this; }
		endianness_t& operator ^=(const endianness_t& rhs) { pack(unpack() ^ rhs.unpack()); return *this; }
		endianness_t& operator >>=(int shift) { pack(unpack() >> shift); return *this; }
		endianness_t& operator <<=(int shift) { pack(unpack() << shift); return *this; }
		endianness_t& operator =(const type& rhs) { pack(rhs); return *this; }
	};

	struct swapped_endianness_impl
	{
		template<typename RT, typename T>
		static constexpr RT pack(T value)
		{
			return swap(value);
		}

		template<typename T>
		static constexpr T unpack(T value)
		{
			return swap(value);
		}
	};

	struct native_endianness_impl
	{
		template<typename RT, typename T>
		static constexpr RT pack(T value)
		{
			return value;
		}

		template<typename T>
		static constexpr T unpack(T value)
		{
			return value;
		}
	};

	enum class endian
	{
		little,
		big,
		unknown
	};

	static constexpr endian native_endianness =
		(0x01234567 & 0xff) == 0x01 ? endian::big :
		(0x01234567 & 0xff) == 0x67 ? endian::little : endian::unknown;

	static_assert(native_endianness != endian::unknown, "unsupported endianness");

	template<typename T> using be_t = endianness_t<T, std::conditional_t<native_endianness == endian::big, native_endianness_impl, swapped_endianness_impl>>;
	template<typename T> using le_t = endianness_t<T, std::conditional_t<native_endianness == endian::little, native_endianness_impl, swapped_endianness_impl>>;

	template<typename T> using ne_t = endianness_t<T, native_endianness_impl>;

	template<typename T> using be = std::conditional_t<native_endianness == endian::big, T, be_t<T>>;
	template<typename T> using le = std::conditional_t<native_endianness == endian::little, T, le_t<T>>;

	template<typename T>
	struct alignas(sizeof(T)) unknown_endian_t
	{
		using type = std::remove_cv_t<T>;
		type packed_data;
	};

	template<typename T> constexpr T unpack(const unknown_endian_t<T>& data, endian endian)
	{
		return endian == native_endianness ? data.packed_data : (T)swap((simple_type_for_t<T>)data.packed_data);
	}

	template<typename T> constexpr unknown_endian_t<T> pack(T value, endian endian = native_endianness)
	{
		return{ endian == native_endianness ? value : (T)swap((simple_type_for_t<T>)value) };
	}

	template<typename T> void pack(unknown_endian_t<T>& resut, T value, endian endian = native_endianness)
	{
		resut = pack(value, endian);
	}

	template<typename T>
	using ue = unknown_endian_t<T>;
}