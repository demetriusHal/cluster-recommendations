#include "lsh.h"

#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include <cstdlib>
#include <ctime>

#include <limits>



double euclidean_dist(const point<double>& p1, point<double>& p2) {

  double s = 0;
  for (int i=0; i < p1.c.size(); i++)
    s += (p1.c[i]-p2.c[i])*(p1.c[i]-p2.c[i]);

  return sqrt(s);
}

double angular_dist(const point<double>& p1, point<double>& p2) {
  double s = 0;
  //note the the correct way to do this is to store all the
  // r1,r2 and not compute them each time
  double r1 = 0,r2 = 0;
  for (int i=0; i < p1.c.size(); i++) {
    s += p1.c[i]*p2.c[i];
    r1 += p1.c[i]*p1.c[i];
    r2 += p2.c[i]*p2.c[i];
  }
  s = s/sqrt(r1*r2);
  if (r1 == 0 || r2 == 0)
    return 1.0;
  return 1.0-s;
}

static point<double>* exhaustive_search(
  std::vector<point<double>*>& list,point<double>& item) {

  double best_dist = std::numeric_limits<double>::max(),dist;
  point<double>* best_p;
  for (int i=0; i < list.size(); i++) {
    dist = euclidean_dist(item, *list[i]);
    if (dist < best_dist) {
      best_dist = dist;
      best_p = (list[i]);
    }

  }
  return best_p;
}

template <>
void
  lsh_map<double>::smart_range(point<double>& p,
    std::unordered_set<point<double>*>& set,
  std::unordered_map<point<double>*, double>& dists) {

  point<double>* cp;
  std::vector<double>* hash_value;
  for (int i=0; i < lsh_L; i++) {
    if (lsh_metric == m_euclidean)
      hash_value = hash_point(p, i);
    else
      hash_value = angular_hash_point(p, i);

    std::vector<mapped_info<double>>& v = this->stored_map[i][gen_key(hash_value)];
    for (int j=0; j < v.size(); j++) {
      if (*(v[j].gs) != *(hash_value))
        continue;
      cp = v[j].ptr;
      double dist  = (lsh_metric == m_euclidean)?euclidean_dist(p, *cp):
        angular_dist(p, *cp);

      dists[cp] = dist;
      set.insert(cp);


    }
    delete hash_value;
  }
}





#include "parser.h"

#ifdef __UNIT_MAIN_LSH__

int main(int argc, char *argv[]) {

  using namespace std;
  double radius = 500.0;
  int arg_k,arg_L;
  arg_k = arg_L = -1;
  string infile(""), queryfile(""), outfile("out.txt");
  int formatted_out = 0;

  for (int i=1; i < argc; i++) {
    if (!strcmp(argv[i], "-d"))
      infile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-o"))
      outfile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-q"))
      queryfile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-k"))
      arg_k = atoi(argv[i+1]);
    if (!strcmp(argv[i], "-L"))
      arg_L = atoi(argv[i+1]);
    if (!strcmp(argv[i], "--formatted-out"))
      formatted_out = 1;
  }

  if (arg_k == -1 || arg_L == -1 || !infile.compare("")
  || !queryfile.compare("")) {
    throw "Bad arguments exception!";
  }

  inp_parser ip;

  ip.open(infile.c_str());
  ip.preparse();
  lsh_map<double> lm(arg_k, arg_L, ip.get_dimension(), 400, m_euclidean);
  point<double>* item = nullptr;
  int i = 0;


  //insert points;
  lm.tot_points = ip.get_size();
  while ((item = ip.next_item()) != nullptr) {


    lm.insert(item);
  }

  std::vector<point<double>*> old_points = ip.container();
  ip.close();

  ip.open(queryfile.c_str());
  ip.preparse();

  /*data concerning statistics*/
  int stime;
  double timeHash;
  double timeEx;
  double lerror = -1.0;
  double ttime = 0.0;
  double actual_d;
  double est_d;
  int c = 0;
  FILE *fp = fopen(outfile.c_str(), "w");
  point_range<double>* pr;


  const point<double> *p;
  item = nullptr;
  while((item = ip.next_item()) != nullptr) {
    c++;
    if (formatted_out) {

      fprintf(fp, "Query: %d\n", c);
      fprintf(fp, "R-near neighbors:\n");
      stime = clock();
      pr = lm.lookup_range(*item, radius);
      timeHash = ((double)(clock()-stime))/CLOCKS_PER_SEC;

      for (auto& item: pr->pc)
        fprintf(fp, "%d\n", item->id);
      stime = clock();
      p = exhaustive_search(old_points, *item);
      timeEx = ((double)(clock()-stime))/CLOCKS_PER_SEC;
      ttime += timeHash;
      actual_d = euclidean_dist(*p, *item);
      if (pr->pc.size() > 0) {
        est_d = euclidean_dist(*item, *(pr->nn));

        if (est_d/actual_d > lerror) {
          lerror = est_d/actual_d;
        }
      }
      actual_d = euclidean_dist(*p, *item);
      fprintf(fp, "Nearest neighbor: %d\n", p->id);
      if (pr->pc.size() > 0)
        fprintf(fp, "DistanceLSH: %lf\n", est_d);
      fprintf(fp, "DistanceActual: %lf\n", actual_d);
      fprintf(fp, "tLSH: %lf\n", timeHash);
      fprintf(fp, "tTrue: %lf\n", timeEx);

      delete item;
      delete pr;
      continue;
    } // else just print normal distances

    p = lm.lookup(*item, 200.0);
    if (p == nullptr) {
      std::cout << "no point found " << std::endl;
    }
    else {
      std::cout << "Found p with distance :" << euclidean_dist(*p, *item) << std::endl;
    }
    delete item;
  }
  std::cout << "Used :" << lm.memory_size() << endl;
  fclose(fp);
  ip.close();

  cout << "Largest error : " << lerror << endl;
  cout << "Avg time : " << ttime/c << endl;
}

#endif

//unit main 1
#ifdef __UNIT_MAIN_1__
int main(void) {
  using namespace std;
  inp_parser ip;
  ip.open("siftsmall/input_small");
  ip.preparse();


  point<int>* item = nullptr;

  lsh_map<int> lm(4,5,4);
  for (int j=0; j < 5; j++) {
    item = ip.next_item();



  vector<int>* v;

  v = lm.hash_point(*item, 1);
  cout << v->size() << endl;

  for (int i=0; i < v->size(); i++)
    cout << v->at(i) << " ";
  cout << endl;
  }
}

#endif
