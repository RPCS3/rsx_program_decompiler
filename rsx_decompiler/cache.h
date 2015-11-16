#pragma once
#include <unordered_map>

template<typename InfoType, typename DataType, typename Hasher = InfoType::hash_t, typename Equal = std::equal_to<InfoType>>
class cache_t
{
	std::unordered_map<InfoType, DataType, Hasher, Equal> m_entries;

public:
	bool invalidate(const InfoType& key)
	{
		if (auto found = m_entries.find(key))
		{
			m_entries.erase(found);
			return true;
		}

		return false;
	}

	DataType& entry(const InfoType& key)
	{
		return m_entries[key];
	}

	const DataType& entry(const InfoType& key) const
	{
		return m_entries.at(key);
	}

	void each(std::function<void(std::pair<const InfoType, DataType>)> function)
	{
		for (auto &entry : m_entries)
		{
			function(entry);
		}
	}
};