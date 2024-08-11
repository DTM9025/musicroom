// Music Room Interface
// --------------------
// wikiupdate.cpp - Touhou Wiki Track Info Updating
// --------------------
// "©" Nmlgc, 2010-2011
// "©" DTM9025, 2024

#include <bgmlib/platform.h>

#include <bgmlib/list.h>
#include <bgmlib/config.h>
#include <bgmlib/infostruct.h>
#include <bgmlib/bgmlib.h>
#include <bgmlib/ui.h>
#include <bgmlib/utils.h>

#include <fxascii.h>
#include <FXRex.h>

#include <curl/curl.h>

// Wiki stuff to remove
static const FXString Face = "''";
static const FXString Tmpl[2] = {"{{", "}}"};
static const FXString Link[2] = {"[[", "]]"};
static const FXString HTML[2] = {"<", ">"};
static const FXString Space("   ");

static const FXint regexp_mode(FXRex::Normal|FXRex::Capture|FXRex::IgnoreCase|FXRex::Newline);

// Lots of wiki normalization functions

// Returns the end of the template starting after [t]
static FXint GetBounds(FXString& Str, FXint* s, FXint t, const FXString& markup_open, const FXString& markup_close)
{
	FXint n = 0;	// Template nest counter

	if(s)	*s = Str.find('|', t);

	// Don't stumble over nested templates
	for(n = 1; (t < Str.length() - 1) && (n != 0); t++)
	{
		if(!compare(&Str[t], markup_open, markup_open.length()))		n++;
		else if(!compare(&Str[t], markup_close, markup_close.length()))	n--;
	}

	Str[t-1] = '|';
	return t;
}

size_t write_data(char* File, size_t size, size_t nmemb, FXString* Page)
{
	Page->append(File, size * nmemb);
	return size * nmemb;
}

void RemoveWikiLinks(FXString& Str, const FXString& s_del = Link[0], const FXString& t_del = Link[1])
{
	FXint s = 0, t = 0, c = 0, len;

	while((s = Str.find(s_del, s)) != -1)
	{
		t = Str.find('|', s);
		c = Str.find(t_del, s);

		if((uint)t < (uint)c)	len = t - s + 1;
		else                 	len = t_del.length();

		Str.erase(s, len);
		Str.erase(c - len, t_del.length());
	}
}

FXRex::Error RemoveTemplate(FXString& Str, const FXString& Name)
{
	FXint s = 0, t = 0;
	FXRex	Rex;
	FXRex::Error Ret;

	if(Ret = Rex.parse(Name, regexp_mode))	return Ret;

	while(Rex.match(Str, &s, &t, FXRex::Forward, 1, s))
	{
		Str.erase(s, t-s);
		t = GetBounds(Str, NULL, s, Tmpl[0], Tmpl[1]);
		Str.erase(t-1, 2);
	}
	return Ret;
}

FXRex::Error RemoveLastElm(FXString& Str, const FXString& Name)
{
	FXint s = 0, t = 0;
	FXString Sub;
	FXRex	Rex;
	FXRex::Error Ret;

	if(Ret = Rex.parse(Name, regexp_mode))	return Ret;

	while(Rex.match(Str, &s, &t, FXRex::Forward, 1, s))
	{
		Str.erase(s, t-s);
		t = GetBounds(Str, NULL, s, Tmpl[0], Tmpl[1]);
		if( -1 != (s = Str.find('|', s)) )	Str.erase(s, t+1-s);
		else								Str.erase(t-1, 2);
	}
	return Ret;
}

void RemoveHTMLPairs(FXString& Str, const FXString& markup_open, const FXString& markup_close)
{
	FXint s = 0, t = 0;

	FXRex regexp_html[2];

	regexp_html[0].parse(markup_open + ".*?" + markup_close, regexp_mode);
	regexp_html[1].parse(markup_open + "/.*?" + markup_close, regexp_mode);

	for(ushort c = 0; c < 2; c++)
	{
		while(regexp_html[c].match(Str, &s, &t, FXRex::Forward, 1, s))
		{
			Str.erase(s, t-s);
		}
	}
}

// Translate MediaWiki indents to [Repl]
static void TranslateIndents(FXString& WD, const FXString& Repl = Space)
{
	if(WD.empty())	return;

	FXint s = 0, t;
	const FXchar Indent = ':';
	FXRex regexp_indent("^:", regexp_mode);

	while(regexp_indent.match(WD, &s, &t, FXRex::Forward, 1, s))
	{
		while(WD[s] == Indent)
		{
			WD.replace(s, 1, Repl.text(), Repl.length());
			s += Repl.length();
		}
	}
}

inline FXString TemplateElm(const FXString& WD, const FXString& Tag)
{
	FXString Ret(NamedValue(WD, Tag, "=", "|"));
	return Ret.trim();
}

inline FXString TemplateElm_Comment(const FXString& WD, const FXString& Tag)
{
	FXString Ret(NamedValue(WD, Tag, "=", "|"));
	TranslateIndents(Ret);
	return Ret.trimEnd();
}

FXuint AskForUpdate(TrackInfo* TI, const FXString& Type, FXString* Old, FXString* New, FXuint* Ret, bool ShowStat = true)
{
	if(New->empty() || *Ret == UPDATE_NOALL) return *Ret;

	// Normalize new-lines.
	FXString o = *Old, n = *New;
	o.substitute("\r\n", "\n");
	n.substitute("\r\n", "\n");
	if(o == n) return *Ret;

	FXString Msg;
	if(*Ret != UPDATE_YESALL && !Old->empty())
	{
		Msg.format("Update %s on Track #%d?\n", Type, TI->Number);
		*Ret = BGMLib::UI_Update(Msg, *Old, *New);

		if(*Ret != UPDATE_YES && *Ret != UPDATE_YESALL)	return *Ret;
	}
	if(ShowStat)	Msg.format(" --> #%d: %s -> %s\n", TI->Number, *Old, *New);
	else			Msg.format(" --> #%d: updated %s\n", TI->Number, Type);
	BGMLib::UI_Stat(Msg);
	*Old = *New;	// The actual assignment!
	return *Ret;
}

// Compares two strings, returns false if one is empty
bool CompareEx(FXString& s1, FXString s2)
{
	if(s1.empty() || s2.empty())	return false;
	return !comparecase(s1,s2);
}

static TrackInfo* Eval_MusicRoom(GameInfo* GI, FXString& WD, FXuint* MsgRet)
{
	static const FXString Wiki_Category("category");
	static const FXString Wiki_Name[LANG_COUNT] = {"title", "titleEN"};
	static const FXString Wiki_Comment[LANG_COUNT] = {"comment", "translation"};
	static const FXString Wiki_Source("source");

	FXString Category;
	IntString Name, Comment;
	FXuint TrackNo;
	List<IntString>	Afterword;
	ListEntry<IntString>* TrackCmt, *NewCmt;
	IntString* TC, *NC;
	FXString SrcDesc, SrcVal;
	TrackInfo* Cur = NULL;
	ushort s = 1, l;

	removeSub(WD, Tmpl[1]);

	Category = TemplateElm(WD, Wiki_Category);
	TrackNo = Category.after('#').toUInt();

	for(l = 0; l < LANG_COUNT; l++)
	{
		Name[l] = TemplateElm(WD, Wiki_Name[l]);	removeSub(Name[l], LineBreak[1]);
		Comment[l] = TemplateElm_Comment(WD, Wiki_Comment[l]);
	}

	// Multiple source handling
	do
	{
		// JP comment
		SrcVal.format("%s%d", Wiki_Comment[LANG_JP], s);	
		SrcVal = TemplateElm_Comment(WD, SrcVal);

		// Because some comments skip the first number.
		if(SrcVal.empty() && s == 1)
		{
			s++;
			continue;
		}
		if(SrcVal.empty())	break;
		NC = &Afterword.Add()->Data;

		NC->s[LANG_JP].assign(SrcVal);

		// EN comment
		SrcVal.format("%s%d", Wiki_Comment[LANG_EN], s);
		NC->s[LANG_EN] = TemplateElm_Comment(WD, SrcVal);

		// Source (if present)
		SrcDesc.format("%s%d", Wiki_Source, s);
		SrcDesc = TemplateElm(WD, SrcDesc);

		if(!SrcDesc.empty())
		{
			SrcDesc.prepend('(');	SrcDesc.append(")\n");
			NC->s[LANG_JP].prepend(SrcDesc);
			NC->s[LANG_EN].prepend(SrcDesc);
		}

		s++;
	}
	while(1);

	ListEntry<TrackInfo>* CurTrack;
	if(TrackNo)	CurTrack = GI->Track.Get(TrackNo - 1);
	else		CurTrack = GI->Track.First();

	for(ushort Temp = 0; (Temp < GI->TrackCount) && CurTrack; Temp++)	// No spoilers for trial versions! :-)
	{
		Cur = &(CurTrack->Data);

		if(CompareEx(Cur->Name[LANG_JP], Name[LANG_JP]) || CompareEx(Cur->Name[LANG_EN], Name[LANG_EN]))
		{
			// OK, found the right track
			for(l = 0; l < LANG_COUNT; l++)
			{
				if(AskForUpdate(Cur, BGMLib::LI[l].GUILang + " title", &Cur->Name[l], &Name[l], MsgRet) == UPDATE_CANCEL)	return Cur;
				if(AskForUpdate(Cur, BGMLib::LI[l].GUILang + " comment", &Cur->Comment[l], &Comment[l], MsgRet, false) == UPDATE_CANCEL)	return Cur;
			}

			NewCmt = Afterword.First();
			TrackCmt = Cur->Afterword.First();
			while(NewCmt)
			{
				NC = &NewCmt->Data;
				SrcDesc = NC->s[0].left(NC->s[0].find('\n'));

				if(!TrackCmt)	TrackCmt = Cur->Afterword.Add();
				TC = &TrackCmt->Data;

				if(!SrcDesc.empty())	SrcDesc.prepend(" ");
				SrcDesc.prepend(" supplementary comment");

				for(l = 0; l < LANG_COUNT; l++)
				{
					if(AskForUpdate(Cur, BGMLib::LI[l].GUILang + SrcDesc, &TC->s[l], &NC->s[l], MsgRet, false) == UPDATE_CANCEL)	return Cur;
				}

				NewCmt = NewCmt->Next();
				TrackCmt = TrackCmt->Next();
			}
			
			CurTrack = NULL;
		}
		else	CurTrack = CurTrack->Next();
	}
	return Cur;
}

static void Eval_MusicRoom_Cmt(TrackInfo* TI, const ushort& Index, FXString& WD, FXuint* MsgRet)
{
	static const FXString Wiki_Cmt[LANG_COUNT] = {"ja", "en"};

	FXString New;
	FXString Caption;

	ListEntry<IntString>* TrackCmt = TI->Afterword.Get(Index);
	if(!TrackCmt)	TrackCmt = TI->Afterword.Add();

	Caption.format(" comment (paragraph #%d)", Index + 1);

	for(ushort l = 0; l < LANG_COUNT; l++)
	{
		New = TemplateElm_Comment(WD, Wiki_Cmt[l]);
		if(AskForUpdate(TI, BGMLib::LI[l].GUILang + Caption, &TrackCmt->Data.s[l], &New, MsgRet, false) == UPDATE_CANCEL)	return;
	}

	return;
}

bool Download(CURL* C, const FXString& URL, FXString* Buffer)
{
	FXString Str;

	curl_easy_setopt(C, CURLOPT_WRITEDATA, Buffer);
	curl_easy_setopt(C, CURLOPT_URL, URL);
	CURLcode CRet = curl_easy_perform(C);
	if(CRet != CURLE_OK)
	{
		Str = curl_easy_strerror(CRet);
		Str.prepend("\nERROR: ");
		Str.append('\n');
		BGMLib::UI_Stat(Str);
		return false;
	}
	return true;
}

static bool Update_Finish(CURL* C);
static bool Update_Cancel(CURL* C);
static bool Update_Complete(CURL* C);

static const FXString Wiki_MusicRoom(Tmpl[0] + "MusicRoom");
static const FXString Wiki_MusicRoom_Cmt(Wiki_MusicRoom + "/Cmt");

bool Update(GameInfo* GI, FXString& WikiURL)
{
	if(GI->WikiPage.empty())	return false;

	bool Ret;
	FXuint MsgRet;

	FXString URL, Page, Str;
	FXint s = 0, t, Index = 0;
	FXint p = 0;
	CURL* C = NULL;
	TrackInfo* TI = NULL;
	ListEntry<IntString>* TrackCmt = NULL;

	ulong RevID;

	URL = WikiURL;
	if( (s = URL.find("/wiki")) != -1)	URL.trunc(s);

	Str.format("Getting track info from (%s).\n  (%s)... ", URL, GI->WikiPage);
	BGMLib::UI_Stat(Str);

	curl_global_init(CURL_GLOBAL_ALL);

	C = curl_easy_init();

	curl_easy_impersonate(C, "chrome116", 1);
	curl_easy_setopt(C, CURLOPT_SSL_VERIFYPEER, FALSE);
	curl_easy_setopt(C, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(C, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(C, CURLOPT_WRITEFUNCTION, write_data);

DL:
	GI->WikiPage.substitute(' ', '_');
	URL.format(WikiURL.text(), GI->WikiPage);

	if(!Download(C, URL, &Page))	return Update_Finish(C);

	// Get revision id
	// ---------------
	if((s = Page.find("<revision>")) != 1)
	{
		s = Page.find("<id>", s) + 4;
		t = Page.find("</id>", s);
		RevID = Page.mid(s, t - s).toULong();
	}

	if(RevID == GI->WikiRev)
	{
		BGMLib::UI_Stat("\nTrack info is already up-to-date.\n");
		return Update_Finish(C);
	}
	// ---------------

	// Prepare buffer
	// --------------
	if((s = Page.find("<text ", s)) == -1)
	{
		BGMLib::UI_Stat("\nWiki data is corrupted, update not possible!");
		return Update_Finish(C);
	}

	Page = Page.right(Page.length() - s);
	s = 0;

	// Resolve Wiki redirects
	// ----------------------
	Str = NamedValue(Page, "#REDIRECT", "[[", "]]");
	if(!Str.empty())
	{
		GI->WikiPage = Str;
		Str.format("\nResolving #REDIRECT to (%s)...", GI->WikiPage);
		BGMLib::UI_Stat(Str);
		goto DL;
	}
	// ----------------------

	BGMLib::UI_Stat("\n ...done, evaluating...\n");
		
	removeSub(Page, Face);
	RemoveTemplate(Page, "\\{\\{lang\\|.*?\\|");

	// Remove references
	{
		FXRex regexp_ref;
		if(!regexp_ref.parse("\\{\\{ref\\|.*?\\}\\}", regexp_mode))
		{
			while(regexp_ref.match(Page, &s, &t, FXRex::Forward, 1, s))
			{
				Page.erase(s, t-s);
			}
		}
	}

	RemoveLastElm(Page, "\\{\\{ruby-.*?\\|");
	RemoveWikiLinks(Page);

	// Empty space in some comments
	const char Empty[2] = {LineBreak[1], ' '};
	FXchar Space[4] = {' ', Moonspace[0], Moonspace[1], Moonspace[2]};	// STUPID "IDEOGRAPHIC WHITE SPACE"

	Page.substitute(Empty, 2, &LineBreak[1], 1, true);

								Page.substitute(Space, 4, Space, 1, true);
	Space[0] = '=';         	Page.substitute(Space, 4, Space, 1, true);
	Space[0] = LineBreak[1];	Page.substitute(Space, 4, Space, 1, true);
	
	Page.substitute("  ", " ");
	// HTML
	Page.substitute("&gt;", ">");
	Page.substitute("&lt;", "<");

	// Remove other references
	{
		FXRex regexp_ref;
		if(!regexp_ref.parse("<ref>.*?</ref>", regexp_mode))
		{
			while (regexp_ref.match(Page, &s, &t, FXRex::Forward, 1, s))
			{
				Page.erase(s, t-s);
			}
		}
	}

	// Too less time for a proper fix right now
	// TODO: Investigate linebreak behavior in Touhou Wiki
	Page.substitute("<br />\n", "\n"); // Hopefully this renders stuff correctly.
	Page.substitute("<br />", "\n");

	// Magical Astronomy...
	{
		FXRex regexp_sup("<sup>", regexp_mode);
		FXRex regexp_external("\\[.*? ", regexp_mode);

		s = 0;
		// Closing tag gets deleted automatically below
		while(regexp_sup.match(Page, &s, &t, FXRex::Forward, 1, s))
		{
			Page.replace(s, t-s, "^", 1);
		}
		s = 0;
		while(regexp_external.match(Page, &s, &t, FXRex::Forward, 1, s))
		{
			Page.erase(s, t-s);
			if( (s = Page.find(']', s)) != -1)	Page.erase(s);
		}
	}

	RemoveHTMLPairs(Page, HTML[0], HTML[1]);

	Page.substitute("&quot;", "\"");
	// --------------

	// Start processing...
	s = t = p = 0;
	
	FXRex regexp_musicroom("\\{\\{MusicRoom(?!/Cmt)", regexp_mode);
	FXRex regexp_musicroom_cmt("\\{\\{MusicRoom/Cmt", regexp_mode);

	Ret = regexp_musicroom.match(Page, &s, &t, FXRex::Forward, 1, p);

	// s = start position of current template (temporary)
	// t = (end) position of next MusicRoom template to evaluate
	// p = end position of a MusicRoom/Cmt template
	while(Ret)
	{
		t = GetBounds(Page, &s, t, Tmpl[0], Tmpl[1]);
		TI = Eval_MusicRoom(GI, Page.mid(s, t-s), &MsgRet);
		p = t;
		
		if(MsgRet == UPDATE_CANCEL)	return Update_Cancel(C);
		else if(MsgRet == UPDATE_NOALL)	break;

		// Next MusicRoom
		if(!(Ret = regexp_musicroom.match(Page, &s, &t, FXRex::Forward, 1, p)))	t = Page.length();
		
		if(!TI)	continue;

		// If we find a table end before the next MusicRoom template,
		// we may probably have some {{MusicRoom/Cmt}}s there

		s = Page.find("|}", p + 1);
		if(s < t)
		{
			Index = 0;
			
			while(regexp_musicroom_cmt.match(Page, &s, &p, FXRex::Forward, 1, p, t))
			{
				p = GetBounds(Page, &s, p, Tmpl[0], Tmpl[1]);
				Eval_MusicRoom_Cmt(TI, Index, Page.mid(s, p-s), &MsgRet);
				Index++;

				if(MsgRet == UPDATE_CANCEL)	return Update_Cancel(C);
				else if(MsgRet == UPDATE_NOALL)	Ret = false;
			}
		}
	}
	
	GI->WikiRev = RevID;

	return Update_Complete(C);
}

static bool Update_Finish(CURL* C)
{
	BGMLib::UI_Stat("\n");
	curl_easy_cleanup(C);
	curl_global_cleanup();
	return false;
}

static bool Update_Cancel(CURL* C)
{
	BGMLib::UI_Stat("Update canceled.\n");
	Update_Finish(C);
	return false;
}

static bool Update_Complete(CURL* C)
{
	BGMLib::UI_Stat("Update completed.\n");
	Update_Finish(C);
	return true;
}