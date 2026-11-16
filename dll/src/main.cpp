// Who Was This From Again: gives the shared radiant letters their own names and keeps their text.
#include "pch.h"

#include "BookHook.h"
#include "Letters.h"
#include "Log.h"
#include "Naming.h"
#include "Papyrus.h"
#include "Settings.h"
#include "Store.h"

namespace
{
	class ContainerSink : public RE::BSTEventSink<RE::TESContainerChangedEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override
		{
			if (a_event) {
				try {
					Naming::OnContainerChanged(*a_event);
				} catch (...) {
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};
	ContainerSink g_sink;

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			Letters::Resolve();
			RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESContainerChangedEvent>(&g_sink);
			if (!BookHook::Install()) {
				Log::Problem("Letters will still be named, but saved text will not be shown when you read them");
			}
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			SKSE::GetTaskInterface()->AddTask([] {
				Log::Event("Game loaded: {} letter(s) remembered in this save", Store::Size());
				// letters carried when the mod was installed (or saved without it): name the ones whose quest is still running, as a pickup would
				Naming::ProcessInventory("game loaded", true, true);
				if (Settings::Get().reapplyOnLoad) {
					Naming::RenameAll("game loaded (bReapplyOnLoad)");
				}
			});
			break;
		default:
			break;
		}
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	logs::info("Who Was This From Again {} loaded, game {}", SKSE::PluginDeclaration::GetSingleton()->GetVersion().string("."), a_skse->RuntimeVersion().string("."));
	Settings::Get().Load("game start");
	Papyrus::Install();

	const auto serial = SKSE::GetSerializationInterface();
	serial->SetUniqueID('WWTF');
	serial->SetSaveCallback(Store::Save);
	serial->SetLoadCallback(Store::Load);
	serial->SetRevertCallback(Store::Revert);

	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
