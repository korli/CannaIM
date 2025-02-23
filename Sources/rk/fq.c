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

#ifdef __CYGWIN32__
#include <fcntl.h> /* for O_BINARY */
#endif

#define dm_xdm dm_extdata.ptr

struct xqm
{
  off_t ex_boff;
  long ex_bsiz;
};

struct RUT*
allocRUT(unsigned long hn)
{
  struct RUT* tempo;

  if (!(tempo = (struct RUT*)calloc(1, sizeof(struct RUT))))
    return ((struct RUT*)0);
  if (!(tempo->dp =
          (struct CTdata**)calloc((size_t)hn, sizeof(struct CTdata*)))) {
    free(tempo);
    return ((struct RUT*)0);
  }
  return tempo;
}

static int
WrToRut(struct RUT* ruc, unsigned long csn, unsigned long tick)
{
  unsigned long whn;
  struct CTdata *wd, **pwd;

  whn = HashFunc(csn);
  for (pwd = ruc->dp + whn, wd = *pwd; wd; pwd = &wd->next, wd = *pwd) {
    if (wd->ct[0] == csn) {
      WriteCT(csn, tick, wd->ct) return 0;
    }
  }
  if (!(wd = (struct CTdata*)calloc(1, sizeof(struct CTdata))))
    return -1;
  *pwd = wd;
  WriteCT(csn, tick, wd->ct) return 1;
}

static unsigned long
UpdateFrst(struct RUT* ruc)
{
  unsigned long wmin, wtick, frst, lc;
  struct CTdata* wd;

  wmin = 0xffffffffL;
  frst = 0xfffffL;

  for (lc = 0; lc < HN; lc++) {
    for (wd = *(ruc->dp + lc); wd; wd = wd->next) {
      if (wmin > (wtick = wd->ct[1])) {
        frst = wd->ct[0];
        wmin = wtick;
      }
    }
  }
  if (frst == (unsigned long)0xffffffff)
    return (unsigned long)0L;
  return frst;
}

static int
deleteCT(struct RUT* ruc, unsigned long csn)
{
  unsigned long whn;
  struct CTdata *wd, **pre;

  whn = HashFunc(csn);

  for (pre = ruc->dp + whn, wd = *pre;; pre = &wd->next, wd = *pre) {
    if (!wd)
      return 0;
    if (wd->ct[0] == csn)
      break;
  }
  *pre = wd->next;
  free(wd);
  return 1;
}

unsigned long
searchRut(struct RUT* ruc, unsigned long csn)
{
  unsigned long whn;
  struct CTdata* wd;

  whn = HashFunc(csn);
  for (wd = *(ruc->dp + whn); wd; wd = wd->next) {
    if (wd->ct[0] == csn)
      return wd->ct[1];
  }
  return (unsigned long)0L;
}

static struct CTdata*
searchCTadd(struct RUT* ruc, unsigned long csn)
{
  unsigned long whn;
  struct CTdata* wd;

  whn = HashFunc(csn);
  for (wd = *(ruc->dp + whn); wd; wd = wd->next) {
    if (wd->ct[0] == csn)
      return wd;
  }
  return (struct CTdata*)0;
}

int
entryRut(struct RUT* ruc, unsigned long csn, unsigned long tick)
{
  struct CTdata* wpadd;
  int retval;

  retval = 1;
  if (ruc->cs < ruc->sz)
    switch (WrToRut(ruc, csn, tick)) {
      case 0:
        break;
      case 1:
        if (++(ruc->cs) == ruc->sz)
          ruc->frst = UpdateFrst(ruc);
        break;
      case -1:
        return 0;
    }
  else {
    wpadd = searchCTadd(ruc, csn);
    if (wpadd) {
      WriteCT(csn, tick, wpadd->ct) if (csn == ruc->frst) ruc->frst =
        UpdateFrst(ruc);
    } else {
      if (deleteCT(ruc, ruc->frst)) {
        if (WrToRut(ruc, csn, tick) < 0) {
          ruc->cs -= 1L;
          retval = 0;
        }
      } else
        retval = 0;
      ruc->frst = UpdateFrst(ruc);
    }
  }
  return retval;
}

static struct WRT*
allocWRT(unsigned long size)
{
  struct WRT* tempo;

  if (!(tempo = (struct WRT*)calloc(1, sizeof(struct WRT))))
    return ((struct WRT*)0);
  if (!(tempo->buf = (unsigned char*)calloc(1, 5 * size))) {
    free(tempo);
    return ((struct WRT*)0);
  }
  tempo->sz = size;
  return tempo;
}

static struct WRT*
readWRT(int fr)
{
  unsigned long wsz, wcs, wfrst, wtm;
  unsigned char ll[4];
  struct WRT* wrt;

  if (read(fr, (char*)ll, 4) != 4)
    return (struct WRT*)0;
  wsz = bst4_to_l(ll);
  if (read(fr, (char*)ll, 4) != 4)
    return (struct WRT*)0;
  wcs = bst4_to_l(ll);
  if (read(fr, (char*)ll, 4) != 4)
    return (struct WRT*)0;
  wfrst = bst4_to_l(ll);
  if (read(fr, (char*)ll, 4) != 4)
    return (struct WRT*)0;
  wtm = bst4_to_l(ll);
  if (!(wrt = allocWRT(wsz)))
    return (struct WRT*)0;

  wrt->cs = wcs;
  wrt->frst = wfrst;
  wrt->tm = wtm;
  if (wsz) {
    if (read(fr, wrt->buf, (unsigned)5 * wsz) != 5 * (int)wsz) {
      freeWRT(wrt) return (struct WRT*)0;
    }
  }
  return wrt;
}

static int
writeToWRT(int fr, struct WRT* wrt)
{
  unsigned char ll[4];

  l_to_bst4(wrt->sz, ll) if (write(fr, (char*)ll, 4) != 4) return 0;
  l_to_bst4(wrt->cs, ll) if (write(fr, (char*)ll, 4) != 4) return 0;
  l_to_bst4(wrt->frst, ll) if (write(fr, (char*)ll, 4) != 4) return 0;
  l_to_bst4(wrt->tm, ll) if (write(fr, (char*)ll, 4) != 4) return 0;
  if (wrt->sz) {
    if (write(fr, wrt->buf, (unsigned)5 * wrt->sz) != 5 * (int)wrt->sz)
      return 0;
  }
  return 1;
}

static void
abolishNV(struct NV* nv)

{
  struct NVE *p, **q, *r;
  unsigned i;

  if (nv && nv->tsz && nv->buf) {
    for (i = 0, q = nv->buf + i; i < nv->tsz; i++, q = nv->buf + i) {
      for (p = *q; p; p = r) {
        r = p->next;
        if (p->data)
          free(p->data);
        free(p);
      }
    }
    free(nv->buf);
    free(nv);
  }
}

static struct NV*
readNV(int fd)
{
  struct NV* vn;
  unsigned char ll[4], *buf, *p;
  long i, cnt;

  vn = (struct NV*)malloc(sizeof(struct NV));
  if (vn) {
    if (read(fd, (char*)ll, 4) == 4) {
      vn->sz = bst4_to_l(ll);
      if (read(fd, (char*)ll, 4) == 4) {
        cnt = bst4_to_l(ll);
        if (read(fd, (char*)ll, 4) == 4) {
          vn->tsz = bst4_to_l(ll);
          if (read(fd, (char*)ll, 4) == 4) {
            goto read_ok;
          }
        }
      }
    }
    free(vn);
  }
  return NULL;

read_ok:

  vn->cnt = vn->csz = 0L;
  vn->head.left = vn->head.right = &vn->head;
  if (vn->sz) {
    if (!(vn->buf =
            (struct NVE**)calloc((size_t)vn->tsz, sizeof(struct NVE*)))) {
      free(vn);
      return NULL;
    }
    if (!(buf = (unsigned char*)malloc((size_t)vn->sz)) ||
        read(fd, buf, (unsigned int)vn->sz) != (int)vn->sz) {
      free(vn->buf);
      if (buf)
        free(buf);
      free(vn);
      return NULL;
    }
    for (p = buf, i = 0L; i < cnt; i++, p += *p * 2 + 2)
      if ((unsigned long)(p - buf) + *p * 2 + 2 < vn->sz)
        _RkRegisterNV(vn, p + 2, (int)*p, (int)*(p + 1));
    free(buf);
  } else {
    free(vn);
    return NULL;
  }
  vn->head.right->left = &vn->head;
  vn->head.left->right = &vn->head;
  return (vn);
}

static int
writeNV(int fd, struct NV* nv)
{
  unsigned char ll[4];
  unsigned char *buf = NULL, *r;
  struct NVE *p, **q;
  unsigned long i;

  if (!nv)
    return (-1);
  if (nv->buf) {
    if (!(buf = (unsigned char*)malloc((size_t)nv->sz)))
      return (-1);
    for (r = buf, i = 0L, q = nv->buf; i < nv->tsz; i++, q = nv->buf + i) {
      for (p = *q; p; q = &p->next, p = *q) {
        if ((unsigned long)(r - buf) + *(p->data) * 2 + 2 < nv->sz) {
          memcpy(r, p->data, *(p->data) * 2 + 2);
          r += *(p->data) * 2 + 2;
        } else {
          i = nv->tsz;
          break;
        }
      }
    }
  }

  l_to_bst4(nv->sz, ll) if (write(fd, (char*)ll, 4) == 4)
  {
    l_to_bst4(nv->cnt, ll) if (write(fd, (char*)ll, 4) == 4)
    {
      l_to_bst4(nv->tsz, ll) if (write(fd, (char*)ll, 4) == 4)
      {
        l_to_bst4((unsigned long)0, ll) if (write(fd, (char*)ll, 4) == 4)
        {
          if (!nv->sz || (buf && write(fd, buf, (int)nv->sz) == (int)nv->sz)) {
            goto write_ok;
          }
        }
      }
    }
  }
  if (buf)
    free(buf);
  return (-1);

write_ok:
  if (buf)
    free(buf);
  return (0);
}

static void
freeRUT(struct RUT* ruc)
{
  struct CTdata *wd, *nex;
  unsigned long lc;

  for (lc = 0; lc < HN; lc++) {
    for (wd = *(ruc->dp + lc); wd; wd = nex) {
      nex = wd->next;
      free(wd);
    }
  }
  free(ruc->dp);
  free(ruc);
}

struct RUT*
LoadRUC(int fr)
{
  struct WRT* wruc;
  struct RUT* ruc;
  unsigned long lc, csn, tick;

  if (!(wruc = readWRT(fr)))
    return (struct RUT*)0;

  if (!(ruc = allocRUT(HN))) {
    freeWRT(wruc) return (struct RUT*)0;
  }

  ruc->sz = wruc->sz;
  ruc->cs = 0L;
  ruc->frst = wruc->frst;
  ruc->tm = wruc->tm;

  for (lc = 0; lc < wruc->cs; lc++) {
    unsigned char* tmp = wruc->buf + 5 * lc;
    csn = a_csn(tmp);
    tick = _RkGetTick(0) - a_tick(wruc->buf + 5 * lc);
    if (!entryRut(ruc, csn, tick)) {
      freeRUT(ruc);
      ruc = (struct RUT*)0;
    }
  }
  freeWRT(wruc) return ruc;
}

// static int SaveRUC(int /*fr*/, struct RUT * /*ruc*/);

static int
SaveRUC(int fr, struct RUT* ruc)
{
  struct WRT* wruc;
  struct CTdata* wdp;
  unsigned long lc, count;
  int retval;

  if (!ruc)
    return 0;
  retval = 1;
  if (!(wruc = allocWRT(ruc->sz))) {
    freeRUT(ruc);
    return 0;
  }
  wruc->sz = ruc->sz;
  wruc->cs = ruc->cs;
  wruc->frst = ruc->frst;
  wruc->tm = ruc->tm;

  count = 0L;
  for (lc = 0L; lc < HN; lc++) {
    for (wdp = *(ruc->dp + lc); wdp; wdp = wdp->next) {
      WriteVal(wdp->ct[0], _RkGetTick(0) - wdp->ct[1], wruc->buf + 5 * count)
        count++;
    }
  }
  if (count != ruc->cs) {
    retval = 0;
  }
  if (!writeToWRT(fr, wruc))
    retval = 0;
  freeWRT(wruc) return retval;
}

static int
FQscan(struct DF* df, struct DM* codm, char* file, int* w)
{
  int count = 0;
  struct HD hd;
  struct DM *dm, *dmh;
  unsigned char ll[4];
  unsigned long bitsiz, bitoff;
  off_t off;
  int fd;

  *w = 1;
  if ((fd = open(file, 2)) < 0) {
    *w = 0;
    if ((fd = open(file, 0)) < 0)
      return -1;
  }
#ifdef __CYGWIN32__
  setmode(fd, O_BINARY);
#endif

  for (off = 0; _RkReadHeader(fd, &hd, off) >= 0;) {
    long start = off;

    if (!hd.data[HD_DMNM].ptr ||
        (strncmp(".fq",
                 (char*)hd.data[HD_DMNM].ptr +
                   strlen((char*)hd.data[HD_DMNM].ptr) - (sizeof(".fq") - 1),
                 sizeof(".fq") - 1) &&
         strncmp(".cld",
                 (char*)hd.data[HD_DMNM].ptr +
                   strlen((char*)hd.data[HD_DMNM].ptr) - (sizeof(".cld") - 1),
                 sizeof(".cld") - 1))) {
      break;
    }
    if (!codm->dm_xdm ||
        (long)((struct ND*)codm->dm_xdm)->rec != hd.data[HD_REC].var ||
        (long)((struct ND*)codm->dm_xdm)->can != hd.data[HD_CAN].var)
      break;
    if (hd.flag[HD_CRC]) {
      if (!((struct ND*)codm->dm_xdm)->crc_found ||
          (long)((struct ND*)codm->dm_xdm)->crc != hd.data[HD_CRC].var)
        break;
    } else {
      if (((struct ND*)codm->dm_xdm)->crc_found ||
          (long)((struct ND*)codm->dm_xdm)->time != hd.data[HD_TIME].var)
        break;
    }
    off += hd.data[HD_HSZ].var;
    lseek(fd, off, 0);
    read(fd, (char*)ll, 4);
    off += 4;
    bitsiz = L4TOL(ll);
    bitoff = off;
    off += bitsiz;
    lseek(fd, off, 0);
    dmh = &df->df_members;
    for (dm = dmh->dm_next; dm != dmh; dm = dm->dm_next) {
      if (!strcmp(dm->dm_dicname, (char*)hd.data[HD_CODM].ptr)) {
        struct xqm* xqm;

        if (!(xqm = (struct xqm*)malloc(sizeof(struct xqm))))
          break;
        dm->dm_extdata.ptr = (pointer)xqm;
        xqm->ex_boff = bitoff;
        xqm->ex_bsiz = bitsiz;
        dm->dm_flags |= DM_EXIST;
        dm->dm_offset = start;
        count++;
        break;
      }
    }
    _RkClearHeader(&hd);
  }
  _RkClearHeader(&hd);
  if (!count) {
    close(fd);
    return -1;
  }
  df->df_size = off;
  df->df_extdata.var = (long)fd;
  return fd;
}

int
FQopen(struct DM* dm, struct DM* qm, char* file, int mode)
{
  struct DF* df;
  struct DD* dd;
  struct xqm* xqm;
  int writable;
  int fd;

  /* missing file info ? */
  if (!(df = qm->dm_file) || !(dd = df->df_direct))
    return -1;
  /* initialize df */
  if (!df->df_rcount) {
    df->df_extdata.var = (long)FQscan(df, dm, file, &writable);
    if (df->df_extdata.var < 0)
      return -1;
    if (writable)
      df->df_flags |= DF_WRITABLE;
    else
      df->df_flags &= ~DF_WRITABLE;
    df->df_flags |= DF_EXIST;
    dd->dd_rcount++;
  }
  /*
   *  this member is not included.
   */
  if (!(qm->dm_flags & DM_EXIST))
    return -1;
  if (strcmp(dm->dm_dicname, qm->dm_dicname))
    return -1;
  /* */
  xqm = (struct xqm*)qm->dm_extdata.ptr;
  fd = df->df_extdata.var;

  qm->dm_rut = NULL;
  qm->dm_nv = NULL;
  /* dispatch */
  qm->dm_qbits = (unsigned char*)malloc((unsigned)xqm->ex_bsiz);
  if (!qm->dm_qbits)
    return -1;
  lseek(fd, xqm->ex_boff, 0);
  read(fd, (char*)qm->dm_qbits, (int)xqm->ex_bsiz);
  qm->dm_rut = LoadRUC(fd);
  qm->dm_nv = readNV(fd);
  df->df_rcount++;
  if ((mode & DM_WRITABLE) && (df->df_flags & DF_WRITABLE)) {
    qm->dm_flags |= DM_WRITABLE;
  }
  return 0;
}

/*
 * CLOSE
 */
/*ARGSUSED*/
void
FQclose(struct RkContext* cx, struct DM* dm, struct DM* qm, char* file)
{
  struct DF* df = qm->dm_file;
  struct xqm* xqm;
  int fd = (int)df->df_extdata.var;

  xqm = (struct xqm*)qm->dm_extdata.ptr;
  if (xqm) {
    if (qm->dm_qbits) {
      if (qm->dm_flags & DM_UPDATED) {
        lseek(fd, xqm->ex_boff, 0);
        write(fd, (char*)qm->dm_qbits, (int)xqm->ex_bsiz);
      }
      free(qm->dm_qbits);
      qm->dm_qbits = NULL;
    }
  }
  if (qm->dm_rut) {
    if (qm->dm_flags & DM_UPDATED)
      SaveRUC(fd, qm->dm_rut);
    freeRUT(qm->dm_rut);
    qm->dm_rut = NULL;
  }
  if (qm->dm_nv) {
    if (qm->dm_flags & DM_UPDATED)
      writeNV(fd, qm->dm_nv);
    abolishNV(qm->dm_nv);
    qm->dm_nv = NULL;
  }
  qm->dm_flags &= ~DM_UPDATED;
  if (--df->df_rcount == 0) {
    struct DM *dmh, *ddm;

    close(fd);
    dmh = &df->df_members;
    for (ddm = dmh->dm_next; ddm != dmh; ddm = ddm->dm_next) {
      xqm = (struct xqm*)ddm->dm_extdata.ptr;
      if (xqm) {
        free(xqm);
        ddm->dm_extdata.ptr = NULL;
      }
    }
  }
}

int
FQsync(struct RkContext* cx, struct DM* dm, struct DM* qm, char* file)
/* ARGSUSED */
{
  struct DF* df = qm->dm_file;
  struct xqm* xqm;
  int rv;
  int fd = (int)df->df_extdata.var;

  rv = 0;
  xqm = (struct xqm*)qm->dm_extdata.ptr;
  if (xqm) {
    if (qm->dm_qbits) {
      if (qm->dm_flags & DM_UPDATED) {
        lseek(fd, xqm->ex_boff, 0);
        if (write(fd, (char*)qm->dm_qbits, (int)xqm->ex_bsiz) !=
            (int)xqm->ex_bsiz)
          rv = -1;
        if (qm->dm_rut)
          rv = SaveRUC(fd, qm->dm_rut) - 1;
        if (qm->dm_nv)
          rv = writeNV(fd, qm->dm_nv);
      }
      if (!rv)
        qm->dm_flags &= ~DM_UPDATED;
    }
  }
  return (rv);
}

/* vim: set sw=2: */
