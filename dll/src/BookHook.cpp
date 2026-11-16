#include "pch.h"

#include <MinHook.h>

#include <thread>

#include "BookHook.h"
#include "Letters.h"
#include "Log.h"
#include "Naming.h"
#include "Settings.h"
#include "Store.h"

namespace BookHook
{
	namespace
	{
		using OpenFn = void (*)(const RE::BSString&, const RE::ExtraDataList*, RE::TESObjectREFR*, RE::TESObjectBOOK*, const RE::NiPoint3&, const RE::NiMatrix3&, float, bool);
		OpenFn g_original = nullptr;

		bool LooksUnfilled(std::string_view a_text)
		{
			return a_text.find("[...]") != std::string_view::npos || a_text.find("<Alias") != std::string_view::npos;
		}

		std::atomic<std::uint64_t> g_opens{ 0 };  // counts book opens, so a late look knows whether it is still the same reading

		// After the engine filled in a letter whose quest is still alive, its text is the truth: remember it.
		// The screen fills a moment after the open call, so this is tried a few times. True when it is settled.
		bool RefreshFromScreen(const Store::Key& a_key, bool a_last)
		{
			const auto entry = Store::Find(a_key);
			const std::string shown = RE::BookMenu::GetDescription().c_str() ? RE::BookMenu::GetDescription().c_str() : "";
			if (!entry) {
				return true;
			}
			if (shown.empty() || LooksUnfilled(shown)) {
				if (a_last) {
					Log::Event("  reading screen: text not usable as the saved copy (empty or still has fill-in tags), keeping the fill-in made at pickup");
				}
				return false;
			}
			if (entry->text == shown && entry->exact) {
				return true;
			}
			auto updated = *entry;
			const bool changed = updated.text != shown;
			updated.text = shown;
			updated.exact = true;
			Store::Put(a_key, updated);
			Log::Event("  reading screen: saved the text the game shows{}", changed ? " (differs from the fill-in made at pickup)" : " (same as at pickup)");
			return true;
		}

		void LookLater(const Store::Key& a_key, std::uint64_t a_open, int a_attempt)
		{
			std::thread([a_key, a_open, a_attempt] {
				std::this_thread::sleep_for(std::chrono::milliseconds(400));
				SKSE::GetTaskInterface()->AddTask([a_key, a_open, a_attempt] {
					try {
						if (g_opens.load() != a_open) {
							return;  // another book was opened since
						}
						const auto ui = RE::UI::GetSingleton();
						if (!ui || !ui->IsMenuOpen(RE::BookMenu::MENU_NAME)) {
							return;
						}
						if (!RefreshFromScreen(a_key, a_attempt >= 4) && a_attempt < 4) {
							LookLater(a_key, a_open, a_attempt + 1);
						}
					} catch (...) {
					}
				});
			}).detach();
		}

		void Detour(const RE::BSString& a_text, const RE::ExtraDataList* a_list, RE::TESObjectREFR* a_ref, RE::TESObjectBOOK* a_book, const RE::NiPoint3& a_pos,
			const RE::NiMatrix3& a_rot, float a_scale, bool a_default)
		{
			std::optional<Store::Key> refresh;
			const auto                open = ++g_opens;
			try {
				const auto kind = a_book ? Letters::KindFor(a_book->GetFormID()) : nullptr;
				const auto link = kind ? Naming::QuestLink(a_list) : std::nullopt;
				if (link) {
					const Store::Key key{ a_book->GetFormID(), link->quest, link->instance };
					if (const auto entry = Store::Find(key)) {
						const bool alive = Letters::InstanceAlive(link->questForm, link->instance);
						if (alive) {
							refresh = key;  // the game can still fill it in; afterwards keep what it shows
						} else if (Settings::Get().keepText && !entry->text.empty()) {
							Log::Event("Book Menu: {} from {} (instance {}) - its quest no longer holds the names, showing the saved text ({} characters)", kind->label,
								link->questForm->GetFormEditorID(), link->instance, entry->text.size());
							const RE::BSString saved(entry->text);
							g_original(saved, a_list, a_ref, a_book, a_pos, a_rot, a_scale, a_default);
							return;
						} else {
							Log::Event("Book Menu: {} from {} (instance {}) - quest data gone and no saved text used (KeepText {})", kind->label,
								link->questForm->GetFormEditorID(), link->instance, Settings::Get().keepText ? "on, nothing saved" : "off");
						}
					}
				}
			} catch (const std::exception& e) {
				Log::Problem("Book Menu hook: {} (showing the game's own text)", e.what());
			}
			g_original(a_text, a_list, a_ref, a_book, a_pos, a_rot, a_scale, a_default);
			if (refresh) {
				LookLater(*refresh, open, 0);
			}
		}
	}

	bool Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(50122, 51053) };
		if (MH_Initialize() != MH_OK) {
			Log::Problem("Book Menu hook: MinHook did not start");
			return false;
		}
		if (MH_CreateHook(reinterpret_cast<LPVOID>(target.address()), reinterpret_cast<LPVOID>(&Detour), reinterpret_cast<LPVOID*>(&g_original)) != MH_OK ||
		    MH_EnableHook(reinterpret_cast<LPVOID>(target.address())) != MH_OK) {
			Log::Problem("Book Menu hook: could not hook the open call at +{:X}", target.offset());
			return false;
		}
		Log::Event("Book Menu hook installed at +{:X}", target.offset());
		return true;
	}
}
