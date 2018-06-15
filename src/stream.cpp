// Music Room Interface
// --------------------
// stream.cpp - Wave streaming
// --------------------
// "©" Nmlgc, 2010-2011

#include "musicroom.h"
#include <FXFile.h>
#include <bgmlib/libvorbis.h>
#include "stream.h"
#include <bgmlib/packmethod.h>
#include <bgmlib/ui.h>

#ifdef WIN32

// Initialization
// --------------
Streamer::Streamer()
{
	Active = false;
	DS = NULL;
	SB = NULL;
	Write = 0;

	ZeroMemory(&Fmt, sizeof(WAVEFORMATEX));
	Track = NULL;
}

bool Streamer::Init(void* xid)
{
	MMRESULT Ret;

	BGMLib::UI_Stat("Initializing DirectSound streaming...\n");

	// Fill wave format structure
	Fmt.wFormatTag = WAVE_FORMAT_PCM;
	Fmt.nChannels = 2;
	Fmt.nSamplesPerSec = 44100;
	Fmt.wBitsPerSample = 16;
	Fmt.nBlockAlign = (Fmt.wBitsPerSample >> 3) * Fmt.nChannels;
    Fmt.nAvgBytesPerSec = Fmt.nBlockAlign * Fmt.nSamplesPerSec;
	Fmt.cbSize = 0;

	Ret = DirectSoundCreate8(NULL, &DS, NULL);

	if(Ret != DS_OK)
	{
		BGMLib::UI_Stat("WARNING: DirectSound device could not be opened.\nTrack playback won't be available.\n");
	}
	else	Active = true;
	
	Ret = DS->SetCooperativeLevel((HWND)xid, DSSCL_PRIORITY);

	DSBUFFERDESC BD;
	BD.dwSize = sizeof(DSBUFFERDESC);
	BD.dwBufferBytes = BlockSize * BlockCount;
	BD.dwFlags = DSBCAPS_LOCDEFER | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;
	BD.lpwfxFormat = &Fmt;
	BD.guid3DAlgorithm = DS3DALG_DEFAULT;
	BD.dwReserved = 0;

	Ret = DS->CreateSoundBuffer(&BD, &SB, NULL);
	ClearBuffer();

	return Active;
}

FXint Streamer::run()
{
	ulong Play, CurBE;
	StopReq = false;

	while(!StopReq)
	{
		sleep(TIMEOUT);
		if(New)
		{
			if(!ActiveGame->Scanned)	New = NULL;
			SwitchTrack();
		}
		else if(Track && CurFile.isOpen())
		{
			SetVolume();

			// Fuck notifications, I'm writing my own, TRANSPARENT system.
			// We're streaming on each call, unless the new data would extend until after the current play cursor.

			// Since the write cursor is always placed after the play cursor, this will, after starting the process,
			// result in a "chase" situation, where the (usually faster) write cursor tries to catch up to the play cursor.
			// Once the buffer end reached it, we wait until it passed again and then slowly fill up the bytes it just passed
			// without intersecting.

			// So it's basically a reversed version of the internal DirectSound method. And mine is way more sane, if you ask me.

			SB->GetCurrentPosition(&Play, NULL);
			CurBE = Write + BlockSize;
			
			if( (CurBE > BufferSize) && Play < (Write - BlockSize))	Play += BufferSize;

			if(!BETWEEN_EQUAL(Play, Write, CurBE))
			{
				if(StreamFrame(Write, BlockSize))	Write = CurBE;
				if(Write > BufferSize)	Write -= BufferSize;
			}
		}
	}
	StopReq = false;
	detach();
	return 1;
}

void Streamer::StreamFrame_WAV(char* Buffer, const ulong& Size)
{
	Pos = pcm_read_bgm(CurFile, Buffer, Size, Track);
}

void Streamer::StreamFrame_OGG(char* Buffer, const ulong& Size)
{
	Pos = ov_read_bgm(&SF, Buffer, Size, Track);
}

bool Streamer::StreamFrame(const ulong& Offset, const ulong& Size)
{
	if(!ActiveGame)	return false;

	HRESULT Ret;
	ulong ReadBufferSize[2];
	char* ReadBuffer[2];

	Ret = SB->Lock(Offset, Size, (void**)&ReadBuffer[0], &ReadBufferSize[0], (void**)&ReadBuffer[1], &ReadBufferSize[1], 0);

	for(ushort b = 0; b < 2; b++)
	{
		if(ActiveGame->Vorbis)	StreamFrame_OGG(ReadBuffer[b], ReadBufferSize[b]);
		else					StreamFrame_WAV(ReadBuffer[b], ReadBufferSize[b]);
	}
	
	Ret = SB->Unlock(ReadBuffer[0], ReadBufferSize[0], ReadBuffer[1], ReadBufferSize[1]);
	return true;
}

bool Streamer::SwitchTrack_WAV(TrackInfo* NewTrack, FXString& NewFN)
{
	FXuint NewFNHash = NewFN.hash();
	if(!Track || !CurFile.isOpen() || (CurFNHash != NewFNHash) )
	{
		CloseFile();

		if(!ActiveGame->OpenBGMFile(CurFile, NewTrack))	return false;
	}
	Pos = NewTrack->GetStart(FMT_BYTE, SilResolve());
	CurFile.position(Pos);

	SB->SetFrequency(NewTrack->Freq);
	CurFNHash = NewFNHash;

	return true;
}

bool Streamer::SwitchTrack_OGG(TrackInfo* NewTrack, FXString& NewFN)
{
	FXuint NewFNHash = NewFN.hash();
	if(ActiveGame->CryptKind)
	{
		// Decrypt temporary play file from archive and load it
		CloseFile();

		if(!DumpDecrypt(ActiveGame, NewTrack, OggPlayFile))	return false;
		CurFile.open(OggPlayFile, FXIO::Reading);
		ov_open_callbacks(&CurFile, &SF, NULL, 0, OV_CALLBACKS_FXFILE);
	}
	else if(Track && CurFNHash == NewFNHash)
	{
		ov_pcm_seek(&SF, NewTrack->GetStart(FMT_SAMPLE, SilResolve()));
	}
	else
	{
		CloseFile();

		if(!OpenVorbisBGM(CurFile, SF, ActiveGame, NewTrack))	return false;
	}
	SB->SetFrequency(NewTrack->Freq);
	CurFNHash = NewFNHash;
	
	return true;
}

// Always called by thread
bool Streamer::SwitchTrack()
{
	bool Ret;

	TrackInfo* Load = New;
	// SB->Stop();
	ClearBuffer();
		
	if(!New)
	{
		Track = NULL;
		return true;
	}

	do
	{
		if(Load != New)	Load = New;

		FXString NewFN = ActiveGame->DiskFN(Load);

		if(ActiveGame->Vorbis)	Ret = SwitchTrack_OGG(Load, NewFN);
		else					Ret = SwitchTrack_WAV(Load, NewFN);
	}
	while(Load != New);	// ... yeah, maybe someone scrolls through very fast and already changed [New] to a different track than the one we loaded

	if(!Ret)
	{
		New = NULL;
		return Ret;
	}

	Track = Load;
	New = Load = NULL;
	
	// Initial load
	// ------------
	ulong DXWrite = 0, Play = 0, c = 1;

	// That was the only case when it was still bugged!
	while(Play == 0 || DXWrite == 0)
	{
		SB->SetCurrentPosition(c += 32);	// Skip a bit ahead to reduce pops and clicks
		SB->GetCurrentPosition(&Play, &DXWrite);
	}

	// StreamFrame(DXWrite, BlockSize);

	Write = DXWrite + BlockSize;
	if(Write > BufferSize)	Write -= BufferSize;
	// ------------

	if(!StopReq)	SB->Play(0, 0, DSBPLAY_LOOPING);
	return Ret;
}

void Streamer::SetVolume()
{
	long VolLog = log10f(MAX(Volume, 1)) * abs(DSBVOLUME_MIN / 2) + DSBVOLUME_MIN;
	VolLog = MIN(VolLog, 0);
	
	SB->SetVolume(VolLog);
}

void Streamer::RequestTrackSwitch(TrackInfo* NewTrack)
{
	if(NewTrack != Track)	New = NewTrack;
}

void Streamer::Play()
{
	if(!New)	SB->Play(0, 0, DSBPLAY_LOOPING);	// Fixes game/track switching artifacts from the last song
	if(!running())	start();			// (SwitchTrack will turn the stream back on in those cases)
	StopReq = false;
}

void Streamer::Stop()
{
	if(running())	StopReq = true;
	if(SB)	SB->Stop();
	while(StopReq && !New)
	{
		sleep(TIMEOUT);
	}
}

void Streamer::ClearBuffer()
{
	char* b;
	ulong s;

	if(!Active)	return;

	SB->Lock(0, BufferSize, (void**)&b, &s, NULL, NULL, DSBLOCK_ENTIREBUFFER);
	ZeroMemory(b, s);
	SB->Unlock(b, s, NULL, NULL);
}

void Streamer::CloseFile()
{
	if(CurFile.isOpen())
	{
		ov_clear(&SF);
		CurFile.close();
		Track = NULL;
	}
	FXFile::remove(OggPlayFile);
	ClearBuffer();	// Necessary for game switches!
}

void Streamer::Exit()
{
	if(!Active)	return;

	Stop();
	CloseFile();

	SB->Release();
	DS->Release();
	BGMLib::UI_Stat("Streamer closed.\n");
	Active = false;
}

#endif

// Front end
// ---------
bool StreamerFront::Init(void* xid)	{return Streamer::Inst().Init(xid);}
TrackInfo* StreamerFront::CurTrack()	{return Streamer::Inst().Track;}
void StreamerFront::RequestTrackSwitch(TrackInfo* NewTrack)	{return Streamer::Inst().RequestTrackSwitch(NewTrack);}
void StreamerFront::Play()	{return Streamer::Inst().Play();}
void StreamerFront::Stop()	{return Streamer::Inst().Stop();}
void StreamerFront::CloseFile()	{return Streamer::Inst().CloseFile();}
void StreamerFront::Exit()	{return Streamer::Inst().Exit();}

ulong StreamerFront::Pos()
{
	Streamer& Str = Streamer::Inst();
	if(!Str.Track)	return 0;

	ulong Ret = Str.Pos;
	if(ActiveGame->Vorbis)	Ret <<= 2;
	if(Str.Track->FS == 0)	Ret -= Str.Track->GetStart(FMT_BYTE, SilResolve());
	return Ret;
}
// ---------
