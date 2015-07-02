#include <iostream>
#include "bloomFilter.hpp"

using namespace std;

int main(int argc, char **argv) {
    if(argc != 3) {
        cout << "Usage:\n\t" << argv[0] << " number_of_elements expected_error\n";
        return -1;
    }

    srand(time(NULL));
    size_t elements = atoi(argv[1]);
    if(!elements || argv[1][0] == '-') {
        cout << "Number of elements must be greater than 0\n";
        return -1;
    }

    double err = atof(argv[2]);
    if(err > 1 || err <= 0) {
        cout << "Expected error interval -> ]0,1[\n";
        return -1;
    }

    bloomFilter bf(elements, err);
    size_t count = 0;
    for(size_t ix = 0; ix < elements; ix++) {
        if(bf[make_pair(ix, rand())])
            count++;
    }

    bf.info();
    cout << "Real error\t\t : " << (double)count/(double)elements*100 << "\%\n";
    return 0;
}