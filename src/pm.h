// Music Room Interface
// --------------------
// pm.h - Additional Pack Method subclasses
// --------------------
// "�" Nmlgc, 2010-2011

#ifndef MUSICROOM_PM_H
#define MUSICROOM_PM_H

// Pack Methods
#define PM_NONE 0x0	// No BGM extraction support, only for tagging
#define BGMDIR  0x1	// BGM Directory containing multiple wave files (th06, Kioh Gyoku)
#define BGMDAT  0x2	// raw PCM data in a single file (usually thbgm.dat). Also supports Vorbis-compressed versions of this file.
#define BMWAV   0x3	// wave files in a single archive with header (only th075)
#define BMOGG   0x4	// Vorbis files and SFL loop info in a single archive with header (other Tasofro games)
#define PBG6    0x5	// Vorbis files and SLI loop info in a single PBG6 encrypted archive (Banshiryuu)
#define BMPAK   0x6	// Vorbis files and SFL loop info in .pak files with header (th135 - th155)
#define BMCGA   0x7 // Vorbis files and INI loop info in .cga/b files with footer (th175)

// Encryptions
// -----------
#define CR_NONE   0x0 // No encryption

// Tasofro
#define CR_SUIKA    0x1 // Simple progressive XOR-encrypted header with lots of junk data, unencrypted files (th075, MegaMari)
#define CR_TENSHI   0x2 // Mersenne Twister pseudorandom encrypted header with an optional progressive XOR layer, static XOR-encrypted files (th105, th123, PatchCon)

// PBG6
#define CR_YUITIA 0x3 // Banshiryuu PBG6 encryption 
// Yeah, that was the best I came up with :-)
// -----------

// Tasofro TH13.5 - TH15.5
#define CR_KOKORO   0x4 // RSA Encrypted
#define CR_SUMIREKO 0x5 // RSA Encrypted

// Tasofro TH17.5
#define CR_YUUMA  0x6 // CGA Encryption

// PAK Versions
// -----------
#define PV_KOKORO   0x0
#define PV_SUMIREKO 0x1

#include <bgmlib/pm_zun.h>
#include <bgmlib/pm_tasofro.h>
#include <bgmlib/pm_tfpk.h>
#include <bgmlib/pm_tfcga.h>

// PM_PBG6
// -------

class PM_PBG6 : public PackMethod
{
protected:
	PM_PBG6()	{ID = PBG6;}

	void MetaData(GameInfo* GI, FXFile& In, const ulong& Pos, const ulong& Size, TrackInfo* TI);	// .SLI format
	void AudioData(GameInfo* GI, FXFile& In, const ulong& Pos, const ulong& Size, TrackInfo* TI);	// Additionally reads frequency from the first few bytes of the Vorbis file (because it's inconsistent between the tracks... great)
	void GetPosData(GameInfo* GI, FXFile& In, ulong& Files, char* toc, ulong& tocSize);

	void CryptStep(ulong* pool1, ulong* pool2, ulong& ecx);	// Decryption algorithm

public:
	ulong Decrypt(volatile FXulong& d, char* dest, const char* source, const ulong& size);	// Returns source bytes actually read
	ulong DecryptFile(GameInfo* GI, FXFile& In, char* Out, const ulong& Pos, const ulong& Size, volatile FXulong* p = NULL);

	bool ParseGameInfo(ConfigFile& NewGame, GameInfo* GI);
	bool ParseTrackInfo(ConfigFile& NewGame, GameInfo* GI, ConfigParser* TS, TrackInfo* NewTrack);		// return true if position data should be read from config file

	bool TrackData(GameInfo* GI);	// Reads archive header

	GameInfo* Scan(const FXString& Path);	// Scans [Path] for a game packed with this method

	SINGLETON(PM_PBG6);
};
// -------

#endif /* MUSICROOM_PM_H */
