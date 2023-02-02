#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct rstack {
  void** data;
  int index;
  int size;
} rstack;

rstack* init_rstack (int size){
  rstack* st = (rstack* )malloc(sizeof(rstack));
  void** data = malloc(sizeof(void*)*size);
  st->index = -1;
  st->data = data;
  st->size = size;

  return st;
}

void rstack_push (rstack* st, void* elem){
  st->data[++st->index] = elem;
}

void* rstack_pop(rstack* st){
  if (st->index == -1) return NULL;
  return st->data[st->index--];
}

void* rstack_peek(rstack* st){
  if (st->index == -1) return NULL;
  return st->data[st->index];
}

void free_rstack (rstack* st) { 
  free(st->data);
  free(st);
}

bool rstack_empty (rstack* st) {
  return st->index == -1;
}

typedef struct vector {
  void** data;
  int n_elements;
  int size;
} vector;

typedef struct vector_it {
  vector* vec;
  int index;
  int n_elements;
} vector_it;

vector* init_vector (int size) {
  vector* vec = malloc(sizeof(vector));
  vec->data = malloc( sizeof(void*) * size);
  vec->n_elements = 0;
  vec->size = size;
  return vec;
}

void free_vector (vector* vec) {

  free(vec->data);
  free(vec);
}

void vector_insert_at (vector* vec, int index, void* elem) {

  if (vec->n_elements == vec->size){
    void** tmp_data = calloc(vec->size*2* sizeof(void*), 1);
    //vec->data =  realloc(vec->data, vec->size * 2 * sizeof(void *));
    
    memcpy(tmp_data, vec->data, vec->size*sizeof(void*));
    free(vec->data);
    vec->data = tmp_data;
    vec->size *= 2;
  }
  
  for (int i = vec->n_elements; i > index; i--) {
    vec->data[i] = vec->data[i-1];
  }
  vec->data[index] = elem;
  
  vec->n_elements++;
}

bool vector_has (vector* vec, void* elem) {

  for (int i = 0; i < vec->n_elements; i++) {
    if (vec->data[i] == elem) return true;
  }

  return false;

}

void vector_push (vector* vec, void* elem) {
  assert(elem != 0);
  vector_insert_at(vec, vec->n_elements, elem);
}

void* vector_get (vector* vec, int pos) {
  return vec->data[pos];
}

void vector_delete_at (vector* vec, int index) {

  int limit_index = vec->n_elements - 1;

  for (int i = index; i < limit_index; i++) {
    vec->data[i] = vec->data[i+1];
  }

  vec->n_elements--;
}

void vector_shift (vector* vec) {
  vector_delete_at(vec, 0);
}

void* vector_pop (vector* vec) {
  return vec->data[--vec->n_elements];
}

vector_it* vector_get_it (vector* vec) {
  vector_it* it = malloc(sizeof(vector_it));
  it->index = 0;
  it->n_elements = vec->n_elements;
  it->vec = vec;

  return it;
}

bool vector_empty (vector* vec) {
  return vec->n_elements == 0;
}

void* vector_it_next (vector_it* it) {

  if (it->index == it->n_elements) return NULL;
  
  assert(it->n_elements > 0);
  assert(it->index >= 0);
  assert(it->vec->data[it->index] != 0);

  return it->vec->data[it->index++];
}

void vector_it_shift (vector_it* it) {
  --it->index;
  --it->n_elements;
  vector_shift(it->vec);
}