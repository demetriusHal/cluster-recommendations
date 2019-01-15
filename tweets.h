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
    ;
  }

  void process_tweets(const std::vector<std::vector<std::string>>&);
  void process_crypto_terms(const std::vector<std::vector<std::string>>&);
  void process_dict(const std::vector<std::vector<std::string>>&);
  
  std::vector<std::vector<double>> generate_ujs();

  /*
  @crypto_pos , maps a cryptocurrencies string to an id
  ids are the implicit index derived from the line in the file
  @cc_id maps a term to a cryptocurrency id
  @tweet_score , maps a tweet id to it's score (sum of terms)
  //TODO if the crypt score contains values in a full integer range
  maybe use array?

  @crypto_id maps each tweet_id to a cryptocurrency id;
  */



  //maps every cryptocurrecy to an id

  
  std::unordered_map<std::string , int64_t> crypto_pos;
  std::unordered_map<std::string, int64_t> cc_id;
  //double* tweet_score;
  std::unordered_map<uint64_t, double> crypt_score;
  std::unordered_map<uint64_t, int64_t> crypto_id;

  std::unordered_map<std::string, double> lexicon;
};

#endif