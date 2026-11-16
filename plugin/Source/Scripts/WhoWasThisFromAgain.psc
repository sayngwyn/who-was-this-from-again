Scriptname WhoWasThisFromAgain Hidden
{Natives of the Who Was This From Again SKSE plugin (WhoWasThisFromAgain.dll). Called by the menu script.}

; Reads the settings files again (the DLL also reads them at game start).
Function ReloadSettings() global native

; Reloads the settings, then gives every remembered letter you carry the name the current settings ask for.
; Returns how many letters were renamed.
Int Function RenameLetters() global native

; How many letters the current save remembers.
Int Function LettersSaved() global native

String Function GetVersion() global native
