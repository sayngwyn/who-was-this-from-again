#pragma once

namespace Naming
{
	// A container-changed event (any thread): schedules a look at the player's inventory when one of our
	// letters moved to the player.
	void OnContainerChanged(const RE::TESContainerChangedEvent& a_event);

	// Looks at every letter copy the player carries (main thread). a_capture: read new copies from their
	// quests and remember them; otherwise only put back names for copies already remembered. a_onlyUnknown (a save
	// was loaded): capture only copies the mod has no record of, never re-read the ones it already remembers.
	void ProcessInventory(const char* a_why, bool a_capture, bool a_onlyUnknown = false);

	// Gives every remembered copy the player carries the name the current settings ask for. Returns how many.
	int RenameAll(const char* a_why);

	// Remembered text for a copy opened in the Book Menu: returns the entry for the extra list, if any.
	struct Seen
	{
		RE::FormID   quest = 0;
		std::int32_t instance = -1;
		RE::TESQuest* questForm = nullptr;
	};
	std::optional<Seen> QuestLink(const RE::ExtraDataList* a_list);
}
