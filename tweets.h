#ifndef __TWEETS_H__
#define __TWEETS_H__

/*class for accumulating all data
 *regarding tweets 
 */

#include "twitter_user.h"

#include <unordered_map>
#include <vector>
#include <string>

class tweets {



  public:
  tweets() {
    ;
  }
  ~tweets() {
    delete[] crypto_names;
    for (auto& item: user_vectors) {
      delete item.second;

    }
    delete[] tweet_ids;
    for (int i=0; i < clustering_ip.container().size(); i++)
      delete clustering_ip.container()[i];
    clustering_ip.close();
  }

  void process_tweets(const std::vector<std::vector<std::string>>&);
  void process_crypto_terms(const std::vector<std::vector<std::string>>&);
  void process_dict(const std::vector<std::vector<std::string>>&);
  
  void generate_ujs(const std::vector<std::vector<std::string>>&);
  void generate_cjs(const std::vector<std::vector<point<double>*>>&);
  


  /*
  @crypto_pos , maps a cryptocurrencies string to an id
  ids are the implicit index derived from the line in the file
  @cc_id maps a term to a cryptocurrency id
  @tweet_score , maps a tweet id to it's score (sum of terms)
  //TODO if the crypt score contains values in a full integer range
  maybe use array?

  @crypto_id maps each tweet_id to a cryptocurrency id;
  */

  int ncryptos;
 //maps every cryptocurrecy to an id
  //map :: user_id -> vectors
  std::unordered_map<uint64_t, twitter_user*> user_vectors;
  twitter_user** clusters_vectors;
  size_t csize;

  std::string* crypto_names;
  uint64_t* tweet_ids;

  std::unordered_map<std::string , int64_t> crypto_pos;
  std::unordered_map<std::string, int64_t> cc_id;
  //double* tweet_score;
  std::unordered_map<uint64_t, double> sent_score;
  std::unordered_map<uint64_t, int64_t> crypto_id;

  std::unordered_map<std::string, double> lexicon;


  inp_parser clustering_ip;
  void get_clusters();


  void run_lsh();
  void run_clustering();

  void run_user_clustering();
};

#endif