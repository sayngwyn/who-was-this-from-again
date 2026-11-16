#include "pch.h"

#include "Log.h"
#include "Store.h"

namespace Store
{
	namespace
	{
		constexpr std::uint32_t kUniqueId = 'WWTF';
		constexpr std::uint32_t kRecordLetters = 'LTRS';
		constexpr std::uint32_t kVersion = 1;

		std::mutex                   g_lock;
		std::map<Key, Entry>         g_entries;

		bool WriteString(SKSE::SerializationInterface* a_intfc, const std::string& a_text)
		{
			const auto len = static_cast<std::uint32_t>(a_text.size());
			return a_intfc->WriteRecordData(len) && (len == 0 || a_intfc->WriteRecordData(a_text.data(), len));
		}

		bool ReadString(SKSE::SerializationInterface* a_intfc, std::string& a_text)
		{
			std::uint32_t len = 0;
			if (!a_intfc->ReadRecordData(len) || len > 4u * 1024u * 1024u) {
				return false;
			}
			a_text.assign(len, '\0');
			return len == 0 || a_intfc->ReadRecordData(a_text.data(), len);
		}
	}

	std::optional<Entry> Find(const Key& a_key)
	{
		std::scoped_lock lock(g_lock);
		const auto       it = g_entries.find(a_key);
		if (it == g_entries.end()) {
			return std::nullopt;
		}
		return it->second;
	}

	void Put(const Key& a_key, const Entry& a_entry)
	{
		std::scoped_lock lock(g_lock);
		g_entries[a_key] = a_entry;
	}

	std::size_t Size()
	{
		std::scoped_lock lock(g_lock);
		return g_entries.size();
	}

	std::vector<std::pair<Key, Entry>> All()
	{
		std::scoped_lock lock(g_lock);
		return { g_entries.begin(), g_entries.end() };
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		std::scoped_lock lock(g_lock);
		if (!a_intfc->OpenRecord(kRecordLetters, kVersion)) {
			Log::Problem("Co-save: could not open the letters record, nothing saved");
			return;
		}
		bool ok = a_intfc->WriteRecordData(static_cast<std::uint32_t>(g_entries.size()));
		for (const auto& [key, e] : g_entries) {
			ok = ok && a_intfc->WriteRecordData(key.book) && a_intfc->WriteRecordData(key.quest) && a_intfc->WriteRecordData(key.instance) &&
			     WriteString(a_intfc, e.kindId) && WriteString(a_intfc, e.sender) && WriteString(a_intfc, e.subject) && WriteString(a_intfc, e.text) &&
			     a_intfc->WriteRecordData(static_cast<std::uint8_t>(e.exact ? 1 : 0));
		}
		Log::Event("Co-save: wrote {} letter(s){}", g_entries.size(), ok ? "" : " (WRITE FAILED)");
	}

	void Load(SKSE::SerializationInterface* a_intfc)
	{
		std::scoped_lock lock(g_lock);
		g_entries.clear();
		std::uint32_t type = 0, version = 0, length = 0;
		int           kept = 0, dropped = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type != kRecordLetters || version != kVersion) {
				Log::Problem("Co-save: unknown record {:08X} version {} skipped", type, version);
				continue;
			}
			std::uint32_t count = 0;
			if (!a_intfc->ReadRecordData(count)) {
				Log::Problem("Co-save: the letters record is unreadable");
				continue;
			}
			for (std::uint32_t i = 0; i < count; ++i) {
				Key           key;
				Entry         e;
				std::uint8_t  exact = 0;
				if (!a_intfc->ReadRecordData(key.book) || !a_intfc->ReadRecordData(key.quest) || !a_intfc->ReadRecordData(key.instance) ||
				    !ReadString(a_intfc, e.kindId) || !ReadString(a_intfc, e.sender) || !ReadString(a_intfc, e.subject) || !ReadString(a_intfc, e.text) ||
				    !a_intfc->ReadRecordData(exact)) {
					Log::Problem("Co-save: letter {} of {} is cut short, the rest are dropped", i + 1, count);
					break;
				}
				e.exact = exact != 0;
				RE::FormID book = 0, quest = 0;
				if (!a_intfc->ResolveFormID(key.book, book) || !a_intfc->ResolveFormID(key.quest, quest)) {
					++dropped;  // the plugin that made the book or quest is gone from this load order
					continue;
				}
				key.book = book;
				key.quest = quest;
				g_entries[key] = std::move(e);
				++kept;
			}
		}
		Log::Event("Co-save: read {} letter(s), {} dropped (their plugin is no longer loaded)", kept, dropped);
	}

	void Revert(SKSE::SerializationInterface*)
	{
		std::scoped_lock lock(g_lock);
		g_entries.clear();
	}
}
