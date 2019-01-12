#include "parser.h"


#include <sstream>
#include <iostream>
#include <algorithm>

inp_parser::inp_parser(void): open_file(false),pid(0) {


}

void inp_parser::open(std::string filename) throw() {

  if (open_file)
    return;


  file.open(filename);
  open_file = true;
  ended = false;
  is_parsed = false;
  parsed.clear();
  parsed_idx = -1;
  pid = 1;
  // get the next element and guess the dimension

  std::string line;
  std::getline(file,line);
  std::replace(line.begin(), line.end(), ',', ' ');
  if (line.compare("") == 0)
    throw "Empty file!";

  element = new point<double>();
  std::stringstream sstream(line);

  double n;
  int toss;
  sstream >> toss;
  for (;;) {
    sstream >> n;
    if (!sstream)
      break;
    element->c.push_back(n);
  }
  element->id = pid++;
  dimension = element->c.size();

  return;
}

void inp_parser::preparse(void) throw() {

  if (!open_file)
    throw "no file opened to preparse!";

  if (is_parsed)
    return ;
  if (ended)
    return;

  is_parsed = true;
  parsed.push_back(element);
  std::string line;
  double n;

  point<double> *item;
  for (;;) {
    std::getline(file,line);
    std::replace(line.begin(), line.end(), ',', ' ');
    if (line.compare("") == 0)
      break;
    item = new point<double>();
    std::stringstream sstream(line);
    int toss;
    sstream >> toss;
    for (int i=0; i < dimension; i++) {
      sstream >> n;
      item->c.push_back(n);
    }
    item->id = pid++;
    parsed.push_back(item);
  }

  parsed_idx = 0;
}

struct point<double>* inp_parser::next_item(void) {
  if (is_parsed) {
    if (ended == true)
      return nullptr;
    parsed_idx++;
    if (parsed_idx == parsed.size())
      ended = true;
    return parsed[parsed_idx-1];
  }

  if (!open_file)
    return nullptr;
  if (ended)
    return nullptr;

  point<double>* rv = this->element;

  std::string line;
  std::getline(file,line);
  std::replace(line.begin(), line.end(), ',', ' ');
  if (line.compare("") == 0) {
    ended = true;
    return rv;
  }

  double n;
  std::stringstream sstream(line);
  //TODO consider adding a constuctor on struct point with
  //reserved number of spots to make sure
  element = new point<double>();
  int toss;
  sstream >> toss;
  for (int i=0; i < dimension; i++) {
    sstream >> n;
    element->c.push_back(n);
  }

  element->id = pid++;
  return rv;
}

void inp_parser::close(void) {
  if (open_file)
    file.close();
  open_file = false;
  return ;
}




#ifdef _UNIT_DEBUG_
int main(void) {

  using namespace std;
  inp_parser ip;

  ip.open("twitter_dataset_small_v2.csv");
  ip.preparse();

  point<double>* item;
  int i = 0;
  while ((item = ip.next_item()) != nullptr) {

//    cout << item->c.size() << ":";
    for (int i=0;i < item->c.size()-1; i++)
      cout << item->c[i] << " ";
    cout << item->c[item->c.size()-1] << endl;

  }
  ip.close();
}
#endif
