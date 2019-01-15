#include "tweets.h"
#include "parser.h"

#include <sstream> 
#include <cmath>
#include <algorithm>

#include "lsh.h"
#include "kmeans.h"
#include "tsv_parser.h"

using vector_2d = std::vector<std::vector<std::string>> ;


double angular_dist(const twitter_user& p1, twitter_user& p2) {
  double s = 0;
  //note the the correct way to do this is to store all the
  // r1,r2 and not compute them each time
  double r1 = 0,r2 = 0;
  for (int i=0; i < p1.scores.size(); i++) {
    s += p1.scores[i]*p2.scores[i];
    r1 += p1.scores[i]*p1.scores[i];
    r2 += p2.scores[i]*p2.scores[i];
  }
  s = s/sqrt(r1*r2);
  if (r1 == 0 || r2 == 0)
    return 1.0;
  return 1.0-s;
}

double angular_dist(const twitter_user& p1, point<double>& p2) {
  double s = 0;
  //note the the correct way to do this is to store all the
  // r1,r2 and not compute them each time
  double r1 = 0,r2 = 0;
  for (int i=0; i < p1.scores.size(); i++) {
    s += p1.scores[i]*p2.c[i];
    r1 += p1.scores[i]*p1.scores[i];
    r2 += p2.c[i]*p2.c[i];
  }
  s = s/sqrt(r1*r2);
  if (r1 == 0 || r2 == 0)
    return 1.0;
  return 1.0-s;
}

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
  tweet_ids = new uint64_t[vref.size()];
  //calculate the sentimental value for each tweet
  for (int i=0; i < vref.size(); i++) {
    double sum = 0.0;
    std::stringstream s(vref[i][1]);
    s >> tid;
    tweet_ids[i] = tid;
    for (int j=2; j < vref[i].size(); j++) {
      sum += (lexicon.find(vref[i][j]) != lexicon.end())?
        lexicon[vref[i][j]]:0.0;
    }
    sent_score[tid] = sum/sqrt(sum*sum+15);
  }
}

//learn to what cryptocurrency does each term correspond to
void tweets::process_crypto_terms(const vector_2d& vref) {

  ncryptos = vref.size();
  crypto_names = new std::string[ncryptos];
  for (int i=0;i < vref.size(); i++) {
    crypto_pos[vref[i][0]] = i;
    crypto_names[i] = vref[i][0];
    for (int j=0; j < vref[i].size(); j++) {
      cc_id[vref[i][j]] = i;
    }
  }
}


void tweets::process_dict(const vector_2d& vref) {
  double score;
  for (auto& mapping: vref) {
    //std::cout << mapping[0] << " - " << mapping[1] << std::endl;
    std::stringstream s(mapping[1]);
    s >> score;
    lexicon[mapping[0]] = score;
  }
}

void tweets::generate_ujs(const vector_2d& vref) {

  uint64_t uid,tid;
  
  for (int i=0;i < vref.size(); i++) {

    std::stringstream us(vref[i][0]);
    std::stringstream tw(vref[i][1]);
    us >> uid;
    tw >> tid;
    
    if (user_vectors.find(uid) == user_vectors.end())
      user_vectors[uid] = new twitter_user(uid, this->ncryptos);
    user_vectors[uid]->put_score(crypto_id[tid], sent_score[tid]);



  }
  for (auto& uptr: user_vectors)
    uptr.second->normalise();

}

void tweets::get_clusters() {
  
  clustering_ip.open("dataset/twitter_dataset_small_v2.csv");
  clustering_ip.preparse();

  std::vector<point<double>*>& points = clustering_ip.container();
  kmeans_info kmi = parse_config(std::string("config.json"));
  kmi.mtr = m_angular;
  kmeans_cluster clusterer(kmi, points);
  
  clusterer.simple_init();
  for (int i=0; i < kmi.maxIter; i++) {
    clusterer.assign();
    clusterer.update_centroids();
    if (clusterer.get_epsilon() < kmi.eps)
      break;
  }

  


  generate_cjs(clusterer.results());
}

void tweets::generate_cjs(const std::vector<std::vector<point<double>*>>& clusters) {
  clusters_vectors = new twitter_user*[clusters.size()];
  uint64_t tid;
  csize = clusters.size();
  for (int i=0; i < clusters.size(); i++) {
    clusters_vectors[i] = new twitter_user(i, this->ncryptos);
    for (int j=0; j < clusters[i].size(); j++) {
      tid = tweet_ids[clusters[i][j]->id-1];
      clusters_vectors[i]->put_score(
        crypto_id[tid],sent_score[tid]);
    }

  }

  for (int i=0; i < csize; i++)
    clusters_vectors[i]->normalise();
}


void tweets::run_user_clustering() {


  std::vector<point<double>*> user_points;

    for (auto& item: this->user_vectors) {
      
      point<double>* p = item.second->to_point();
      user_points.push_back(p);
  }
  kmeans_info kmi = parse_config(std::string("config.json"));
  kmi.mtr = m_angular;
  kmi.cluster_K = 50;
  kmeans_cluster clusterer(kmi, user_points);

  clusterer.simple_init();
  for (int i=0; i < kmi.maxIter; i++) {
    clusterer.assign();
    clusterer.update_centroids();
    if (clusterer.get_epsilon() < kmi.eps)
      break;
  }

  auto clusters = clusterer.results();
  
  double sum = 0.0;
  std::vector<double> tmp;
  for (int i=0; i < clusters.size(); i++) {
    for (auto& vec: clusters[i]) {
      tmp.assign(this->ncryptos, 0.0);
      
      auto& item = user_vectors[(uint64_t)vec->id];
      for (int j=0; j < this->ncryptos; j++) {
        
        if (!item->known[j]) {
          sum = 0.0;
          for (auto& it: clusters[i]) {
            sum += 1.0*(1.0-angular_dist(*item,*it))*(it->c[j]);
          }
          tmp[j] = sum;

        } else {
          tmp[j] = std::numeric_limits<double>::lowest();

        }
      }
      printf("<u%ld> ",vec->id);
      for (int j=0; j < 5; j++) {
        auto dptr = std::max_element(tmp.begin(), tmp.end());
        printf("%s ", this->crypto_names[dptr-tmp.begin()].c_str());
        *dptr = std::numeric_limits<double>::lowest();
    }
    printf("\n");

    }
  }

}

void tweets::run_lsh() {

  lsh_map<double> lm(4, 5, this->ncryptos, 0.0001, m_angular);

  std::vector<point<double>*> user_points;

  for (auto& item: this->user_vectors) {
    point<double>* p = item.second->to_point();
    user_points.push_back(p);
    lm.insert(p);
  }
  std::set<std::pair<double ,point<double>*>>* neighb;
  int i=0;
  double sum = 0.0;
  std::vector<double> tmp;
  printf("Cosine LSH\n");
  for (auto& item: this->user_vectors) {
    //TEMPLATED
    neighb = lm.lookup_nearest(*user_points[i], 20);
    tmp.assign(this->ncryptos, 0.0);
    for (int j=0; j < this->ncryptos; j++) {
      if (!item.second->known[j]) {
        sum = 0.0;
        for (auto& it: *neighb) {
          sum += 1.0*(1.0-it.first)*(it.second->c[j]);
        }

        tmp[j] = sum;
      } else
        tmp[j] = std::numeric_limits<double>::lowest();
    }
    
    printf("<u%ld> ",item.first);


    for (int j=0; j < 5; j++) {
      auto dptr = std::max_element(tmp.begin(), tmp.end());
      printf("%s ", this->crypto_names[dptr-tmp.begin()].c_str());
      *dptr = std::numeric_limits<double>::lowest();
    }
    printf("\n");

    i++;
  }
}


void tweets::run_clustering() {
  int i=0;
  double sum = 0.0;
  std::vector<double> tmp;
  printf("Clustering:\n");
  for (auto& item: this->user_vectors) {
    //TEMPLATED
    tmp.assign(this->ncryptos, 0.0);
    for (int j=0; j < this->ncryptos; j++) {
      if (!item.second->known[j]) {
        sum = 0.0;
        for (int k=0; k < csize; k++) {
          sum += 1.0*(1.0-angular_dist(*item.second,*(clusters_vectors[k])))*(clusters_vectors[k]->scores[j]);
        }

        tmp[j] = sum;
      } else
        tmp[j] = std::numeric_limits<double>::lowest();
    }
    
    printf("<u%ld> ",item.first);

    for (int j=0; j < 5; j++) {
      auto dptr = std::max_element(tmp.begin(), tmp.end());
      printf("%s ", this->crypto_names[dptr-tmp.begin()].c_str());
      *dptr = std::numeric_limits<double>::lowest();
    }
    printf("\n");

    i++;
  }
}

#ifdef __UNIT_MAIN_TWEETS__

#include <cstdio>

int main() {

  tsv_parser tsv;

  tsv.parse_file("vader_lexicon.csv");
  std::vector<std::vector<std::string>>* data;
  data = tsv.get_data();
  //for (int i=0; i < data->size(); i++)
  //    printf("%s : %s\n", data->at(i)[0].c_str(), data->at(i)[1].c_str());
  tweets t;

  t.process_dict(*data);

  tsv.destroy_data();
  for (auto& item: t.lexicon) {
    //printf("%s : %lf\n", item.first.c_str() ,item.second);
  }

  tsv.parse_file("coins_queries.csv");
  
  t.process_crypto_terms(*tsv.get_data());

  for (auto& item: t.cc_id) {
    //printf("<%s> : <%ld>\n", item.first.c_str() ,item.second);
  }

  tsv.parse_file("dataset/tweets_dataset_small.csv");
  
  t.process_tweets(*tsv.get_data());
  for (auto& item: t.crypto_pos) {
    //printf("<%s> : <%ld>\n", item.first.c_str() ,item.second);
  }

  for (auto& item: t.sent_score) {
    printf("<%ld> : <%lf>\n", item.first ,item.second);
  }

  t.generate_ujs(*tsv.get_data());

  lsh_map<double> lm(4, 5, t.ncryptos, 400, m_angular);


  for (auto& item: t.user_vectors) {
    printf("%ld :", item.first);
    for (int i=0; i < item.second->scores.size(); i++) {
      if (item.second->known[i])
        printf("{ %d : %lf }", i, item.second->scores[i]);
      
    }
    lm.insert(item.second->to_point());
    printf("\n");
  }



}

#endif

#include <cstdio>
#include <ctime>

int main() {

  tsv_parser tsv;

  tsv.parse_file("vader_lexicon.csv");
  std::vector<std::vector<std::string>>* data;
  data = tsv.get_data();

  tweets t;

  t.process_dict(*data);
  tsv.destroy_data();
  tsv.parse_file("coins_queries.csv");
  t.process_crypto_terms(*tsv.get_data());
  tsv.destroy_data();

  tsv.parse_file("dataset/tweets_dataset_small.csv");
  t.process_tweets(*tsv.get_data());
  t.generate_ujs(*tsv.get_data());
  t.get_clusters();

  int st;
  printf("Ok\n");
  st = clock();
  t.run_user_clustering();
  printf("exec_time = %lf\n", ((double)clock()-st)/CLOCKS_PER_SEC);
  st = clock();
  t.run_lsh();
  printf("exec_time = %lf\n", ((double)clock()-st)/CLOCKS_PER_SEC);
  st = clock();
  t.run_clustering();
  printf("exec_time = %lf\n", ((double)clock()-st)/CLOCKS_PER_SEC);
  
  

}