// bayes.cc -- Bayesian estimates of PCFGs from unsupervised training data
//
// Mark Johnson, 15th October 2005
//
// These estimators find the rule weights that optimize the posterior
// likelihood using numerical methods.

const char usage[] =
"bayes [-d debuglevel] [-R runs]\n"
"   [-g grammar] [-p prune] [-l maxlen] [-m minits] [-n maxits]\n"
"   [-S randseed] [-j jitter] [-J per-iteration-jitter]\n"
"   [-b betastart] [-B betastop] [-N nbeta] yieldfile\n";

#include "tao-optimizer.h"
#include "grammar.h"
#include "expected-counts.h"

#include <string>

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


struct Q {  // Q is the function to be optimized
  grammar g;
  const si_t si;
  FILE* yieldfp;
  FILE* tracefp;
  FILE* summaryfp;
  int debuglevel;
  int maxsentlen;
  FLOAT minruleprob;
  FLOAT min_q;
  FLOAT min_entropy;
  int   niterations;
  FLOAT H_scale_factor;
  FLOAT Q_scale_factor;

  Q(grammar g, const si_t si, FILE* yieldfp, FILE* tracefp, FILE* summaryfp,
    int debuglevel, int maxsentlen, FLOAT minruleprob, FLOAT H_scale_factor, 
    FLOAT Q_scale_factor) 
    : g(g), si(si), yieldfp(yieldfp), tracefp(tracefp), summaryfp(summaryfp), 
      debuglevel(debuglevel), maxsentlen(maxsentlen), minruleprob(minruleprob),
      min_q(0), min_entropy(0), niterations(0), H_scale_factor(H_scale_factor),
      Q_scale_factor(Q_scale_factor) { }

  FLOAT operator() (int n, double x[], double dQ_dx[]) {

    ++niterations;

    if (summaryfp && debuglevel >= 1000)
      fprintf(summaryfp, " iteration %d, ", niterations);

    assert(g->nrules == size_t(n));

    FLOAT *parent_sum = (FLOAT *) CALLOC((g->nnts+1), sizeof(FLOAT));

    for (size_t i = 0; i < g->nrules; ++i) {
      size_t parent = g->rules[i]->e[0];
      assert(parent <= g->nnts);
      assert(finite(x[i]));
      assert(x[i] >= 0.0);
      parent_sum[parent] += x[i];
    }

    for (size_t i = 0; i < g->nrules; ++i) {
      size_t parent = g->rules[i]->e[0];
      assert(parent_sum[parent] > 0);
      g->weights[i] = x[i]/parent_sum[parent];
      assert(finite(g->weights[i]));
    }

    // write_grammar(stderr, g, si, minruleprob);

    FLOAT *rule_counts = (FLOAT *) CALLOC(g->nrules, sizeof(FLOAT));
    FLOAT sum_yieldweights;
    FLOAT sum_neglog_prob = expected_rule_counts(g, si, yieldfp, tracefp, summaryfp, 
						 debuglevel, maxsentlen, minruleprob, 
						 rule_counts, &sum_yieldweights);
   
    FLOAT Q = Q_scale_factor * sum_neglog_prob;
    FLOAT *dQ_dw = (FLOAT *) CALLOC(g->nrules, sizeof(FLOAT));

    for (size_t i = 0; i < g->nrules; ++i)
      dQ_dw[i] = - Q_scale_factor * rule_counts[i]/g->weights[i];

    if (H_scale_factor != 0) 
      for (size_t i = 0; i < g->nrules; ++i) {
	FLOAT w = g->weights[i];
	if (w > 0) {
	  Q -= Q_scale_factor * H_scale_factor * w * log(w);
	  dQ_dw[i] -= Q_scale_factor * H_scale_factor * (log(w) + 1);
	}
      }  

    FLOAT *parent_wdQ = (FLOAT *) CALLOC((g->nnts+1), sizeof(FLOAT));

    for (size_t i = 0; i < g->nrules; ++i) {
      FLOAT w = g->weights[i];
      size_t parent = g->rules[i]->e[0];
      parent_wdQ[parent] += w * dQ_dw[i];
    }

    for (size_t i = 0; i < g->nrules; ++i) {
      size_t parent = g->rules[i]->e[0];
      dQ_dx[i] = (dQ_dw[i] - parent_wdQ[parent])/parent_sum[parent];
    }

    FREE(parent_wdQ);
    FREE(dQ_dw);
    FREE(rule_counts);
    FREE(parent_sum);

    FLOAT entropy = sum_neglog_prob/(log(2)*sum_yieldweights);

    if (niterations == 1 || entropy < min_entropy)
      min_entropy = entropy;

    if (niterations == 1 || Q < min_q)
      min_q = Q;

    return Q;
  }
}; // Q{}


int      
main(int argc, char **argv)
{
  si_t  si = make_si(1024);
  FILE  *grammarfp = stdin;
  FILE  *yieldfp;
  FILE	*tracefp = stderr;  	/* set this to NULL to stop trace output */
  FILE	*summaryfp = stderr;	/* set this to NULL to stop parse stats */

  tao_environment tao_env(argc, argv, usage);

  {
    const char* grammarfile = tao_env.get_cstr_option("-g");
    if (grammarfile) {
      grammarfp = fopen(grammarfile, "r");
      if (grammarfp == NULL) {
	fprintf(stderr, "Error: couldn't open grammarfile %s\n%s",
		grammarfile, usage);
	exit(EXIT_FAILURE);
      }
    }
  }

  grammar g0 = read_grammar(grammarfp, si);
  set_rule_weights(g0, g0->weights);      /* normalize rule counts */

  minruleprob = tao_env.get_double_option("-p", 1e-7);
  int maxsentlen = tao_env.get_int_option("-l");
  FLOAT jitter0 = tao_env.get_double_option("-j");
  
  {
    int seed = tao_env.get_int_option("-S", 0);
    if (seed > 0) 
      srand(seed+97);
  }

  int debuglevel = tao_env.get_int_option("-d", 0);
  int nruns = tao_env.get_int_option("-R", 1);

  {
    const char* yieldfile = tao_env.get_cstr_option("-y");
    if (!yieldfile) {
      fprintf(stderr, "Error: expect a yieldfile\n\n%s\n", usage);
      exit(EXIT_FAILURE);
    }
    if ((yieldfp = fopen(yieldfile, "r")) == NULL) {
      fprintf(stderr, "Error: Couldn't open yieldfile %s\n%s", yieldfile, usage);
      exit(EXIT_FAILURE);
    } 
  }

  FLOAT H_scale_factor = tao_env.get_double_option("-Hfactor", 0);
  FLOAT Q_scale_factor = tao_env.get_double_option("-Q", 1);

  signal(SIGINT, write_grammar_);

  for (int irun = 0; irun < nruns; ++irun) {

    grammar g = copy_grammar(g0, si);

    if (summaryfp && debuglevel >= 100)
      fprintf(summaryfp, "\nStarting run %d\n", irun);
    
    g_global = g;
    si_global = si;
    
    if (jitter0 > 0)
      jitter_weights(g, jitter0);

    Q q(g, si, yieldfp, tracefp, summaryfp, debuglevel, maxsentlen, minruleprob, H_scale_factor, Q_scale_factor);

    tao_constrained_optimizer<Q> x(g->nrules, q);
    for (size_t i = 0; i < g->nrules; ++i) {
      x[i] = g->weights[i];
      x.lower_bound[i] = minruleprob*minruleprob;
      x.upper_bound[i] = 1.0;
    }

    x.optimize();

    for (size_t i = 0; i < g->nrules; ++i)
      g->weights[i] = x[i];
    set_rule_weights(g, g->weights);

    prune_grammar(g, si, minruleprob);

    FLOAT entropy = q.min_entropy;
    if (debuglevel >= 0)
      fprintf(stdout, "Run %d, %d iterations, %g bits per word, %ld rules\n", irun, q.niterations,
	      entropy, (long) g->nrules);

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
