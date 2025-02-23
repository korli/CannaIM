/* Copyright 1994 NEC Corporation, Tokyo, Japan.
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

#include "RKintern.h"
#include "patchlevel.h"
#include <canna/jrkanji.h>

#include <errno.h>
#include <sys/stat.h>

static unsigned long now_context = 0L;

#define cx_gwt cx_extdata.ptr
#define STRCMP(d, s) strcmp((char*)(d), (char*)(s))

struct RkGram SG;
struct RkParam SX;

/* RkInitialize: Renbunsetsu Henkan shokika
 *	subeteno Renbunsetsu henkan kannsuu wo siyou suru maeni
 *      itido dake call suru koto.
 * returns: -1/0
 */
static struct RkContext* CX;

#ifdef MMAP
/* If you compile with Visual C++, then please comment out the next 3 lines. */
#include <fcntl.h>     /* mmap */
#include <sys/mman.h>  /* mmap */
#include <sys/types.h> /* mmap */
int fd_dic = -1;       /* mmap */
#endif

#ifdef WINDOWS_STYLE_FILENAME
#define DEFAULTGRAMDIC "/canna/fuzokugo.cbd"
#endif

#ifndef DEFAULTGRAMDIC
#define DEFAULTGRAMDIC "/canna/fuzokugo.d"
#endif

static int
_RkInitialize(char* ddhome, int numCache)
{
  size_t i = strlen(ddhome);
  struct RkParam* sx = &SX;
  struct DD* dd = &sx->dd;
  char *gramdic, *path;
  int con;
#ifdef __EMX__
  struct stat statbuf;
#endif

#ifdef MMAP
  if ((fd_dic == -1) && (fd_dic = open("/dev/zero", O_RDWR)) < 0) {
    con = -1;
    goto return_con;
  }
#endif

  if (sx->flag & SX_INITED) {
    con = -1;
    goto return_con;
  }

  gramdic = malloc(strlen(DEFAULTGRAMDIC) + i + 1);
  if (gramdic) {
    strcpy(gramdic, ddhome);
    strcat(gramdic, DEFAULTGRAMDIC);
    SG.gramdic = RkOpenGram(gramdic);
    free(gramdic);
    if (SG.gramdic) {
      /* confirm user/ and group/ directory */
      path = malloc(strlen(ddhome) + strlen(USER_DIC_DIR) + 2);
      if (path) {
        strcpy(path, ddhome);
        strcat(path, "/");
        strcat(path, USER_DIC_DIR);
        if (mkdir(path, MKDIR_MODE) < 0 && errno != EEXIST) {
          free(path);
        } else {
          free(path);

          path = malloc(strlen(ddhome) + strlen(GROUP_DIC_DIR) + 2);
          if (path) {
            strcpy(path, ddhome);
            strcat(path, "/");
            strcat(path, GROUP_DIC_DIR);
            if (mkdir(path, MKDIR_MODE) < 0 && errno != EEXIST) {
              free(path);
            } else {
              free(path);

              sx->word = NULL;
              dd->dd_next = dd->dd_prev = dd;
              sx->ddhome = strdup(ddhome);
              if (sx->ddhome) {
                SG.P_BB = RkGetGramNum(SG.gramdic, "BB");
                SG.P_NN = RkGetGramNum(SG.gramdic, "NN");
                SG.P_T00 = RkGetGramNum(SG.gramdic, "T00");
                SG.P_T30 = RkGetGramNum(SG.gramdic, "T30");
                SG.P_T35 = RkGetGramNum(SG.gramdic, "T35");
#ifdef LOGIC_HACK
                SG.P_KJ = RkGetGramNum(SG.gramdic, "KJ");
#endif
                SG.P_Ftte = RkGetGramNum(SG.gramdic, "Ftte");
                CX = (struct RkContext*)calloc(INIT_CONTEXT,
                                               sizeof(struct RkContext));
                if (CX) {
                  now_context += INIT_CONTEXT;
                  if (_RkInitializeCache(numCache) == 0) {
                    sx->ddpath = _RkCreateDDP(SYSTEM_DDHOME_NAME);
                    if (sx->ddpath) {
                      con = RkwCreateContext();
                      if (con >= 0) {
                        sx->flag |= SX_INITED;
                        goto return_con;
                      }
                      _RkFreeDDP(sx->ddpath);
                      sx->ddpath = NULL;
                    }
                    _RkFinalizeCache();
                  }
                  free(CX);
                  now_context = 0L;
                }
                free(sx->ddhome);
              }
            }
          }
        }
      }
      RkCloseGram(SG.gramdic);
    }
  }
  con = -1;
return_con:
  return con;
}

int
RkwInitialize(char* ddhome)
{
  /*
   * Word:	????
   * Cache:	36B*512 	= 20KB
   * Heap:	30*1024B	= 30KB
   */
  return (ddhome ? _RkInitialize(ddhome, 512 * 10) : -1);
}

/* RkFinalize: Renbunsetu henkan shuuryou shori
 *
 */
static void
_RkFinalizeWord() /* finalize free word list */
{
  struct nword *w, *t;

  /* dispose each page in list */
  for (w = SX.page; w; w = t) {
    t = w->nw_next;
    free(w);
  }
  SX.word = NULL;
  SX.page = NULL;
  SX.word_in_use = 0;
  SX.page_in_use = 0;
}

void
RkwFinalize()
{
  struct RkParam* sx = &SX;
  int i;

  /* already initialized */
  if (!(sx->flag & SX_INITED))
    return;
  /* houchi sareta context wo close */
  for (i = 0; (unsigned long)i < now_context; i++)
    if (IS_LIVECTX(&CX[i]))
      RkwCloseContext(i);
  free(CX);
  now_context = 0L;
  /* sonohoka no shuuryou shori */
  _RkFinalizeWord();
  _RkFinalizeCache();
  free(sx->ddhome);
  sx->ddhome = NULL;
  _RkFreeDDP(sx->ddpath);
  RkCloseGram(SG.gramdic);
  sx->flag &= ~SX_INITED;
}

/* RkGetSystem: System heno pointer wo motomeru
 */
struct RkParam*
RkGetSystem()
{
  return (&SX);
}

/* RkGetSystemDD: System heno pointer wo motomeru
 */
struct DD*
RkGetSystemDD()
{
  struct RkParam* sx;
  return (((sx = RkGetSystem()) && sx->ddpath) ? sx->ddpath[0] : NULL);
}

/* RkGetContext: Context heno pointer wo motomeru
 *	-> RKintern.h
 */
struct RkContext*
RkGetContext(int cx_num)
{
  return (IsLiveCxNum(cx_num) ? &CX[cx_num] : NULL);
}

struct RkContext*
RkGetXContext(int cx_num)
{
  struct RkContext* cx;

  cx = RkGetContext(cx_num);
  if (cx)
    if (!IS_XFERCTX(cx))
      cx = NULL;
  return (cx);
}

void
_RkEndBun(struct RkContext* cx)
{
  struct DD** ddp = cx->ddpath;
  int c;

  cx->flags &= ~(CTX_XFER | CTX_XAUT);
  cx->concmode &=
    ~(RK_CONNECT_WORD | RK_MAKE_WORD | RK_MAKE_KANSUUJI | RK_MAKE_EISUUJI);
  for (c = 0; c < 4; c++) {
    struct MD *head, *md, *nd;

    head = cx->md[c];
    for (md = head->md_next; md != head; md = nd) {
      struct DM* dm = md->md_dic;
      struct DF* df = dm->dm_file;
      struct DD* dd = df->df_direct;

      nd = md->md_next;
      if (md->md_flags & MD_MPEND) /* release pending */
        md->md_flags &= ~MD_MPEND;
      if (md->md_flags & MD_UPEND) /* unmount pending */
        _RkUmountMD(cx, md);
      else if (!_RkIsInDDP(ddp, dd)) /* unreachable */
        _RkUmountMD(cx, md);
    }
  }
}

/* RkSetDicPath
 *
 */

int
RkwSetDicPath(int cx_num, char* path)
{
  struct RkContext* cx = RkGetContext(cx_num);
  struct DD** new;

  new = _RkCreateDDP(path);
  if (new) {
    _RkFreeDDP(cx->ddpath);
    cx->ddpath = new;
    return (0);
  }
  return (-1);
}

/*
  fillContext -- コンテクスト構造体の決まったところに値を埋めてやる。

  return value:
    0 OK
   -1 ダメ
 */

static int
fillContext(int cx_num)
{
  struct RkContext* cx = &CX[cx_num];
  int i;

  /* create mount list headers */
  for (i = 0; i < 4; i++) {
    struct MD* mh;

    if (!(mh = (struct MD*)calloc(1, sizeof(struct MD)))) {
      int j;

      for (j = 0; j < i; j++) {
        free(cx->md[i]);
        cx->md[i] = NULL;
      }
      return -1;
    }
    mh->md_next = mh->md_prev = mh;
    mh->md_dic = NULL;
    mh->md_flags = 0;
    cx->md[i] = mh;
  }
  cx->dmprev = NULL;
  cx->qmprev = NULL;
  cx->nv = NULL;
  cx->ddpath = NULL;
  cx->kouhomode = (unsigned long)0;
  cx->concmode = 0;
  cx->litmode = (unsigned long*)calloc(MAXLIT, sizeof(unsigned long));
  cx->gram = &SG;
  if (cx->litmode) {
    for (i = 0; i < MAXLIT; i++) {
      cx->litmode[i] = 0x87654321;
    }
    cx->poss_cont = 0;
#ifdef EXTENSION_NEW
    cx->cx_gwt = (pointer)malloc(sizeof(struct _rec) * sizeof(unsigned char));
    if (cx->cx_gwt) {
      struct _rec* gwt;
      memset(cx->cx_gwt, 0, sizeof(struct _rec) * sizeof(unsigned char));
      gwt = (struct _rec*)cx->cx_gwt;
      gwt->gwt_cx = -1; /* means no GetWordTextdic context
                           is available */
      gwt->gwt_dicname = NULL;
      cx->flags = CTX_LIVE | CTX_NODIC;
      return 0;
    }
    free(cx->litmode);
#else
    cx->flags = CTX_LIVE | CTX_NODIC;
    return 0;
#endif
  }
  return -1;
}

int
RkwCreateContext()
{
  int cx_num, i;
  struct RkContext* newcx;

  /* saisho no aki context wo mitsukeru */
  for (cx_num = 0; cx_num < (int)now_context; cx_num++) {
    if (!CX[cx_num].flags) {
      /* create mount list headers */
      if (fillContext(cx_num) == 0) {
        return cx_num;
      }
    }
  }
  newcx = (RkContext*)realloc(
    CX, (size_t)sizeof(RkContext) * (now_context + ADD_CONTEXT));
  if (newcx) {
    CX = newcx;
    for (i = now_context; i < (int)now_context + ADD_CONTEXT; i++) {
      CX[i].flags = (unsigned)0;
    }
    cx_num = now_context;
    now_context += ADD_CONTEXT;
    if (fillContext(cx_num) == 0) {
      return cx_num;
    }
  }
  return (-1);
}

int
RkwCloseContext(int cx_num)
{
  struct RkContext* cx;
  int i;

  if (!(cx = RkGetContext(cx_num)))
    return (-1);
  /* terminate bunsetu henkan */
  if (IS_XFERCTX(cx))
    RkwEndBun(cx_num, 0);
  _RkFreeDDP(cx->ddpath);
  cx->ddpath = NULL;
  /* subete no jisho wo MD suru */
  for (i = 0; i < 4; i++) {
    struct MD *mh, *m, *n;

    /* destroy mount list */
    mh = cx->md[i];
    if (mh) {
      for (m = mh->md_next; m != mh; m = n) {
        n = m->md_next;
        _RkUmountMD(cx, m);
      }
      free(mh);
      cx->md[i] = NULL;
    }
  }
  cx->dmprev = NULL;
  cx->qmprev = NULL;
  /* convertion table */
  if (cx->litmode) {
    free(cx->litmode);
    cx->litmode = NULL;
  }
  cx->flags = 0;

  /* free grammatical dictionary */
  cx->gram->refcount--;
  if (cx->gram->refcount == 0 && cx->gram != &SG) {
    RkCloseGram(cx->gram->gramdic);
    free(cx->gram);
  }
  cx->gram = NULL;

#ifdef EXTENSION_NEW
  if (cx->cx_gwt) {
    struct _rec* gwt = (struct _rec*)cx->cx_gwt;
    if (gwt) {
      RkwCloseContext(gwt->gwt_cx);
      if (gwt->gwt_dicname)
        free(gwt->gwt_dicname);
      free(gwt);
    }
    cx->cx_gwt = (pointer)0;
  }
  freeTdn(cx);
#endif
  return 0;
}

/* RkDuplicateContext
 *	onaji naiyou no context wo sakuseisuru
 */
int
RkwDuplicateContext(int cx_num)
{
  struct RkContext* sx;
  int dup = -1;

  dup = RkwCreateContext();
  if (dup >= 0) {
    int i;
    struct RkContext* dx;

    sx = RkGetContext(cx_num);
    if (sx) {
      dx = RkGetContext(dup);

      /* use the same grammatical information */
      dx->gram = sx->gram;
      dx->gram->refcount++;
      if (!(sx->flags & CTX_NODIC)) {
        dx->flags &= ~CTX_NODIC;
      }

      /* copy the mount list */
      for (i = 0; i < 4; i++) {
        struct MD *mh, *md;

        /* should mount dictionaries in reverse order */
        mh = sx->md[i];
        for (md = mh->md_prev; md != mh; md = md->md_prev)
          _RkMountMD(dx, md->md_dic, md->md_freq, md->md_flags & MD_WRITE, 0);
      }
      dx->ddpath = _RkCopyDDP(sx->ddpath);
      if (sx->litmode && dx->litmode)
        for (i = 0; i < MAXLIT; i++)
          dx->litmode[i] = sx->litmode[i];
    } else {
      RkwCloseContext(dup);
      return -1;
    }
  }
  return (dup);
}

/* RkMountDic: append the specified dictionary at the end of the mount list */
int
RkwMountDic(int cx_num, /* context specified */
            char* name, /* the name of dictonary */
            int mode)   /* mount mode */
{
  struct RkContext* cx;
  int firsttime;

  if (!name)
    return (-1);
  cx = RkGetContext(cx_num);
  if (cx) {
    struct DM *dm, *qm;

    firsttime = (cx->flags & CTX_NODIC) ? 1 : 0;
    if (firsttime) { /* 最初にマウント*しようと*したら降ろす */
      cx->flags &= ~CTX_NODIC;
    }

    dm = _RkSearchDicWithFreq(cx->ddpath, name, &qm);
    if (dm) {
      struct MD* mh = cx->md[dm->dm_class];
      struct MD *md, *nd;
      int count = 0;

      /* search the dictionary */
      for (md = mh->md_next; md != mh; md = nd) {
        nd = md->md_next;
        if (md->md_dic == dm) { /* already mounted */
          /* cancel the previous unmount */
          if (md->md_flags & MD_UPEND)
            md->md_flags &= ~MD_UPEND;
          count++;
        }
      }
      if (!count) {
        return _RkMountMD(cx, dm, qm, mode, firsttime);
      }
    }
  }
  return (-1);
}

/* RkUnmountDic: removes the specified dictionary from the mount list */
int
RkwUnmountDic(int cx_num, char* name)
{
  struct RkContext* cx;
  int i;

  if (!name)
    return (-1);
  cx = RkGetContext(cx_num);
  if (cx) {
    for (i = 0; i < 4; i++) {
      struct MD* mh = cx->md[i];
      struct MD *md, *nd;

      for (md = mh->md_next; md != mh; md = nd) {
        struct DM* dm = md->md_dic;
        char* ename;

        ename = md->md_freq ? md->md_freq->dm_nickname : dm->dm_nickname;
        nd = md->md_next;
        if (!STRCMP(ename, name)) {
          _RkUmountMD(cx, md);
        }
      }
    }
    return (0);
  }
  return (-1);
}

/* RkRemountDic: relocate the specified dictionary among the mount list */
int
RkwRemountDic(int cx_num, /* context specified */
              char* name, /* the name of dictonary */
              int mode)   /* mount mode */
{
  struct RkContext* cx;
  int i, isfound = 0;
  char* ename;

  if (!name)
    return (-1);
  cx = RkGetContext(cx_num);
  if (cx) {
    for (i = 0; i < 4; i++) {
      struct MD* mh = cx->md[i];
      struct MD *md, *pd;

      /* do in reverse order */
      for (md = mh->md_prev; md != mh; md = pd) {
        struct DM* dm = md->md_dic;

        ename = md->md_freq ? md->md_freq->dm_nickname : dm->dm_nickname;
        pd = md->md_prev;
        if (!STRCMP(ename, name)) {
          /* remove from mount list */
          md->md_prev->md_next = md->md_next;
          md->md_next->md_prev = md->md_prev;
          /* insert according to the mode */
          if (!mode) { /* sentou he */
            md->md_next = mh->md_next;
            md->md_prev = mh;
            mh->md_next->md_prev = md;
            mh->md_next = md;
          } else { /* saigo he */
            md->md_next = mh;
            md->md_prev = mh->md_prev;
            mh->md_prev->md_next = md;
            mh->md_prev = md;
          }
          isfound++;
        }
      }
    }
    if (isfound)
      return (0);
  }
  return (-1);
}

/* RkGetDicList: collects the names of the mounted dictionaies */
int
RkwGetMountList(int cx_num, char* mdname, int maxmdname)
{
  struct RkContext* cx;
  struct MD *mh, *md;
  int p, i, j;
  int count = -1;

  cx = RkGetContext(cx_num);
  if (cx) {
    i = count = 0;
    for (p = 0; p < 4; p++) {
      mh = cx->md[p];
      for (md = mh->md_next; md != mh; md = md->md_next) {
        struct DM* dm = md->md_dic;
        char* name;

        if (md->md_flags & (MD_MPEND | MD_UPEND)) {
          continue;
        }
        name = md->md_freq ? md->md_freq->dm_nickname : dm->dm_nickname;
        j = i + strlen(name) + 1;
        if (j + 1 < maxmdname) {
          if (mdname) {
            strcpy(mdname + i, name);
          }
          i = j;
          count++;
        }
      }
    }
    if (i + 1 < maxmdname && mdname)
      mdname[i++] = (char)0;
  }
  return (count);
}

/* RkGetDicList: collects the names of dictionary */

struct dics
{
  char *nickname, *dicname;
  int dictype;
};

static int
diccmp(const struct dics* a, const struct dics* b)
{
  int res;

  res = strcmp(a->nickname, b->nickname);
  if (res == 0) {
    res = strcmp(a->dicname, b->dicname);
    if (res == 0) {
      if (a->dictype == b->dictype) {
        res = 0;
      } else if (a->dictype == DF_FREQDIC) {
        res = -1;
      } else if (b->dictype == DF_FREQDIC) {
        res = 1;
      } else if (a->dictype == DF_PERMDIC) {
        res = -1;
      } else if (b->dictype == DF_PERMDIC) {
        res = 1;
      } else {
        res = 0;
      }
    }
  }
  return res;
}

int
RkwGetDicList(int cx_num, char* mdname, int maxmdname)
{
  struct RkContext* cx;
  struct DD **ddp, *dd;
  struct DF *df, *fh;
  struct DM *dm, *mh;
  int i, j, k, n;
  int count = -1;
  struct dics* diclist;

  /* まず数を数える */
  if ((cx = RkGetContext(cx_num)) && (ddp = cx->ddpath)) {
    count = 0;
    for (i = 0; (dd = ddp[i]) != NULL; i++) {
      fh = &dd->dd_files;
      for (df = fh->df_next; df != fh; df = df->df_next) {
        mh = &df->df_members;
        for (dm = mh->dm_next; dm != mh; dm = dm->dm_next) {
          count++;
        }
      }
    }

    if (count <= 0) {
      return -1; /* 正確な数が分からなかった */
    }

    /* 辞書リストの配列を malloc する */
    diclist = (struct dics*)malloc(count * sizeof(struct dics));
    if (diclist) {
      struct dics *dicp = diclist, *prevdicp = NULL;

      for (i = 0; (dd = ddp[i]) != NULL; i++) {
        fh = &dd->dd_files;
        for (df = fh->df_next; df != fh; df = df->df_next) {
          mh = &df->df_members;
          for (dm = mh->dm_next; dm != mh; dm = dm->dm_next) {
            dicp->nickname = dm->dm_nickname;
            dicp->dicname = dm->dm_dicname;
            dicp->dictype = df->df_type;
            dicp++;
          }
        }
      }
      qsort(diclist,
            count,
            sizeof(struct dics),
            (int (*)(const void*, const void*))diccmp);

      n = count;
      for (i = j = 0, dicp = diclist; i < n; i++, dicp++) {
        if (prevdicp && !strcmp(prevdicp->nickname, dicp->nickname)) {
          /* prev と今の辞書とで nickname が一致している場合 */
          count--;
        } else {
          k = j + strlen(dicp->nickname) + 1;
          if (k + 1 < maxmdname) {
            if (mdname) {
              strcpy(mdname + j, dicp->nickname);
              j = k;
            }
          }
          prevdicp = dicp;
        }
      }
      if (j + 1 < maxmdname && mdname) {
        mdname[j++] = 0;
      }
      free(diclist);
    } else {
      count = -1; /* やっぱり正確な数が分からなかった */
    }
  }
  return (count);
}

/* RkGetDirList: collects the names of directories */
int
RkwGetDirList(int cx_num, char* ddname, int maxddname)
{
  struct RkContext* cx;
  struct DD **ddp, *dd;
  int p, i, j;
  int count = -1;

  if ((cx = RkGetContext(cx_num)) && (ddp = cx->ddpath)) {
    i = count = 0;
    for (p = 0; (dd = ddp[p]) != NULL; p++) {
      j = i + strlen(dd->dd_name) + 1;
      if (j + 1 < maxddname) {
        if (ddname)
          strcpy(ddname + i, dd->dd_name);
        i = j;
        count++;
      }
    }
    if (i + 1 < maxddname && ddname)
      ddname[i++] = (char)0;
  }
  return (count);
}

/* RkDefineDic
 *	mount the dictionary onto the specified context.
 */
int
RkwDefineDic(int cx_num, char* name, Wchar* word)
{
  struct RkContext* cx;
  int i;

  if ((cx = RkGetContext(cx_num)) && word && name) {
    char* prevname = NULL;

    if (cx->dmprev)
      prevname = cx->dmprev->dm_nickname;
    if (cx->qmprev)
      prevname = cx->qmprev->dm_nickname;

    if (prevname && !STRCMP(prevname, name))
      return (
        DST_CTL(cx->dmprev, cx->qmprev, DST_DoDefine, word, cx->gram->gramdic));
    else {
      for (i = 0; i < 4; i++) {
        struct MD* mh = cx->md[i];
        struct MD *md, *nd;

        for (md = mh->md_next; md != mh; md = nd) {
          struct DM* dm = md->md_dic;
          struct DM* qm = md->md_freq;
          char* dname = NULL;

          if (dm)
            dname = dm->dm_nickname;
          if (qm)
            dname = qm->dm_nickname;

          if (dname) {
            if (!STRCMP(dname, name)) {
              cx->dmprev = dm;
              cx->qmprev = qm;
              return (DST_CTL(dm, qm, DST_DoDefine, word, cx->gram->gramdic));
            }
          }
          nd = md->md_next;
        }
      }
    }
  }
  return (-1);
}

/* RkDeleteDic
 *	mount the dictionary onto the specified context.
 */
int
RkwDeleteDic(int cx_num, char* name, Wchar* word)
{
  struct RkContext* cx;
  int i;

  if ((cx = RkGetContext(cx_num)) && name) {
    char* prevname = NULL;

    if (cx->dmprev)
      prevname = cx->dmprev->dm_nickname;
    if (cx->qmprev)
      prevname = cx->qmprev->dm_nickname;

    if (prevname && !STRCMP(prevname, name))
      return (
        DST_CTL(cx->dmprev, cx->qmprev, DST_DoDelete, word, cx->gram->gramdic));
    else {
      for (i = 0; i < 4; i++) {
        struct MD* mh = cx->md[i];
        struct MD *md, *nd;

        for (md = mh->md_next; md != mh; md = nd) {
          struct DM* dm = md->md_dic;
          struct DM* qm = md->md_freq;
          char* dname = NULL;

          if (dm)
            dname = dm->dm_nickname;
          if (qm)
            dname = qm->dm_nickname;

          if (dname) {
            if (!STRCMP(dname, name)) {
              cx->dmprev = dm;
              cx->qmprev = qm;
              return (DST_CTL(dm, qm, DST_DoDelete, word, cx->gram->gramdic));
            }
          }
          nd = md->md_next;
        }
      }
    }
  }
  return (-1);
}

/* The following code is as simulating the code in
 lib/RKC API.  In case STANDALONE, it becomes possible for libRK to be
 linked with libcanna directly. */

int
RkwSetAppName(int Context, char* name)
{
  return 0;
}

char*
RkwGetServerName(void)
{
  return NULL;
}

int
RkwGetProtocolVersion(int* majorp, int* minorp)
{
  *majorp = CANNA_MAJOR_MINOR / 1000;
  *minorp = CANNA_MAJOR_MINOR % 1000;
  return 0;
}

int
RkwGetServerVersion(int* majorp, int* minorp)
{
  *majorp = CANNA_MAJOR_MINOR / 1000;
  *minorp = CANNA_MAJOR_MINOR % 1000;
  return 0;
}

int
RkwSetUserInfo(char* user, char* group, char* topdir)
{
  return 1;
}
