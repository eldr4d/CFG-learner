/* ioh.c
 *
 * (c) Mark Johnson, 12th July 2004
 */

const char usage[] =
"ioh: The Inside-Outside algorithm for maximum likelihood PCFG estimation\n"
"     with an entropic prior.\n"
"\n"
"Written by Mark_Johnson@Brown.edu, 25th October 2005.\n"
"\n"
"Usage:\n"
"\n"
"io [-d debuglevel] [-R nruns] [-H entropy-weight]\n"
"   [-g grammar] [-s stoptol] [-p prune] [-l maxlen] [-m minits] [-n maxits]\n"
"   [-S randseed] [-j jitter] [-J per-iteration-jitter]\n"
"   [-b betastart] [-B betastop] [-N nbeta] yieldfile\n"
"\n"
"where:\n"
"\n"
" yieldfile is a file containing the strings to be parsed,\n"
" grammar is a file containing the rules of the grammar to be re-estimated (default: stdin),\n"
" re-estimation stops when the negative log probability of the strings changes less than stoptol\n"
"     (default: 1e-5)\n"
" rules with lower probability than prune (default 0) are pruned from the grammar during training,\n"
" sentences longer than maxlen are ignored during training (default: include all sentences),\n"
" at least minits of EM training are performed (default 1),\n"
" at most maxits of EM training are performed (default: run until convergence),\n"
" positive values of debug cause increasingly more debugging information to be written (default: 0),\n"
" after each EM re-estimation the current rules and rule weights are written to tracefile.\n"
"\n";

#include "local-trees.h"
#include "grammar.h"
#include "expected-counts.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static grammar g_global = NULL;
static si_t    si_global;

static FLOAT minruleprob = 1e-7;     /* prune rules with less than this weight */

static void
write_grammar_(int interrupt)
{
  if (g_global)
    write_grammar(stdout, g_global, si_global, minruleprob);
  exit(EXIT_FAILURE);
}

int      
main(int argc, char **argv)
{
  si_t  si = make_si(1024);
  FILE  *grammarfp = stdin;
  FILE  *yieldfp;
  FILE	*tracefp = stderr;  	/* set this to NULL to stop trace output */
  FILE	*summaryfp = stderr;	/* set this to NULL to stop parse stats */
  grammar g0, g = NULL;
  int	maxlen = 0, minits = 0, maxits = 0;
  FLOAT stoptol = 1e-7;
  int nbeta = 1;
  FLOAT betastart = 0, betastop = 1;
  FLOAT jitter0 = 0, jitter = 0;
  int debuglevel = 0, nruns = 1, irun = 0;
  FLOAT Hfactor = 0;

  {
    int chr;
    
    while ((chr = getopt(argc, argv, "g:s:p:l:m:n:d:t:b:B:N:j:J:S:d:R:H:")) != -1) 
      switch (chr) {
      case 'g': 
	grammarfp = fopen(optarg, "r");
	if (grammarfp == NULL) {
	  fprintf(stderr, "Error: couldn't open grammarfile %s\n%s",
		  optarg, usage);
	  exit(EXIT_FAILURE);
	}
	break;
      case 's':
	stoptol = atof(optarg);
	break;
      case 'p':
	minruleprob = atof(optarg);
	break;
      case 'l':
	maxlen = atoi(optarg);
	break;
      case 'm':
	minits = atoi(optarg);
	break;
      case 'n':
	maxits = atoi(optarg);
	break;
      case 'b':
	betastart = atof(optarg);
	break;
      case 'B':
	betastop = atof(optarg);
	break;
      case 'N':
	nbeta = atoi(optarg);
	break;
      case 'j':
	jitter0 = atof(optarg);
	break;
      case 'J':
	jitter = atof(optarg);
	break;
      case 'S':
	srand(atoi(optarg)+97);
	break;
      case 'd':
	debuglevel = atoi(optarg);
	break;
      case 'R':
	nruns = atoi(optarg);
	break;
      case 'H':
	Hfactor = atof(optarg);
	break;
      case '?':
      default:
	fprintf(stderr, "Error: unknown command line flag %c\n\n%s\n",
		chr, usage);
	exit(EXIT_FAILURE);
	break;
      }
  }
    
  if (optind + 1 != argc) {
    fprintf(stderr, "Error: expect a yieldfile\n\n%s\n", usage);
    exit(EXIT_FAILURE);
  }

  if ((yieldfp = fopen(argv[optind], "r")) == NULL) {
    fprintf(stderr, "Error: Couldn't open yieldfile %s\n%s", argv[optind], usage);
    exit(EXIT_FAILURE);
  }
   
  g0 = read_grammar(grammarfp, si);
  set_rule_weights(g0, g0->weights);      /* normalize rule counts */

  signal(SIGINT, write_grammar_);

  for (irun = 0; irun < nruns; ++irun) {
    FLOAT entropy;

    g = copy_grammar(g0, si);

    if (summaryfp && debuglevel >= 100)
      fprintf(summaryfp, "\nStarting run %d\n", irun);
    
    g_global = g;
    si_global = si;
    
    if (nbeta <= 1) {
      if (jitter0 > 0)
	jitter_weights(g, jitter0);
      entropy = inside_outside_H(g, si, yieldfp, tracefp, summaryfp, debuglevel, maxlen,
				 minits, maxits, stoptol, minruleprob, jitter, 1, Hfactor); 
    }
    else {
      int ibeta;
      for (ibeta = 0; ibeta < nbeta; ++ibeta) {
	FLOAT beta = betastart + ibeta*(betastop - betastart)/(nbeta - 1.0);
	
	if (jitter0 > 0)
	  jitter_weights(g, jitter0);
	
	if (summaryfp && debuglevel >= 500)
	  fprintf(summaryfp, " Beta = %g\n", beta);
	entropy = inside_outside_H(g, si, yieldfp, tracefp, summaryfp, debuglevel, maxlen,
				   minits, maxits, stoptol, minruleprob, jitter, beta, Hfactor);
      }
    }
    
    if (debuglevel >= 0)
      fprintf(stdout, "Run %d entropy %g, %ld rules\n", irun, entropy, (long) g->nrules);

    if (debuglevel >= 100)
      write_grammar(stdout, g, si, minruleprob);

    fflush(stdout);

    free_grammar(g);
  }
  
  free_grammar(g0);
  si_free(si);
    
  if (mmm_blocks_allocated) 
    fprintf(stderr, "Error in mrf(): %ld memory block(s) not deallocated\n", 
	    mmm_blocks_allocated);
  
  /* check that everything has been deallocated */
  assert(mmm_blocks_allocated == 0);		
  exit(EXIT_SUCCESS);
}
