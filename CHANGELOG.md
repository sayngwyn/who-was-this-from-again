# Changelog

## 1.0.0

First release.

- Gives each copy of the shared courier letters, bounty notices and contracts its own name when it reaches your bag: kind, who it is from where the game knows it, and what it is about. For example "Bounty from Whiterun: Halted Stream Camp" and "Inheritance from Whiterun: Alvor".
- Keeps each letter's text. The base game turns a letter's text into "[...]" once its quest ends; the mod saves the text when you pick the letter up and shows it when you read it.
- Fifteen kinds of letter: Letter of Inheritance, Letter from a Friend, Bounty (from jarls, stewards and innkeepers), Contract, Theft Contract, Bounty (carried by bounty hunters), Courier's Letter, Treasure Hunter's Note, Request, Letter from Calcelmo, Urag's Note, and the Dawnguard letters (Orders, Letter from the Vampire, Incriminating Letter, Assassin's Writ).
- Settings in an ini that works for everyone, and a menu for SkyUI users (needs MCM Helper): switch letter groups on or off, show or hide the sender and the subject, pick one of three name styles, switch text keeping off, and rename the letters you already carry.
- No vanilla record is edited, so it works beside mods that rewrite letter text. Safe to add mid-game.
- Letters you already carry when you add the mod: the next time a save loads, the ones whose quest is still running are named and keep their text; the ones whose quest has already ended stay as they were (the game has dropped their details), and the log counts them.
- Removing the mod from a save in progress has not been tested yet. Keep a save from before you added it.
- One DLL for Skyrim SE 1.5.97 and AE 1.6.640, 1.6.1170 and 1.7.104 (Address Library). On 1.6.640 MCM Helper does not run, so settings there are in the ini.
- The log (`Documents\My Games\Skyrim Special Edition\SKSE\WhoWasThisFromAgain.log`) says the version, your game version and settings, and every letter it named or skipped, and why.
