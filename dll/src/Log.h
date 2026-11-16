#pragma once

// The mod's log: always on, written only when something happens (a letter seen, a decision, a
// failure), never per frame. Capped so a long session or a loop can't grow it: past the cap one line
// says so and the rest are dropped. A fresh file each launch (SKSE's logger does that).
namespace Log
{
	inline std::atomic<int> g_events{ 0 };
	inline std::atomic<int> g_problems{ 0 };
	constexpr int           kMaxEvents = 600;
	constexpr int           kMaxProblems = 100;

	// A decision or an outcome.
	template <class... Args>
	void Event(std::format_string<Args...> a_fmt, Args&&... a_args)
	{
		const int n = ++g_events;
		if (n <= kMaxEvents) {
			logs::info("{}", std::format(a_fmt, std::forward<Args>(a_args)...));
		} else if (n == kMaxEvents + 1) {
			logs::info("Log limit reached ({} lines); the rest of this session's decisions are not logged", kMaxEvents);
		}
	}

	// Something went wrong or looks wrong.
	template <class... Args>
	void Problem(std::format_string<Args...> a_fmt, Args&&... a_args)
	{
		const int n = ++g_problems;
		if (n <= kMaxProblems) {
			logs::warn("{}", std::format(a_fmt, std::forward<Args>(a_args)...));
		} else if (n == kMaxProblems + 1) {
			logs::warn("Problem log limit reached ({} lines); the rest are not logged", kMaxProblems);
		}
	}

	// One line, no raw newlines, cut at a_cap characters.
	inline std::string Flat(const std::string& a_text, std::size_t a_cap = 160)
	{
		std::string out;
		for (const char c : a_text) {
			if (c == '\r') {
				continue;
			}
			out += (c == '\n') ? std::string("\\n") : std::string(1, c);
			if (out.size() >= a_cap) {
				out += "...";
				break;
			}
		}
		return out;
	}
}
