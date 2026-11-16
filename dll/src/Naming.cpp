#include "pch.h"

#include "Letters.h"
#include "Log.h"
#include "Naming.h"
#include "Settings.h"
#include "Store.h"

namespace Naming
{
	namespace
	{
		std::set<std::string> g_reported;  // skip reasons already logged once

		void ReportOnce(const std::string& a_key, const std::string& a_line)
		{
			if (g_reported.insert(a_key).second) {
				Log::Event("{}", a_line);
			}
		}

		// The inventory list reads a letter's name from the reference the copy came from (the copy's extra data points at it
		// with ExtraReferenceHandle), not from the copy's own text data, so both get the name.
		bool SetOriginalName(RE::ExtraDataList* a_list, const std::string& a_name)
		{
			const auto handle = a_list ? a_list->GetByType<RE::ExtraReferenceHandle>() : nullptr;
			const auto original = handle ? handle->GetOriginalReference() : nullptr;
			const auto data = original ? original->extraList.GetByType<RE::ExtraTextDisplayData>() : nullptr;
			if (!data) {
				return false;
			}
			const char* now = data->displayName.c_str();
			if (!now || a_name != now) {
				data->displayName = a_name.c_str();
			}
			return true;
		}

		void SetName(RE::ExtraDataList* a_list, RE::ExtraTextDisplayData* a_data, const std::string& a_name, const char* a_what)
		{
			if (a_name.empty()) {
				return;
			}
			const bool mirrored = SetOriginalName(a_list, a_name);
			const char* now = a_data->displayName.c_str();
			if (now && a_name == now) {
				return;
			}
			a_data->displayName = a_name.c_str();
			Log::Event("  {}: named '{}'{}", a_what, a_name, mirrored ? "" : " (this copy has no original reference, only its own name is set)");
		}

		// Each letter copy in the player's inventory: callback(entry base, text data).
		template <class F>
		void ForEachCopy(F&& a_fn)
		{
			const auto player = RE::PlayerCharacter::GetSingleton();
			const auto changes = player ? player->GetInventoryChanges() : nullptr;
			if (!changes || !changes->entryList) {
				return;
			}
			for (const auto entry : *changes->entryList) {
				if (!entry || !entry->object || !entry->extraLists) {
					continue;
				}
				const auto kind = Letters::KindFor(entry->object->GetFormID());
				if (!kind) {
					continue;
				}
				for (const auto list : *entry->extraLists) {
					const auto data = list ? list->GetByType<RE::ExtraTextDisplayData>() : nullptr;
					if (data) {
						a_fn(*kind, entry->object->As<RE::TESObjectBOOK>(), data, list);
					}
				}
			}
		}
	}

	std::optional<Seen> QuestLink(const RE::ExtraDataList* a_list)
	{
		const auto data = a_list ? a_list->GetByType<RE::ExtraTextDisplayData>() : nullptr;
		if (!data || !data->ownerQuest || data->ownerInstance.underlying() < 0) {
			return std::nullopt;
		}
		return Seen{ data->ownerQuest->GetFormID(), data->ownerInstance.underlying(), data->ownerQuest };
	}

	void OnContainerChanged(const RE::TESContainerChangedEvent& a_event)
	{
		if (a_event.newContainer != 0x14 || a_event.itemCount <= 0 || !Letters::KindFor(a_event.baseObj)) {
			return;
		}
		const auto id = a_event.baseObj;
		SKSE::GetTaskInterface()->AddTask([id] {
			Log::Event("Letter {:08X} reached the player ({})", id, "pickup");
			ProcessInventory("pickup", true);
		});
	}

	void ProcessInventory(const char* a_why, bool a_capture, bool a_onlyUnknown)
	{
		const auto& settings = Settings::Get();
		int         seen = 0, named = 0, goneNew = 0;
		ForEachCopy([&](const Letters::Kind& a_kind, RE::TESObjectBOOK* a_book, RE::ExtraTextDisplayData* a_data, RE::ExtraDataList* a_list) {
			++seen;
			const std::string what = std::format("{} copy", a_kind.label);
			if (!a_data->ownerQuest || a_data->ownerInstance.underlying() < 0) {
				ReportOnce(std::format("nolink{:p}", static_cast<void*>(a_data)), std::format("  {}: no quest link on this copy (named by someone else, or already edited), left alone", what));
				return;
			}
			const Store::Key key{ a_book->GetFormID(), a_data->ownerQuest->GetFormID(), a_data->ownerInstance.underlying() };
			auto             entry = Store::Find(key);
			// alive: the quest is running and this copy belongs to an instance it still holds (not an older run of a quest that started again)
			const bool       alive = a_data->ownerQuest->IsRunning() && Letters::InstanceAlive(a_data->ownerQuest, key.instance);

			if (a_capture && alive) {
				if (!Letters::GroupOn(a_kind.group, settings)) {
					if (!entry) {
						ReportOnce(std::format("off{:08X}{:08X}{}", key.book, key.quest, key.instance), std::format("  {} from {}: group switched off, not named", what, a_data->ownerQuest->GetFormEditorID()));
						return;
					}
				} else {
					const auto parts = Letters::ReadParts(a_kind, a_data->ownerQuest, key.instance);
					const bool differs = !a_onlyUnknown && entry && (entry->sender != parts.sender || entry->subject != parts.subject);
					if (!entry || differs) {
						int         unresolved = 0;
						std::string text = Letters::ResolveText(a_book, a_data->ownerQuest, key.instance, unresolved);
						Store::Entry fresh{ a_kind.id, parts.sender, parts.subject, text, false };
						Store::Put(key, fresh);
						entry = fresh;
						Log::Event("  {} from {} (instance {}): saved sender '{}', subject '{}', {} characters of text{}", what,
							a_data->ownerQuest->GetFormEditorID(), key.instance, parts.sender, parts.subject, text.size(),
							unresolved ? std::format(" ({} fill-in(s) could not be filled)", unresolved) : std::string());
					}
				}
			}

			if (entry) {
				const auto* kind = Letters::KindById(entry->kindId);
				const auto  parts = Letters::Parts{ entry->sender, entry->subject };
				if (kind) {
					const auto before = std::string(a_data->displayName.c_str() ? a_data->displayName.c_str() : "");
					SetName(a_list, a_data, Letters::BuildName(*kind, parts, settings), what.c_str());
					if (before != a_data->displayName.c_str()) {
						++named;
					}
				}
			} else if (!alive) {
				// counted once per quest instance, summed below
				goneNew += g_reported.insert(std::format("gone{:08X}{:08X}{}", key.book, key.quest, key.instance)).second ? 1 : 0;
			}
		});
		if (goneNew) {
			Log::Event("  {} letter cop{} in your bag come{} from a quest that is no longer running, or from an older run of a quest that started again: the game has dropped what they need, so {} left as {} {}",
				goneNew, goneNew == 1 ? "y" : "ies", goneNew == 1 ? "s" : "", goneNew == 1 ? "it is" : "they are", goneNew == 1 ? "it" : "they", goneNew == 1 ? "was" : "were");
		}
		Log::Event("Inventory look ({}): {} letter cop{}, {} named or renamed", a_why, seen, seen == 1 ? "y" : "ies", named);
	}

	int RenameAll(const char* a_why)
	{
		const auto& settings = Settings::Get();
		int         count = 0;
		ForEachCopy([&](const Letters::Kind&, RE::TESObjectBOOK* a_book, RE::ExtraTextDisplayData* a_data, RE::ExtraDataList* a_list) {
			if (!a_data->ownerQuest || a_data->ownerInstance.underlying() < 0) {
				return;
			}
			const auto entry = Store::Find({ a_book->GetFormID(), a_data->ownerQuest->GetFormID(), a_data->ownerInstance.underlying() });
			if (!entry) {
				return;
			}
			if (const auto* kind = Letters::KindById(entry->kindId)) {
				SetName(a_list, a_data, Letters::BuildName(*kind, { entry->sender, entry->subject }, settings), "rename");
				++count;
			}
		});
		Log::Event("Rename letters ({}): {} remembered cop{} renamed under the current settings", a_why, count, count == 1 ? "y" : "ies");
		return count;
	}
}
