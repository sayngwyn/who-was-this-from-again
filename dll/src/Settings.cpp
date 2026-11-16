#include "pch.h"

#include "Log.h"
#include "Settings.h"

namespace
{
	constexpr auto kDefaults = "Data\\MCM\\Config\\WhoWasThisFromAgain\\settings.ini";
	constexpr auto kUser = "Data\\MCM\\Settings\\WhoWasThisFromAgain.ini";

	using Ini = std::map<std::string, std::string>;  // "section|key" (lower case) -> value

	std::string Lower(std::string a_text)
	{
		std::transform(a_text.begin(), a_text.end(), a_text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return a_text;
	}

	std::string Trim(const std::string& a_text)
	{
		const auto first = a_text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) {
			return {};
		}
		return a_text.substr(first, a_text.find_last_not_of(" \t\r\n") - first + 1);
	}

	// A small ini reader: [Section], key=value, ';' and '#' comment lines. Later files overwrite earlier values.
	bool ReadIni(const char* a_path, Ini& a_into)
	{
		std::ifstream in(a_path);
		if (!in) {
			return false;
		}
		std::string line;
		std::string section;
		bool first = true;
		while (std::getline(in, line)) {
			if (first && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
				line.erase(0, 3);  // MCM Helper writes a UTF-8 byte order mark
			}
			first = false;
			line = Trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') {
				continue;
			}
			if (line[0] == '[') {
				const auto end = line.find(']');
				section = Lower(Trim(line.substr(1, end == std::string::npos ? std::string::npos : end - 1)));
				continue;
			}
			const auto eq = line.find('=');
			if (eq == std::string::npos) {
				continue;
			}
			a_into[section + "|" + Lower(Trim(line.substr(0, eq)))] = Trim(line.substr(eq + 1));
		}
		return true;
	}

	bool Bool(const Ini& a_ini, const char* a_section, const char* a_key, bool a_default)
	{
		const auto it = a_ini.find(std::string(a_section) + "|" + Lower(a_key));
		if (it == a_ini.end()) {
			return a_default;
		}
		const auto v = Lower(it->second);
		if (v == "1" || v == "true" || v == "yes" || v == "on") {
			return true;
		}
		if (v == "0" || v == "false" || v == "no" || v == "off") {
			return false;
		}
		Log::Problem("Setting {}={} is not a yes/no value: using {}", a_key, it->second, a_default ? "on" : "off");
		return a_default;
	}

	int Int(const Ini& a_ini, const char* a_section, const char* a_key, int a_default)
	{
		const auto it = a_ini.find(std::string(a_section) + "|" + Lower(a_key));
		if (it == a_ini.end()) {
			return a_default;
		}
		char* end = nullptr;
		const long v = std::strtol(it->second.c_str(), &end, 10);
		if (end == it->second.c_str()) {
			Log::Problem("Setting {}={} is not a number: using {}", a_key, it->second, a_default);
			return a_default;
		}
		return static_cast<int>(v);
	}
}

void Settings::Load(const char* a_why)
{
	Ini        ini;
	const bool haveDefaults = ReadIni(kDefaults, ini);
	const bool haveUser = ReadIni(kUser, ini);

	inheritance = Bool(ini, "groups", "bInheritance", true);
	bounties = Bool(ini, "groups", "bBounties", true);
	contracts = Bool(ini, "groups", "bContracts", true);
	notes = Bool(ini, "groups", "bNotes", true);
	dawnguard = Bool(ini, "groups", "bDawnguard", true);
	showSender = Bool(ini, "display", "bShowSender", true);
	showSubject = Bool(ini, "display", "bShowSubject", true);
	keepText = Bool(ini, "display", "bKeepText", true);
	reapplyOnLoad = Bool(ini, "display", "bReapplyOnLoad", false);
	int s = Int(ini, "display", "iStyle", 0);
	if (s < 0 || s > 2) {
		Log::Problem("Setting iStyle={} is outside 0 to 2: using 0", s);
		s = 0;
	}
	style = static_cast<Style>(s);

	Log::Event("Settings ({}): defaults file {}, your file {}; {}", a_why, haveDefaults ? "found" : "MISSING (built-in defaults used)",
		haveUser ? "found" : "not there (defaults only)", Summary());
}

std::string Settings::Summary() const
{
	return std::format("groups inheritance={} bounties={} contracts={} notes={} dawnguard={}; sender={} subject={} style={} keepText={} reapplyOnLoad={}",
		inheritance, bounties, contracts, notes, dawnguard, showSender, showSubject, static_cast<int>(style), keepText, reapplyOnLoad);
}
