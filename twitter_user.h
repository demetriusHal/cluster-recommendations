#ifndef __TWITTER_USER__
#define __TWITTER_USER__

#include <vector>
#include <iostream>
#include <limits>



class twitter_user {

  public:


  twitter_user(const int& id, const int& ncryptos):known(ncryptos),scores(ncryptos), id(id) {
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

  


};





#endif