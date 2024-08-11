// Music Room Interface
// --------------------
// enc_vorbis.cpp - Built-in Vorbis encoding
// --------------------
// "©" Nmlgc, 2011
// "©" DTM9025, 2024

#include "musicroom.h"
#include <bgmlib/ui.h>
#include <bgmlib/list.h>
#include <bgmlib/config.h>

#include <FXHash.h>
#include <FXStream.h>
#include <FXDialogBox.h>
#include <FXVerticalFrame.h>
#include <FXGroupBox.h>
#include <FXCheckButton.h>
#include <FXLabel.h>
#include <FXThread.h>

#include <FXIO.h>
#include <FXFile.h>

#include "extract.h"
#include "enc_vorbis.h"

#include <bgmlib/libvorbis.h>
#include "tag_base.h"
#include "tag_vorbis.h"
#include "tagger.h"
#include <th_tool_shared/LCVorbisQuality.h>

#include <assert.h>

// Settings
// --------
void Encoder_Vorbis::FmtReadConfig(ConfigParser* Sect)
{
	Sect->LinkValue("quality", TYPE_FLOAT, &Quality);
	Sect->LinkValue("chain_stream_assemble", TYPE_BOOL, &ChainStreamAssemble);
}

void Encoder_Vorbis::DlgCreate(FXVerticalFrame* Frame, FXDialogBox* Target, const FXuint& Msg)
{
	FXVerticalFrame* MSFrame;

	VQ = new LCVorbisQuality(Frame, Quality);

	MSFrame = new FXVerticalFrame(Frame, LAYOUT_FILL);
		MS = new FXCheckButton(MSFrame, "Create chained bitstream output files where possible");

		new FXLabel(MSFrame,
			"This is the recommended mode of operation.\n"
			"\n"
			"For lossless sources, the looping part will only be encoded once and copied for each\n"
			"loop, instead of constructing a full looped and faded dump and encoding it in one go.\n"
			"\n"
			"If the game's BGM is already encoded in chained bitstream Ogg format, those streams\n"
			"are copied without re-encoding. Fades are then re-encoded with the above quality.\n"
			"\n"
			"However, many players don't support chained bitstream Ogg files correctly, even though\n"
			"this feature is explicitly stated in the Ogg specification. If the extracted files don't\n"
			"work on your setup, disable this option, or trash your player (I recommend the latter).\n"
			"\n"
			"(If you share those files, choose an archiver that generates a high compression\n"
			"ratio on those. This way, additional loops won't add to the archive size!)",
			NULL, LABEL_NORMAL | JUSTIFY_LEFT);

	MS->setCheck(ChainStreamAssemble);
}

bool Encoder_Vorbis::DlgApply(FXDialogBox* Parent)
{
	Quality = VQ->getQuality();
	ChainStreamAssemble = MS->getCheck() != FALSE;
	return true;
}
// --------

// Encoding
// --------

// Storage
static OggVorbis_EncState ES;
static OggVorbis_File VF;
static MRTag_Ogg*	TF;

static bool CheckRIFF(FXFile& In, uint* Freq = NULL, ulong* DataSize = NULL)
{
	ulong Temp;

	In.readBlock(&Temp, 4);	// "RIFF"
	if(memcmp(&Temp, "RIFF", 4))	return false;
	
	In.position(4, FXIO::Current);
	In.readBlock(&Temp, 4);	// "WAVE"
	if(memcmp(&Temp, "WAVE", 4))	return false;

	In.position(12, FXIO::Current);
	In.readBlock(&Temp, 4);	// Frequency
	if(Freq)	*Freq = Temp;

	In.position(8, FXIO::Current);
	In.readBlock(&Temp, 4);	// "data"

	if(memcmp(&Temp, "data", 4))	return false;
	
	In.readBlock(&Temp, 4);	// data size
	if(DataSize)	*DataSize = Temp;
	return true;
}

FXString Encoder_Vorbis::Init(GameInfo* GI)
{
	FXString Ret;
	if(ChainStreamAssemble)
		Ret = "Bulding (chained) bitstream files where possible.\n"
		"If those don't work with your player, disable this option in the settings dialog.\n";
	else
		Ret.format("%s BGM to unchained files. To build chained bitstream files\n"
		"%swhere possible, enable this option in the settings dialog.\n",
		GI->Vorbis ? "Re-encoding" : "Encoding", GI->Vorbis ? "without re-encoding " : "");

	Ret.prepend("(Ogg) ");

	return Ret;
}

bool Encoder_Vorbis::Encode(const FXString& DestFN, const FXString& SrcFN, Extract_Vals& V)
{
	FXString Str;
	uint Freq = 44100;
	ulong	Temp = 0;
	bool eos = false;

	V.Buf = (char*)realloc(V.Buf, OV_BLOCK / 2);
	V.d = 0;

	V.In.open(SrcFN, FXIO::Reading);
	V.Out.open(DestFN, FXIO::Writing);

	// In
	CheckRIFF(V.In, &Freq, &Temp);
	if(Temp > 0)	MW->ProgConnect(&V.d, Temp);

	// Out
	if(!ES.setup(&V.Out, Freq, Quality / 10.0f))	return false;

	ogg_stream_init(&ES.stream_out, rand());
	vorbis_write_headers(V.Out, &ES.stream_out, &ES.vi, &TF->vc);

	while(!eos && !StopReq)
	{
		int Read = V.In.readBlock(V.Buf, OV_BLOCK / 2);
		ES.encode_pcm(V.Buf, Read);
		V.d += Read;
		if(Read == 0)	eos = true;
	}
	ES.clear();
	V.In.close();
	V.Out.close();
	SAFE_FREE(V.Buf);

	SAFE_DELETE(TF);
	if(StopReq)	return StopReq = false;
	else		return true;
}

bool Encoder_Vorbis::Extract(TrackInfo* TI, FXString& EncFN, GameInfo* GI, Extract_Vals& V)
{
	Extractor& Ext = Extractor::Inst();

	FXString Str;
	bool CSA;	// Local chain stream assemble

	memset(&VF, 0, sizeof(OggVorbis_File));

	// The most special cases...
	// -------------------------
	if(GI->Vorbis && TI->FS != 0)
	{
		if( (TI->Loop == 0) || (FadeDur == 0.0f && LoopCnt == 1) )
		{
			Str.format("Directly copying %s...", V.DisplayFN.text());
			BGMLib::UI_Stat_Safe(Str);

			DumpDecrypt(GI, TI, EncFN);
			if(StopReq)	return StopReq = false;
			
			return V.TagEngine = true;
		}
	}
	// We're always writing ourselves
	V.TagEngine = false;

	// Determine if we can assemble, if requested
	CSA = ChainStreamAssemble;
	if(CSA)
	{
		// Check source file
		// ----------------
		GI->OpenBGMFile(V.In, TI);
		if(GI->Vorbis)
		{
			if(!GI->CryptKind)
			{
				ulong Start;

				// Directly decode from the original BGM file
				if(ov_open_callbacks(&V.In, &VF, NULL, 0, OV_CALLBACKS_FXFILE))	return false;
				if(VF.links == 1)	CSA = false;

				// This is necessary because seeking to 0 apparently breaks the codebooks
				Start = TI->GetStart(FMT_SAMPLE, CSA ? 0 : SilResolve());
				if(!(CSA && !Start) )	ov_pcm_seek(&VF, Start);
			}
			else	CSA = false;
		}
		else
		{
			V.In.position(TI->GetStart(FMT_BYTE, SilResolve()));
		}
	}

	// Prepare tags
	// ------------
	Tagger& T = Tagger::Inst();
	TF = new MRTag_Ogg;

	T.TagBasic(TF, ActiveGame, TI);
	T.TagExt(TF, ActiveGame, TI);
	// -------------------------

	if(CSA)	BGMLib::UI_Stat_Safe("(chained) ");
	else
	{
		if(ChainStreamAssemble)	BGMLib::UI_Stat_Safe("(unchained) ");
		return Extract_Default(TI, EncFN, GI, V);
	}

	// Alright, from here on, we're in bitstream assembling mode
	Str.format("Building %s...", V.DisplayFN.text());
	BGMLib::UI_Stat_Safe(Str);

	V.Init(TI, GI->Vorbis);

	V.Out.open(EncFN, FXIO::Writing);

	long serialno;
	int Link;
	ogg_int64_t CopySamples = -1;
	ogg_int64_t StartSample = 0;
	ogg_int64_t StreamLen;
	long EncLen;

	long c = 0;
	
	short* f;

	V.Buf = (char*)malloc(OV_BLOCK);
	
	// Start transfer
	// --------------
	
	if(GI->Vorbis)
	{
		V.FadeBytes >>= 2;
		V.FadeStart >>= 2;
	}

	srand(FXThread::time());	// This is no crypto-system, but we still aren't as stupid as Sony

	serialno = rand();
	StartSample = V.ts_ext - V.ts_data;

	if(!GI->Vorbis || V.FadeBytes != 0)	ES.setup(&V.Out, TI->Freq, Quality / 10.0f);
		
	// Intro
	ogg_stream_init(&ES.stream_out, serialno);

	if(GI->Vorbis)
	{
		Link = ov_bitstream_seek(&VF, V.ts_data, true);
		vorbis_write_headers(V.Out, &ES.stream_out, &ES.vi, &TF->vc);

		StreamLen = ov_pcm_total(&VF, Link);

		EncLen = TI->GetByteLength(SilResolve(), LoopCnt, FadeDur);
	}
	else
	{
		BGMLib::UI_Stat_Safe("intro...");
		vorbis_write_headers(V.Out, &ES.stream_out, &ES.vi, &TF->vc);

		EncLen = TI->GetByteLength(SilResolve(), 1, fabs(FadeDur));
		if(V.FadeStart < EncLen)	EncLen = V.FadeStart + V.FadeBytes;

		StreamLen = V.tl - ( (TI->FS != 0) ? 0 : V.ts_data);
	}
	MW->ProgConnect(&V.d, EncLen);

	if(StreamLen > V.FadeStart)
	{
		StreamLen = CopySamples = V.FadeStart;
		V.FadeStart = 0;
	}
	else	V.FadeStart -= (StreamLen - StartSample);

	if(GI->Vorbis)
	{
		V.d += ogg_packetcopy(V.Out, &ES.stream_out, &VF, CopySamples, StartSample) * 4;
		if(StopReq)	return StopReq = false;
		if(CopySamples == -1)
		{
			// It's better to assume that the bitstreams of one track are adjacent
			ov_raw_seek(&VF, VF.offsets[VF.current_link + 1]);
			Link = VF.current_link;
		}
		else					ov_pcm_seek(&VF, V.ts_ext + CopySamples);
	}
	else if(!ES.encode_file(V.In, (StreamLen - StartSample), V.Buf, OV_BLOCK, V.d, &StopReq))	return StopReq = false;
	
	V.Out.flush();

	// If we're lossless, we'll always need to encode the loop at this point.
	FXFile LF;
	if(!GI->Vorbis && V.FadeStart > 0)
	{
		bool Ret;
		OggVorbis_EncState LS;
		long Rem = MIN( (V.te - V.tl), (ulong)V.FadeStart);
		BGMLib::UI_Stat_Safe("loop...");

		LF.open(DecodeFile, FXIO::Writing);
		LS.setup(&LF, TI->Freq, Quality / 10.0f);

		ogg_stream_init(&LS.stream_out, ++serialno);
		vorbis_write_headers(LF, &LS.stream_out, &LS.vi, &TF->vc);
		LF.flush();

		Ret = LS.encode_file(V.In, Rem, V.Buf, OV_BLOCK, V.d, &StopReq);
		
		// Reset encoding to outfile
		LS.clear();
		LF.close();

		if(!Ret)
		{
			FXFile::removeFiles(DecodeFile);
			return StopReq = false;
		}

		LF.open(DecodeFile);

		ov_open_callbacks(&LF, &VF, NULL, 0, OV_CALLBACKS_FXFILE);
		ov_bitstream_seek(&VF, 0, true);
		Link = -1;
		V.FadeStart >>= 2;

		BGMLib::UI_Stat_Safe("chain...");
	}
		
	// Can we still copy?
	for(ushort l = 0; (l < LoopCnt) && (V.FadeStart != 0); l++)
	{
		ogg_int64_t Ret;

		ogg_stream_reset_serialno(&ES.stream_out, ++serialno);
		vorbis_write_headers(V.Out, &ES.stream_out, &ES.vi, &TF->vc);

		StreamLen = ov_pcm_total(&VF, Link);
		if(StreamLen >= V.FadeStart)
		{
			StreamLen = CopySamples = V.FadeStart;
			V.FadeStart = 0;
		}
		else	V.FadeStart -= StreamLen;

		Ret = ogg_packetcopy(V.Out, &ES.stream_out, &VF, CopySamples);
		if(StopReq)	return StopReq = false;

		if(GI->Vorbis)
		{
			V.d += Ret * 4;
			if(CopySamples == -1)	ov_raw_seek(&VF, VF.offsets[VF.current_link]);
			else					ov_pcm_seek(&VF, V.tl + CopySamples);
		}
		else
		{
			if(CopySamples == -1)
			{
				// If we intend to copy packets, we _always have to use_ bitstream seeking!
				ov_bitstream_seek(&VF, 0, true);
				V.In.position(V.tl);
			}
			else	V.In.position(V.tl + (CopySamples << 2));
		}
	}

	if(!GI->Vorbis)	ov_clear(&VF);
	
	V.Out.flush();
	assert(V.FadeStart == 0);

	// (Decode and re-)encode fades
	if(V.FadeBytes != 0 && !StopReq)
	{
		if(GI->Vorbis)
		{
			BGMLib::UI_Stat_Safe("re-encoding fade...");
			V.FadeBytes <<= 2;
		}
		else	BGMLib::UI_Stat_Safe("fade...");

		// Sometimes, the dsp state happens to break, so...
		ES.clear();
		ES.setup(&V.Out, TI->Freq, Quality / 10.0f);
		if(CopySamples != 0)
		{
			ogg_stream_clear(&ES.stream_out);
			ogg_stream_init(&ES.stream_out, ++serialno);
			vorbis_write_headers(V.Out, &ES.stream_out, &ES.vi, &TF->vc);
		}

		V.Out.flush();

		//ret = ( vorbis_encode_setup_managed(&vi,2,44100,-1,VF.vi->bitrate_nominal,-1) ||
		//	   vorbis_encode_ctl(&vi,OV_ECTL_RATEMANAGE2_SET,NULL) ||
		//		vorbis_encode_setup_init(&vi));

		if(StopReq)	return StopReq = false;
		
		long Rem = V.FadeBytes;
		while((Rem > 0) && !StopReq)
		{
			int Read = MIN(OV_BLOCK, Rem);

			// Yup, that former streaming function takes care of everything
			if(GI->Vorbis)	ov_read_bgm(&VF, V.Buf, Read, TI);
			else			pcm_read_bgm(V.In, V.Buf, Read, TI);

			Rem -= Read;

			f = (short*)&V.Buf[0];
			for(c; c < V.FadeBytes - Rem; c += 4)	f = V.FA->Eval(f, c, V.FadeBytes);

			ES.encode_pcm(V.Buf, Read);

			V.d += Read;
		}
		// Finalize
		ES.encode_pcm(NULL, 0);
	}
	if(StopReq)	return StopReq = false;
	MW->ProgConnect();

	SAFE_FREE(V.Buf);
	// --------------
	V.In.close();
	
	ES.clear();
	ov_clear(&VF);

	V.Out.close();
	SAFE_DELETE(TF);
	
	return true;
}

void Encoder_Vorbis::FmtStop()
{
	while(StopReq)	FXThread::sleep(TIMEOUT);
	ES.clear();
	ogg_stream_clear(&ES.stream_out);
	ov_clear(&VF);
	SAFE_DELETE(TF);
}
