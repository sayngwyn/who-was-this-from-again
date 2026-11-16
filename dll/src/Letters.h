#pragma once

#include "Settings.h"

// The shared radiant letters the mod names, and how a copy's name and text are worked out.
namespace Letters
{
	enum class Group
	{
		kInheritance,
		kBounties,
		kContracts,
		kNotes,
		kDawnguard
	};

	struct Kind
	{
		const char* id;       // stable id saved in the co-save
		const char* label;    // "Bounty"
		Group       group;
		const char* plugin;   // the BOOK's plugin
		RE::FormID  local;    // the BOOK's id in that plugin
		const char* sender;   // alias name, "hold-of:<alias>" (the hold that person is in), or nullptr
		const char* subject;  // alias name or nullptr
	};

	// What a letter says about who and where, as read from its quest.
	struct Parts
	{
		std::string sender;
		std::string subject;
	};

	// Looks up all BOOKs once the game's data is loaded (Dawnguard's ids depend on the load order).
	void Resolve();

	// The kind for a BOOK's FormID, or nullptr.
	const Kind* KindFor(RE::FormID a_book);
	const Kind* KindById(const std::string& a_id);

	bool GroupOn(Group a_group, const Settings& a_settings);

	// Reads sender and subject from the quest a copy belongs to. a_complete is false when a part the kind
	// should have could not be read (the name then simply leaves it out).
	Parts ReadParts(const Kind& a_kind, RE::TESQuest* a_quest, std::int32_t a_instance);

	// Whether the quest still holds the stored alias text for this instance (the engine fills the letter's
	// fill-ins from it; once it is gone the letter reads "[...]").
	bool InstanceAlive(RE::TESQuest* a_quest, std::int32_t a_instance);

	// The book's text with the quest's fill-ins put in. a_unresolved counts tags that could not be filled.
	std::string ResolveText(RE::TESObjectBOOK* a_book, RE::TESQuest* a_quest, std::int32_t a_instance, int& a_unresolved);

	// The name for a copy under the current settings.
	std::string BuildName(const Kind& a_kind, const Parts& a_parts, const Settings& a_settings);
}
