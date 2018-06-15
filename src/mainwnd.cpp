// Music Room Interface
// --------------------
// mainwnd.cpp - GUI
// --------------------
// "©" Nmlgc, 2010-2011

#include "musicroom.h"
#include <fx.h>
#include "widgets.h"
#include "mainwnd.h"
#include "parse.h"
#include <bgmlib/config.h>
#include <bgmlib/packmethod.h>
#include <bgmlib/bgmlib.h>
#include <bgmlib/ui.h>
#include <th_tool_shared/utils.h>
#include <th_tool_shared/LCDirFrame.h>
#include <th_tool_shared/LCLangFrame.h>
#include "encode.h"
#include "extract.h"
#include "tagger.h"
#include "scan.h"

extern MainWnd* MWBack;
MainWndFront* MW;
FXFont*	Monospace;

// Message Map
FXDEFMAP(MainWnd) MMMainWnd[] =
{
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_CHANGE_ACTION_STATE, MainWnd::onChangeActionState),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_STOP_SHOW, MainWnd::onStopShow),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_SHOW_NOTICE, MainWnd::onShowNotice),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_LOAD_BGM_INFO, MainWnd::onLoadBGMInfo),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_SELECT_DIR, MainWnd::onSelectDir),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_LOAD_GAME, MainWnd::onLoadGame),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_SWITCH_GAME, MainWnd::onSwitchGame),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_FILLTABLE_BASIC, MainWnd::onFillTableBasic),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_UPDATE_STRINGS, MainWnd::onCmdStrings),
	// FXMAPFUNCS(SEL_UPDATE, MainWnd::MW_UPDATE_STRINGS, MainWnd::MW_UPDATE_STRINGS_END, MainWnd::onUpdStrings),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_UPDATE_LENGTHS, MainWnd::onCmdLengths),
	FXMAPFUNC(SEL_CHANGED, MainWnd::MW_UPDATE_LENGTHS, MainWnd::onCmdLengths),
	FXMAPFUNC(SEL_CHANGED, MainWnd::MW_CHANGE_TRACK, MainWnd::onChangeTrack),
	FXMAPFUNC(SEL_TIMEOUT, MainWnd::MW_PROG_REDRAW, MainWnd::onProgRedraw),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_TOGGLE_PLAY, MainWnd::onTogglePlay),
	FXMAPFUNC(SEL_TIMEOUT, MainWnd::MW_PLAY_STAT, MainWnd::onPlayStat),
	FXMAPFUNC(SEL_CHANGED, MainWnd::MW_FN_PATTERN, MainWnd::onFNPattern),
	FXMAPFUNCS(SEL_COMMAND, MainWnd::MW_UPDATE_ENC, MainWnd::MW_UPDATE_ENC_END, MainWnd::onUpdEnc),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_ENC_SETTINGS, MainWnd::onEncSettings),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_EXTRACT_ALL, MainWnd::onExtract),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_EXTRACT_SEL, MainWnd::onExtract),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_STOP, MainWnd::onStop),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_TAG_UPDATE, MainWnd::onTagUpdate),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_THREAD_MSG, MainWnd::onThreadMsg),
	FXMAPFUNC(SEL_CHORE, MainWnd::MW_THREAD_STAT, MainWnd::onThreadStat),
	FXMAPFUNC(SEL_COMMAND, MainWnd::MW_ACT_FINISH, MainWnd::onActFinish),
};

FXIMPLEMENT(MainWnd, FXMainWindow, MMMainWnd, ARRAYNUMBER(MMMainWnd));

MainWnd::MainWnd(FXApp* App, FXIcon* Ico)
	: FXMainWindow(App, PrgName, Ico, NULL, DECOR_TITLE | DECOR_MINIMIZE | DECOR_MAXIMIZE | DECOR_CLOSE | DECOR_BORDER | DECOR_STRETCHABLE | LAYOUT_MIN_WIDTH, 0, 0, 600)
{
	// Layout managers
	FXVerticalFrame*	Main;
	FXGroupBox*	GameBox;
		FXHorizontalFrame* LabelFrame;
	FXSplitter* TrackFrame;
	FXPacker* TrackEdge;
	FXPacker* CommentEdge;
		LCLangFrame*	LangBox;
	FXGroupBox* PatternBox;
	FXHorizontalFrame* OutFrame;
	FXVerticalFrame* OutRightFrame;
		FXGroupBox* ParamBox;
		FXMatrix* ParamFrame;
		FXHorizontalFrame* FadeFrame;
	FXGroupBox* OutDirBox;
	FXGroupBox* EncBox;
		FXVerticalFrame* EncFrame;
	FXVerticalFrame* StartFrame;
	FXGroupBox*	StatBox;
	FXVerticalFrame* StatFrame;
	
	FXColor TmpClr;
	FXString TmpStr;

	Extractor& Ext = Extractor::Inst();

	CurTrack = NULL;
	EncBtn = NULL;
	CFGFail = false;
	Lock = false;
	Monospace = new FXFont(getApp(), "Courier New", 8);

	Main = new FXVerticalFrame(this, LAYOUT_FILL);

	WikiDT.connect(WikiUpdate);

	// Language

	LangBox = new LCLangFrame(Main, "Tag Language", &::Lang, this, MW_UPDATE_STRINGS, GROUPBOX_NORMAL | FRAME_GROOVE | LAYOUT_FILL_X);
		
		GetWiki = new FXCheckButton(LangBox->Frame, "Get most recent track info from Touhou Wiki", &WikiDT, FXDataTarget::ID_VALUE, CHECKBUTTON_NORMAL | LAYOUT_RIGHT);
		GetWiki->update();

	// Game
	GameBox = new FXGroupBox(Main, "Game", GROUPBOX_NORMAL | FRAME_GROOVE | LAYOUT_FILL);

		GameList = new LCListBox(GameBox, this, MW_SWITCH_GAME, LISTBOX_NORMAL | FRAME_SUNKEN | FRAME_THICK | LAYOUT_FILL_X | JUSTIFY_CENTER_X);
		GameList->appendItem(NoGame);
		GameList->setNumVisible(10);

		TmpClr = GameList->getTextColor();
		GameList->setTextColor(GameList->getBackColor());
		GameList->setBackColor(TmpClr);
		GameList->getPane()->setBorderColor(GameList->getTextColor());

		GameDir = new LCDirFrame(GameBox, "", this, MW_SELECT_DIR, DIRFRAME_READONLY);
		GameDir->DlgCaption = "Select Game Directory...";
		GameDir->setButtonText(GameDir->DlgCaption);


	// Track Overview
	LabelFrame = new FXHorizontalFrame(GameBox, LAYOUT_FILL_X, 0, 0, 0, 0, DEFAULT_SPACING, DEFAULT_SPACING, 0, 0, 0, 0);
	new FXLabel(LabelFrame, "Track Overview", NULL, LABEL_NORMAL | LAYOUT_LEFT);

#ifdef _WIN32
		VolDT.connect(Volume);
		TrackVol = new FXSlider(LabelFrame, &VolDT, FXDataTarget::ID_VALUE, SLIDER_NORMAL | SLIDER_HORIZONTAL | SLIDER_ARROW_DOWN | SLIDER_TICKS_BOTTOM | LAYOUT_RIGHT | LAYOUT_FIX_WIDTH, 0, 0, 100);
		TrackPlay = new FXCheckButton(LabelFrame, "Play selected track", this, MW_TOGGLE_PLAY, CHECKBUTTON_NORMAL | LAYOUT_RIGHT, 0, 0, 0, 0, 0, 25);

		TrackVol->setRange(0, 100);
		TrackVol->setTickDelta(25);
#endif
		TrackFrame = new FXSplitter(GameBox, SPLITTER_HORIZONTAL | SPLITTER_TRACKING | LAYOUT_FILL);
		  TrackEdge = new FXPacker(TrackFrame, FRAME_SUNKEN | LAYOUT_MIN_WIDTH, 0, 0, 300, 0, 0, 0, 0, 0, 0, 0);
		CommentEdge = new FXPacker(TrackFrame, FRAME_SUNKEN | LAYOUT_MIN_WIDTH, 0, 0, 300, 0, 0, 0, 0, 0, 0, 0);
		TrackView = new LCTable(TrackEdge, this, MW_CHANGE_TRACK, TABLE_COL_SIZABLE | LAYOUT_FILL);

		TmpClr = TrackView->getTextColor();
		TrackView->setTextColor(TrackView->getBackColor());
		TrackView->setBackColor(TmpClr);
		TrackView->setCellColor(0, 0, TmpClr);	TrackView->setCellColor(0, 1, TmpClr);
		TrackView->setCellColor(1, 0, TmpClr);	TrackView->setCellColor(1, 1, TmpClr);

		TrackView->insertColumns(0, 2);
		TrackView->setColumnText(0, "#");
		TrackView->setColumnText(1, "Name");
		
		TrackView->setRowHeaderWidth(0);
		TrackView->setVisibleRows(6);

		TrackView->fitColumnsToContents(0, TrackView->getNumColumns());

	// Comment
	Comment = new LCText(CommentEdge, NULL, 0, TEXT_READONLY | TEXT_WORDWRAP | LAYOUT_FILL);
	Comment->setColors();
	// --------------

	// Output Directory
	OutDirBox = new FXGroupBox(Main, "Output Directory", GROUPBOX_NORMAL | FRAME_GROOVE | LAYOUT_FILL_X);
		OutDir = new LCDirFrame(OutDirBox, OutPath);

	OutFrame = new FXHorizontalFrame(Main, LAYOUT_FILL_X);

	// Filename Pattern
	PatternBox = new FXGroupBox(OutFrame, "Filename Pattern", GROUPBOX_NORMAL | FRAME_GROOVE | LAYOUT_FILL);
		FNField = new FXTextField(PatternBox, 32, this, MW_FN_PATTERN, TEXTFIELD_NORMAL);

		FNExample = new FXLabel(PatternBox, "");

		TmpStr.format("Tokens:\n\n"
			     "%s - Two-digit track number\n"
				 "%s - Track name (Japanese)\n"
				 "%s - Track name (English)\n"
				 "%s - Track name (current language)", GetToken(0), GetToken(1), GetToken(2), GetToken(3));

		new FXLabel(PatternBox, TmpStr, NULL, JUSTIFY_LEFT);

		FNField->setText(FNPattern);

	// Encoding Format
	EncDT.connect(EncFmt);

	EncBox = new FXGroupBox(OutFrame, "Output Format", GROUPBOX_NORMAL | FRAME_GROOVE | LAYOUT_FILL_Y);
		EncFrame = new FXVerticalFrame(EncBox, LAYOUT_FILL, 0, 0, 0, 0, 0, 0, 0, 0);

		ListEntry<Encoder*>* CurEnc = Encoders.First();
		FXushort EncIndex = 1;
		EncBtn = new FXRadioButton*[Encoders.Size()];
		while(CurEnc)
		{
			EncBtn[EncIndex - 1] = new FXRadioButton(EncFrame, CurEnc->Data->Ext, this, MW_UPDATE_ENC + EncIndex, RADIOBUTTON_NORMAL | LAYOUT_CENTER_X | LAYOUT_CENTER_Y);
			EncIndex++;
			CurEnc = CurEnc->Next();
		}

		new FXButton(EncFrame, "Settings...", NULL, this, MW_ENC_SETTINGS, BUTTON_NORMAL | LAYOUT_FILL_X);
		
		if(EncFmt > 0)	EncBtn[EncFmt - 1]->handle(this, FXSEL(SEL_COMMAND, ID_CHECK), NULL);

		// We have to misuse this label as the general minimum dialog size,
		// because FXSplitter apparently doesn't care about LAYOUT_MIN_WIDTH...
		// EncMsg = new FXLabel(EncBox, "Encoding options can be configured in the musicroom.cfg file. "
		                            //  "You can also use own encoders.", 0, LABEL_NORMAL | LAYOUT_CENTER_X | LAYOUT_FILL_X | LAYOUT_FIX_WIDTH, 0, 0, 600);

	// Parameters
	OutRightFrame = new FXVerticalFrame(OutFrame, LAYOUT_FILL_Y, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	TagUpdate = new FXButton(OutRightFrame, "Update tags", NULL, this, MW_TAG_UPDATE, BUTTON_NORMAL | LAYOUT_FILL);

	ParamBox = new FXGroupBox(OutRightFrame, "Parameters", GROUPBOX_NORMAL | FRAME_GROOVE | LAYOUT_FILL_X);
		ParamFrame = new FXMatrix(ParamBox, 2, LAYOUT_RIGHT | MATRIX_BY_COLUMNS, 0, 0, 0, 0, 0, 0, 0, DEFAULT_SPACING, 8, 8);

			new FXLabel(ParamFrame, "Loop Count: ", NULL, LABEL_NORMAL | LAYOUT_RIGHT | LAYOUT_CENTER_Y);	
			LoopField = new FXSpinner(ParamFrame, 6, this, MW_UPDATE_LENGTHS, SPIN_NORMAL | FRAME_SUNKEN | FRAME_THICK | LAYOUT_LEFT);

			new FXLabel(ParamFrame, "Fade Duration: ", NULL, LABEL_NORMAL | LAYOUT_RIGHT  | LAYOUT_CENTER_Y);

		FadeFrame = new FXHorizontalFrame(ParamFrame, LAYOUT_FILL_X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			FadeField = new FXRealSpinner(FadeFrame, 6, this, MW_UPDATE_LENGTHS, SPIN_NORMAL | FRAME_SUNKEN | FRAME_THICK);
			new FXLabel(FadeFrame, " seconds", NULL, LABEL_NORMAL | LAYOUT_CENTER_Y);

		AlgDT.connect(FadeAlgID);

			new FXLabel(ParamFrame, "Fade Algorithm: ", NULL, LABEL_NORMAL | LAYOUT_RIGHT | LAYOUT_CENTER_Y);
			AlgBox = new FXListBox(ParamFrame, &AlgDT, FXDataTarget::ID_VALUE, FRAME_SUNKEN | FRAME_THICK | LAYOUT_FILL_X);
		
		LoopField->setRange(1, 10);
		FadeField->setRange(-60, 60);
		FadeField->setIncrement(0.5);

		LoopField->setValue(LoopCnt);
		FadeField->setValue(FadeDur);

		ListEntry<FadeAlg*>* CurFA = Ext.FAs.First();
		while(CurFA)
		{
			AlgBox->appendItem(CurFA->Data->Name);
			CurFA = CurFA->Next();
		}
		AlgBox->setNumVisible(Ext.FAs.Size());

		RemoveSilence = new FXCheckButton(ParamBox, "Remove opening silence", this, MW_UPDATE_LENGTHS, CHECKBUTTON_NORMAL);
		RemEnabled = RemoveSilence->isEnabled();

	StartFrame = new FXVerticalFrame(OutRightFrame, LAYOUT_FILL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		StartAll = new FXButton(StartFrame, "Extract all", NULL, this, MW_EXTRACT_ALL, BUTTON_NORMAL | LAYOUT_FILL);
		StartSel = new FXButton(StartFrame, "Extract selected", NULL, this, MW_EXTRACT_SEL, BUTTON_NORMAL | LAYOUT_FILL);
		Stop = new FXButton(StartFrame, "Stop", NULL, this, MW_STOP, BUTTON_NORMAL | LAYOUT_FILL);

		Stop->hide();

	// Status
	StatBox = new FXGroupBox(Main, "Status", GROUPBOX_NORMAL | FRAME_GROOVE | LAYOUT_BOTTOM | LAYOUT_FILL);
		StatFrame = new FXVerticalFrame(StatBox, FRAME_SUNKEN | LAYOUT_FILL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		Stat = new LCText(StatFrame, NULL, 0, TEXT_READONLY | TEXT_SHOWACTIVE | LAYOUT_FILL);

		Stat->setVisibleRows(8);
		// Stat->setFont(Monospace);
		Stat->setColors();

		ProgBar = new FXProgressBar(StatFrame, &ProgDT, FXDataTarget::ID_VALUE, PROGRESSBAR_NORMAL | PROGRESSBAR_HORIZONTAL | PROGRESSBAR_PERCENTAGE | LAYOUT_FILL_X);
		ProgBar->hide();

		FXColor Base = getApp()->getBaseColor();
		PlayStat = new FXStatusLine(StatFrame, NULL, LAYOUT_BOTTOM | LAYOUT_FILL_X);
		PlayStat->hide();

	handle(this, FXSEL(SEL_COMMAND, MW_CHANGE_ACTION_STATE), false);
}

void MainWnd::create()
{
	FXMainWindow::create();

	getApp()->forceRefresh();
	getApp()->beginWaitCursor();

	StreamerFront::Inst().Init(id());

	RemoveSilence->handle(this, FXSEL(SEL_COMMAND, SilResolve() ? ID_CHECK : ID_UNCHECK), (void*)SilResolve());
	TrackPlay->handle(this, FXSEL(SEL_COMMAND, Play ? ID_CHECK : ID_UNCHECK), (void*)Play);
	handle(this, FXSEL(SEL_COMMAND, MW_TOGGLE_PLAY), (void*)Play);

	handle(this, FXSEL(SEL_COMMAND, MW_LOAD_BGM_INFO), NULL);

	PrintStat(WebPageDesc(WebPage));
	getApp()->endWaitCursor();
}

long MainWnd::onChangeActionState(FXObject* Sender, FXSelector Message, void* ptr)
{
	char State = (char)ptr;

	if(State & MW_ACT_EXTRACT)	{StartAll->enable();  StartSel->enable();}
	else						{StartAll->disable(); StartSel->disable();}

	if(State & MW_ACT_TAG)	TagUpdate->enable();
	else					TagUpdate->disable();
	return 1;
}

long MainWnd::onStopShow(FXObject* Sender, FXSelector Message, void* ptr)
{
	static FXColor BackColor;	// For some reason, it resets to white after re-enabling
	bool State = ptr != 0;
	if(State)
	{
		StartAll->hide(); StartSel->hide();
		TagUpdate->disable();

		BackColor = GameList->getBackColor();
		GameList->disable();

		AlgBox->disable();
		Stop->show(); Stop->enable();
	}
	else	
	{
		StartAll->show(); StartSel->show();
		TagUpdate->enable();

		GameList->enable();
		GameList->setBackColor(BackColor);

		AlgBox->enable();
		Stop->hide();
	}
	StartAll->recalc();	StartSel->recalc();	Stop->recalc();
	return 1;
}

long MainWnd::onShowNotice(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXString* Notice = (FXString*)ptr;

	getApp()->beep();
	FXMessageBox::warning(this, MBOX_OK, "Please read: A personal appeal from the programmer", Notice->text());
	Notice->clear();
	return 1;
}

static bool LoadIcon(GameInfo* GI, const FXString& Str)
{
	FXFile File;
	ushort Size;
	
	if(File.open(Str + ".ico"))
	{
		Size = File.size();

		char* ICO = (char*)malloc(Size);
		File.readBlock(ICO, Size);
		File.close();
		GI->Icon = new FXICOIcon(MWBack->getApp(), ICO);
		GI->Icon->scale(16, 16);
		GI->Icon->create();
		free(ICO);

		return true;
	}
	return false;
}

long MainWnd::onLoadBGMInfo(FXObject* Sender, FXSelector Message, void* ptr)
{
	ListEntry<GameInfo>* CurGame;
	GameInfo* GI;
	FXString Str;

	if(!BGMLib::LoadBGMInfo())	GameDir->Select->disable();

	FXSystem::setCurrentDirectory(BGMLib::InfoPath);
	
	CurGame = BGMLib::Game.First();
	while(CurGame)
	{
		GI = &CurGame->Data;

		Str = FXPath::stripExtension(GI->InfoFile);
		LoadIcon(GI, Str);
		LGD->LinkValue(Str, TYPE_STRING, &GI->Path);

		GameList->appendItem(GI->NumName(Lang), GI->Icon, GI);
		CurGame = CurGame->Next();
	}

	FXSystem::setCurrentDirectory(AppPath);

	if(CFGFail)
	{
		Str.format("Couldn't load the configuration file (%s)!", CfgFile);
		BGMLib::UI_Stat(Str);
	}
	return 1;
}

void MainWnd::LoadGame(GameInfo* GI)
{
	StreamerFront& Str = StreamerFront::Inst();
	PrevTrackID = -1;

	if(GI && !GI->HaveTrackData)	GI->ParseTrackData();

	ActiveGame = GI;
	GameList->setCurrentItem(GameList->findItemByData(ActiveGame), true);

	CurTrack = NULL;

	setTitle(PrgName);
	handle(this, FXSEL(SEL_COMMAND, MW_FILLTABLE_BASIC), NULL);

	if(!GI)
	{
		Comment->setText("");
		GameDir->Field->setText("");
		RemoveSilence->enable();
		RemEnabled = RemoveSilence->isEnabled();
		handle(this, FXSEL(SEL_COMMAND, MW_CHANGE_ACTION_STATE), false);
		TrackView->fitColumnsToContents(0, TrackView->getNumColumns());
	}
	else
	{
		ListEntry<TrackInfo>* New = GI->Track.First();
		if(New)	CurTrack = &New->Data;

		if(!GI->Path.empty())
		{
			GameDir->Field->setText(GI->Path);
			handle(this, FXSEL(SEL_COMMAND, MW_UPDATE_LENGTHS), NULL);

			LGD->LinkValue(FXPath::stripExtension(GI->InfoFile), TYPE_STRING, &GI->Path, false);

			// Show Vorbis warning
			if(GI->Vorbis)
			{
				FXString Warn("This game has it's music stored in Ogg Vorbis format.\n\n"
							  "Adding loops and fades, or encoding it into another lossy format\n"
							  "such as MP3 will result in a slight drop in quality, while lossless\n"
							  "formats will only needlessly increase the extracted file size.\n\n");

				Warn += GI->PM->PMInfo(GI);
				FXMessageBox::warning(this, MBOX_OK, PrgName.text(), Warn.text());
			}
		}
		else	GameDir->Field->setText("n/a");

		SetComment(CurTrack);

		char Act = MW_ACT_TAG;
		if(!GI->Path.empty())	Act |= MW_ACT_EXTRACT;

		handle(this, FXSEL(SEL_COMMAND, MW_CHANGE_ACTION_STATE), (void*)Act);

		if((GI->CryptKind && GI->Vorbis) || !GI->SilenceScan)	RemoveSilence->disable();
		else	RemoveSilence->enable();
		RemEnabled = RemoveSilence->isEnabled();

		Str.RequestTrackSwitch(CurTrack);
		if(Play)	Str.Play();

		if(!Notice.empty())	handle(this, FXSEL(SEL_COMMAND, MW_SHOW_NOTICE), &Notice);
	}

	handle(FNField, FXSEL(SEL_CHANGED, MW_FN_PATTERN), NULL);
}

void MainWnd::LoadGame(FXString& Path)
{
	StreamerFront& Str = StreamerFront::Inst();
	if(Play)	Str.Stop();
	Str.CloseFile();

	ActiveGame = BGMLib::ScanGame(Path);
	if(!ActiveGame || !ActiveGame->Init(Path))	ActiveGame = NULL;
	else										PerformScans(ActiveGame);
	LoadGame(ActiveGame);
}

long MainWnd::onSelectDir(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXString& NewGameDir = *((FXString*)ptr);
	if(ActiveGame && (NewGameDir == ActiveGame->Path))	return 1;

	StreamerFront& Str = StreamerFront::Inst();
	if(Play)	Str.Stop();
	Str.CloseFile();

	LoadGame(NewGameDir);

	return 1;
}

long MainWnd::onSwitchGame(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXint i = (FXint)ptr;
	LCListBox* LB = (LCListBox*)Sender;

	GameInfo* New = (GameInfo*)LB->getItemData(i);
	GameInfo* Verify;
	FXString Str;

	if(New == ActiveGame)	return 1;

	StreamerFront& S = StreamerFront::Inst();
	if(Play)	S.Stop();
	S.CloseFile();

	if(New)
	{
		Str.format("\nSwitching to %s...\n", New->DelimName(Lang));
		BGMLib::UI_Stat(Str);

		if(!New->Path.empty())
		{
			// Verify if the path is still correct
			if(FXSystem::setCurrentDirectory(New->Path) &&
				(Verify = New->PM->Scan(New->Path)) &&
				(Verify->Name[0] == New->Name[0]))
			{
				New->Init(New->Path);
				PerformScans(New);
			}
			else
			{
				Str.format("Saved directory to this game (%s) is invalid.\n", New->Path);
				BGMLib::UI_Error(Str);
				New->Scanned = false;
				LGD->LinkValue(FXPath::stripExtension(New->InfoFile), 0, NULL);
				New->Path.clear();
			}
			FXSystem::setCurrentDirectory(AppPath);
		}
	}

	LoadGame(New);

	return 1;
}

long MainWnd::onLoadGame(FXObject* Sender, FXSelector Message, void* ptr)
{
	char* Dir = (char*)ptr;
	LoadGame(FXString(Dir));
	return 1;
}

long MainWnd::onFillTableBasic(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXint Row = 0;
	TrackInfo* Track;

	TrackView->removeRows(0, TrackView->getNumRows());
	if(!ActiveGame)	return 1;

	TrackView->insertRows(0, ActiveGame->TrackCount);

	if(ActiveGame->Scanned)
	{
		if(TrackView->getNumColumns() < 3)
		{
			TrackView->insertColumns(2);
			TrackView->setColumnText(2, "Length");
		}
	}
	else
	{
		if(TrackView->getNumColumns() >= 3)	TrackView->removeColumns(2);
	}

	ListEntry<TrackInfo>* CurTrack = ActiveGame->Track.First();
	for(ushort Temp = 0; Temp < ActiveGame->TrackCount; Temp++)
	{
		Track = &(CurTrack->Data);

		if(Track->GetStart() == 0 && Track->FS != 0)
		{
			TrackView->removeRows(Row);
			CurTrack = CurTrack->Next();
			continue;
		}

		TrackView->setItemJustify(Row, 1, FXTableItem::LEFT|FXTableItem::CENTER_Y);
		TrackView->setItemText(Row, 0, FXString::value(Track->Number));

		TrackView->getItem(Row, 0)->setData(Track);

		Row++;

		CurTrack = CurTrack->Next();
	}

	handle(this, FXSEL(SEL_COMMAND, MW_UPDATE_STRINGS), (void*)Lang);

	return 1;
}

long MainWnd::onCmdStrings(FXObject* Sender, FXSelector Message, void* ptr)
{
	ListEntry<GameInfo>* CurGame = BGMLib::Game.First();
	FXint l = 1;
	GameInfo* GI;

	ushort NewLang = (ushort)ptr;

	TranslateGameNames(Stat, NewLang);

	// Update list box game names
	while(CurGame)
	{
		GI = &CurGame->Data;
		GameList->setItemText(l++, GI->NumName(NewLang));
		CurGame = CurGame->Next();
	}

	if(!ActiveGame)	return 1;
	
	// Update table strings
	FXint Row = 0;
	TrackInfo* Track;

	ListEntry<TrackInfo>* CurTI = ActiveGame->Track.First();
	for(ushort Temp = 0; Temp < ActiveGame->TrackCount; Temp++)
	{
		Track = &(CurTI->Data);

		if(!(Track->GetStart() == 0 && Track->FS != 0) )
		{
			TrackView->setItemText(Row, 1, Track->Name[NewLang]);
			Row++;
		}
		CurTI = CurTI->Next();
	}

	SetComment(CurTrack);
	TrackView->fitColumnsToContents(0, TrackView->getNumColumns());

	handle(FNField, FXSEL(SEL_CHANGED, MW_FN_PATTERN), NULL);

	setTitle(PrgName + " - " + ActiveGame->Name[NewLang]);

	return 1;
}

long MainWnd::onCmdLengths(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXint Row = 0;
	ulong Len;
	TrackInfo* TI;
	
	LoopCnt = LoopField->getValue();
	FadeDur = FadeField->getValue();
	SilRem = RemoveSilence->getCheck() == TRUE;

	if(!ActiveGame || !ActiveGame->Scanned)	return 1;

	ListEntry<TrackInfo>* CurTI = ActiveGame->Track.First();
	for(ushort Temp = 0; Temp < ActiveGame->TrackCount; Temp++)
	{
		TI = &CurTI->Data;
		if(TI->GetStart() == 0 && TI->FS != 0)
		{
			CurTI = CurTI->Next();
			continue;
		}

		Len = TI->GetByteLength(SilResolve(), LoopCnt, FadeDur);

		TrackView->setItemText(Row, 2, TI->LengthString(Len));

		Row++;
		CurTI = CurTI->Next();
	}

	return 1;
}

void MainWnd::SetComment(TrackInfo* TI)
{
	if(TI)
	{
		ListEntry<IntString>* C;
		FXString Cmt = TI->GetComment(Lang);
		Comment->setText(Cmt);
		if(!Cmt.empty())	Comment->appendText("\n – ");
		else				Comment->appendText(Cmp[Lang] + ": ");
		C = ActiveGame->Composer.Get(TI->CmpID);
		if(C)	Comment->appendText(C->Data[Lang], true);
	}
	else	Comment->setText("", 0, true);
}

#define PLAYSTAT_TIMEOUT 250000000

long MainWnd::onChangeTrack(FXObject* Sender, FXSelector Message, void* ptr)
{
	LCTable* Table = (LCTable*)(Sender);
	
	FXint TrackID = Table->getCurrentRow();

	CurTrack = (TrackInfo*)Table->getItem(TrackID, 0)->getData();

	SetComment(CurTrack);

	if(TrackID != PrevTrackID)
	{
		StreamerFront::Inst().RequestTrackSwitch(CurTrack);
		PrevTrackID = TrackID;
	}

	handle(FNField, FXSEL(SEL_CHANGED, MW_FN_PATTERN), NULL);
	return 1;
}

long MainWnd::onTogglePlay(FXObject* Sender, FXSelector Message, void* ptr)
{
	Play = ptr != 0;
	StreamerFront& Str = StreamerFront::Inst();
	onPlayStat(Sender, Message, PlayStat);
	if(Play)	Str.Play();
	else		Str.Stop();
	return 1;
}

long MainWnd::onPlayStat(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXString Stat;
	StreamerFront& Str = StreamerFront::Inst();

	FXStatusLine* PS = (FXStatusLine*)ptr;
	bool Show = PS->shown();
	ulong Cur, Len;

	TrackInfo* TI = Str.CurTrack();
	if(Play && ActiveGame && TI)
	{
		Len = TI->GetByteLength(SilResolve(), 1, 0);
		Cur = Str.Pos();

		Stat.format("Playing: %s %s  //  %s, %.0f Hz  //  %s / %s", TI->GetNumber(), TI->Name[Lang], ActiveGame->Vorbis ? "Vorbis" : "PCM", TI->Freq, TI->LengthString(Cur), TI->LengthString(Len));

		PlayStat->setNormalText(Stat);
		if(!Show)	PlayStat->show();
	}
	else if(Show)	PS->hide();

	if(PS->shown() != Show)
	{
		PS->recalc();	PS->update();
		recalc();	update();
	}

	if(Play)	getApp()->addTimeout(this, FXSEL(SEL_TIMEOUT, MW_PLAY_STAT), PLAYSTAT_TIMEOUT, PS);
	else		getApp()->removeTimeout(this, FXSEL(SEL_TIMEOUT, MW_PLAY_STAT));
	return 1;
}

long MainWnd::onFNPattern(FXObject* Sender, FXSelector Message, void* ptr)
{
	if(!ActiveGame)	return 1;

	FXString NewExample = "";

	FXTextField* Field = (FXTextField*)(Sender);
	FNPattern = Field->getText();

	if(CurTrack != NULL)	NewExample = PatternFN(CurTrack);
	else					NewExample = PatternFN(&(ActiveGame->Track.First()->Data));

	if(!NewExample.empty())
	{
		NewExample.prepend(Example);
		FNExample->setText(NewExample);
	}
	return 1;
}

long MainWnd::onUpdEnc(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXushort Send = (FXSELID(Message) - MW_UPDATE_ENC - 1);

	for(ulong Count = 0; Count < Encoders.Size(); Count++)
	{
		if(Send != Count)	EncBtn[Count]->handle(this, FXSEL(SEL_COMMAND, ID_UNCHECK), NULL);
		else
		{
			EncFmt = Send + 1;
			EncBtn[Count]->handle(this, FXSEL(SEL_COMMAND, ID_CHECK), NULL);
		}
	}

	handle(FNField, FXSEL(SEL_CHANGED, MW_FN_PATTERN), NULL);

	return 1;
}

long MainWnd::onEncSettings(FXObject* Sender, FXSelector Message, void* ptr)
{
	EncSettingDlg Dialog(this);
	Dialog.execute(PLACEMENT_OWNER);
	return 1;
}

bool MainWnd::CheckOutDir()
{
	FXString Dir = OutDir->getDir();
	bool Empty = Dir.empty();
	if(Empty)	FXMessageBox::error(MWBack->getApp()->getActiveWindow(), MBOX_OK, PrgName.text(), "Please specify an output directory.");
	else		OutPath = Dir;
	return !Empty;
}

static short GetTableTrackNo(LCTable* Table, const FXint& Row)
{
	TrackInfo* TI;

	if(Row < 0)	return 0;
	
	TI = (TrackInfo*)Table->getItem(Row, 0)->getData();
	if(TI)	return TI->Number;
	else	return 0;
}

long MainWnd::onExtract(FXObject* Sender, FXSelector Message, void* ptr)
{
	short ExtStart = -1, ExtEnd = -1;
	bool Sel = FXSELID(Message) == MW_EXTRACT_SEL;
	FXString Drive = FXPath::drive(OutPath);
	
	Drive = FXPath::simplify(Drive.upper() + PATHSEP);

	// Directory Testing
	// -----------------
	if(!CheckOutDir())	return 1;
	if(!FX::FXSystem::setCurrentDirectory(Drive))
	{
		FXMessageBox::error(getApp(), MBOX_OK, PrgName.text(), "Drive %s does not exist!", Drive.upper().text());
		return 1;
	}

	if(!FX::FXSystem::setCurrentDirectory(OutPath))
	{
		if(!FX::FXDir::createDirectories(OutPath))
		{
			FXMessageBox::error(getApp(), MBOX_OK, PrgName.text(), "Error creating directory %s!", OutPath.text());
			return 1;
		}
	}
	FX::FXSystem::setCurrentDirectory(AppPath);
	// -----------------

	GameDir->disable();
	handle(this, FXSEL(SEL_COMMAND, MW_STOP_SHOW), (void*)true);

	if(Sel)
	{
		ExtStart = GetTableTrackNo(TrackView, TrackView->getSelStartRow()) - 1;
		ExtEnd = GetTableTrackNo(TrackView, TrackView->getSelEndRow());
		
		if(ExtStart == -1)
		{
			ExtStart = 0;
			ExtEnd = 1;
		}
	}
	else
	{
		ExtStart = 0;
		ExtEnd = ActiveGame->TrackCount + 1;
	}

	Extractor::Inst().Start(ExtStart, ExtEnd, FadeAlgID);

	return 1;
}

long MainWnd::onTagUpdate(FXObject* Sender, FXSelector Message, void* ptr)
{
	if(!CheckOutDir())	return 1;

	FXuint Ret;

	if(!FXSystem::setCurrentDirectory(OutPath))
	{
		FXMessageBox::error(getApp(), MBOX_OK, PrgName.text(), "Error entering directory %s!", OutPath.text());
		return 1;
	}

	Ret = FXMessageBox::question(this, MBOX_YES_NO, PrgName.text(),
		"This will update already extracted files from this game in the output directory with the current tag information.\n"
		"Please verify that the given filename pattern _and the output format_ matches the files to be updated!\n\n"
		"If the filenames don't match exactly, make sure the files have the correct game title in the \"album\" tag.\n"
		"This way, they can be found by automatic search.\n\n"
		"Proceed?");

	if(Ret == MBOX_CLICKED_NO)	return 1;

	GameDir->disable();
	handle(this, FXSEL(SEL_COMMAND, MW_STOP_SHOW), (void*)true);

	Tagger::Inst().start();

	return 1;
}

long MainWnd::onStop(FXObject* Sender, FXSelector Message, void* ptr)
{
	Extractor& Ext = Extractor::Inst();
	Tagger& Tag = Tagger::Inst();

	Lock = true;

	     if(Ext.Active)	Ext.Stop();
	else if(Tag.Active)
	{
		Tag.Stop();
		((FXButton*)Sender)->disable();
	}
	Lock = false;
	ProgConnect();
	return 1;
}

#define PROGBAR_TIMEOUT 20000000

long MainWnd::onProgRedraw(FXObject* Sender, FXSelector Message, void* ptr)
{
	FXulong& Now = *((FXulong*)ptr);
	static FXulong Last = 0;

	getApp()->addTimeout(this, MW_PROG_REDRAW, PROGBAR_TIMEOUT, ptr);
	if(Last != Now)
	{
		ProgBar->update();
		Last = Now;
	}
	return 1;
}

FXbool MainWnd::close(FXbool notify)
{
	FXString Str;

	Str = OutDir->getDir();
	if(!Str.empty())	OutPath = Str;

	onStop(this, FXSEL(SEL_COMMAND, MW_STOP), NULL);
	StreamerFront::Inst().Exit();

	SAFE_DELETE_ARRAY(EncBtn);
	SAFE_DELETE(Monospace);

	FXMainWindow::close(notify);
	return true;
}

void MainWnd::PrintStat(const FXString& NewStat)
{
	::PrintStat(getApp(), Stat, NewStat);
}

void MainWnd::ProgConnect(volatile FXulong* Var, FXuint Max)
{
	if(!Var)
	{
		ProgDT.connect();
		getApp()->removeTimeout(this, MW_PROG_REDRAW);
	}
	else
	{
		ProgDT.connect((FXulong&)*Var);
		getApp()->addTimeout(this, MW_PROG_REDRAW, PROGBAR_TIMEOUT, (void*)Var);
	}
	if(Lock)	return;

	if(!Var)
	{
		ProgBar->hide();
		ProgBar->setTotal(0);
	}
	else
	{
		ProgBar->show();
		ProgBar->setTotal(Max);
	}
	ProgBar->recalc();	ProgBar->update();
	recalc();	update();
}

// Cross-thread messaging
// ----------------------

// Helper structure for onThreadMsg.
// Instances of this structure are dynamically allocated by MainWndFront::ThreadMsg, 
// and deleted by onThreadMsg.

struct CrossThreadMsg
{
	FXThread*	Thread;
	const FXString*	Str;
	volatile FXuint*	Ret;	// Message return value
	FXuint opts;	// Message box options

	CrossThreadMsg(FXThread* t, const FXString* s, volatile FXuint* r, const FXuint& o)
		: Thread(t), Str(s), Ret(r), opts(o)
	{}
};

long MainWnd::onThreadMsg(FXObject* Sender, FXSelector Message, void* ptr)
{
	CrossThreadMsg* CTM = *((CrossThreadMsg**)ptr);

	*(CTM->Ret) = FXMessageBox::question(this, CTM->opts, PrgName.text(), CTM->Str->text());

	CTM->Thread->resume();
	SAFE_DELETE(CTM);
	return 1;
}
// ----------------------

long MainWnd::onThreadStat(FXObject* Sender, FXSelector Message, void* ptr)
{
	// Let's be extra safe
	FXint Len = StatCache.length();
	PrintStat(StatCache);
	StatCache.erase(0, Len);
	return 1;
}

long MainWnd::onActFinish(FXObject* Sender, FXSelector Message, void* ptr)
{
	handle(this, FXSEL(SEL_COMMAND, MW_STOP_SHOW), false);
	GameDir->enable();
	FXSystem::setCurrentDirectory(AppPath);

	return 1;
}

// Front end
// ---------
MainWndFront::MainWndFront()
{
	MsgChan = new FXMessageChannel(getApp());
	MW = MWBack;
}

void BGMLib::UI_Stat(const FXString& Msg)
{
	MWBack->PrintStat(Msg);
}

void BGMLib::UI_Stat_Safe(const FXString& Msg)
{
	MWBack->StatCache.append(Msg);
	MWBack->getApp()->addChore(MWBack, FXSEL(SEL_CHORE, MainWnd::MW_THREAD_STAT), NULL);
}

uint BGMLib::UI_Update(const FXString& Msg, const FXString& Old, const FXString& New)
{
	return LCUpdateMessage::question(MWBack->getApp(), MBOX_YES_YESALL_NO_NOALL_CANCEL, PrgName, Msg, Old, New);
}

void BGMLib::UI_Error(const FXString& Msg)
{
	UI_Stat("ERROR: " + Msg);
}

void BGMLib::UI_Error_Safe(const FXString& Msg)
{
	UI_Stat_Safe("ERROR: " + Msg);
}

void BGMLib::UI_Notice(const FXString& Notice)
{
	MWBack->Notice.assign(Notice);
}

void MainWndFront::ProgConnect(volatile FXulong* Var, FXuint Max)	{return MWBack->ProgConnect(Var, Max);}
FXApp* MainWndFront::getApp()	{return MWBack->getApp();}
void MainWndFront::ThreadMsg(FXThread* Thread, const FXString& Str, volatile FXuint* Ret, FXuint opts)
{
	CrossThreadMsg* CTM = new CrossThreadMsg(Thread, &Str, Ret, opts);
	
	MsgChan->message(MWBack, FXSEL(SEL_COMMAND, MainWnd::MW_THREAD_MSG), &CTM, sizeof(CrossThreadMsg**));
	Thread->suspend();
}
void MainWndFront::ActFinish()	{MsgChan->message(MWBack, FXSEL(SEL_COMMAND, MainWnd::MW_ACT_FINISH));}

void MainWndFront::Clear()	{SAFE_DELETE(MsgChan);}
MainWndFront::~MainWndFront()	{Clear();}
// ---------