#include "twitter_user.h"





#include <iostream>


void twitter_user::normalise() {
  double sum = 0.0;
  uint64_t count = 0;
  for (int i=0; i < scores.size(); i++) {
    if (known[i]) {
      sum += scores[i];
      count++;
    }

  }
  this->avg = sum/count;
  for (int i=0; i < scores.size(); i++) {
    if (known[i]) {
      scores[i] -= avg;
    } 
    else {
      scores[i] = 0.0;
    }
    

  }
}