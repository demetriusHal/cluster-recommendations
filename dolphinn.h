#ifndef __DOLPHINN_H__
#define __DOLPHINN_H__


#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <stack>

#include <cmath>
#include <climits>
#include <limits>

#include "lsh.h"
#include "parser.h"

typedef uint64_t key_type;

template <class T>
T uniform(T , T);
template <class T>
T normal();
template <class T>
int toInt(T);





template <class T>
class dolphinn_map {


  public:
    void insert(point<T>* );
    const point<T>* query(point<T>&);
    point_range<T>* query_range(point<T>&, double);


    dolphinn_map(int k, int M, int probes, int D, double w=4.0,
       metric=m_euclidean);

    void smart_range(point<double> &p, double radius,
         std::unordered_set<point<double>*>& set,
       std::unordered_map<point<double>*, double>& dists,
       std::unordered_set<point<double>*>& fullset);


  private:

    std::unordered_map<key_type, std::vector<point<T>*>> cube;
    std::vector<std::unordered_map<int, int>> randomizer;
    std::vector<hyperplane<double>> hash_functions;
    //assigning bits to hash values works lazily
    //we are onyl going to assing a bit as soon as
    // we require to use it and we will keep its value on the
    // random bit map
    int k;
    int M;
    int probes;
    int dimension;
    int window;

    const metric lsh_metric;

    key_type get_key(point<T>& p);
    int euclidean_hash(point<T>&, hyperplane<double>& );

    key_type angular_key(point<T> &p);
    int angular_hash(point<T>&, hyperplane<double>& );
};

class combination_generator {

  public:
    key_type get_next_comb();

    combination_generator(int n, key_type num):num(num),finish(false), n(n), k(0) {
    }
    void reset();
    void set_k(int newk) {
      k = newk;
      reset();
    }

    bool finished() {
      return finish;
    }

  private:
    key_type current;
    key_type num;
    bool finish;
    std::stack<int> selection_stack;
    int n,k;

};



template <class T>
dolphinn_map<T>::dolphinn_map(int k, int M, int probes, int D, double w, metric m)
:k(k),M(M),probes(probes),dimension(D),window(w), hash_functions(k),
lsh_metric(m_euclidean), randomizer(k) {
  hyperplane<double>* hp = hyperplane<double>::random_hp(dimension, window);
  for (int j=0; j < k; j++)
    hash_functions[j] = *(hp);
  delete hp;
}

template <class T>
int dolphinn_map<T>::euclidean_hash(point<T>& pt, hyperplane<double>& hp) {

  double sum = 0.0;
  for (int i=0; i < dimension; i++)
    sum += (pt.c[i])*(hp.c[i]);
  sum += hp.t;
  //to int needs to be implemented
  return toInt(sum/window);
}

template <class T>
int dolphinn_map<T>::angular_hash(point<T>& pt, hyperplane<double>& hp) {

  double sum = 0.0;
  for (int i=0; i < dimension; i++)
    sum += (pt.c[i])*(hp.c[i]);
  sum += hp.t;
  //to int needs to be implemented
  return (sum > 0.0)?1:0;
}



template <class T>
void dolphinn_map<T>::insert(point<T>* p) {
  if (lsh_metric == m_euclidean)
    cube[get_key(*p)].push_back(p);
  else
    cube[angular_key(*p)].push_back(p);
}


template <class T>
key_type dolphinn_map<T>::get_key(point<T> &p) {

  int hashval;
  key_type key = 0;
  for (int i=0; i < this->k; i++) {
    hashval = euclidean_hash(p, hash_functions[i]);
    if (randomizer[i].find(hashval) == randomizer[i].end())
      randomizer[i][hashval] = rand()%2;

    key = (key << 1) + randomizer[i][hashval];
  }
  return key;

}

template <class T>
key_type dolphinn_map<T>::angular_key(point<T> &p) {

  int hashval;
  key_type key = 0;
  for (int i=0; i < this->k; i++) {
    hashval = angular_hash(p, hash_functions[i]);


    key = (key << 1) + hashval;
  }
  return key;

}




template <class T>
const point<T>* dolphinn_map<T>::query(point<T>& p) {
  key_type key = (lsh_metric == m_euclidean)?get_key(p):angular_key(p);
  combination_generator cg(this->k, key);
  point<T> *best = nullptr;
  double best_dist = std::numeric_limits<double>::max(),dist;
  int probes = 0;
  int M = 0;
  for (int i=0; i < this->k-1; i++) {

    cg.set_k(i);
    key_type key;
    key = cg.get_next_comb();
    while (1) {
      probes++;

      if (cube.find(key) != cube.end()) {
        std::vector<point<T>*>& v = cube[key];
        for (int j=0; j < v.size(); j++) {

          dist = (lsh_metric == m_euclidean)?euclidean_dist(*v[j], p):
          angular_dist(*v[j], p);
          if (dist < best_dist) {
            best = (v[j]);
            best_dist = dist;
          }
          M++;
          if (M >= this->M) {
            return best;
          }
        }
      }
      if (probes > this->probes)
        return best;
      if (cg.finished())
        break;
      key = cg.get_next_comb();
    }
  }
  return best;

}

template <class T>
point_range<T>* dolphinn_map<T>::query_range(point<T>& p, double radius) {
  key_type key = (lsh_metric == m_euclidean)?get_key(p):angular_key(p);
  combination_generator cg(this->k, key);
  point_range<T>* rv = new point_range<T>();
  double best_dist = std::numeric_limits<double>::max(),dist;
  int probes = 0;
  int M = 0;
  for (int i=0; i < this->k-1; i++) {

    cg.set_k(i);
    key_type key;
    key = cg.get_next_comb();
    while (1) {
      probes++;

      if (cube.find(key) != cube.end()) {
        std::vector<point<T>*>& v = cube[key];
        for (int j=0; j < v.size(); j++) {

          dist = (lsh_metric == m_euclidean)?euclidean_dist(*v[j], p):
          angular_dist(*v[j], p);
          //if distance is less than radius
          if (dist < radius) {
            if (dist < best_dist) {
              best_dist = dist;
              rv->nn = (v[j]);
            }
            rv->pc.insert((v[j]));
          }
          M++;
          if (M >= this->M) {
            return rv;
          }
        }
      }
      if (probes > this->probes)
        return rv;
      if (cg.finished())
        break;
      key = cg.get_next_comb();
    }
  }
  return rv;

}


#endif
