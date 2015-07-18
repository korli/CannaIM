/* Copyright 1993 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

/* $Id: RK.h,v 1.10 2003/09/21 10:16:49 aida_s Exp $ */
#ifndef		_RK_h
#define		_RK_h


#include <canna/sysdep.h>

typedef	struct {
   int		ylen;		/* yomigana no nagasa (in byte) */ 
   int		klen;		/* kanji no nagasa (in byte) */
   int		rownum;		/* row number */
   int		colnum;		/* column number */
   int		dicnum;		/* dic number */
}		RkLex;

typedef	struct {
   int		bunnum;		/* bunsetsu bangou */
   int		candnum;	/* kouho bangou */
   int		maxcand;  	/* sou kouho suu */
   int		diccand;	/* jisho ni aru kouho suu */
   int		ylen;		/* yomigana no nagasa (in byte) */ 
   int		klen;		/* kanji no nagasa (in byte) */
   int		tlen;		/* tango no kosuu */
}		RkStat;

struct DicInfo {
    unsigned char	*di_dic;
    unsigned char	*di_file;
    int			di_kind;
    int			di_form;
    unsigned		di_count;
    int			di_mode;
    long		di_time;
};

/* romaji/kanakanji henkan code */
#define	RK_XFERBITS	4	/* bit-field width */
#define	RK_XFERMASK	((1<<RK_XFERBITS)-1)
#define	RK_NFER		0	/* muhenkan */
#define	RK_XFER		1	/* hiragana henkan */
#define	RK_HFER		2	/* hankaku henkan */
#define	RK_KFER		3	/* katakana henkan */
#define	RK_ZFER		4	/* zenkaku  henkan */

#define	RK_CTRLHENKAN		0xf
#define	RK_HENKANMODE(flags)	(((flags)<<RK_XFERBITS)|RK_CTRLHENKAN)

#define RK_TANBUN		0x01
#define RK_MAKE_WORD		0x02
#define RK_MAKE_EISUUJI		0x04
#define RK_MAKE_KANSUUJI	0x08

/* RkRxDic
 *	romaji/kana henkan jisho 
 */
struct RkRxDic	{
    int                 dic;		/* dictionary version: see below */
    unsigned char	*nr_string;	/* romaji/kana taiou hyou */
    int			nr_strsz;	/* taiou hyou no size */
    unsigned char	**nr_keyaddr;	/* romaji key no kaishi iti */
    int			nr_nkey;	/* romaji/kana taiou suu */
    unsigned char       *nr_bchars;     /* backtrack no trigger moji */
    unsigned char       *nr_brules;     /* backtrack no kanouseino aru rule */
};

#define RX_KPDIC 0 /* new format dictionary */
#define RX_RXDIC 1 /* old format dictionary */
#define RX_PTDIC 2 /* large format dictionary (almost equal to KPDIC) */

/* kanakanji henkan */

/* romaji hennkan code */
#define	RK_FLUSH	0x8000	/* flush */
#define	RK_SOKON	0x4000	/* sokuon shori */
#define RK_IGNORECASE	0x2000  /* ignore case */

#define	RK_BIN		0
#define	RK_TXT		0x01

#define	RK_MWD	        0
#define	RK_SWD		1
#define	RK_PRE		2
#define	RK_SUC		3

#define KYOUSEI		0x01		/* jisho_overwrite_mode */

#define	Rk_MWD		0x80		/* jiritsugo_jisho */
#define	Rk_SWD		0x40		/* fuzokugo_jisho */
#define	Rk_PRE		0x20		/* settougo_jisho */
#define	Rk_SUC		0x10		/* setsubigo_jisho */

/* permission for RkwChmod() */
#define RK_ENABLE_READ   0x01
#define RK_DISABLE_READ  0x02
#define RK_ENABLE_WRITE  0x04
#define RK_DISABLE_WRITE 0x08
/* chmod for directories */
#define RK_USR_DIR       0x3000
#define RK_GRP_DIR       0x1000
#define RK_SYS_DIR       0x2000
#define RK_DIRECTORY     (RK_USR_DIR | RK_GRP_DIR | RK_SYS_DIR)
/* chmod for dictionaries */
#define RK_USR_DIC       0	/* specify user dic */
#define RK_GRP_DIC       0x4000	/* specify group dic */
#define RK_SYS_DIC       0x8000	/* specify system dic */

#define PL_DIC		 0x0100
#define PL_ALLOW	 0x0200
#define PL_INHIBIT	 0x0400
#define PL_FORCE	 0x0800

#define	NOENT	-2	/* No such file or directory		*/
#define	IO	-5	/* I/O error				*/
#define	NOTALC	-6	/* Cann't alloc. 			*/
#define	BADF	-9	/* irregal argument			*/
#define	BADDR	-10	/* irregal dics.dir	 		*/
#define	ACCES	-13	/* Permission denied 			*/
#define	NOMOUNT	-15	/* cannot mount				*/
#define	MOUNT	-16	/* file already mounted			*/
#define	EXIST	-17	/* file already exits			*/
#define	INVAL	-22	/* irregal argument			*/
#define	TXTBSY	-26	/* text file busy			*/
#define BADARG	-99	/* Bad Argment				*/
#define BADCONT -100	/* Bad Context				*/
#define OLDSRV    -110
#define NOTUXSRV  -111
#define NOTOWNSRV -112

/* kanakanji henkan */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANNAWC_DEFINED
# if defined(CANNA_NEW_WCHAR_AWARE)
#  define CANNAWC_DEFINED
#  ifdef CANNA_WCHAR16
typedef canna_uint16_t cannawc;
#  else
typedef canna_uint32_t cannawc;
#  endif
# elif defined(_WCHAR_T)
#  error "You can't use old wide character for RK interface"
# endif
#endif

#ifdef CANNAWC_DEFINED

void RkwFinalize(void);
int RkwInitialize(char *);
int RkwCreateContext(void);
int RkwCloseContext(int);
int RkwDuplicateContext(int);
int RkwSetDicPath(int, char *);
int RkwGetDirList(int, char *,int);
int RkwGetDicList(int, char *,int);
int RkwMountDic(int, char *, int);
int RkwUnmountDic(int, char *);
int RkwRemountDic(int, char *, int);
int RkwSync(int, char *);
int RkwGetMountList(int, char *, int);
int RkwDefineDic(int, char *, cannawc *);
int RkwDeleteDic(int, char *, cannawc *);
int RkwBgnBun(int, cannawc *, int, int);
int RkwEndBun(int, int);
int RkwGoTo(int, int);
int RkwLeft(int);
int RkwRight(int);
int RkwXfer(int, int);
int RkwNfer(int);
int RkwNext(int);
int RkwPrev(int);
int RkwResize(int, int);
int RkwEnlarge(int);
int RkwShorten(int);
int RkwSubstYomi(int, int, int, cannawc *, int);
int RkwStoreYomi(int, cannawc *, int);
int RkwGetLastYomi(int, cannawc *, int);
int RkwFlushYomi(int);
int RkwRemoveBun(int, int);
int RkwGetStat(int, RkStat *);
int RkwGetYomi(int, cannawc *, int);
int RkwGetHinshi(int, cannawc *, int);
int RkwGetKanji(int, cannawc *, int);
int RkwGetKanjiList(int, cannawc *, int);
int RkwGetLex(int, RkLex *, int);
int RkwCvtHira(cannawc *, int, cannawc *, int);
int RkwCvtKana(cannawc *, int, cannawc *, int);
int RkwCvtHan(cannawc *, int, cannawc *, int);
int RkwCvtZen(cannawc *, int, cannawc *, int);
int RkwCvtEuc(cannawc *, int, cannawc *, int);
int RkwCreateDic(int, char *, int);
int RkwQueryDic(int, char *, char *, struct DicInfo *);
void RkwCloseRoma(struct RkRxDic *);
struct RkRxDic * RkwOpenRoma(char *);
int RkwSetUserInfo(char *, char *, char *);
char * RkwGetServerName(void);
int RkwGetServerVersion(int *, int *);
int RkwListDic(int, char *, char *, int);
int RkwCopyDic(int, char *, char *, char *, int);
int RkwRemoveDic(int, char *, int);
int RkwRenameDic(int, char *, char *, int);
int RkwChmodDic(int, char *, int);
int RkwGetWordTextDic(int, unsigned char *,
					 unsigned char *, cannawc *, int);
int RkwGetSimpleKanji(int, char *, cannawc *, int,
					 cannawc *, int, cannawc *, int);
int RkwStoreRange(int, cannawc *, int);

#endif

void	RkFinalize(void);
int     RkInitialize(char *);
int    	RkCreateContext(void);
int     RkCloseContext(int);
int	RkDuplicateContext(int);
int	RkSetDicPath(int, char *);
int	RkGetDirList(int, char *,int);
int	RkGetDicList(int, char *,int);
int	RkMountDic(int, char *, int);
int	RkUnmountDic(int, char *);
int	RkRemountDic(int, char *, int);
int	RkSync(int, char *);
int	RkGetMountList(int, char *, int);
int	RkDefineDic(int, char *, char *);
int	RkDeleteDic(int, char *, char *);
int	RkBgnBun(int, char *, int, int);
int	RkEndBun(int, int);
int	RkGoTo(int, int);
int	RkLeft(int);
int	RkRight(int);
int	RkXfer(int, int);
int	RkNfer(int);
int	RkNext(int);
int	RkPrev(int);
int	RkResize(int, int);
int	RkEnlarge(int);
int	RkShorten(int);
int	RkSubstYomi(int, int, int, char *, int);
int	RkStoreYomi(int, char *, int);
int	RkGetLastYomi(int, char *, int);
int	RkFlushYomi(int);
int	RkRemoveBun(int, int);
int	RkGetStat(int, RkStat *);
int	RkGetYomi(int, unsigned char *, int);
int	RkGetHinshi(int, unsigned char *, int);
int	RkGetKanji(int, unsigned char *, int);
int	RkGetKanjiList(int, unsigned char *, int);
int	RkGetLex(int, RkLex *, int);
int	RkCvtHira(unsigned char *, int, unsigned char *, int);
int	RkCvtKana(unsigned char *, int, unsigned char *, int);
int	RkCvtHan(unsigned char *, int, unsigned char *, int);
int	RkCvtZen(unsigned char *, int, unsigned char *, int);
int	RkCvtNone(unsigned char *, int, unsigned char *, int);
int	RkCvtEuc(unsigned char *, int, unsigned char *, int);
int	RkQueryDic(int, char *, char *, struct DicInfo *);

#ifdef __cplusplus
}
#endif

#if defined(ENGINE_SWITCH)
struct rkfuncs {
  int (*GetProtocolVersion)(int *, int *);
  char *(*GetServerName)(void);
  int (*GetServerVersion)(int *, int *);
  int (*Initialize)(char *);
  void (*Finalize)(void);
  int (*CreateContext)(void);
  int (*DuplicateContext)(int);
  int (*CloseContext)(int);
  int (*SetDicPath)(int, char *);
  int (*CreateDic)(int, unsigned char *, int);
  int (*SyncDic)(int, char *);
  int (*GetDicList)(int, char *, int);
  int (*GetMountList)(int, char *, int);
  int (*MountDic)(int, char *, int);
  int (*RemountDic)(int, char *, int);
  int (*UnmountDic)(int, char *);
  int (*DefineDic)(int, char *, wchar_t *);
  int (*DeleteDic)(int, char *, wchar_t *);
  int (*GetHinshi)(int, wchar_t *, int);
  int (*GetKanji)(int, wchar_t *, int);
  int (*GetYomi)(int, wchar_t *, int);
  int (*GetLex)(int, RkLex *, int);
  int (*GetStat)(int, RkStat *);
  int (*GetKanjiList)(int, wchar_t *, int);
  int (*FlushYomi)(int);
  int (*GetLastYomi)(int, wchar_t *, int);
  int (*RemoveBun)(int, int);
  int (*SubstYomi)(int, int, int, wchar_t *, int);
  int (*BgnBun)(int, wchar_t *, int, int);
  int (*EndBun)(int, int);
  int (*GoTo)(int, int);
  int (*Left)(int);
  int (*Right)(int);
  int (*Next)(int);
  int (*Prev)(int);
  int (*Nfer)(int);
  int (*Xfer)(int, int);
  int (*Resize)(int, int);
  int (*Enlarge)(int);
  int (*Shorten)(int);
  int (*StoreYomi)(int, wchar_t *, int);
  int (*SetAppName)(int, char *);
  int (*SetUserInfo)(char *, char *, char *);
  int (*QueryDic)(int, char *, char *, struct DicInfo *);
  int (*CopyDic)(int, char *, char *, char *, int);
  int (*ListDic)(int, char *, char *, int);
  int (*RemoveDic)(int, char *, int);
  int (*RenameDic)(int, char *, char *, int);
  int (*ChmodDic)(int, char *, int);
  int (*GetWordTextDic)(int, unsigned char	*, unsigned char *,
			     wchar_t *, int);
  int (*GetSimpleKanji)(int, char *, wchar_t *, int, wchar_t *, int,
			     wchar_t *, int);
};
#endif /* ENGINE_SWITCH */

#ifdef CANNA_EXP_PREDEFINED
#undef CANNA_EXP_PREDEFINED
#else
#undef canna_export
#endif

#endif	/* _RK_h */
/* don't add stuff after this line */
