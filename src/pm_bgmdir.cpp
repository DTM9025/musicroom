// Touhou Project BGM Extractor
// ----------------------------
// pm_bgmdir.cpp - Parsing for BGM Directory games (only th06)
// ----------------------------
// "�" Nameless, 2010

#include "thbgmext.h"
#include "config.h"

void PM_BGMDir::ParseGameInfo(ConfigFile &NewGame, GameInfo *GI)
{
	NewGame.GetValue("game", "bgmdir", TYPE_STRING, (void*)TempStr);	GI->BGMDir = TempStr;	TempStr[0] = '\0';

	GI->BGMFile.clear();

	PMGame.Add(&GI);
}

bool PM_BGMDir::ParseTrackInfo(ConfigFile &NewGame, GameInfo *GI, const char* TN, TrackInfo *NewTrack)
{
	NewGame.GetValue(TN, "filename", TYPE_STRING, (void*)TempStr);	NewTrack->FN = TempStr;
	NewTrack->Start[0] = GI->HeaderSize;

	return true;	// Read position info from parsed file
}

// Scanning
// --------
GameInfo* PM_BGMDir::Scan(FXString& Path)
{
	FXString* Dirs = NULL;
	FXint DirCount = 0;

	ListEntry<GameInfo*>* CurGame = PMGame.First();
	while(CurGame)
	{
		DirCount = FXDir::listFiles(Dirs, ".", CurGame->Data->BGMDir, FXDir::NoFiles | FXDir::CaseFold | FXDir::HiddenDirs);
		SAFE_DELETE_ARRAY(Dirs);

		if(DirCount == 1)
		{
			GamePath += PATHSEP + CurGame->Data->BGMDir;
			return CurGame->Data;
		}
		CurGame = CurGame->Next();
	}
	return NULL;
}

// --------