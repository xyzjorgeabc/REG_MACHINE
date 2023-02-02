#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include "utils.c"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct context {
  unsigned int n_or;
  unsigned int n_chars;
} context;

char* regtopostfix (char* regexp, int len) {

  context* context_stack = malloc(sizeof(context)*10);
  context* cs_p = context_stack;

  char* postfix = malloc(len*2);
  char* postfix_p = postfix;
  unsigned int n_or = 0;
  unsigned int n_exps = 0;


  while (*regexp) 
  switch (*regexp) {

    default:
      if (n_exps > 1) {
        *postfix_p = '.';
        ++postfix_p;
        --n_exps;
      }

      ++n_exps;
      *postfix_p = *regexp;
      ++postfix_p;
      ++regexp;
    break;

    case '+':
    case '*':
    case '?':
      *postfix_p = *regexp;
      ++postfix_p;
      ++regexp;
    break;

    case '|':
      while (n_exps > 1){
        *postfix_p = '.';
        ++postfix_p;
        --n_exps;
      }
      n_exps = 0;
      ++n_or;
      ++regexp;
    break;

    case '(':
      while (n_exps > 1) {
        *postfix_p = '.';
        ++postfix_p;
        --n_exps;
      }
      cs_p->n_chars = n_exps;
      cs_p->n_or = n_or;
      n_exps = 0;
      n_or = 0;
      ++cs_p;
      ++regexp;
    break;

    case ')':
      while (n_exps > 1) {
        *postfix_p = '.';
        ++postfix_p;
        --n_exps;
      }
      while (n_or > 0) {
        *postfix_p = '|';
        ++postfix_p;
        --n_or;
      }
      --cs_p;
      n_exps = cs_p->n_chars;
      n_or = cs_p->n_or;
      ++n_exps;
      ++regexp;
    break;
  }

  while (n_exps > 1) {
    *postfix_p = '.';
    ++postfix_p;
    --n_exps;
  }
  while (n_or > 0) {
    *postfix_p = '|';
    ++postfix_p;
    --n_or;
  }

  *postfix_p = '\0';

  return postfix;
}

#define empty_char (0xFF+1)

// REPRESENTACIÓN DE UN ESTADO LIMITADO A LA CONSTRUCCIÓN DE KEN THOMPSON.
// UN ESTADO PUEDE TENER UNA SOLA TRANSICIÓN DE ALFANUMÉRICO O UNO O DOS TRANSICIONES EPSILON.

typedef struct state { 
  int symbol; // puede ser alfanumérico o empty_char
  struct state* main_out;
  struct state* secondary_out; // solo tiene un valor válido si symbol es emptychar y main_out es válido.

} state;

// UNA VEZ LA NFA FINAL SE HA CONSTRUIDO, OUT ES EL ESTADO DE ACEPTACIÓN.
typedef struct nfa {
  state* entry; // estado incial de la nfa.
  state* out; // estado salida de la nfa.
} nfa;

state* make_state () {

  state* s = calloc(sizeof(state), 1);
  s->secondary_out = NULL;
  s->main_out = NULL;

  return s;
}

nfa* make_nfa (int c) {

  nfa* nf = malloc(sizeof(nfa));
  state* st = make_state();;

  nf->out = make_state();
  st->symbol = c;
  st->main_out = nf->out;

  nf->entry = make_state();
  nf->entry->symbol = empty_char;
  nf->entry->main_out = st;
  return nf;
}

void free_state(state* st) {
  free(st);
}

nfa* nfa_one_or_more (nfa* nf) {

  state* new_entry = make_state();
  state* new_out = make_state();

  new_entry->symbol = empty_char;
  new_entry->main_out = nf->entry;

  nf->out->symbol = empty_char;
  nf->out->main_out = nf->entry;
  nf->out->secondary_out = new_out;

  nf->entry = new_entry;
  nf->out = new_out;

  return nf;
}

nfa* nfa_one_or_none (nfa* nf) {

  state* new_entry = make_state();
  state* new_out = make_state();

  new_entry->symbol = empty_char;
  new_entry->main_out = nf->entry;
  new_entry->secondary_out = new_out;

  nf->out->symbol = empty_char;
  
  if (nf->out->main_out)
    nf->out->secondary_out = new_out;
  else
    nf->out->main_out = new_out;
  
  nf->entry = new_entry;
  nf->out = new_out;

  return nf;
}

nfa* nfa_none_or_more (nfa* nf) {

  nfa_one_or_more(nf);
  nfa_one_or_none(nf);

  return nf;
}


nfa* nfa_union (nfa* hnfa1, nfa* hnfa2) {

  state* new_entry = make_state();
  state* new_out = make_state();

  hnfa1->out->symbol = empty_char;
  
  if (hnfa1->out->main_out) 
    hnfa1->out->secondary_out = new_out;
  else 
    hnfa1->out->main_out = new_out;

  if (hnfa2->out->main_out) 
    hnfa2->out->secondary_out = new_out;
  else 
    hnfa2->out->main_out = new_out;

  new_entry->symbol = empty_char;

  new_entry->main_out = hnfa1->entry;
  new_entry->secondary_out = hnfa2->entry;

  hnfa1->entry = new_entry;
  hnfa1->out = new_out;

  free(hnfa2);
  return hnfa1;
}

nfa* nfa_concatenate (nfa* hnfa1, nfa* hnfa2) {

  hnfa1->out->symbol = empty_char;
  state* tmp_state = hnfa2->entry;

  hnfa1->out->main_out = hnfa2->entry->main_out;
  hnfa1->out->secondary_out = hnfa2->entry->secondary_out;

  hnfa1->out = hnfa2->out;
  
  free(hnfa2->entry);
  free(hnfa2);
  return hnfa1;
}

nfa* postfix_to_nfa (char* postfix) {
  //TODO: MINIMIZAR TRANSICIONES EPSILON.

  rstack * nfa_st = init_rstack(256);
  nfa* curr_nfa;
  nfa* hnfa1;
  nfa* hnfa2;

  while (*postfix)
  switch (*postfix) {

    default:
      curr_nfa = make_nfa(*postfix);
      rstack_push(nfa_st, curr_nfa);
      ++postfix;
    break;

    case '|':
      hnfa2 = rstack_pop(nfa_st);
      hnfa1 = rstack_pop(nfa_st);
      curr_nfa = nfa_union(hnfa1, hnfa2);
      rstack_push(nfa_st, curr_nfa);
      ++postfix;
    break;

    case '.':
      hnfa2 = rstack_pop(nfa_st);
      hnfa1 = rstack_pop(nfa_st);
      curr_nfa = nfa_concatenate(hnfa1, hnfa2);
      rstack_push(nfa_st, curr_nfa);
      ++postfix;
    break;

    case '+':
      hnfa1 = rstack_pop(nfa_st);
      curr_nfa = nfa_one_or_more(hnfa1);
      rstack_push(nfa_st, curr_nfa);
      ++postfix;
    break;

    case '?':
      hnfa1 = rstack_pop(nfa_st);
      curr_nfa = nfa_one_or_none(hnfa1);
      rstack_push(nfa_st, curr_nfa);
      ++postfix;
    break;

    case '*':
      hnfa1 = rstack_pop(nfa_st);
      curr_nfa = nfa_none_or_more(hnfa1);
      rstack_push(nfa_st, curr_nfa);
      ++postfix;
    break;
  }

  curr_nfa = rstack_pop(nfa_st);
  assert(rstack_empty(nfa_st));

  return curr_nfa;
}

void merge_epsilon_closure (vector* active_states) {

  vector* epsilon_transition_active_states = init_vector(8);
  vector_it* it;
  state* st = NULL;

  it = vector_get_it(active_states);
  while ((st = vector_it_next(it))) {
    if (st->symbol == empty_char) {
      vector_push(epsilon_transition_active_states, st);
    }
  }

  free(it);
  // avanzamos todas las transiciones epsilon hasta que
  // no podemos avanzar más.
  while (!vector_empty(epsilon_transition_active_states)){
    vector_it* epsilon_ts_it = vector_get_it(epsilon_transition_active_states);
    state* st;
    while ((st = vector_it_next(epsilon_ts_it))) {

      if (st->symbol == empty_char) {

        if (st->main_out && !vector_has(epsilon_transition_active_states, st->main_out))
          vector_push(epsilon_transition_active_states, st->main_out);

        if (st->secondary_out && !vector_has(epsilon_transition_active_states, st->secondary_out))
          vector_push(epsilon_transition_active_states, st->secondary_out);

      }

      if (!vector_has(active_states, st))
        vector_push(active_states, st);

      vector_it_shift(epsilon_ts_it);
    }
    free(epsilon_ts_it);
  }

  free_vector(epsilon_transition_active_states);
}

int match_nfa (nfa* nf, char* str) {

  vector* active_states = init_vector(8);
  vector_it* it;
  state* st = NULL;
  int i = 0;
  int last_f = -1;

  vector_push(active_states, nf->entry);

  while (str[i]) {
    if (vector_empty(active_states)) {
      free_vector(active_states);
      return last_f;
    }
    merge_epsilon_closure(active_states);

    //por cada estado en el que nos encontramos nos movemos si es posible.
    it = vector_get_it(active_states);
    while ((st = vector_it_next(it))) {
      if (st == nf->out) last_f = i;
      if (st->symbol == str[i]) {
        vector_push(active_states, st->main_out);
      }
      vector_it_shift(it);
    }

    free(it);
    ++i;
  }

  merge_epsilon_closure(active_states);

  it = vector_get_it(active_states);
  while ((st = vector_it_next(it))) {
    if (st == nf->out) last_f = i-1;
  }

  free(it);
  free_vector(active_states);
  return last_f;
}

void prep_jump_table (unsigned char* pat, int pat_len, int jumps[256]) {
  for (int i = 0; i < 256; i++) jumps[i] = -1;
  for (int i = 0; i < pat_len; i++) jumps[pat[i]] = i;
}

unsigned char* bm (unsigned char* str, int str_len, unsigned char* pat, int pat_len, int jumps[256]) {

  if (str_len < pat_len) return NULL;
  
  int j_reset = pat_len-1;
  int j = 0;

  unsigned char*  end = str+(str_len-1);
  unsigned char* k;
  int skip = 0;
  
  while (str <= end && j != -1) {
    j = j_reset;
    k = str + (j_reset+1);
    while (j != -1){

      if (*k != pat[j]){
        skip = j - jumps[*k];
        
        if (skip > 0 ) str+=skip;
        else ++str;
        
        break;
      }

      --j;
      --k;
      continue;
    }
  }

  if (j == -1) return ++k;
  return NULL;
}

int main (int argc, char** argv) {

  char* regex;
  char* mode;
  FILE* file;
  struct stat st;
  size_t literal_len;
  char* literal_text;


  if (argc == 4) {

    regex = argv[1];
    mode = argv[2];

    if (strcmp(mode, "-f") == 0) {

      if (stat(argv[3], &st)) {
        printf("COULD NOT OBTAIN INPUT FILE INFO");
        return EXIT_FAILURE;
      }

      if (st.st_size > 1024*1024*1024) {
        printf("FILE TOO BIG LOL");
        return 0;
      }

      file = fopen(argv[3], "r");
      if (file == NULL) {
        printf("COULD NOT OPEN INPUT FILE INFO");
        return EXIT_FAILURE;
      }
      
      literal_len = st.st_size;
      literal_text = malloc(literal_len + 1);
      literal_len = fread(literal_text, 1 ,st.st_size, file);
    }
    else if (strcmp(mode, "-l") == 0) {
      literal_text = argv[3];
      literal_len = strlen(argv[3]);
    }
    else {
      printf("INCORRECT PARAMETERS\n");
      printf("NEED: \"regexpr\" -f|-l \"filename\"|\"literaltext\"\n");
      return EXIT_SUCCESS;
    }


    char* postfix = regtopostfix(regex, strlen(regex));
    nfa* nf = postfix_to_nfa(postfix);

    int i = 0;
    int match;
    int matches = 0;

    while (i < literal_len) {

      match = match_nfa(nf, literal_text + i);
      i += (match != -1 ? match+1 : 1);
      if (match != -1) ++matches;
      
    }

    printf("-- matches:  %i --", matches);
  }
  else {
    printf("INCORRECT NUMBER OF PARAMETERS \n");
    printf("NEED: \"regexpr\" -f|-l \"filename\"|\"literaltext\"\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
