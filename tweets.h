#ifndef __TWEETS_H__
#define __TWEETS_H__

/*class for accumulating all data
 *regarding tweets 
 */

#include <unordered_map>
#include <vector>
#include <string>

class tweets {



  public:
  tweets() {
    crypto_id = nullptr;
  }

  void process_tweets(const std::vector<std::vector<std::string>>&);
  void process_crypto_terms(const std::vector<std::vector<std::string>>&);
  
  std::vector<std::vector<double>> generate_ujs();

  



    //maps every cryptocurrecy to an id
  std::unordered_map<std::string , int> crypto_pos;
  std::unordered_map<std::string, int> cc_id;
  double* tweet_score;
  uint32_t* crypto_id;
};

#endif