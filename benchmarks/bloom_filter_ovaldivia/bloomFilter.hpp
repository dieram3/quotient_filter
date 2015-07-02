#include <vector>
#include <cmath>

static const size_t seed = time(NULL);
static const uint primes[20] = {
    3079,     6151,      12289,     24593,     49157,     98317,     196613,
    393241,   786433,    1572869,   3145739,   6291469,   12582917,  25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741};

class bloomFilter {
private:
  std::vector<bool> bloom;
  uint keys;
  double err;

  // http://stackoverflow.com/questions/7222143/unordered-map-hash-function-c
  // Boost hashing method.
  size_t key(const std::pair<uint, uint> &elem) {
    std::hash<uint> hasher;
    size_t _seed = seed;
    _seed ^= hasher(elem.first) + 0x9e3779b9 + (_seed << 6) + (_seed >> 2);
    _seed <<= 31;
    _seed ^= hasher(elem.second) + 0x9e3779b9 + (_seed << 6) + (_seed >> 2);
    return _seed;
  }

public:
  bloomFilter(size_t elements, double _err = 0.001) {
    err = _err;
    size_t size = -(elements * log(err) / 0.48045301391); // ln(2)^2

    // We only use 80% of the calculated size to store the data.
    size *= 0.80;
    // comment this line if the error rate is really important, this will
    // provide lower error rate's
    // at the cost of not saving 20% of the storage space.

    bloom.resize(size);
    for (size_t ix = 0; ix < size; ix++)
      bloom[ix] = false;

    keys = size / elements * 0.69314718056; // ln(2)
    if (keys > 20)
      keys = 20;
  }

  ~bloomFilter() { bloom.clear(); }

  // It checks if the element is in the array, if it's not, the element is
  // added.
  bool operator[](const std::pair<uint, uint> &elem) {
    bool check = true;
    size_t khash = key(elem);
    for (size_t ix = 0; ix < keys; ix++) {
      size_t k = (khash ^ primes[ix]) % bloom.size();
      check &= bloom[k];
      bloom[k] = true;
    }

    return check;
  }

  size_t size() { return bloom.size(); }

  double error() { return err; }

  uint functions() { return keys; }

  void info() {
    std::cout << "Number of hash functions : " << functions() << "\n";
    std::cout << "Size (number of bits)    : " << size() << "\n";
    std::cout << "Expected error rate      : " << error() * 100 << "\%\n";
  }
};