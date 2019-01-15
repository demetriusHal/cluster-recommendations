#ifndef __PARSER_H__
#define __PARSER_H__

#include <fstream>
#include <string>
#include <vector>

#include <iostream>

// this class works like a file
//it will stream the data read from a file


template <class T>
struct point {
  std::vector<T> c;
  uint64_t id;

  point(point<T> *p):id(p->id) {
    for (int i=0; i < p->c.size(); i++)
      c.push_back(p->c[i]);
  }
  point() :id(0){

  }
  point(uint64_t nid,std::vector<T>& v):c(v), id(nid){
    ;
  }
  
};

template <class T>
bool operator<(const point<T>& lhs, const point<T>& rhs) 
{return false;}

class inp_parser {

  public:
  inp_parser();



  void open(std::string) throw();
  void close();
  void preparse() throw();   //parse everything at once
                             //don't waste time pasring during execution

  struct point<double>* next_item();

  int get_dimension(void) {
    return dimension;
  }

  int get_size(void) {
    return parsed.size();
  }

  std::vector<point<double>*>& container() {
    return parsed;
  }

  private:

  std::ifstream file;
  bool ended;
  bool open_file;
  bool is_parsed;
  int dimension;
  int pid;


  int parsed_idx;

  point<double>* element;
  std::vector<point<double>*> parsed;


};

#endif
