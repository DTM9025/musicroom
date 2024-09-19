// Music Room Interface
// --------------------
// musicroom.cpp - Main starting code
// --------------------
// "©" Nmlgc, 2010-2011
// "©" DTM9025, 2024

#include "musicroom.h"
#include <math.h>
#include <fx.h>
#include "mainwnd.h"
#include <bgmlib/config.h>
#include <bgmlib/bgmlib.h>
#include <th_tool_shared/utils.h>

#ifdef PROFILING_LIBS
LARGE_INTEGER operator - (const LARGE_INTEGER& a, const LARGE_INTEGER& b)	{LARGE_INTEGER r; r.QuadPart = a.QuadPart - b.QuadPart; return r;}
LARGE_INTEGER operator + (const LARGE_INTEGER& a, const LARGE_INTEGER& b)	{LARGE_INTEGER r; r.QuadPart = a.QuadPart + b.QuadPart; return r;}
#endif

bool ReadConfig(ConfigFile* Cfg);
void SetupPM();

// Globals
// =======

ConfigFile MainCFG;
ConfigParser* LGD;	// Local Game Directory section in [LGDFile]

// GUI
// ---
MainWnd* MWBack;	// Back end GUI class
bool Play;	// Play selected track?
ushort FadeAlgID = 0;	// Fade algorithm
bool SilRem = true;	// Remove opening silence?
bool RemEnabled = true;
bool SilResolve() { return SilRem && RemEnabled; }
int Volume = 100;
// ---

// Game
// ----
GameInfo* ActiveGame = NULL;
// ----

// [update]
// --------
bool WikiUpdate;
FXString WikiURL;
// --------

// Encoders
// --------
List<Encoder*> Encoders;
FXushort EncFmt;
bool ShowConsole; // Show encoding console during the process
// --------

ushort LoopCnt;	// Song loop count (2 = song gets repeated once)
float FadeDur;	// Fade duration
FXString AppPath;
FXString OutPath;	// Output directory

// =======

// String Constants
      FXString PrgName = "Music Room Interface";
	  FXString WebPage = "https://github.com/DTM9025/musicroom";
const FXString PrgVer = "v2.6.0";
const FXString NoGame = "(no game loaded)";
      FXString CfgFile = "musicroom.cfg";
	  FXString LGDFile = "gamedirs.cfg";
const FXString Example = "Example: ";
const FXString DumpFile = "extract";
const FXString DecodeFile = DumpFile + ".raw";
      FXString OggDumpFile = "decode.ogg";
	  FXString OggPlayFile = "play.ogg";
const FXString Cmp[LANG_COUNT] = {L"作曲者", "Composer"};

extern const unsigned char ICON_AKYU[];

int main(int argc, char* argv[])
{
	FXIcon* AppIcon;

	SetupPM();

	// Setup directories
	// -----------------
	AppPath = CorrectAppPath(argv[0]);
	
	OggDumpFile.prepend(FXSystem::getTempDirectory() + SlashString);
	OggPlayFile.prepend(FXSystem::getTempDirectory() + SlashString);
	CfgFile.prepend(AppPath);
	LGDFile.prepend(AppPath);
	// -----------------

	// Read config files
	// -----------------
	ConfigFile MainCFG(CfgFile);
	ReadConfig(&MainCFG);

	ConfigFile LocalGameDir(LGDFile);
	LocalGameDir.Load();
	LGD = LocalGameDir.CheckSection("gamedirs");
	// -----------------

	FXApp App(PrgName, "DTM9025");
	App.init(argc, argv);

	PrgName += " " + PrgVer;

	AppIcon = new FXICOIcon(&App, ICON_AKYU);

	MWBack = new MainWnd(&App, AppIcon);
	MW = &MainWndFront::Inst();
	App.create();

	MWBack->show(PLACEMENT_SCREEN);

	if(argc > 1)	MWBack->handle(NULL, FXSEL(SEL_COMMAND, MainWnd::MW_LOAD_GAME), argv[1]);

	FXint Ret = App.run();

	// -------
	// Cleanup
	// -------

	LocalGameDir.Save();
	LocalGameDir.Clear();

	MainCFG.Save();
	MainCFG.Clear();

	// Clear encoders
	ListEntry<Encoder*>* CurEnc = Encoders.First();
	while(CurEnc)
	{
		SAFE_DELETE(CurEnc->Data);
		CurEnc = CurEnc->Next();
	}
	Encoders.Clear();

	MW->Clear();

	BGMLib::Clear();

	SAFE_DELETE(AppIcon);

#ifdef WIN32
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
#endif
	return Ret;
}
