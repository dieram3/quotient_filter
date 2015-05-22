#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <queue>
#include <iterator>
#include <algorithm>

using namespace std;

class Node {
 public:
  map<int, Node *> children;
  int value, counter;

  Node(int val, int cnt) {
    value = val;
    counter = cnt;
  }

  Node() {
    value = 0;
    counter = 0;
  }
};

class Tree {
 public:
  Node *root;
  Tree() { root = new Node(); }
  void insert(const vector<int> &v) {
    Node *aux = root;
    for (const auto &val : v) {
      if (aux->children[val] == NULL) {
        aux->children[val] = new Node(val, 0);
      }
      aux->children[val]->counter++;
      aux = aux->children[val];
    }
  }
};

Tree build_tree(vector<vector<int>> &data) {
  Tree t;
  for (const auto &v : data) {
    t.insert(v);
  }
  return t;
}

vector<vector<int>> level_values(Tree &t) {
  int nodeCount = 0;
  vector<vector<int>> res;
  queue<Node *> cola;
  int nivel = -1;
  cola.push(NULL);
  cola.push(t.root);
  while (cola.size() > 1) {
    Node *n = cola.front();
    cola.pop();
    if (n == NULL) {
      nivel++;
      cola.push(NULL);
      res.push_back(vector<int>());
    } else {
      nodeCount++;
      res[nivel].push_back(n->counter);
      for (auto v : n->children) {
        cola.push(v.second);
      }
    }
  }
  cout << "Node count: " << nodeCount << endl;
  return res;
}

vector<vector<int>> level_labels(Tree &t) {
  vector<vector<int>> res;
  queue<Node *> cola;
  int nivel = -1;
  cola.push(NULL);
  cola.push(t.root);
  while (cola.size() > 1) {
    Node *n = cola.front();
    cola.pop();
    if (n == NULL) {
      nivel++;
      cola.push(NULL);
      res.push_back(vector<int>());
    } else {
      res[nivel].push_back(n->value);
      for (auto v : n->children) {
        cola.push(v.second);
      }
    }
  }
  return res;
}

int bits(int val) {
  int cnt = 0;
  while (val) {
    cnt++;
    val /= 2;
  }
  return cnt;
}

size_t sizeForArray(vector<int> &v) {
  auto m = max_element(v.begin(), v.end());
  auto b = bits(*m);
  return (size_t)b * v.size();
}
