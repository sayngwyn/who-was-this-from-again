#include "pch.h"

#include "Letters.h"
#include "Log.h"
#include "Naming.h"
#include "Papyrus.h"
#include "Settings.h"
#include "Store.h"

namespace
{
	constexpr auto kScript = "WhoWasThisFromAgain";

	// The menu calls this after it changes a setting (the player's file is already written then).
	void ReloadSettings(RE::StaticFunctionTag*)
	{
		Settings::Get().Load("menu changed a setting");
	}

	// The menu's "Rename letters I have" button: reload, then give every remembered copy in the bag the current name.
	std::int32_t RenameLetters(RE::StaticFunctionTag*)
	{
		Settings::Get().Load("rename letters I have");
		return Naming::RenameAll("button");
	}

	std::int32_t LettersSaved(RE::StaticFunctionTag*) { return static_cast<std::int32_t>(Store::Size()); }

	RE::BSFixedString GetVersion(RE::StaticFunctionTag*)
	{
		return SKSE::PluginDeclaration::GetSingleton()->GetVersion().string(".");
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("ReloadSettings", kScript, ReloadSettings);
		a_vm->RegisterFunction("RenameLetters", kScript, RenameLetters);
		a_vm->RegisterFunction("LettersSaved", kScript, LettersSaved);
		a_vm->RegisterFunction("GetVersion", kScript, GetVersion);
		return true;
	}
}

void Papyrus::Install()
{
	SKSE::GetPapyrusInterface()->Register(Register);
}
