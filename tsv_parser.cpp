#include <fstream>

#include "tsv_parser.h"


void tsv_parser::parse_file(std::string filename) {
  std::ifstream newfile;

  newfile.open(filename);

  std::string line;
  std::string item;
  data = new std::vector<std::vector<std::string>>();
  int i = 0;
  while (newfile.good()) {
    
    std::getline(newfile, line, '\n');
    if (line.size() == 0)
      break;

    data->push_back({});

    size_t prev = 0;
    size_t idx;
    while ((idx = line.find('\t', prev)) != -1) {  
      data->at(i).push_back(line.substr(prev,idx-prev));
      prev = idx+1;
    }
    data->at(i).push_back(line.substr(prev,line.length()-prev-1));
    i++;
  }
  newfile.close();
  
}

#ifdef __UNIT_MAIN__

#include <iostream>

using namespace std;

int main() {

  tsv_parser tp;
  tp.parse_file("coins_queries.csv");
  auto data = tp.get_data();

  for (auto row : *data) {
    for (auto item: row) {

      cout << "`"<<item <<"`" << " " ;
    }
    cout << endl;
  }
  tp.destroy_data();
}

#endif

