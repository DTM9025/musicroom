// Music Room Interface
// --------------------
// extract.cpp - Extraction and encoding functions
// --------------------
// "©" Nmlgc, 2010-2011
// "©" DTM9025, 2024

#include "musicroom.h"

#include <math.h>

// FOX
#include <FXHash.h>
#include <FXStream.h>
#include <FXFile.h>
#include <FXThread.h>
#include <FXMessageChannel.h>
#include <FXMessageBox.h>

#include <FXStat.h>
#include <FXSystem.h>

#include <bgmlib/libvorbis.h>
#include "extract.h"
#include "tagger.h"
#include <bgmlib/ui.h>
#include <bgmlib/packmethod.h>

const ushort WAV_HEADER_SIZE = 44;

// Opens [GI->BGMFile], writes handles to [File] and [VF], and seeks to [TI]
bool OpenVorbisBGM(FXFile& File, OggVorbis_File& VF, GameInfo* GI, TrackInfo* TI)
{
	if(!GI->OpenBGMFile(File, TI))	return false;

	if(ov_open_callbacks(&File, &VF, NULL, 0, OV_CALLBACKS_FXFILE))	return false;
	ov_pcm_seek(&VF, TI->GetStart(FMT_SAMPLE, SilResolve()));
	return true;
}

// Adapted from those anonymous Coolier th**bgm.c files
void makeheader(char *header,int datasize, uint Freq)
{
	int i;
	short s;
	memcpy(header,"RIFF",4);
	i = datasize + 36;
	memcpy(header+4,&i,4);
	memcpy(header+8,"WAVEfmt ",8);
	i = 16;
	memcpy(header+16,&i,4);
	s = 1;
	memcpy(header+20,&s,2);
	s = 2;
	memcpy(header+22,&s,2);
	i = Freq;
	memcpy(header+24,&i,4);
	i = 4 * Freq;
	memcpy(header+28,&i,4);
	s = 4;
	memcpy(header+32,&s,2);
	s = 16;
	memcpy(header+34,&s,2);
	memcpy(header+36,"data",4);
	i = datasize;
	memcpy(header+40,&i,4);
}

class FadeAlg_Linear : public FadeAlg
{
public:
	short*	Eval(short* f, long& c, long& Len)
	{
		double Step = (double)c / (double)Len;
		*f = (double)*f * (1.0 - Step);	f++;
		*f = (double)*f * (1.0 - Step);	f++;
		return f;
	}

	FadeAlg_Linear()	{Name = "Linear";}
	SINGLETON(FadeAlg_Linear);
};

class FadeAlg_Exp : public FadeAlg
{
public:
	short*	Eval(short* f, long& c, long& Len)
	{
		double Step = (double)c / (double)Len;
		*f = (double)*f * (-pow(0.05, Step) * (Step - 1.0));	f++;
		*f = (double)*f * (-pow(0.05, Step) * (Step - 1.0));	f++;
		return f;
	}

	FadeAlg_Exp()	{Name = "Exponential";}
	SINGLETON(FadeAlg_Exp);
};

// Decryption thread
// -----------------
FXint Decrypter::run()
{
	FXFile In;
	bool Ret;

	if(!In.open(Src))	return 0;

	VF->Clear();
	VF->Create(TI->FS);

	Ret = (GI->PM->DecryptMusic(GI, In, VF->Buf, TI, &VF->Write) != 0);
	In.close();
	detach();
	return Ret;
}

ulong Decrypter::Start(GameInfo* _GI, TrackInfo* _TI, VFile* _VF)
{
	if(running())	return 0;

	Src = _GI->DiskFN(TI);
	GI = _GI;
	TI = _TI;
	VF = _VF;

	start();
	return TI->FS;
}

void Decrypter::Stop()
{
	if(VF)	VF->Write = 0;
}
// -----------------

Extractor::Extractor()
{
	FAs.Add()->Data = &FadeAlg_Linear::Inst();
	FAs.Add()->Data = &FadeAlg_Exp::Inst();
}

Extract_Vals::Extract_Vals()
{
	ts_data = ts_ext = tl = te = 0;
	Len = FadeStart = FadeBytes = f = 0;
	Buf = NULL;
	d = 0;
	StopReq = &Encoder::StopReq;
	Ret = &Extractor::Ret;
}

Extract_Vals::Extract_Vals(TrackInfo* TI, const bool& Fmt)
{
	Extract_Vals();
	Init(TI, Fmt);
}

void Extract_Vals::Init(TrackInfo* TI, const bool& Fmt)
{
	TI->GetPos(Fmt, SilResolve(), &ts_ext, &tl, &te);
	TI->GetPos(Fmt, false, &ts_data);

	Len = TI->GetByteLength(SilResolve(), LoopCnt, FadeDur);

	FadeBytes = (ulong)(fabs(FadeDur) * TI->Freq * 4.0f);
	
	if( (tl == te) || (tl == 0)) FadeBytes = 0;

	FadeBytes = MIN(FadeBytes, Len);
	FadeStart = Len - FadeBytes;
	f = 0;
}

void Extract_Vals::Clear()
{
	In.close();
	Out.close();
	SAFE_FREE(Buf);
	d = 0;
	ts_data = ts_ext = tl = te = 0;
	Len = FadeStart = FadeBytes = f = 0;
}

static void CalcFade(Extract_Vals& V, long& BufSize, FadeAlg* FA)
{
	V.FadeStart -= BufSize;

	if(V.FadeStart <= 0)
	{
		short* f = (short*)&V.Buf[MAX(BufSize + V.FadeStart, 0)];
		for(; V.f < -(V.FadeStart); V.f += 4)	f = FA->Eval(f, V.f, V.FadeBytes);
	}
}

volatile FXuint Extractor::Ret;

// Single track extraction main function
bool Extractor::ExtractTrack(TrackInfo* TI, FXString& OutFN)
{
	bool Ret;
	EncFN.format("%d.%s", TI->Number, Enc->Ext.lower());
	Enc->Active = true;
	Ret = Enc->Extract(TI, EncFN, ActiveGame, V);
	Enc->Active = (*V.StopReq) = false;
	if(!Ret)	return false;

	if(Move(EncFN, OutFN))
	{
		if(V.TagEngine)	Tag(TI, OutFN);
	}
	Cleanup();
	return true;
}

// Ensures [In] to have the (original) PCM data from [TI]. Decodes the file, if necessary.
bool Extractor::PrepareInput(TrackInfo* TI, GameInfo* GI, Extract_Vals& V)
{
	if(!GI->Vorbis)
	{
		if(!ActiveGame->OpenBGMFile(V.In, TI))	return false;
		V.In.position(V.ts_ext);
	}
	else
	{
		Decrypter& Dec = Decrypter::Inst();
		OggVorbis_File VF;
		VFile BGM;
		long ret;
		int Link;
		ulong Size;
		bool ReadConn = false;
		
		BGMLib::UI_Stat_Safe("decoding...");

		V.Buf = (char*)realloc(V.Buf, OV_BLOCK);

		// Directly decode from the original BGM file
		if(GI->CryptKind)
		{
			// Open a virtual file
			Dec.Start(GI, TI, &BGM);

			// Wait for the first block...
			while(BGM.Write < (OV_BLOCK * 2));

			ov_open_callbacks(&BGM, &VF, NULL, 0, OV_CALLBACKS_VFILE);
		}
		else
		{
			if(!OpenVorbisBGM(V.In, VF, GI, TI))	return false;
		}
		Size = TI->GetByteLength(SilResolve(), 1, 0);
		V.d = 0;

		V.Out.open(DecodeFile, FXIO::Writing);
		do
		{
			ret = ov_read(&VF, V.Buf, OV_BLOCK, 0, 2, 1, &Link);
			V.d += V.Out.writeBlock(V.Buf, MAX(ret, 0));

			if(Dec.running())	sleep(2000);
			else if(!ReadConn)
			{
				MW->ProgConnect(&V.d, Size);
				ReadConn = true;
			}
			if(*V.StopReq)	{Dec.Stop();	break;}
		}
		while(V.d < Size);
		
		ov_clear(&VF);

		V.Out.close();
		if(GI->CryptKind)	BGM.Clear();
		else				V.In.close();

		if(*V.StopReq)	return *V.StopReq = false;
		else			MW->ProgConnect();

		V.In.open(DecodeFile, FXIO::Reading);
		SAFE_FREE(V.Buf);
	}
	return true;
}

FXString& Extractor::BuildPCM(TrackInfo* TI, Extract_Vals& V)
{
	char Header[WAV_HEADER_SIZE];
	long BufSize;
	long c = 0;	// Fade progression

	DumpFN.format("%d.wav", TI->Number);
	
	V.Out.open(DumpFN, FXIO::Writing);

	makeheader(Header, V.Len, TI->Freq);
	V.Out.writeBlock(Header, WAV_HEADER_SIZE);

	// Start transfer
	// --------------
	// Intro
	if(TI->FS != 0)	BufSize = V.tl;
	else			BufSize = V.tl - V.ts_ext;

	V.Buf = (char*)realloc(V.Buf, BufSize);
	V.In.readBlock(V.Buf, BufSize);

	CalcFade(V, BufSize, V.FA);

	V.Out.writeBlock(V.Buf, BufSize);

	// Loops
	BufSize = V.te - V.tl;
	if(BufSize > 0)
	{
		V.Buf = (char*)realloc(V.Buf, BufSize);

		for(ushort l = 0; l < LoopCnt && !(*V.StopReq); l++)
		{
			V.In.readBlock(V.Buf, BufSize);

			CalcFade(V, BufSize, V.FA);

			V.Out.writeBlock(V.Buf, BufSize);
			V.In.position(V.tl);
		}
	}

	// Fade
	if(V.FadeBytes != 0)
	{
		long Rem = V.FadeBytes - c;
		short* f;

		if(Rem > BufSize)	V.Buf = (char*)realloc(V.Buf, BufSize);
		BufSize = Rem;

		while(Rem > 0 && !(*V.StopReq))
		{
			ulong Read = MIN(V.te - V.tl, (ulong)Rem);

			V.In.readBlock(V.Buf, Read);
			Rem -= Read;
		
			f = (short*)&V.Buf[0];
			for(c; c < BufSize - Rem; c += 4)	f = V.FA->Eval(f, c, V.FadeBytes);

			V.Out.writeBlock(V.Buf, Read);
			V.In.position(V.tl);
		}
	}
	SAFE_FREE(V.Buf);
	V.Out.close();
	return DumpFN;
}

bool Extractor::Move(FXString& EncFN, FXString& OutFN)
{
	if(!Active)	return false;

	if(!FXFile::moveFiles(EncFN, OutFN, true))
	{
		FXString Str;
		FXuint Cancel;

		Str.format("Couldn't write %s!\nTarget directory may be write-protected. Cancel extraction?", OutFN);
		MW->ThreadMsg(this, Str, &Cancel, MBOX_YES_NO);

		if(Cancel == MBOX_CLICKED_YES)
		{
			Ret = MBOX_CLICKED_CANCEL;
			return false;
		}
		else
		{
			// Try a second time...
			return FXFile::moveFiles(EncFN, OutFN, true);
		}
	}
	return true;
}

uint Extractor::Tag(TrackInfo* TI, FXString& OutFN)
{
	if(!Active)	return Ret;

	BGMLib::UI_Stat_Safe("tagging...");
	if(Tagger::Inst().Tag(TI, OutFN, Ext) == SUCCESS)	BGMLib::UI_Stat_Safe("done.");

	return Ret;
}

uint Extractor::Cleanup()
{
	BGMLib::UI_Stat_Safe("\n");
	MW->ProgConnect();
	V.Clear();
	FX::FXFile::remove(DumpFN);
	FX::FXFile::remove(EncFN);
	FX::FXFile::remove(DecodeFile);

	return Ret;
}

// Extractor
// ---------
bool Extractor::Start(const short& ExtStart, const short& ExtEnd, const ushort& FadeAlgID)
{
	FXString Stat;

	Cur = ExtStart;
	Last = ExtEnd;

	Enc = Encoders.Get(EncFmt - 1)->Data;
	Ext = Enc->Ext.lower();
	V.FA = FAs.Get(MIN(FadeAlgID, FAs.Size() - 1))->Data;

	Stat = "\nExtraction started.\n";
	Stat.append(Enc->Init(ActiveGame));
	Stat.append("-------------------\n");
	BGMLib::UI_Stat_Safe(Stat);

	Ret = MBOX_CLICKED_YES;

	cancel();
	start();

	return Active = true;
}

int Extractor::run()
{
	FXString OutFN, Str;
	
	FXSystem::setCurrentDirectory(FXSystem::getTempDirectory());

	CurTrack = ActiveGame->Track.Get(Cur);
	do
	{
		if(!CurTrack->Data.Start[0] && !ActiveGame->BGMFile.empty())	continue;

		V.DisplayFN = PatternFN(&CurTrack->Data);
		OutFN = OutPath + V.DisplayFN;
		OutFN = OutFN.substitute("\"", "'"); //Replace double quotes which can't be part of filenames.

		if(FXStat::exists(OutFN))
		{
			if(Ret != MBOX_CLICKED_YESALL && Ret != MBOX_CLICKED_NOALL)
			{
				Str = OutFN + " already exists.\nOverwrite?";
				MW->ThreadMsg(this, Str, &Ret, MBOX_YES_YESALL_NO_NOALL_CANCEL);
			}
			if(Ret == MBOX_CLICKED_CANCEL)	break;
			if((Ret != MBOX_CLICKED_YES) && (Ret != MBOX_CLICKED_YESALL))	continue;
		}

		if(!ExtractTrack(&CurTrack->Data, OutFN) || Ret == 0)	break;
		if(Ret == MBOX_CLICKED_CANCEL)	break;
	}
	while( CurTrack && (CurTrack = CurTrack->Next()) && ++Cur < Last);

	Finish();

	return 1;
}

void Extractor::Stop()
{
	if(!Active)	return;
	Ret = 0;
	Enc->Stop();
}

bool Extractor::Finish()
{
	FXString Msg = "\n-------------------\n";

	if(Ret == MBOX_CLICKED_CANCEL)	Msg += "Extraction canceled.";
	else if(Ret == 0)				Msg += "Extraction stopped.";
	else                          	Msg += "Extraction finished.";
	Msg += "\n";
	BGMLib::UI_Stat_Safe(Msg);

	Cleanup();

	CurTrack = NULL;
	Cur = Last = 0;
	MW->ActFinish();

	detach();

	return Active = false;
}
// ---------
