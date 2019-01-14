#include "parser.h"

#include "dolphinn.h"
#include "lsh.h"

#include "kmeans.h"

#include <iostream>
#include <unordered_set>
#include <unordered_map>

#include <limits>

#include <cstring>
#include <cstdio>
#include <ctime>

#include <jsoncpp/json/json.h>

void swap(point<double>*& a, point<double>*& b) {

  point<double>* tmp = a;
  a = b;
  b = tmp;

}

std::vector<uint64_t>* kmeans_cluster::get_random_set(uint64_t npoints, uint64_t k) {
  // to do this efficiently for bigger ks, we need more memory
  //however, for at less than 1% of the size of the pointset
  //simpling picking K points and repicking whenver an already picked
  //is chosen, works just fine

  //big k
  int rnd;
  std::vector<uint64_t>* result = new std::vector<uint64_t>();
  if (100*k > npoints) {
    uint64_t *idxs = new  uint64_t[npoints];
    for (int i=0; i< npoints; i++)
      idxs[i] = i;
    for (int i=0; i < k; i++) {
      rnd = rand()%(npoints-i)+i;
      result->push_back(idxs[rnd]);
      idxs[rnd] = i;

    }
    delete idxs;
  }
  else {

    std::unordered_set <uint64_t> ps;
    for (int i=0; i < k; i++) {

      rnd = rand()%npoints;

      while (ps.find(rnd) != ps.end()) {
        rnd = rand()%npoints;
      }
      ps.insert(rnd);
      result->push_back(rnd);

    }
  }
  return result;

}


std::pair<int ,double> kmeans_cluster::get_min_dist(std::vector<point<double>*>& centr, int n,
                                    point<double> *p, int ignore = -1) {

  double dist, best = std::numeric_limits<double>::max();
  int besti = -1;
  for (int i=0; i < n; i++) {

    if (i == ignore)
      continue;
    if (info.mtr == m_angular)
      dist = angular_dist(*p, *centr[i]);
    else
      dist = euclidean_dist(*p, *centr[i]);
    if (dist < best) {
      best = dist;
      besti=i;
    }
  }
  return std::pair<int, double>(besti, best);
}

void kmeans_cluster::plusplus() {
  uint64_t npoints = points.size();

  int ran = rand()%npoints;
  centroids.push_back(new point<double>(points[ran]));
  swap(points[ran], points[npoints-1]);

  double dist,prev=0.0;
  double* partials = new double[npoints];
  npoints--;
  for (int i=1; i < info.cluster_K; i++) {
    prev = 0;
    /*compute P[i] */

    for (int j=0; j < npoints; j++) {
      dist = get_min_dist(centroids ,i , points[j]).second;
      partials[j] = dist*dist+prev;
      prev = partials[j];
    }
    //TODO check if this random works properly
    double d = ((double)rand()/RAND_MAX)*partials[npoints-1];
    //std::cout << "s = " << partials[npoints-1] << std::endl;
    for (int j=0; j < npoints; j++) {
      if (d < partials[j]) {
        //std::cout << "i = " << j << std::endl;
        centroids.push_back(new point<double>(points[j+1]));
        swap(points[j+1], points[npoints-1]);
        npoints--;
        break;
      }
    }


  }
  delete[] partials;
}



void kmeans_cluster::simple_init() {

  uint64_t npoints = points.size();
  std::vector<uint64_t>* ranset = get_random_set(npoints, info.cluster_K);

  for (int i=0; i < info.cluster_K; i++) {
    centroids.push_back(new point<double>(points[ranset->at(i)]));
  }
  delete ranset;
}

void kmeans_cluster::pam_update() {

  for (int j=0; j < info.cluster_K; j++) {
    double best_dist = std::numeric_limits<double>::max();
    int best_i;
    point<double> *tmp = centroids[j];
    for (int i=0; i < clusters[j].size(); i++) {
      double sum = 0.0;
      for (int k=0; k < clusters[j].size(); k++) {
        sum += (info.mtr == m_euclidean)?
        euclidean_dist(*clusters[j][i],*clusters[j][k]):
        angular_dist(*clusters[j][i],*clusters[j][k]);
      }
      sum /= clusters[j].size();
      if (sum < best_dist) {
        best_i = i;
        best_dist = sum;
      }


    }
    delete centroids[j];
    centroids[j] = new point<double>(clusters[j][best_i]);
  }
}

//lloyd's assignment
//TODO ,maybe keep some allocated space for clusters
void kmeans_cluster::assign() {

  clear();
  int k;

  for (int i=0; i < points.size(); i++) {
    k = get_min_dist(centroids, info.cluster_K, points[i]).first;
    clusters[k].push_back(points[i]);
  }

}

double compute_min_dist(std::vector<point<double>*> centr, metric m) {

  double best = std::numeric_limits<double>::max();
  for (int i=0; i < centr.size(); i++) {
    for (int j=i+1; j < centr.size(); j++) {

      double dist;
      if (m == m_angular)
        dist = angular_dist(*centr[j], *centr[i]);
      else
        dist = euclidean_dist(*centr[j], *centr[i]);
      if (dist < best) {
        best = dist;
      }
    }

  }
  return best;
}


void kmeans_cluster::lsh_assign() {
  // this is the hard part
  //get the entire non-colidded bucket instead of multiple range searches
  //keep all the elements + their distances
  std::unordered_set<point<double>*>* sets = new std::unordered_set<point<double>*>[info.cluster_K];
  std::unordered_map<point<double>*, double>* dists = new
    std::unordered_map<point<double>*, double>[info.cluster_K];

  for (int i=0; i < info.cluster_K; i++) {
    lmap->smart_range(*centroids[i], sets[i], dists[i]);
    //std::cout << sets[i].size() << std::endl;
  }

  double d0 = compute_min_dist(centroids, info.mtr);
  //maybe layerize sets according to the d0

  std::vector<point<double>*> items_to_delete;
  for (int i=0; i < info.cluster_K; i++) {
    for (int j=i; j < info.cluster_K; j++) {
      for (auto point: sets[i]) {
        if (sets[j].find(point) != sets[j].end()) {
          if (dists[i][point] < dists[j][point])
            sets[j].erase(point);
          else
            items_to_delete.push_back(point);
        }
      }
      for (auto point: items_to_delete)
        sets[i].erase(point);
      items_to_delete.clear();
    }
  }
  /* now the sets are correct*/

  std::unordered_set<point<double>*> final_set;
  clear();

  for (int i=0; i < info.cluster_K; i++) {

    for (auto item: sets[i]) {
      clusters[i].push_back(item);
      final_set.insert(item);
    }

  }
  int k;

  for (int i=0; i < points.size(); i++) {
    if (final_set.find(points[i]) != final_set.end())
      continue;
    k = get_min_dist(centroids, info.cluster_K, points[i]).first;
    clusters[k].push_back(points[i]);
  }
  delete[] sets;
  delete[] dists;
  return ;
}

void kmeans_cluster::lsh_init() {
  lmap = new lsh_map<double>(info.lsh_k, info.lsh_L, dimens,info.w, info.mtr);

  for (int i=0; i < points.size(); i++)
    lmap->insert(points[i]);

}

void kmeans_cluster::hp_init() {
  dmap = new dolphinn_map<double>(info.hyper_k, info.hyper_M, info.hyper_probes,
     dimens,info.mtr);

  for (int i=0; i < points.size(); i++)
    dmap->insert(points[i]);

}

void kmeans_cluster::hp_assign()  {

  int numofiters = 10;
  double d0 = 2*compute_min_dist(centroids, info.mtr);
  //std::cout << d0 << std::endl;

  std::unordered_set<point<double>*>* sets = new std::unordered_set<point<double>*>[info.cluster_K];
  std::unordered_map<point<double>*, double>* dists = new
    std::unordered_map<point<double>*, double>[info.cluster_K];

  std::unordered_set<point<double>*> final_set;
  std::vector<point<double>*> items_to_delete;
  clear();
  for (;numofiters > 0 ;numofiters--) {

    for (int i=0; i < info.cluster_K; i++) {
      dmap->smart_range(*centroids[i], d0, sets[i], dists[i], final_set);
      //std::cout << sets[i].size() << std::endl;
    }
    for (int i=0; i < info.cluster_K; i++) {
      for (int j=i; j < info.cluster_K; j++) {
        for (auto point: sets[i]) {
          if (sets[j].find(point) != sets[j].end()) {
            if (dists[i][point] < dists[j][point])
              sets[j].erase(point);
            else
              items_to_delete.push_back(point);
          }

        }
        for (auto point: items_to_delete)
          sets[i].erase(point);
        items_to_delete.clear();
      }
    }
    for (int i=0; i < info.cluster_K; i++) {

      for (auto item: sets[i]) {
        clusters[i].push_back(item);
      }
      sets[i].clear();
    }
    d0 *= 2;
  }
  int k;

  for (int i=0; i < points.size(); i++) {
    if (final_set.find(points[i]) != final_set.end())
      continue;
    k = get_min_dist(centroids, info.cluster_K, points[i]).first;
    clusters[k].push_back(points[i]);
  }



  delete[] sets;
  delete[] dists;
}


void kmeans_cluster::update_centroids() {

  double total_epsilon = 0.0;

  for (int i=0; i < info.cluster_K; i++) {

    point<double>* sump = new point<double>;
    sump->c.reserve(dimens);
    for (int j=0; j < dimens; j++)
      sump->c.push_back(0.0);

    for (int j=0; j < clusters[i].size(); j++) {
      for (int k=0; k < dimens; k++) {
        sump->c[k] += clusters[i][j]->c[k];
      }
    }
    for (int k=0; k < dimens; k++) {
      sump->c[k] /= clusters[i].size();
    }

    double dist = euclidean_dist(*centroids[i], *sump);
    total_epsilon += dist;
    delete centroids[i];
    centroids[i] = sump;
  }
  epsilon = total_epsilon/info.cluster_K;
}

double kmeans_cluster::silhouette() {
  double s = 0.0;
  for (int i=0; i < info.cluster_K; i++) {
    double s1 = 0.0;
    //std::cout << clusters[i].size() << std::endl;
    for (int j=0; j < clusters[i].size(); j++) {


      //comptue distance with its won centroid
      double a;
      if (info.mtr == m_angular)
        a = angular_dist(*centroids[i], *clusters[i][j]);
      else
        a = euclidean_dist(*centroids[i], *clusters[i][j]);

      //compute the 2nd best distance!
      double b = get_min_dist(centroids, info.cluster_K,
        clusters[i][j], i).second;
      double mx = (a>b)?a:b;
      //std::cout << a << "  " << b << std::endl;
      s += (b-a)/mx;
      s1 += (b-a)/mx;
    }
    printf("%.4lf ", s1/clusters[i].size());
  }
  printf("   %.4lf",s/points.size());
  return s/points.size();;
}


//clears the contents of existing clusters
void kmeans_cluster::clear() {
  for (int i=0; i < info.cluster_K; i++)
    clusters[i].clear();
}

void kmeans_cluster::print_results(int complete, double time) {

  printf("Algorithm: ");

  std::string init;
  std::string update;
  std::string method;

  if (info.in == simple)
    init.assign("Simple");
  else
    init.assign("Plusplus");

  if (info.method == rs_lsh)
    method.assign("RS LSH");
  else if (info.method == rs_hyper)
    method.assign("RS Hypercube");
  else
    method.assign("Lloyds");

  if (info.ud == kmeans)
    update.assign("kmeans");
  else
    update.assign("PAM");

  printf("%s|%s|%s\n",init.c_str(), update.c_str(), method.c_str());

  printf("Metrics: %s\n", (info.mtr == m_euclidean)?("Euclidean"):"Cosine");

  for (int i=0; i < info.cluster_K; i++) {
    printf("CLUSTER-%2d {size: %ld, centroid: ",i, clusters[i].size());
    for (int j=0; j < std::min((int)centroids[i]->c.size(), 6); j++)
      printf("%.3lf ", centroids[i]->c[j]);
    printf("...}\n");
  }
  printf("clustering_time:%lf\n", time);
  printf("Silhouette: ");
  silhouette();
  printf("\n");
  if (complete) {

    for (int i=0; i < info.cluster_K; i++) {
      printf("CLUSTER-%2d: ", i);
      for (int j=0; j < clusters[i].size(); j++)
        printf(" %d",clusters[i][j]->id);
      printf("\n");
    }
  }
}


static kmeans_info parse_config(std::string file) {
  kmeans_info kmi;
  Json::Value root;
  std::ifstream config(file, std::ifstream::binary);
  config >> root;

  kmi.lsh_k = root.get("lsh_k", 4).asInt();
  kmi.lsh_L = root.get("lsh_L", 5).asInt();
  kmi.hyper_k = root.get("hyper_k", 4).asInt();
  kmi.hyper_M = root.get("hyper_M", 4).asInt();
  kmi.hyper_probes = root.get("hyper_probes", 1280).asInt();
  kmi.w = root.get("window", 0.8).asFloat();
  kmi.cluster_K = root.get("KClusters", 10).asInt();
  kmi.maxIter = root.get("maxIter", 10).asInt();
  kmi.eps = root.get("minEpsilon", 10.0).asFloat();

  std::string val = root.get("init", "simple").asString();
  if (val == "simple")
    kmi.in = simple;
  if (val == "plusplus")
    kmi.in = plusplus;

  val = root.get("assignment", "lloyds").asString();
  if (val == "lloyds")
    kmi.method = lloyds;
  if (val == "rs_lsh")
    kmi.method = rs_lsh;
  if (val == "rs_hyper")
    kmi.method = rs_hyper;

  val = root.get("update", "kmeans").asString();
  if (val == "kmeans")
    kmi.ud = kmeans;
  if (val == "PAM")
    kmi.ud = PAM;

  config.close();
  return kmi;
}

void kmeans_cluster::flush_clusters() {
  char buff[128];
  for (int k=0; k < info.cluster_K; k++) {
    sprintf(buff, "results/results%d.txt", k);
    FILE *f = fopen(buff, "w");
    for (int i=0; i < clusters[k].size(); i++) {
      for (int j=0; j < clusters[k][i]->c.size(); j++)
        fprintf(f, "%lf ", clusters[k][i]->c[j]);
      fprintf(f, "\n");
      }
    fclose(f);
  }
}

#ifdef __UNIT_MAIN_KMEANS__

int main(int argc, char *argv[]) {
  using namespace std;
  metric cm = m_euclidean;
  string infile(""), config(""), outfile("out.txt");
  int formatted_out = 0;

  for (int i=1; i < argc; i++) {
    if (!strcmp(argv[i], "-i"))
      infile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-o"))
      outfile.assign(argv[i+1]);
    if (!strcmp(argv[i], "-c"))
      config.assign(argv[i+1]);
    if (!strcmp(argv[i], "--complete"))
      formatted_out = 1;
    if (!strcmp(argv[i], "-d")) {
      if (!strcmp(argv[i+1], "angular"))
        cm = m_angular;
      else if (!strcmp(argv[i+1], "euclidean"))
        cm = m_euclidean;
    }

  }



  inp_parser ip;
  ip.open(infile.c_str());
  ip.preparse();

  std::vector<point<double>*>& points = ip.container();

  kmeans_info kmi = parse_config(config);
  kmi.mtr = cm;
  /*
  kmi.cluster_K = 10;
  kmi.lsh_k = 4;
  kmi.lsh_L = 5;
  kmi.w = 0.8;
  kmi.hyper_k = (int)log2(ip.get_size())+1;
  kmi.hyper_M = ip.get_size()/(kmi.cluster_K*10);
  kmi.hyper_probes = 1280;


  kmi.hyper_k += (int)log2(ip.get_size());
  std::cout << "INFO:" << std::endl;
  std::cout << points[0]->c.size() << endl;
  std::cout << kmi.w << std::endl;
  std::cout << kmi.lsh_k << std::endl;
  std::cout << kmi.lsh_L << std::endl;
  std::cout << kmi.cluster_K << std::endl;
  std::cout << kmi.cluster_K << std::endl;
  std::cout << kmi.hyper_M << std::endl;
  std::cout << kmi.hyper_probes << std::endl;

  std::cout << kmi.ud << std::endl;
  std::cout << "STARTING:" << std::endl;
  */

  int st = clock();

  kmeans_cluster clusterer(kmi, points);

  if (kmi.in == simple)
    clusterer.simple_init();
  else
    clusterer.plusplus();

  if (kmi.method == rs_lsh)
    clusterer.lsh_init();
  else if (kmi.method == rs_hyper)
    clusterer.hp_init();


  for (int i=0; i < kmi.maxIter; i++) {
    if (kmi.method == rs_lsh)
      clusterer.lsh_assign();
    else if (kmi.method == rs_hyper)
      clusterer.hp_assign();
    else
      clusterer.assign();

    if (i == kmi.maxIter)
      break;

    if (kmi.ud == PAM)
      clusterer.pam_update();
    else
      clusterer.update_centroids();

    //IF THE difference is too little stop
    if (clusterer.get_epsilon() < kmi.eps)
      break;

  }
  int endtime= clock();

  clusterer.print_results(formatted_out,((double)endtime-st)/CLOCKS_PER_SEC);

  //clusterer.flush_clusters();
  //clean up
  for (int i=0; i < points.size(); i++)
    delete points[i];
}

#endif
