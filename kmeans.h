#include "parser.h"

#include "dolphinn.h"
#include "lsh.h"


enum update {kmeans, PAM};
enum assign {lloyds, rs_lsh, rs_hyper};
enum init {simple, plusplus};


struct kmeans_info{
  int lsh_k,lsh_L;
  int hyper_k,hyper_M, hyper_probes;
  update ud;
  assign method;
  init   in;

  double w;
  int cluster_K;
  int maxIter;

  metric mtr;
  double eps;
};


class kmeans_cluster {

  public:
    ~kmeans_cluster() {
      for (int i=0; i < centroids.size(); i++)
        delete centroids[i];
      if (lmap != nullptr)
        delete lmap;
      if (dmap != nullptr)
        delete dmap;
    }
    kmeans_cluster(const kmeans_info& km, std::vector<point<double>*>& pts):
    info(km),points(pts),clusters(info.cluster_K),lmap(NULL),dmap(NULL){
      centroids.reserve(info.cluster_K);
      dimens = pts[0]->c.size();
    }

    double silhouette();
    void flush_clusters();

    std::pair<int ,double> get_min_dist(std::vector<point<double>*>& , int,
                                        point<double>*, int) ;
    //void cluster();

  const std::vector<std::vector<point<double>*>>& results() {
    return clusters;
  }

  double get_epsilon() {
    return epsilon;
  }

  void print_results(int ,double);

  private:
    void clear();
    std::vector<point<double>*>& points;
    kmeans_info info;
    std::vector<std::vector<point<double>*>> clusters;
    std::vector<point<double>*> centroids;



    int dimens;
    lsh_map<double>* lmap;
    dolphinn_map<double>* dmap;

    double epsilon;


    // INIT FUNCTIONS
  public:
    void simple_init();
      std::vector<uint64_t>* get_random_set(uint64_t, uint64_t);
    void plusplus();
    void assign();
    void lsh_assign();
      void lsh_init();
    void hp_assign();
      void hp_init();
    void update_centroids();
    void pam_update();


};


kmeans_info parse_config(std::string file);