Music Room Interface Readme
===========================

1. About
2. Distribution
3. Usage
4. Forcing wiki updates
5. Tag data overview
6. Version History
7. Testimonials

===========================

1. About
--------
This program provides easy playing and extraction of music from the Touhou Project games for Windows.

The music can be encoded into multiple formats (Ogg Vorbis native, FLAC and MP3 via bundled external
encoders), while adding correct tags* in either Japanese or English.
Translations are automatically downloaded from the Touhou Wiki (http://en.touhouwiki.net) if selected.

You can customize the loop count, fade duration and automatic filenaming.

It also can be used as a masstagger for the PC-98 soundtracks from Akyu's Untouched Score
and every single one of ZUN's music CDs.

Version 2 clearly makes this the best Touhou BGM extraction tool ever, right?
This is my way of giving back to the fanbase, and I can't imagine a better way than with this tool.
Especially now.

* Or at least what I think is correct. If you disagree, please let me know,
  or directly edit the Touhou Wiki.
--------

2. Distribution
---------------
The newest version of this program, as well as the source code,
can be downloaded from the following links:

- https://github.com/DTM9025/musicroom/releases/latest
- https://github.com/DTM9025/musicroom

Please only share either one of those links and not the direct link to the current version archive.

If this gets included in any kind of complete Touhou distribution (e.g. a DVD you burn yourself
or a complete torrent), this tool has achieved its purpose.

As for the source code, well, do with it whatever you like. It's the internet after all.

Oh yeah, and this program was not made by anybody at shrinemaiden.org.
And especially not by that Drake guy.
If you downloaded a rehosted version from there, shout at them.
---------------

3. Usage
--------
Should basically be self-explanatory. However, you may directly drop a supported game's directory or
any file in it onto musicroom.exe to automatically load it, instead of clicking through the menus.

This tool supports Vorbis encoded versions of games which normally would have lossless soundtracks.
Of course, if a lossless version is present as well, the program will read from that one.
--------

4. Forcing wiki updates
-----------------------
On updating, the program writes the last processed wiki revision to the updated game's BGM file.
This will skip the update process if the game is loaded again and the Wiki page wasn't changed,
thus not bugging you with the same updating questions over and over.

To force a wiki update, open a game's BGM file with a text editor and replace the <wikirev> key
in the [update] section with any other value (preferrably 0).

Like this:

[update]
wikirev = 0

Then start the program and load the game again.
-----------------------

5. Tag data overview
--------------------

The tagging engine always writes tags in both Japanese and English.
Metadata for the selected language is written to the widely and commonly used tag fields
of the format, while metadata for the other language is written to a "custom" tag field,
prefixed with either [EN] or [JP]. Thus, this "additional" data may not be displayed by many players.

The following table shows an overview about the written tag fields for every format.
The other, not selected language is indicated as !LANG.

(Vorbis comments are used in Ogg and FLAC formats, and ID3v2 is used in MP3.
ID3v1 tags are never written. They don't make much sense when working with non-ASCII strings.)

== Game info == 

Vorbis comment      ID3v2
- - - - - - - - - - - - -
ALBUM ARTIST        TXXX/ALBUM ARTIST
ALBUM               TALB
DISCNUMBER          TPOS
TOTALTRACKS         TRCK (format "[track number]/[total tracks]")
DATE                TDRC
GENRE               TCON
CIRCLE              TXXX/CIRCLE
[!LANG]_CIRCLE      TXXX/[!LANG]_CIRCLE
[!LANG]_ARTIST      TXXX/[!LANG]_ARTIST
[!LANG]_ALBUM       TXXX/[!LANG]_ALBUM


== Track info == 

Vorbis comment      ID3v2 
- - - - - - - - - - - - -
ARTIST              TPE1
COMPOSER            TCOM
TITLE               TIT2
TRACKNUMBER         TRCK (format "[track number]/[total tracks]")
COMMENT             COMM
[!LANG]_COMMENT     TXXX/[!LANG]_COMM
[!LANG]_TITLE       TXXX/[!LANG]_TITLE

--------------------

6. Version History
------------------

Moved to the Changelog text file.

------------------

7. Testimonials
---------------
"I LOVE YOU"
- Nameless Fairy

"Thank you so much for this, sir, you're awesome"
- Nameless Fairy

"I've been looking for something like this for some time."
- Shrinemaiden Forum

"Adorei o programa *-*"
- Santuário Hakurei

"Supersedes all other utilities made so far."
- Touhou Wiki

"NamelessLegacy did a great job with his extractor, I wasn't expecting﻿ the Spirit World tracks either"
- DarkOverord

"Well, thanks for your diligence. Your thorough work and lucky break allowed me and others to enjoy an otherwise difficult to locate "extra" track. I say take pride."
- 0ctopusComp1etely
---------------