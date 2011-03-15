// Music Room Interface
// --------------------
// pm.h - Pack Method subclasses
// --------------------
// "�" Nmlgc, 2010-2011

// Pack Methods
#define PM_NONE 0x0	// No BGM extraction support, only for tagging
#define BGMDIR  0x1	// BGM Directory containing multiple wave files (th06, Kioh Gyoku)
#define BGMDAT  0x2	// raw PCM data in a single file (usually thbgm.dat). Also supports Vorbis-compressed versions of this file.
#define BMWAV   0x3	// wave files in a single archive with header (only th075)
#define BMOGG   0x4	// Vorbis files and SFL loop info in a single archive with header (other Tasofro games)
#define PBG6    0x5	// Vorbis files and SLI loop info in a single PBG6 encrypted archive (Banshiryuu)

// Encryptions
// -----------
#define CR_NONE   0x0 // No encryption

// Tasofro
#define CR_SUIKA  0x1 // Simple progressive XOR-encrypted header with lots of junk data, unencrypted files (th075, MegaMari)
#define CR_TENSHI 0x2 // Mersenne Twister pseudorandom encrypted header with an optional progressive XOR layer, static XOR-encrypted files (th105, th123, PatchCon)

// PBG6
#define CR_YUITIA 0x3 // Banshiryuu PBG6 encryption 
// Yeah, that was the best I came up with :-)
// -----------

#include <bgmlib/pm_zun.h>

class PM_Tasofro : public PackMethod
{
private:
	ulong HeaderSizeV2(FX::FXFile& In);

	bool IsValidHeader(char* hdr, const FXuint& hdrSize, const FXushort& Files);

	void DecryptHeaderV1(char* hdr, const FXuint& hdrSize);
	void DecryptHeaderV2(char* hdr, const FXuint& hdrSize, const FXushort& Files);

protected:
	virtual void GetPosData(GameInfo* GI, FX::FXFile& In, FXushort& Files, char* hdr, FXuint& hdrSize) = 0;

	ulong HeaderSize(GameInfo* GI, FX::FXFile& In, const FXushort& Files);
	bool DecryptHeader(GameInfo* GI, char* hdr, const FXuint& hdrSize, const FXushort& Files);

	bool CheckCryptKind(ConfigFile& NewGame, const uchar& CRKind);	// Checks if given encryption kind is valid

public:
	bool TrackData(GameInfo* GI);
};

// PM_BMWav
// --------
class PM_BMWav : public PM_Tasofro
{
protected:
	PM_BMWav()	{ID = BMWAV;}

	void GetPosData(GameInfo* GI, FX::FXFile& In, FXushort& Files, char* hdr, FXuint& hdrSize);

public:
	bool CheckBMTracks(GameInfo* Target);

	bool ParseGameInfo(ConfigFile& NewGame, GameInfo* GI);
	bool ParseTrackInfo(ConfigFile& NewGame, GameInfo* GI, ConfigParser* TS, TrackInfo* NewTrack);		// return true if position data should be read from config file
	GameInfo* Scan(const FXString& Path);	// Scans [Path] for a game packed with this method

	SINGLETON(PM_BMWav);
};
// --------

// PM_BMOgg
// --------

class PM_BMOgg : public PM_Tasofro
{
protected:
	PM_BMOgg()	{ID = BMOGG;}

	void MetaData(GameInfo* GI, FXFile& In, const ulong& Pos, const ulong& Size, TrackInfo* TI);	// SFL Format
	void GetPosData(GameInfo* GI, FX::FXFile& In, FXushort& Files, char* hdr, FXuint& hdrSize);

public:
	bool ParseGameInfo(ConfigFile& NewGame, GameInfo* GI);
	bool ParseTrackInfo(ConfigFile& NewGame, GameInfo* GI, ConfigParser* TS, TrackInfo* NewTrack);		// return true if position data should be read from config file

	inline ulong DecryptBuffer(const uchar& CryptKind, char* Out, const ulong& Pos, const ulong& Size);	// Contains the decryption algorithm

	ulong DecryptFile(GameInfo* GI, FXFile& In, char* Out, const ulong& Pos, const ulong& Size, volatile FXulong* p = NULL);

	GameInfo* Scan(const FXString& Path);	// Scans [Path] for a game packed with this method

	SINGLETON(PM_BMOgg);
};
// --------

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
