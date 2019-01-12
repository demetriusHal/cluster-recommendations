#include "dolphinn.h"



#include <limits>
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstdio>

#include <ctime>





void combination_generator::reset(void) {

  //empty the stack
  while (!selection_stack.empty())
    selection_stack.pop();

  current = num;
  for (int i=0; i < k ; i++) {
    selection_stack.push(i);
    current ^= (1 << i);
  }

  finish = false;

}

key_type combination_generator::get_next_comb(void) {

  if (finish)
    return 0;

  key_type rv = current;

  int cn = n-1;
  for(;!selection_stack.empty();) {

    if (selection_stack.top() == cn)
      selection_stack.pop();
    else
      break;
    cn--;
  }

  if (selection_stack.empty()) {
    finish = true;
    return rv;
  }

  int top = selection_stack.top()+1;
  selection_stack.pop();
  selection_stack.push(top);

  key_type mask = ((uint64_t)1 << (top-1))-1;
  current &= mask;
  current |= (num >> (top-1)) << (top-1) ;

  current ^= ((uint64_t)1 << top);

  for (int i=top+1; selection_stack.size() != k; i++) {
    selection_stack.push(i);
    current ^= ((uint64_t)1 << i);
  }
  return rv;
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
void dolphinn_map<double>::smart_range(point<double>& p, double radius,
  std::unordered_set<point<double>*>& set,
std::unordered_map<point<double>*, double>& dists,
std::unordered_set<point<double>*>& fullset) {
  key_type key = (lsh_metric == m_euclidean)?get_key(p):angular_key(p);
  combination_generator cg(this->k, key);

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
        std::vector<point<double>*>& v = cube[key];
        for (int j=0; j < v.size(); j++) {
          if (fullset.find(v[j]) != fullset.end())
            continue;
          if (dists.find(v[j]) == dists.end()) {
            dist = (lsh_metric == m_euclidean)?euclidean_dist(*v[j], p):
            angular_dist(*v[j], p);
            dists[v[j]] = dist;
          }
          else
            dist = dists[v[j]];

          //if distance is less than radius
          if (dist < radius)
            set.insert(v[j]);

          M++;
          if (M >= this->M) {
            return ;
          }
        }
      }
    }
    if (probes > this->probes)
      return ;
    if (cg.finished())
      break;
    key = cg.get_next_comb();
  }
}




#include "parser.h"


#ifdef __UNIT_MAIN_DOLPHINN__

int main(int argc, char *argv[]) {

  using namespace std;
  double radius = 500.0;
  int arg_k,arg_M,arg_probes;
  arg_probes  = arg_M = -1;
  arg_k = 0;
  string infile(""), queryfile(""), outfile("out.txt");
  int formatted_out = 0;
  FILE *fp = nullptr;

  for (int i=0; i < argc; i++) {
    if (!strcmp(argv[i], "-d"))
      infile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-o"))
      outfile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-q"))
      queryfile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-k"))
      arg_k = atoi(argv[i+1]);
    if (!strcmp(argv[i], "-M"))
      arg_M = atoi(argv[i+1]);
    if (!strcmp(argv[i], "-probes"))
      arg_probes = atoi(argv[i+1]);
    if (!strcmp(argv[i], "--formatted-out"))
      formatted_out = 1;

  }
  if (arg_probes == -1 || arg_k > 100 || arg_M == -1 || !infile.compare("")
  || !queryfile.compare("")) {
    throw "Bad arguments exception!";
  }

  if (formatted_out)
    fp = fopen(outfile.c_str(), "w");

  inp_parser ip;

  ip.open(infile.c_str());
  ip.preparse();
  dolphinn_map<double> lm((int)log2(ip.get_size())+arg_k, arg_M, arg_probes,
  ip.get_dimension(), 400.0);
  point<double>* item = nullptr;
  int i = 0;
  while ((item = ip.next_item()) != nullptr) {

    lm.insert(item);
  }

  std::vector<point<double>*> old_points = ip.container();


  ip.close();
  //open the query file
  ip.open(queryfile.c_str());
  ip.preparse();

  long double avg_dist = 0.0;
  double dist;
  int c = 0;
  /*data concerning statistics*/
  int stime;
  double timeHash;
  double timeEx;
  double lerror = -1.0;
  double ttime = 0.0;
  double actual_d;
  double est_d;
  point_range<double>* pr;

  const point<double> *p;
  item = nullptr;
  while((item = ip.next_item()) != nullptr) {
    c++;
    if (formatted_out) {

      fprintf(fp, "Query: %d\n", c);
      fprintf(fp, "R-near neighbors:\n");
      stime = clock();
      pr = lm.query_range(*item, radius);
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

      fprintf(fp, "Nearest neighbor: %d\n", p->id);
      if (pr->pc.size() > 0)
        fprintf(fp, "DistanceHP: %lf\n", euclidean_dist(*item, *(pr->nn)));
      fprintf(fp, "DistanceActual: %lf\n", euclidean_dist(*p, *item));
      fprintf(fp, "tHP: %lf\n", timeHash);
      fprintf(fp, "tTrue: %lf\n", timeEx);

      delete item;
      delete pr;
      continue;
    } // else just print normal distances

    p = lm.query(*item);
    if (p == nullptr) {
      std::cout << "no point found " << std::endl;
    }
    else {
      dist = euclidean_dist(*p, *item);
      std::cout << "Found p with distance :" << dist << std::endl;
      avg_dist += dist;
    }
    delete item;
  }
  std::cout << " Avg dist :" << avg_dist/c<< std::endl;
  ip.close();
  if (fp != nullptr)
    fclose(fp);

  for (int i=0; i < old_points.size(); i++)
    delete old_points[i];

  cout << "Largest error : " << lerror << endl;
  cout << "Avg time : " << ttime/c << endl;
}

#endif

#ifdef __UNIT_MAIN_1__
int main(void) {
  key_type bitstring = 3942323;
  combination_generator cg(5, bitstring);
  for (int i=2; i < 4; i++) {

    cg.set_k(i);
    key_type key = cg.get_next_comb();
    while (1) {

        printf("= %ld\n", key);
        for (int j=0; j < 5; j++) {
          printf("%ld", (key >> (4-j))&1);
        }
        printf("\n");
        if (cg.finished())
          break;
        key = cg.get_next_comb();
    }
  }

}

#endif
