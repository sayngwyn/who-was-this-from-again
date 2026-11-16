Scriptname WhoWasThisFromAgain_MCM extends MCM_ConfigBase
{The Who Was This From Again menu (MCM Helper). Every setting lives in
Data\MCM\Settings\WhoWasThisFromAgain.ini, which the DLL reads; this script only tells the DLL to read it again.}

Event OnSettingChange(string a_ID)
	WhoWasThisFromAgain.ReloadSettings()
EndEvent

Event OnConfigClose()
	WhoWasThisFromAgain.ReloadSettings()
EndEvent

; The "Rename letters I have" button.
Function RenameLetters()
	Int renamed = WhoWasThisFromAgain.RenameLetters()
	If renamed == 1
		ShowMessage("1 letter you carry was renamed.", false)
	Else
		ShowMessage(renamed + " letters you carry were renamed.", false)
	EndIf
EndFunction
