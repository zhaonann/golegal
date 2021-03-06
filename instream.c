#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <glob.h>
#include "instream.h"
#include "modadd.h"

#define MAXSTREAMS 2048

struct goin {
  uint64_t modulus;
  uint64_t totalin;
  uint64_t skipunder;
  statebuf stream[MAXSTREAMS];
  statebuf *heap[MAXSTREAMS];
  int nstreams;
  int incpus;
  int cpuid;
};

uint64_t nstreams(goin *gin)
{
  return gin->nstreams;
}

uint64_t totalread(goin *gin)
{
  return gin->totalin;
}

uint64_t readdelta(statebuf *sb)
{
  int i=0,c;
  uint64_t delta=0LL;

  do {
    if ((c = fgetc(sb->fp)) == EOF) {
      printf("EOF during readdelta in file %s\n",sb->fname);
      exit(1);
    }
    delta |= (uint64_t)(c & 0x7f) << (7 * i++);
  } while (c & 0x80);
  return delta;
}

void fillbuf(goin *gin, statebuf *sb)
{
  uint64_t delta;
  int notok;

  do {
    delta = readdelta(sb);
    sb->state += delta;
    if (!fread(&sb->cnt,sizeof(uint64_t),1,sb->fp)) {
      printf("failed to read state %jo count from file %s\n", (uintmax_t)sb->state, sb->fname);
      exit(1);
    }
    modadd(gin->modulus, &sb->cumcnt, sb->cnt);
    if (sb->state == FINALSTATE) {
      notok = fclose(sb->fp);
      if (notok) {
        printf("Failed to close %s after reading\n",sb->fname);
        exit(1);
      }
      sb->fp = NULL;
      if (sb->cumcnt) {
        printf("file %s corrupt; cumcnt=%ju\n", sb->fname, (uintmax_t)sb->cumcnt);
        exit(1);
      }
      return;
    }
    gin->totalin++;
  } while (sb->state < gin->skipunder);
}

void hpreplace(goin *gin, int i)
{
  statebuf *sb;
  uint64_t oldstate;
  int j;

  oldstate = (sb = gin->heap[i])->state;
  for (fillbuf(gin, sb); (j=2*i+1) < gin->nstreams; i=j) {
    if (j+1 < gin->nstreams) {
      if (gin->heap[j+1]->state == gin->heap[j]->state) {
        if (gin->heap[j]->fp != 0) {
          modadd(gin->modulus, &gin->heap[j]->cnt, gin->heap[j+1]->cnt);
          hpreplace(gin, j+1);
        }
      } else if (gin->heap[j+1]->state < gin->heap[j]->state)
        j++;
    }
    if (sb->state > gin->heap[j]->state) {
      gin->heap[i] = gin->heap[j];
      gin->heap[j] = sb;
    } else if (sb->state == gin->heap[j]->state && sb->fp) {
      modadd(gin->modulus, &sb->cnt, gin->heap[j]->cnt);
      fillbuf(gin, sb = gin->heap[j]);
    } else break;
  }
  assert(sb->state > oldstate);
}

void hpinsert(goin *gin, int i, statebuf *sb)
{
  int p;

  for (fillbuf(gin, gin->heap[i]=sb); (p=(i+1)/2-1) >= 0; i=p) {
    if (gin->heap[p]->state > sb->state) {
      gin->heap[i] = gin->heap[p];
      gin->heap[p] = sb;
    } else if (gin->heap[p]->state == sb->state && sb->fp) {
      modadd(gin->modulus, &gin->heap[p]->cnt, sb->cnt);
      hpreplace(gin, i);
      return;
    } else break;
  }
}

statebuf *minstream(goin *gin)
{
  return gin->heap[0];
}

void deletemin(goin *gin)
{
  hpreplace(gin, 0);
}

goin *openstreams(char *inbase, int incpus, int ncpus, int cpuid, uint64_t modulus, uint64_t skipunder) {
  char inname[FILENAMELEN];
  statebuf *sb;
  FILE *fp;
  int from,to,j,nscan;
  goin *gin;
  glob_t stateglob;
  uint64_t prevstate,state;

  gin = (goin *)calloc(1,sizeof(goin));
  if (gin == NULL) {
    printf ("Failed to allocate goin record of size %d\n", (int)sizeof(goin));
    exit(1);
  }
  gin->totalin = 0LL;
  gin->incpus = incpus;
  gin->cpuid = cpuid;
  gin->modulus = modulus;
  gin->skipunder = skipunder;
  sb = &gin->stream[gin->nstreams = 0];
  int limto = incpus < ncpus ? ncpus : incpus;
  for (from=0; from<incpus; from++) {
    for (to=cpuid; to<limto; to+=ncpus) {
      prevstate = 0L;
      for (j=0; prevstate!=FINALSTATE; j++) {
        sprintf(inname,"%s/fromto.%d.%d/%d.*",inbase,from,to,j); 
        glob(inname, GLOB_NOSORT, NULL, &stateglob);
        if (stateglob.gl_pathc != 1) {
#ifdef OPENOLD
          sprintf(inname,"%s/fromto.%d.%d/%d",inbase,from,to,j); 
          if (!(fp = fopen(inname, "r")))
            break;
          strncpy(sb->fname, inname, FILENAMELEN);
          goto oldcont;
#else
          printf ("%d files matching %s\n", (int)stateglob.gl_pathc, inname);
          exit(1);
#endif
        }
        if (!(fp = fopen(stateglob.gl_pathv[0], "r"))) {
          printf("Failed to open %s\n",stateglob.gl_pathv[0]);
          exit(1);
        }
        if (gin->nstreams == MAXSTREAMS) {
          printf ("#inputfiles exceeds MAXSTREAMS (%d)\n", MAXSTREAMS);
          exit(1);
        }
        nscan = sscanf(rindex(stateglob.gl_pathv[0],'.')+1, "%lo", &state); 
        if (nscan != 1) {
          printf ("No state suffix in %s\n", stateglob.gl_pathv[0]);
          exit(1);
        }
        if (state <= prevstate) {
          printf ("States %jo %jo out of order.\n", (uintmax_t)prevstate, (uintmax_t)state);
          exit(1);
        }
        prevstate = state;
        strncpy(sb->fname, stateglob.gl_pathv[0], FILENAMELEN);
#ifdef OPENOLD
oldcont:
#endif
        globfree(&stateglob);
        // printf("opened %s state %jo\n", sb->fname, (uintmax_t)state);
        sb->fp = fp;
        hpinsert(gin, gin->nstreams++, sb++);
      }
    }
  }
  return gin;
}

#ifdef MAININSTREAM
#include "modulus.h"
#include "states.h"
#include <string.h>

int main(int argc, char *argv[])
{
  int wd,modidx,y,x;
  uint64_t totin,nin;
  uint64_t r, s, modulus;
  char inbase[64];
  statebuf *mb;
  goin *gin;
  int incpus, ncpus, cpuid;

  if (argc!=3) {
    fprintf(stderr, "usage: %s width y\n", argv[0]);
    exit(1);
  }
  setwidth(wd = atoi(argv[1]));
  modidx = 0;
  modulus = -(uint64_t)modulusdeltas[modidx];
  y = atoi(argv[2]);
  x = 0;
  incpus = 1;
  ncpus = 1;
  cpuid = 0;
  sprintf(inbase,"%d.%d/yx.%02d.%02d",wd,modidx,y,x);
  gin = openstreams(inbase, incpus, ncpus, cpuid, modulus, 0);

  if (!nstreams(gin))
    fprintf (stderr, "wanring: no input files\n");
  for (nin=0LL; (mb = minstream(gin))->state != FINALSTATE; nin++) {
    s = mb->state;
    r = reverse(s);
    // printf("%jo %ju %jo\n", (uintmax_t)s, (uintmax_t)mb->cnt, (uintmax_t)r);
    printf("%jo\n", (uintmax_t)(s < r ? s : r));
    deletemin(gin);
  }
  totin = totalread(gin);
  fprintf(stderr, "(%d,%d) size %ju",y,x,(uintmax_t)nin);
  fprintf(stderr, " avg %1.3lf\n", totin/(double)nin);
  return 0;
}
#endif
