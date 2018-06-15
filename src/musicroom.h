// Music Room Interface
// --------------------
// "©" Nmlgc, 2010-2011

#ifndef MUSICROOM_MUSICROOM_H
#define MUSICROOM_MUSICROOM_H

#include <bgmlib/platform.h>

#ifdef _DEBUG
// Enable memory leak display
#include <iostream>
#include <crtdbg.h>
// #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
// #define new DEBUG_NEW
#endif

#include <bgmlib/infostruct.h>
#include "enc_base.h"

namespace FX
{
	class FXApp;
	class FXMainWindow;
	class FXMessageChannel;
	class FXThread;
	class FXFont;
}

#define MSG_FUNC(x) long x(FXObject*, FXSelector, void*)

class MainWndFront
{
protected:
	FXMessageChannel* MsgChan;

	MainWndFront();

public:
	FXMainWindow* MW;

	void ShowNotice(const FXString& Notice);
	void ProgConnect(volatile FXulong* Var = NULL, FXuint Max = 0);	// Connects any variable to the progress bar at the bottom. Call again with [Var = NULL] to unresolve.
	FXApp* getApp();

	void ActFinish();

	// Shows a message box with [opts] from a thread. Return value gets saved in [Ret].
	void ThreadMsg(FXThread* Thread, const FXString& Str, volatile FXuint* Ret, FXuint opts);

	void Clear();
	~MainWndFront();

	static MainWndFront& Inst()
	{
		static MainWndFront Inst;
		return Inst;
	}
};

class StreamerFront
{
public:
	bool Init(void* xid);

	TrackInfo*	CurTrack();
	ulong	Pos();
	void RequestTrackSwitch(TrackInfo* NewTrack);

	void Play();
	void Stop();	// Waits with returning until thread is done!

	void CloseFile();
	void Exit();

	SINGLETON(StreamerFront);
};

// Globals
// =======

// GUI
// ---
extern MainWndFront* MW;
extern bool Play;	// Play selected track?
extern ushort FadeAlgID;	// Fade algorithm
extern bool SilRem;	// Remove opening silence?
extern bool RemEnabled; // Remove Silience Button enabled?
extern int Volume;
extern FXFont*	Monospace;
extern bool SilResolve();
// ---

// Game
// ----
extern GameInfo* ActiveGame;
// ----

// [update]
// --------
extern bool WikiUpdate;
extern FXString WikiURL;
// --------

// Encoders
// --------
extern List<Encoder*> Encoders;
extern FXushort EncFmt;
extern bool ShowConsole; // Show encoding console during the process
// --------

extern ushort LoopCnt;	// Song loop count (2 = song gets repeated once)
extern float FadeDur;	// Fade duration
extern FXString AppPath;
extern FXString OutPath;	// Output directory
// =======

// String Constants
extern       FXString PrgName;
extern       FXString WebPage;
extern const FXString NoGame;
extern       FXString CfgFile;
extern       FXString LGDFile;
extern const FXString Example;
extern const FXString DumpFile;
extern const FXString DecodeFile;
extern       FXString OggDumpFile;
extern       FXString OggPlayFile;
extern const FXString Cmp[LANG_COUNT];
// =======

#endif /* MUSICROOM_MUSICROOM_H */
