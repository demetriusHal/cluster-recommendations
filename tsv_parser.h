#ifndef __TSV_PARSER_H__
//minimalistic tsv parser for quick use

#include <string>
#include <vector>

class tsv_parser {

  public:
  void parse_file(std::string);
  std::vector<std::vector<std::string>>* get_data(void) {
    return data;
  }

  void  destroy_data() {
    delete data;
    data = nullptr;
  }

  ~tsv_parser() {
    if (data != nullptr)
      delete data;
  }
  private:
  std::vector<std::vector<std::string>>* data;
  





};

#endif