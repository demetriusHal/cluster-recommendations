#include "tweets.h"



using vector_2d = std::vector<std::vector<std::string>> ;

void tweets::process_tweets(const vector_2d& vref) {

  crypto_id = new uint32_t[vref.size()];
  for (int i=0; i < vref.size(); i++) {
    for (int j=0; j < vref[i].size(); j++) {
      if (cc_id.find(vref[i][j]) != cc_id.end()) {
        crypto_id[cc_id[vref[i][j]]] = j;
      }
    }

  }
}


void tweets::process_crypto_terms(const vector_2d& vref) {

  for (int i=0;i < vref.size(); i++) {
    crypto_pos[vref[i][0]] = i;
    for (int j=0; j < vref[i].size(); j++) {
      cc_id[vref[i][j]] = i;

    }
  }
}
