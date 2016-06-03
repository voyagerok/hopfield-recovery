#include <stdlib.h>
#include "random_helper.h"

int
flip_coin (double probability)
{
  double rand_val;

  rand_val = (double)(rand() % 10000 + 1) / 10000;

  return rand_val <= probability;
}

