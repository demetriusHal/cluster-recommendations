#include "tweets.h"

#include <sstream> 



using vector_2d = std::vector<std::vector<std::string>> ;

void tweets::process_tweets(const vector_2d& vref) {

  uint64_t tid;
  for (int i=0; i < vref.size(); i++) {
    //2nd column are the tweet ids
    std::stringstream s(vref[i][1]);
    s >> tid;
    crypto_id[tid] = -1;

    for (int j=2; j < vref[i].size(); j++) {
      if (cc_id.find(vref[i][j]) != cc_id.end()) {
        crypto_id[tid] = cc_id[vref[i][j]];
        break;
      }
    }

  }
  //calculate the sentimental value for each tweet
  for (int i=0; i < vref.size(); i++) {
    double sum = 0.0;
    for (int j=2; j < vref[i].size(); j++) {
      sum += (lexicon.find(vref[i][j]) != lexicon.end())?
        lexicon[vref[i][j]]:0.0;
    }

  }
}

//learn to what cryptocurrency does each term correspond to
void tweets::process_crypto_terms(const vector_2d& vref) {

  for (int i=0;i < vref.size(); i++) {
    for (int j=0; j < vref[i].size(); j++) {
      cc_id[vref[i][j]] = i;
    }
  }
}


void tweets::process_dict(const vector_2d& vref) {
  double score;
  for (auto& mapping: vref) {
    std::stringstream s(mapping[1]);
    s >> score;
    lexicon[mapping[0]] = score;
  }
}