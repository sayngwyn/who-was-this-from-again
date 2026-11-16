#pragma once

// Settings. The defaults file ships with the mod (Data\MCM\Config\WhoWasThisFromAgain\settings.ini);
// the player's own values, written by the MCM Helper menu or typed by hand, live in
// Data\MCM\Settings\WhoWasThisFromAgain.ini and win over the defaults. Read at load and again whenever
// the menu changes a setting (the menu calls ReloadSettings).
struct Settings
{
	enum class Style
	{
		kKindFirst = 0,  // Bounty from Whiterun: Halted Stream Camp
		kSubjectFirst = 1,  // Halted Stream Camp - Bounty (Whiterun)
		kShort = 2  // Bounty: Halted Stream Camp
	};

	bool  inheritance = true;
	bool  bounties = true;
	bool  contracts = true;
	bool  notes = true;
	bool  dawnguard = true;
	bool  showSender = true;
	bool  showSubject = true;
	Style style = Style::kKindFirst;
	bool  keepText = true;
	bool  reapplyOnLoad = false;

	static Settings& Get()
	{
		static Settings instance;
		return instance;
	}

	// Reads the defaults, then the player's file over them, and logs the result.
	void Load(const char* a_why);
	std::string Summary() const;
};
