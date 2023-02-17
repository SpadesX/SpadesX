#ifndef MT_H
#define MT_H

#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M      397 /* changes to STATE_VECTOR_LENGTH also require changes to this */

typedef struct tagMTRand {
  unsigned long mt[STATE_VECTOR_LENGTH];
  int index;
} mt_rand_t;

mt_rand_t seed_rand(unsigned long seed);
unsigned long gen_rand_long(mt_rand_t* rand);
double gen_rand(mt_rand_t* rand);

#endif /* #ifndef MT_H */
