// Music Room Interface
// --------------------
// parse.cpp - Main config file parsing and filename patterns
// --------------------
// "�" Nmlgc, 2010-2011

#include "musicroom.h"
#include <bgmlib/config.h>
#include <bgmlib/bgmlib.h>
#include <bgmlib/utils.h>
#include <bgmlib/ui.h>
#include <bgmlib/packmethod.h>
#include <FXPath.h>
#include "enc_custom.h"
#include "enc_vorbis.h"
#include "pm.h"
#include "parse.h"

// Personal appeal and track info wiki updating
bool GameInfo::ParseTrackDataEx(ConfigFile& NewGame)
{
	FXString Str, Key, Notice;
	ConfigParser* TS;
	bool Save = false;

	NewGame.LinkValue("game", "notice", TYPE_STRING, &Notice);
	NewGame.GetValue("game", "silencescan", TYPE_BOOL, &SilenceScan);

	if(WikiUpdate)
	{
		NewGame.LinkValue("update", "wikipage", TYPE_STRING, &WikiPage);
		NewGame.LinkValue("update", "wikirev", TYPE_ULONG, &WikiRev);
		Save = Update(this, WikiURL) || Save;
	}

	if(!Notice.empty())
	{
		BGMLib::UI_Notice(Notice);
		Notice.clear();
		Save = true;
	}

	if(Save)
	{
		ListEntry<TrackInfo>*	CurTrack = Track.First();
		// Link new supplementary comments
		while(CurTrack)
		{
			TrackInfo* NewTrack = &CurTrack->Data;
			TS = NewGame.FindSection(NewTrack->GetNumber());

			// It hurts me to write the same code twice, but this can't be helped...
			ListEntry<IntString>* NewCmt = NewTrack->Afterword.First();
			ushort Cmt = 2;

			while(NewCmt)
			{
				Key.format("comment%d_%s", Cmt, BGMLib::LI[LANG_JP].Code2);
				TS->LinkValue(Key, TYPE_STRING, &NewCmt->Data.s[LANG_JP], false);

				Key.replace(Key.length() - 2, 2, BGMLib::LI[LANG_EN].Code2.text(), 2);
				TS->LinkValue(Key, TYPE_STRING, &NewCmt->Data.s[LANG_EN], false);

				NewCmt = NewCmt->Next();
				Cmt++;
			}
			CurTrack = CurTrack->Next();
		}

		Str.format("Saving new track information in %s...", FXPath::name(NewGame.GetFN()));
		BGMLib::UI_Stat(Str);
		if(!NewGame.Save())
		{
			Str.format("\n%s file!\n", BGMLib::WriteError);
			BGMLib::UI_Stat(Str);
		}
		BGMLib::UI_Stat("done.\n");
	}

	return true;
}

// Filename Patterns
// -----------------
FXString FNPattern;
const FXString Token[] = {"number", "name_jp", "name_en", "name"};
const FXchar TokenDelim = '%';

FXString GetToken(ushort ID)
{
	FXString r;

	if(ID > ARRAYNUMBER(Token))	return "";

	r.format("%c%s%c", TokenDelim, Token[ID], TokenDelim);
	return r;
}

FXString PatternFN(TrackInfo* Track)
{
	if(!Track)	return "";

	FXString FN = FNPattern;

	FXString TrackNo;
	TrackNo.fromInt(Track->Number);
	if((Track->Number) < 10)	TrackNo.prepend('0');

	FN.substitute(GetToken(0), TrackNo);
	FN.substitute(GetToken(1), Track->Name[LANG_JP]);
	FN.substitute(GetToken(2), Track->Name[LANG_EN]);
	FN.substitute(GetToken(3), Track->Name[Lang]);

	// Remove illegal chars
	FN.substitute('/', '_');

	// People would be very angry if U.N. Owen doesn't appear
	removeSub(FN, "?");
	
	// Append encoder
	if(EncFmt > 0)
	{
		FN.append(".");
		FN.append(Encoders.Get(EncFmt - 1)->Data->Ext.lower());
	}

	return FN;
}
// -----------------

static void SetupEnc(ConfigFile* Cfg)
{
	ConfigParser* Enc;
	ListEntry<Encoder*>* New;

	if(Enc = Cfg->FindSection("enc_vorbis"))
	{
		New = Encoders.Add();

		New->Data = new Encoder_Vorbis;
		New->Data->ReadConfig(Enc);
	}
}

bool ReadConfig(ConfigFile* Cfg)
{
	ushort Index = 0;
	FXString Sect;
	ListEntry<Encoder*>* New;

	ConfigParser* Default, *Update, *CurEnc;

	if(!Cfg->Load())	return false;

	Default = Cfg->CheckSection("default");
	Update  = Cfg->CheckSection("update");

	Default->LinkValue("play", TYPE_BOOL, &Play);
	Default->LinkValue("showconsole", TYPE_BOOL, &ShowConsole);
	Default->LinkValue("removesilence", TYPE_BOOL, &SilRem);
	Default->LinkValue("fadealg", TYPE_USHORT, &FadeAlgID);
	Default->LinkValue("loop", TYPE_USHORT, &LoopCnt);
	Default->LinkValue("fade", TYPE_FLOAT, &FadeDur);
	Default->LinkValue("volume", TYPE_INT, &Volume);
	Default->LinkValue("enc", TYPE_USHORT, &EncFmt);
	Default->LinkValue("pattern", TYPE_STRING, &FNPattern);
	Default->LinkValue("outpath", TYPE_STRING, &OutPath);

	BGMLib::Init(Cfg, AppPath);

	if(Update = Cfg->FindSection("update"))
	{
		Update->LinkValue("wikiupdate", TYPE_BOOL, &WikiUpdate);
		Update->LinkValue("wikiurl", TYPE_STRING, &WikiURL);
	}

	// Native encoders
	SetupEnc(Cfg);

	// Custom encoders
	while(1)
	{
		Sect.format("enc%d", ++Index);
		CurEnc = Cfg->FindSection(Sect);
		if(!CurEnc)	break;
	
		New = Encoders.Add();

		New->Data = new Encoder_Custom;
		New->Data->ReadConfig(CurEnc);
	}
	return true;
}

// Invoke the pack methods... yeah, this is necessary after all
void SetupPM()
{
	PM_BGMDir::Inst();
	PM_BGMDat::Inst();
	PM_BMWav::Inst();
	PM_BMOgg::Inst();
	PM_PBG6::Inst();
	PM_TFPK::Inst();
	PM_TFCGA::Inst();
}
