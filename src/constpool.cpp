#include <vector>
#include <unordered_map>
#include <string.h> // For memcpy
#include <stdint.h>
#include <stdio.h>

struct BinaryHash {
  size_t operator()(const std::vector<uint8_t> &key) const {
    size_t result = 0;
    for (uint8_t b : key) {
      result = (result * 31) ^ b;
    }
    return result;
  }
};

struct BinaryEqual {
  bool operator()(const std::vector<uint8_t> &a, const std::vector<uint8_t> & b) const {
    return a == b;
  }
};

class ConstantPool {
  std::unordered_map<std::vector<uint8_t>, int32_t, BinaryHash, BinaryEqual> lookup;
public:
  std::vector<uint8_t> storage;

  template<class T> int32_t addConstant(T value) {
    std::vector<uint8_t> data(sizeof(T));
    memcpy(data.data(), &value, sizeof(T));

    if (lookup.find(data) != lookup.end()) {
      return lookup.at(data);
    }

    int32_t offset = storage.size();
    storage.insert(storage.end(), data.begin(), data.end());
    lookup.insert({data, offset});
    return offset;
  }
};