/*
 * Adapted to TM2C by Vasileios Trigonakis <vasileios.trigonakis@epfl.ch> 
 *
 * File:
 *   hashtable.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Hashtable
 *   Implementation of an integer set using a stm-based/lock-free hashtable.
 *   The hashtable contains several buckets, each represented by a linked
 *   list, since hashing distinct keys may lead to the same bucket.
 *
 * Copyright (c) 2009-2010.
 *
 * hashtable.c is part of Microbench
 * 
 * Microbench is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "hashtable.h"

void
ht_delete(ht_intset_t *set) 
{
  int i;

  for (i = 0; i < maxhtlength; i++) 
    {
      intset_t *iset = set->buckets[i];
      set_delete(iset);
      free(set->buckets[i]);
    }
  free(set->buckets);
  free(set);
}

int
ht_size(ht_intset_t *set)
{
  int size = 0;
  int i;

  for (i = 0; i < maxhtlength; i++)
    {
      intset_t *iset = set->buckets[i];
      size += set_size(iset);
    }
  return size;
}


static void
write_node(node_t* node, val_t val, nxt_t next, int transactional) 
{
  node_t nd;
  nd.val = val;
  nd.next = next;
  if (transactional)
    {
      TX_STORE(node, nd.to_int64, TYPE_INT64);
    }
  else
    {
      NONTX_STORE(node, nd.to_int64, TYPE_INT64);
    }
}

static intset_t*
set_new1() 
{
  intset_t *set;
  node_t *min, *max;

  if ((set = (intset_t *) malloc(sizeof (intset_t))) == NULL) 
    {
      perror("malloc");
      EXIT(1);
    }

  node_t** nodes = (node_t**) pgas_app_alloc_rr(1, 2 * sizeof(node_t));
  min = nodes[0];
  max = (node_t*) (((char*) nodes[0]) + sizeof(node_t));
  write_node(max, VAL_MAX, 0, 0);
  write_node(min, VAL_MIN, OF(max), 0);

  set->head = min;
  return set;
}


ht_intset_t*
ht_new() 
{
  ht_intset_t* set;
  int i;

  if ((set = (ht_intset_t *) malloc(sizeof (ht_intset_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  if ((set->buckets = (void *) malloc((maxhtlength + 1) * sizeof (intset_t*))) == NULL) {
    perror("malloc");
    exit(1);
  }

  for (i = 0; i < maxhtlength; i++)
    {
      set->buckets[i] = set_new1();
    }
  FLUSH;
  return set;
}
