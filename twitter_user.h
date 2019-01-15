#ifndef __TWITTER_USER__
#define __TWITTER_USER__

#include <vector>
#include <iostream>
#include <limits>

#include "parser.h"



class twitter_user {

  public:


  twitter_user(const uint64_t& id, const int& ncryptos):known(ncryptos),scores(ncryptos), id(id) {
    for (int i=0; i < ncryptos; i++)
      known[i] = false;
  }
  


  void normalise();

  void put_score(const uint64_t& cid,const double& score) {
    scores[cid] += score;
    known[cid] = true;
  }
  
  std::vector <double> scores;
  std::vector <bool> known;
  double avg;
  const uint64_t id;

  
  point<double>* to_point() {
    point<double>* pptr = new point<double>(id, scores);
    return pptr;
  }

  

};





#endif