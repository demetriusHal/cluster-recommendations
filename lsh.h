#ifndef __LSH_H__
#define __LSH_H__

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <cmath>
#include <climits>
#include <limits>

#include "parser.h"

//header file for LSH class
// templated for any type (int/double etc)

template <class T>
T uniform(T , T);
template <class T>
T normal();
template <class T>
int toInt(T);

template <class T>
struct mapped_info {
  point<T> *ptr;
  std::vector<T> *gs;
  mapped_info(point<T> *p, std::vector<T> *v):gs(v),ptr(p) {
    ;
  }
};

template <class T>
struct point_range {
  std::unordered_set<point<T>*> pc;
  point<T>* nn;
};


enum metric {m_euclidean, m_angular};

double euclidean_dist(const point<double>&, point<double>& );
double angular_dist(const point<double>&, point<double>& );


template <class T>
struct hyperplane {
  std::vector<T> c;
  T t;

  static hyperplane* random_hp(int dim, double window) {

    hyperplane* h = new hyperplane;
    double rd;
    for (int i=0; i < dim; i++) {
      /*create random hyperplane in N(0,1) using marsaglia's formula*/

      for(;;) {
        double u = (((double)rand())/RAND_MAX)*2-1.0;
        double v = (((double)rand())/RAND_MAX)*2-1.0;
        double s = u*u+v*v;
        if (s < 1.0) {
          rd = u*sqrt(-2*log(s)/s);
          break;
        }
      }

      h->c.push_back(T(rd));
      h->t = T(uniform(0.0, window));
    }

    return h;
  }

};


template <class T>
class lsh_map {

  public:

    lsh_map(int k, int L, int d, double w =4.0, metric=m_euclidean);
    ~lsh_map();


    void insert(point<T>*);
    const point<T>* lookup(point<T>&, double);
    point_range<T>* lookup_range(point<T>&, double);

    uint64_t memory_size();

    uint64_t tot_points;

    void smart_range(point<double>& centr,
      std::unordered_set<point<double>*>& set,
    std::unordered_map<point<double>*, double>& dists);

  private:
    //k hash function for g
    //L gs to look for neighbours
    // i respresent G as a map of vectors
    const int lsh_k;
    const int lsh_L;

    //hashing info
    const int dimension;
    const double window;
    const metric lsh_metric;

    std::vector<std::unordered_map<int64_t, std::vector<mapped_info<T>>>>
     stored_map;
    std::vector<std::vector<hyperplane<double>>> lsh_functions;
    //maybe changed that to T
    std::vector<int64_t> lvector;

    void init();
    void generate_new_functions();
    void check_new_functions();
    std::vector<T>* hash_point(point<T>& , int);
    int euclidean_hash(point<T>& , hyperplane<double>& );
    int64_t gen_key(std::vector<T>*);

    //anglular api functions
    //can include ifdef to not be included in the executable
    int angular_hash(point<T>&, hyperplane<double>& );
    std::vector<T>* angular_hash_point(point<T>& , int);



};

template <class T>
T uniform(T low, T high) {
  return (rand()/RAND_MAX)*high+low;
}
template <class T>
int toInt(T x) {
  return (int)x;
}


template <class T>
lsh_map<T>::lsh_map(int k, int L, int d, double w, metric m):lsh_k(k), lsh_L(L),
dimension(d), window(w), stored_map(L), lsh_functions(L), lsh_metric(m)  {
  init();
  generate_new_functions();
  if (lsh_metric == m_euclidean)
    for (int i=0; i < lsh_k; i++)
      lvector.push_back(rand()%(1 << lsh_k));
  else
    for (int i=0; i < lsh_k; i++)
      lvector.push_back(((1 << i)));

  tot_points = -1;
};

template <class T>
void lsh_map<T>::check_new_functions() {
  for (int i=0; i < lsh_L; i++) {
     for(int j=0; j < lsh_k; j++) {
       for (int k=0; k < dimension; k++)
        std::cout << lsh_functions[i][j].c[k] << " ";
       std::cout << std::endl;
     }
   }
}

template <class T>
lsh_map<T>::~lsh_map() {

  //clear up the used up memory
  /*
  for (auto& it: stored_map[0]) {
    for (int i=0; i < it.second.size(); i++) {
      delete it.second[i].ptr;
    }
  }
  */
  for (int i=0; i < stored_map.size(); i++)
    for (auto& it: stored_map[i]) {
      for (int i=0; i < it.second.size(); i++) {
        delete it.second[i].gs;
      }
    }


}

template <class T>
void lsh_map<T>::insert(point<T>* pt) {
  //insert a point in the lsh map

  std::vector<T>* hash_val;

  for (int i =0; i < lsh_L; i++) {
    //for every LSH function g, map
    std::unordered_map<int64_t, std::vector<mapped_info<T>>>& G
     = this->stored_map[i];

    if (lsh_metric == m_euclidean)
      hash_val = hash_point(*pt, i);
    else
      hash_val = angular_hash_point(*pt, i);
    G[gen_key(hash_val)].push_back(mapped_info<T>(pt, hash_val));

  }


}


//generate a vector which is the concatenation of the
//hash results of a point
template <class T>
std::vector<T>* lsh_map<T>::hash_point(point<T>& pt, int Gi) {


  std::vector<T> *gv = new std::vector<T>(lsh_k);
  std::vector<hyperplane<double>>& hs = lsh_functions[Gi];
  for (int i=0; i < lsh_k; i++)
    gv->operator[](i) = (euclidean_hash(pt, hs[i]));

  return gv;
}

template <class T>
std::vector<T>* lsh_map<T>::angular_hash_point(point<T>& pt, int Gi) {


  std::vector<T> *gv = new std::vector<T>(lsh_k);
  std::vector<hyperplane<double>>& hs = lsh_functions[Gi];
  for (int i=0; i < lsh_k; i++)
    gv->operator[](i) = (angular_hash(pt, hs[i]));

  return gv;
}





//TODO make this work with some random r ...
template <class T>
int64_t lsh_map<T>::gen_key(std::vector<T>* gv) {
  int64_t rv = 0;
  for (int i=0; i < lsh_k; i++)
    rv += gv->at(i)*lvector[i];
}

template <class T>
int lsh_map<T>::euclidean_hash(point<T>& pt, hyperplane<double>& hp) {

  double sum = 0.0;  //ought to be 0
  for (int i=0; i < dimension; i++)
    sum += (pt.c[i])*(hp.c[i]);
  sum += hp.t;
  //to int needs to be implemented
  return toInt(sum/window);
}

template <class T>
int lsh_map<T>::angular_hash(point<T>& pt, hyperplane<double>& hp) {

  double sum = 0.0;
  for (int i=0; i < dimension; i++)
    sum += (pt.c[i])*(hp.c[i]);

  //to int needs to be implemented
  return (sum > 0.0)?1:0;
}



template <class T>
void lsh_map<T>::init() {
  lsh_functions.reserve(lsh_L);
  for (int i=0; i < lsh_L; i++) {
    ;//lsh_functions[i].reserve(lsh_k);
  }
}


template <class T>
void lsh_map<T>::generate_new_functions() {

  hyperplane<double>* hp;
  for (int i=0; i < lsh_L; i++) {
    for (int j=0; j < lsh_k; j++) {
      hp = hyperplane<double>::random_hp(dimension, window);
      lsh_functions[i].push_back(*(hp));
      delete hp;
    }
  }

}

template <class T>
point_range<T>* lsh_map<T>::lookup_range(point<T>& p, double radius) {

  std::vector<T>* hash_value;
  point<T>* cp;
  point_range<T>* rv = new point_range<T>();
  double best_dist = std::numeric_limits<double>::max();

  for (int i=0; i < lsh_L; i++) {
    if (lsh_metric == m_euclidean)
      hash_value = hash_point(p, i);
    else
      hash_value = angular_hash_point(p, i);

    std::vector<mapped_info<T>>& v = this->stored_map[i][gen_key(hash_value)];
    for (int j=0; j < v.size(); j++) {
      if (*(v[j].gs) != *(hash_value))
        continue;
      cp = v[j].ptr;
      double dist  = (lsh_metric == m_euclidean)?euclidean_dist(p, *cp):
        angular_dist(p, *cp);

      //if distance is less than radius
      if (dist < radius) {
        if (dist < best_dist) {
          best_dist = dist;
          rv->nn = cp;
        }
        rv->pc.insert(cp);
      }

    }
    delete hash_value;
  }
  return rv;
}

template <class T>
const point<T>* lsh_map<T>::lookup(point<T>& p, double radius) {

  std::vector<T>* hash_value;
  point<T>* cp;
  point<T>* best_p = nullptr;

  double best_dist = std::numeric_limits<double>::max();

  for (int i=0; i < lsh_L; i++) {
    if (lsh_metric == m_euclidean)
      hash_value = hash_point(p, i);
    else
      hash_value = angular_hash_point(p, i);

    std::vector<mapped_info<T>>& v = this->stored_map[i][gen_key(hash_value)];
    for (int j=0; j < v.size(); j++) {
      if (*(v[j].gs) != *(hash_value))
        continue;
      cp = v[j].ptr;
      double dist  = (lsh_metric == m_euclidean)?euclidean_dist(p, *cp):
        angular_dist(p, *cp);

      if (dist < best_dist && dist <= radius) {
        best_dist = dist;
        best_p = cp;
      }
    }
    delete hash_value;
  }
  return best_p;
}

template <class T>
uint64_t lsh_map<T>::memory_size() {

  uint64_t sum = 0;
  //to be precise we have to check all the keys insertions
  // we can also calculate the approximate memory requirement by a simple
  //multiplication
  for (int i=0; i < stored_map.size(); i++)
    for (auto& it: stored_map[i]) {
      sum += 8;
      sum += it.second.size()*(16+lsh_k*4);
    }
    sum += tot_points*(sizeof(std::vector<T>)+dimension*sizeof(int));
    return sum;
}









//returns the entire bucket alongside with their respective ranges




#endif
