// Music Room Interface
// --------------------
// pm_pbg6.cpp - Parsing for PBG6 archives (Banshiryuu)
// --------------------
// "©" Nmlgc, 2011
// "©" DTM9025, 2024

#include "musicroom.h"
#include <FXFile.h>
#include <bgmlib/packmethod.h>
#include <bgmlib/config.h>
#include <bgmlib/utils.h>
#include "pm.h"

bool PM_PBG6::ParseGameInfo(ConfigFile &NewGame, GameInfo *GI)
{
	GI->CryptKind = CR_YUITIA;
	return PF_PGI_BGMFile(NewGame, GI);
}

bool PM_PBG6::ParseTrackInfo(ConfigFile &NewGame, GameInfo *GI, ConfigParser* TS, TrackInfo *NewTrack)
{
	TS->GetValue("filename", TYPE_STRING, &NewTrack->NativeFN);
	NewTrack->PosFmt = FMT_SAMPLE;
	GI->Vorbis = true;

	return true;	// Read (additional) position info from BGM file. Because they didn't even got _that_ right.
}

// Decryption
// ----------

// Constants
const ulong CP1_SIZE = 0x102;
const ulong CP2_SIZE = 0x400;

void PM_PBG6::CryptStep(ulong* pool1, ulong* pool2, ulong& ecx)
{
	static const ulong cmp = (CP1_SIZE - 1);

	pool2[ecx]++;
	ecx++;
	while(ecx <= cmp)
	{
		pool1[ecx]++;
		ecx++;
	}

	if(pool1[cmp] < 0x10000)	return;

	pool1[0] = 0;

	for(ushort c = 0; c < cmp; c++)
	{
		pool2[c] = (pool2[c] | 2) >> 1;
		pool1[c + 1] = pool1[c] + pool2[c];
	}

	return;
}

ulong PM_PBG6::Decrypt(volatile FXulong& d, char* dest, const char* source, const ulong& size)
{
	// Source byte checks are disabled here because they aren't necessary and I don't want to add yet another member to TrackInfo.
	// Those few extra bytes we load this way are worth my sanity.

	ulong ebx = 0, ecx, edi, esi, edx;
	ulong cryptval[2];
	ulong s = 4;
	d = 0;	// source and destination bytes
	FXulong LastD = d;

	ulong pool1[CP1_SIZE];
	ulong pool2[CP2_SIZE];

	for(ulong c = 0; c < CP1_SIZE; c++)	pool1[c] = c;
	for(ulong c = 0; c < CP2_SIZE; c++)	pool2[c] = 1;
	
	edi = EndianSwap(*(ulong*)source);
	esi = 0xFFFFFFFF;
	
	while(d == LastD)
	{
		edx = 0x100;

		cryptval[0] = esi / pool1[0x101];
		cryptval[1] = (edi - ebx) / cryptval[0];

		ecx = 0x80;
		esi = 0;

		while(1)
		{
			while( (ecx != 0x100) && (pool1[ecx] > cryptval[1]))
			{
				ecx--;
				edx = ecx;
				ecx = (esi+ecx) >> 1;
			}

			if(cryptval[1] < pool1[ecx+1])	break;

			esi = ecx+1;
			ecx = (esi+edx) >> 1;
		}

		*(dest + d) = (char)ecx;	// Write!
		if(++d >= size)	return s;
		LastD++;

		esi = (long)pool2[ecx] * (long)cryptval[0];	// IMUL

		ebx += pool1[ecx] * cryptval[0];
		CryptStep(pool1, pool2, ecx);

		ecx = (ebx + esi) ^ ebx;

		while(!(ecx & 0xFF000000))
		{
			ebx <<= 8;
			esi <<= 8;
			edi <<= 8;

			ecx = (ebx+esi) ^ ebx;

			edi += *(source + s) & 0x000000FF;
			s++;
			// if(++s >= sourcesize)	return s;
		}
		
		while(esi < 0x10000)
		{
			esi = 0x10000 - (ebx & 0x0000FFFF);

			ebx <<= 8;
			esi <<= 8;
			edi <<= 8;

			edi += *(source + s) & 0x000000FF;
			s++;
			// if(++s >= sourcesize)	return s;
		}
	}
	return s;
}
// ----------

#define THRESHOLD_BYTES 32768

// Maybe later I can better refactor this, but for now this is fine.
ulong PM_PBG6::DecryptTrack(GameInfo* GI, FXFile& In, char* Out, TrackInfo* TI, volatile FXulong* p)
{
	return DecryptFile(GI, In, Out, TI->GetStart(), TI->FS, p);
}

ulong PM_PBG6::DecryptFile(GameInfo* GI, FXFile& In, char* Out, const ulong& Pos, const ulong& Size, volatile FXulong* p)
{
	char* Crypt;
	ulong r;
	FXulong t;
	volatile FXulong& d = p ? *p : t;
	
	if(!Out)	return false;
	if(!(Crypt = new char[Size]))	return false;

	if(!In.position(Pos))	return false;
	if(!In.readBlock(Crypt, Size))	return false;

	if(Size > THRESHOLD_BYTES)	MW->ProgConnect(&d, Size);

	r = Decrypt(d, Out, Crypt, Size);
	SAFE_DELETE_ARRAY(Crypt);

	if(Size > THRESHOLD_BYTES)	MW->ProgConnect();

	return r;
}

// Data
// ----
void PM_PBG6::AudioData(GameInfo* GI, FXFile& In, const ulong& Pos, const ulong& Size, TrackInfo* TI)
{
	PackMethod::AudioData(GI, In, Pos, Size, TI);
	
	// Get bitrate by reading 0x28 - 0x2A
	char HeaderPart[0x2A];
	ushort Temp;

	DecryptFile(GI, In, HeaderPart, Pos, 0x2A);
	memcpy(&Temp, HeaderPart + 0x28, 2);
	TI->Freq = Temp;
}

void PM_PBG6::MetaData(GameInfo* GI, FXFile& In, const ulong& Pos, const ulong& Size, TrackInfo* TI)
{
	if( (TI->Loop != 0) && (TI->End != 0))	return;

	char* SLI_Decrypt = new char[Size];
	FXString SLI;

	DecryptFile(GI, In, SLI_Decrypt, Pos, Size);
	
	SLI = SLI_Decrypt;

	if(!TI->Loop)	TI->Loop = NamedValue(SLI, "LoopStart", "=", "\r\n").toULong();
	if(!TI->End)	TI->End = NamedValue(SLI, "LoopLength", "=", "\r\n").toULong() + TI->Loop;

	SAFE_DELETE_ARRAY(SLI_Decrypt);
}

void PM_PBG6::GetPosData(GameInfo* GI, FXFile& In, ulong& Files, char* toc, ulong& tocSize)
{
	char* p = toc;
	FXString FN, FNExt;
	ulong CFPos = 0, CFSize[2] = {0, 0};	// 0: insize, 1: outsize

	for(ulong c = 0; c < Files; c++)
	{
		FN = p;
		p += FN.length() + 1;
		memcpy_advance(CFSize, &p, 8);
		memcpy_advance(&CFPos, &p, 4);
		p += 4;

		PF_TD_ParseArchiveFile(GI, In, FN, "ogg", "ogg.sli", CFPos, CFSize[1]);
	}

	GI->Scanned = true;
}
// ----

bool PM_PBG6::TrackData(GameInfo* GI)
{
	FXFile In;
	ulong Files;
	ulong Pos, Size[2];	// 0: insize, 1: outsize

	char* Toc;

	In.open(GI->BGMFile, FXIO::Reading);
	Size[0] = In.size();
	In.position(4);
	In.readBlock(&Pos, 4);
	In.readBlock(&Size[1], 4);

	Size[0] -= Pos;
	Toc = new char[Size[1]];

	DecryptFile(GI, In, Toc, Pos, Size[1]);

	Files = *((ulong*)Toc);
	GetPosData(GI, In, Files, Toc + 4, Size[1]);

	SAFE_DELETE_ARRAY(Toc);

	// ... done!
	In.close();

	return true;
}

// Scanning
// --------
GameInfo* PM_PBG6::Scan(const FXString& Path)
{
	return PF_Scan_BGMFile(Path);
}
// --------
