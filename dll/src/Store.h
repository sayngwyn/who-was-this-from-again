#pragma once

// What the mod remembers about each letter copy it has named: who/where it is from and its resolved
// text. Kept in the SKSE co-save (one record), keyed by the BOOK, the quest the copy belongs to and the
// quest instance, which stay the same whether the copy is carried, dropped, stored in a chest or picked up
// again, and differ between two letters of one kind that came from different quests.
namespace Store
{
	struct Key
	{
		RE::FormID   book = 0;
		RE::FormID   quest = 0;
		std::int32_t instance = 0;
		auto         operator<=>(const Key&) const = default;
	};

	struct Entry
	{
		std::string kindId;
		std::string sender;
		std::string subject;
		std::string text;       // the letter's text with its fill-ins put in
		bool        exact = false;  // the text is what the reading screen showed (not our own fill-in)
	};

	std::optional<Entry> Find(const Key& a_key);
	void                 Put(const Key& a_key, const Entry& a_entry);
	std::size_t          Size();
	std::vector<std::pair<Key, Entry>> All();

	void Save(SKSE::SerializationInterface* a_intfc);
	void Load(SKSE::SerializationInterface* a_intfc);
	void Revert(SKSE::SerializationInterface* a_intfc);
}
