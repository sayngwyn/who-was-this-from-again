#include "pch.h"

#include "Letters.h"
#include "Log.h"

namespace Letters
{
	namespace
	{
		using G = Group;

		// Alias names are those of the quests in Skyrim.esm / Dawnguard.esm (checked against the loaded game, see
		// reports\2026-11-10-build\RESULT.md). Alias names are matched without regard to case.
		constexpr Kind kKinds[] = {
			{ "inheritance", "Inheritance", G::kInheritance, "Skyrim.esm", 0x01BFF5, "hold-of:Jarl", "DeadFriend" },
			{ "friend", "Letter from a Friend", G::kNotes, "Skyrim.esm", 0x023EE5, nullptr, "Dungeon" },
			{ "bounty", "Bounty", G::kBounties, "Skyrim.esm", 0x095129, "Hold", "BountyLocation" },
			{ "bounty", "Bounty", G::kBounties, "Skyrim.esm", 0x0BD780, "Hold", "BountyLocation" },
			{ "bounty", "Bounty", G::kBounties, "Skyrim.esm", 0x0BD791, "Hold", "BountyLocation" },
			{ "bounty", "Bounty", G::kBounties, "Skyrim.esm", 0x0CBA9B, "Hold", "BountyLocation" },
			{ "contract", "Contract", G::kContracts, "Skyrim.esm", 0x04EF00, "Relative", "Victim" },
			{ "theft", "Theft Contract", G::kContracts, "Skyrim.esm", 0x035B65, "owner", nullptr },
			{ "hunters", "Bounty", G::kBounties, "Skyrim.esm", 0x0BC0A2, "myHoldLocation", nullptr },
			{ "courier", "Courier's Letter", G::kNotes, "Skyrim.esm", 0x1065F5, nullptr, "Dungeon" },
			{ "treasure", "Treasure Hunter's Note", G::kNotes, "Skyrim.esm", 0x0A17B0, nullptr, "Dungeon" },
			{ "request", "Request", G::kNotes, "Skyrim.esm", 0x039FC3, "questgiver", "Victim" },
			{ "calcelmo", "Letter from Calcelmo", G::kNotes, "Skyrim.esm", 0x0A0F46, nullptr, "Item" },
			{ "urag", "Urag's Note", G::kNotes, "Skyrim.esm", 0x03D29D, nullptr, "Book" },
			{ "orders", "Orders", G::kDawnguard, "Dawnguard.esm", 0x007ECB, "Capital", nullptr },
			{ "vampire", "Letter from the Vampire", G::kDawnguard, "Dawnguard.esm", 0x006956, nullptr, "Dungeon" },
			{ "incriminating", "Incriminating Letter", G::kDawnguard, "Dawnguard.esm", 0x0050CA, nullptr, "City" },
			{ "writ", "Assassin's Writ", G::kDawnguard, "Dawnguard.esm", 0x01AA22, nullptr, "TargetDungeon" },
		};
		constexpr std::size_t kCount = sizeof(kKinds) / sizeof(kKinds[0]);

		std::map<RE::FormID, std::size_t> g_byBook;

		bool IEquals(std::string_view a, std::string_view b)
		{
			return a.size() == b.size() &&
			       std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); });
		}

		RE::BGSBaseAlias* FindAlias(RE::TESQuest* a_quest, std::string_view a_name)
		{
			for (const auto alias : a_quest->aliases) {
				if (alias && IEquals(alias->aliasName.c_str(), a_name)) {
					return alias;
				}
			}
			return nullptr;
		}

		// The form the quest stored for an alias when this instance started ("Stores Text").
		RE::TESForm* StoredForm(RE::TESQuest* a_quest, std::int32_t a_instance, std::uint32_t a_aliasID)
		{
			for (const auto data : a_quest->instanceData) {
				if (!data || static_cast<std::int32_t>(data->id) != a_instance) {
					continue;
				}
				for (const auto& s : data->stringData) {
					if (s.aliasID == a_aliasID && s.fullNameFormID) {
						return RE::TESForm::LookupByID(s.fullNameFormID);
					}
				}
			}
			return nullptr;
		}

		struct AliasInfo
		{
			std::string name;
			RE::TESNPC* npc = nullptr;
			bool        ok = false;
		};

		std::string NameOf(RE::TESForm* a_form)
		{
			if (!a_form) {
				return {};
			}
			if (const auto ref = a_form->As<RE::TESObjectREFR>()) {
				const char* n = ref->GetDisplayFullName();
				return n ? n : "";
			}
			const char* n = a_form->GetName();
			return n ? n : "";
		}

		RE::TESNPC* NpcOf(RE::TESForm* a_form)
		{
			if (!a_form) {
				return nullptr;
			}
			if (const auto ref = a_form->As<RE::TESObjectREFR>()) {
				const auto base = ref->GetBaseObject();
				return base ? base->As<RE::TESNPC>() : nullptr;
			}
			return a_form->As<RE::TESNPC>();
		}

		AliasInfo Lookup(RE::TESQuest* a_quest, std::int32_t a_instance, std::string_view a_alias)
		{
			AliasInfo info;
			if (IEquals(a_alias, "Player")) {
				const auto player = RE::PlayerCharacter::GetSingleton();
				info.name = player->GetName() ? player->GetName() : "";
				info.npc = player->GetActorBase();
				info.ok = !info.name.empty();
				return info;
			}
			const auto alias = FindAlias(a_quest, a_alias);
			if (!alias) {
				return info;
			}
			RE::TESForm* form = StoredForm(a_quest, a_instance, alias->aliasID);
			if (!form) {
				// Not stored (the alias does not "Store Text"): the live fill, if the quest has one.
				if (const auto ref = a_quest->GetAliasedRef(alias->aliasID).get()) {
					form = ref.get();
				}
			}
			info.name = NameOf(form);
			info.npc = NpcOf(form);
			info.ok = !info.name.empty();
			return info;
		}

		// The hold a person is in: from where they stand, up the location tree to the first location marked as a hold.
		std::string HoldOf(RE::TESQuest* a_quest, std::string_view a_alias)
		{
			const auto alias = FindAlias(a_quest, a_alias);
			if (!alias) {
				return {};
			}
			const auto ref = a_quest->GetAliasedRef(alias->aliasID).get();
			if (!ref) {
				return {};
			}
			RE::BGSLocation* loc = ref->GetCurrentLocation();
			RE::BGSLocation* last = nullptr;
			std::string      chain;
			for (int guard = 0; loc && guard < 8; loc = loc->parentLoc, ++guard) {
				chain += std::format("{}{}", chain.empty() ? "" : " > ", loc->GetName());
				if (loc->HasKeywordString("LocTypeHold")) {
					Log::Event("  hold of {}: {} (location chain {})", a_alias, loc->GetName(), chain);
					return loc->GetName();
				}
				last = loc;
			}
			if (last) {
				Log::Event("  hold of {}: no hold-marked location in the chain ({}), using {}", a_alias, chain, last->GetName());
				return last->GetName();
			}
			return {};
		}

		std::string Pronoun(const AliasInfo& a_info, std::string_view a_mod)
		{
			const bool female = a_info.npc && a_info.npc->GetSex() == RE::SEX::kFemale;
			if (IEquals(a_mod, "PronounSub")) {
				return female ? "she" : "he";
			}
			if (IEquals(a_mod, "PronounObj")) {
				return female ? "her" : "him";
			}
			if (IEquals(a_mod, "PronounPos")) {
				return female ? "her" : "his";
			}
			if (IEquals(a_mod, "PronounPosObj")) {
				return female ? "hers" : "his";
			}
			if (IEquals(a_mod, "PronounCap")) {
				return female ? "She" : "He";
			}
			return {};
		}
	}

	void Resolve()
	{
		g_byBook.clear();
		const auto data = RE::TESDataHandler::GetSingleton();
		for (std::size_t i = 0; i < kCount; ++i) {
			const auto form = data->LookupForm(kKinds[i].local, kKinds[i].plugin);
			if (!form || !form->As<RE::TESObjectBOOK>()) {
				Log::Event("Letter kind {} ({} {:06X}): not in this load order, skipped", kKinds[i].id, kKinds[i].plugin, kKinds[i].local);
				continue;
			}
			g_byBook[form->GetFormID()] = i;
		}
		Log::Event("Letter books found in this load order: {} of {}", g_byBook.size(), kCount);
	}

	const Kind* KindFor(RE::FormID a_book)
	{
		const auto it = g_byBook.find(a_book);
		return it == g_byBook.end() ? nullptr : &kKinds[it->second];
	}

	const Kind* KindById(const std::string& a_id)
	{
		for (const auto& k : kKinds) {
			if (a_id == k.id) {
				return &k;
			}
		}
		return nullptr;
	}

	bool GroupOn(Group a_group, const Settings& a_settings)
	{
		switch (a_group) {
		case Group::kInheritance: return a_settings.inheritance;
		case Group::kBounties: return a_settings.bounties;
		case Group::kContracts: return a_settings.contracts;
		case Group::kNotes: return a_settings.notes;
		case Group::kDawnguard: return a_settings.dawnguard;
		}
		return true;
	}

	Parts ReadParts(const Kind& a_kind, RE::TESQuest* a_quest, std::int32_t a_instance)
	{
		Parts parts;
		if (a_kind.sender) {
			const std::string_view spec = a_kind.sender;
			if (spec.starts_with("hold-of:")) {
				parts.sender = HoldOf(a_quest, spec.substr(8));
			} else {
				parts.sender = Lookup(a_quest, a_instance, spec).name;
			}
			if (parts.sender.empty()) {
				Log::Event("  sender ({}) could not be read from {}, left out of the name", spec, a_quest->GetFormEditorID());
			}
		}
		if (a_kind.subject) {
			parts.subject = Lookup(a_quest, a_instance, a_kind.subject).name;
			if (parts.subject.empty()) {
				Log::Event("  subject ({}) could not be read from {}, left out of the name", a_kind.subject, a_quest->GetFormEditorID());
			}
		}
		return parts;
	}

	bool InstanceAlive(RE::TESQuest* a_quest, std::int32_t a_instance)
	{
		for (const auto data : a_quest->instanceData) {
			if (data && static_cast<std::int32_t>(data->id) == a_instance) {
				return true;
			}
		}
		return false;
	}

	std::string ResolveText(RE::TESObjectBOOK* a_book, RE::TESQuest* a_quest, std::int32_t a_instance, int& a_unresolved)
	{
		a_unresolved = 0;
		RE::BSString raw;
		a_book->GetDescription(raw, nullptr);
		const std::string_view text = raw.c_str() ? raw.c_str() : "";
		std::string            out;
		out.reserve(text.size() + 64);
		for (std::size_t i = 0; i < text.size();) {
			if (text[i] == '<') {
				const auto end = text.find('>', i);
				if (end != std::string_view::npos) {
					const auto tag = text.substr(i + 1, end - i - 1);
					if (tag.size() > 7 && IEquals(tag.substr(0, 7), "Global=")) {
						// A number the quest keeps for this instance (the inheritance and its tax).
						const auto name = tag.substr(7);
						bool       filled = false;
						if (a_quest->textGlobals) {
							for (const auto global : *a_quest->textGlobals) {
								if (!global || !global->GetFormEditorID() || !IEquals(global->GetFormEditorID(), name)) {
									continue;
								}
								for (const auto data : a_quest->instanceData) {
									if (!data || static_cast<std::int32_t>(data->id) != a_instance) {
										continue;
									}
									for (const auto& v : data->valueData) {
										if (v.global == global) {
											out += std::format("{}", static_cast<std::int64_t>(std::llround(v.value)));  // the game shows whole numbers
											filled = true;
										}
									}
								}
							}
						}
						if (filled) {
							i = end + 1;
							continue;
						}
						++a_unresolved;
						Log::Event("  text tag <{}> could not be filled (quest {}), left as written", tag, a_quest->GetFormEditorID());
					} else if (tag.size() > 5 && IEquals(tag.substr(0, 5), "Alias")) {
						const auto eq = tag.find('=');
						if (eq != std::string_view::npos && (tag[5] == '=' || tag[5] == '.')) {
							const std::string_view mod = tag[5] == '.' ? tag.substr(6, eq - 6) : std::string_view{};
							const auto             info = Lookup(a_quest, a_instance, tag.substr(eq + 1));
							std::string            value;
							if (info.ok || IEquals(tag.substr(eq + 1), "Player")) {
								if (mod.empty()) {
									value = info.name;
								} else if (IEquals(mod, "ShortName")) {
									value = (info.npc && info.npc->shortName.size()) ? info.npc->shortName.c_str() : info.name;
								} else if (IEquals(mod, "Race")) {
									value = (info.npc && info.npc->GetRace() && info.npc->GetRace()->GetName()) ? info.npc->GetRace()->GetName() : "";
								} else {
									value = Pronoun(info, mod);
								}
							}
							if (!value.empty()) {
								out += value;
								i = end + 1;
								continue;
							}
							++a_unresolved;
							Log::Event("  text tag <{}> could not be filled (quest {}), left as written", tag, a_quest->GetFormEditorID());
						}
					}
				}
			}
			out += text[i++];
		}
		return out;
	}

	std::string BuildName(const Kind& a_kind, const Parts& a_parts, const Settings& a_settings)
	{
		const std::string  label = a_kind.label;
		const std::string& sender = a_settings.showSender ? a_parts.sender : std::string();
		const std::string& subject = a_settings.showSubject ? a_parts.subject : std::string();
		std::string        name;
		switch (a_settings.style) {
		case Settings::Style::kSubjectFirst:
			name = subject.empty() ? label : subject + " - " + label;
			if (!sender.empty()) {
				name += " (" + sender + ")";
			}
			break;
		case Settings::Style::kShort:
			name = subject.empty() ? label : label + ": " + subject;
			break;
		case Settings::Style::kKindFirst:
		default:
			name = label;
			if (!sender.empty()) {
				name += " from " + sender;
			}
			if (!subject.empty()) {
				name += ": " + subject;
			}
			break;
		}
		return name;
	}
}
