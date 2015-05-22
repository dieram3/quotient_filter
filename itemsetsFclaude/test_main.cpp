#include "test_level_freqs.cpp"
#include <RunLengthArray.h>
#include <RunLengthBitmap.h>
#include <WaveletTreeNoptrs.h>
#include <algorithm>

using namespace cds_static;
using namespace cds_utils;

int main() {
	vector<vector<int>> data;
	string s;
	int cnt = 0;
	
	while(getline(cin, s)) {
		data.push_back(vector<int>());
		stringstream ss(s);
		int i;
		while((ss >> i)) {
			data[cnt].push_back(i);
		}
		sort(data[cnt].begin(), data[cnt].end());
		cnt++;
	}

	Tree t = build_tree(data);
	auto vals = level_values(t);
	auto vals_label = level_labels(t);
	size_t size_a = 0;
	size_t size_b = 0;
	vector<uint> v;
	for (size_t i = 0 ; i < vals.size();i++)  {
	//	cout << vals[i].size() << endl;
		vector<size_t> r(vals[i].size());
		for (size_t j = 0 ; j < vals[i].size();j++) {
			r.push_back(vals[i][j]);
			v.push_back(vals_label[i][j]);
		}
		if (r.size() != 0 ) {
			RunLengthArray *rla = new RunLengthArray(r);
			RunLengthBitmap *rlb = new RunLengthBitmap(r);
			size_a += rla->GetSize();
			size_b += rlb->GetSize();
			delete rlb;
			delete rla;
		}
		
		
	}
	Array *r = new Array(v);
	MapperNone *mn = new MapperNone();
	BitSequenceBuilder *bsrg = new BitSequenceBuilderRRR(32);
	WaveletTreeNoptrs *wt = new WaveletTreeNoptrs(*r,bsrg,mn);
	cout << "Size wt = " << wt->getSize() <<  " - " << (wt->getSize()*1.0)/(1024*1024.0) << endl;
	cout << "Size using RL array " << size_a << " - " << (1.0*size_a)/(1024*1024.0) << endl;
	cout << "Size using RL Bitmap " << size_b << " - " << (1.0*size_b)/(1024*1024.0) << endl;
	size_t si, total = 0;
	size_t total_len = 0;
	size_t i = 0;
	int m = 0;
	for ( auto v : vals ) {
		m = max(*max_element(v.begin(), v.end()), m);
		total_len += v.size();
		si = sizeForArray(v);
	//	cout << "Nivel " << i++ << " requiere " << si << endl;
		total += si;
	}
	cout << "Espacio total: " << total << endl;
	cout << "Total labels: " << total_len << endl;
	cout << "Labels: " << m << " " << bits(m)*total_len/8 << " " << 1.2*bits(m)*total_len/8.0 << endl;
}
