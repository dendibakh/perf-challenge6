#include "wordcount.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

// Assumptions
// 1. Function should read the input from the file, i.e. caching the input is
// not allowed.
// 2. The input is always encoded in UTF-8.
// 3. Break only on space, tab and newline (do not break on non-breaking space).
// 4. Sort words by frequency AND secondary sort in alphabetical order.

// Implementation rules
// 1. You can add new files but dependencies are generally not allowed unless it
// is a header-only library.
// 2. Your submission must be single-threaded, however feel free to implement
// multi-threaded version (optional).

#ifdef SOLUTION
//
// Your solution here.
//

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <bitset>
#include <queue>
#include <stack>
#include <sstream>
#include <cstring>
#include <numeric>
#include <ctime>
#include <cassert>
#include <random>
#include <unordered_map>
//#include <valarray>
#include <fstream>

#define re return
#define fi first
#define se second
#define mp make_pair
#define pb emplace_back
#define all(x) x.begin(), x.end()
#define sz(x) ((int)(x).size())
#define rep(i, n) for (int i = 0; i < (n); i++)
#define rrep(i, n) for (int i = (n) - 1; i >= 0; i--)

using namespace std;

typedef vector<int> vi;
typedef vector<vi> vvi;
typedef pair<int, int> ii;
typedef vector<ii> vii;
typedef vector<string> vs;

double getTime() {
	re clock() / (double) CLOCKS_PER_SEC;
}

/* ================ actual code starts here ================ */

const int base = 1007;

typedef unsigned long long ull;

inline ull getHash(const string &s) {
	ull ans = 0;
	for (char c : s)
		ans = ans * (ull)base + (c + 128);
	return ans;
}

inline int getSmallHash(const string &s) {
	int ans = 0;
	for (char c : s)
		ans = (ans << 8) | (c + 128);
	return ans;
}

string restore(ull hash) {
	string ans;
	while (hash > 0) {
		ans.push_back(hash % 256 - 128);
		hash /= 256;
	}
	reverse(all(ans));
	return ans;
}

void addWord(const string &cur);

void readData(const string &filePath) {
	const int BS = 524288;
	char *buffer = new char[BS];
	FILE *f = fopen(filePath.c_str(), "r");

	string cur;
	while (1) {
		memset(buffer, 0, BS);
		int count = fread(buffer, BS, 1, f);

		int len = BS;
		if (count != 1)
			len = strlen(buffer) + 1;

		for (int i = 0; i < len; i++) {
			if (buffer[i] >= 0 && buffer[i] <= 32) {
				if (sz(cur)) {
					addWord(cur);
					cur.resize(0);
				}
			}
			else {
				cur += buffer[i];
			}
		}

		if (count != 1)
			break;
	}

	fclose(f);
}

// easy finished

const int LONG_MASK = (1 << 29) - 1;

struct StringData {
	int pos;
	unsigned int pre;
};

vector<StringData> allData;
vi allCount;

bitset<LONG_MASK + 1> firstTime;
bitset<LONG_MASK + 1> secondTime;
vector<int> easy(256 * 256 * 256);
vector<pair<int, StringData>> vfi;
vector<StringData> single[256 * 256];
vector<char> allBytes;

const int FAST_MASK = (1 << 23) - 1;
vector<vector<pair<ull, ull> > > fastMap;

void reserve() {
	fastMap.resize(FAST_MASK + 1);
	vfi.reserve(1 << 23);
	allData.reserve(1 << 23);
}

int addStringBytes(const string &s) {
	int pos = sz(allBytes);
	int len = sz(s);
	for (char c : s) {
		allBytes.push_back(c);
	}
	allBytes.push_back(0);
	len++;
	while (len & 3) {
		len++;
		allBytes.push_back(0);
	}
	return pos;
}

inline StringData createStringData(const string &s) {
	StringData d;
	d.pos = addStringBytes(s);
	char *p = &allBytes[d.pos];
	d.pre = (((p[0] + 256) & 255) << 24u) | (((p[1] + 256) & 255) << 16) | (((p[2] + 256) & 255) << 8) | (((p[3] + 256) & 255));
	return d;
}

void addNewData(const string &s) {
	/*StringData d;
	d.pos = addStringBytes(s);
	allData.emplace_back(d);*/
	allData.pb(createStringData(s));
}

void addData(ull h, const string &s) {
	auto &v = fastMap[h & FAST_MASK];
	int f = 0;
	for (auto &p : v) {
		if (p.fi == h) {
			p.se++;
			f = 1;
			break;
		}
	}

	if (!f) {
		v.pb(h, ((ull)sz(allData)<<32ll) + 1);
		addNewData(s);
	}
}

void addWord(const string &cur) {
	if (sz(cur) <= 3) {
		int h = getSmallHash(cur);
		easy[h]++;
		return;
	}

	ull h = getHash(cur);
	int th = h & LONG_MASK;

	if (!firstTime[th]) {
		firstTime[th] = 1;
		vfi.emplace_back(th, createStringData(cur));
		return;
	}

	secondTime[th] = 1;

	addData(h, cur);
}

inline void addSingle(const StringData &s) {
	single[((unsigned char)allBytes[s.pos] << 8) | (unsigned char)allBytes[s.pos + 1]].pb(s);
//	single[(unsigned char)allBytes[s.pos]].pb(s);
}

int processBitsets() {
	int dno = 0;
	for (auto &o : vfi) {
		if (secondTime[o.fi]) {
			string tmp = &allBytes[o.second.pos];
			ull h = getHash(tmp);
			addData(h, tmp);
		}
		else {
			dno++;
			addSingle(o.second);
		}
	}

//	cout << "Dno = " << dno << endl;
	return dno;
}

void processCounters() {
	allCount.resize(sz(allData));
	//for (auto &p : m)
	for (auto &v : fastMap)
		for (auto &p : v)
			allCount[p.se >> 32] = p.se & ((1ll << 32) - 1);
}

inline bool f1(StringData a, StringData b) {
	if (a.pre != b.pre)
		re a.pre < b.pre;

//	string s1 = &allBytes[a.pos];
//	string s2 = &allBytes[b.pos];
//	re s1 < s2;
	char *p1 = &allBytes[a.pos + 4];
	char *p2 = &allBytes[b.pos + 4];
	while (*p1 == *p2) {
		p1++;
		p2++;
	}

	re (unsigned char)*p1 < (unsigned char)*p2;
}

bool f(pair<int, StringData> a, pair<int, StringData> b) {
	if (a.fi != b.fi)
		re a.fi > b.fi;
	re f1(a.se, b.se);
}

/*
mt19937 rnd(0);

void Sort(StringData *l, StringData *r) {
	if (l >= r)
		re;

	int d = r - l + 1;
	StringData c = l[rnd() % d];
	StringData *sl = l;
	StringData *sr = r;
	while (l <= r) {
		while (l <= r && f1(*l, c))
			l++;
		while (l <= r && !f1(*r, c))
			r--;
		if (l <= r) {
			swap(l->pos, r->pos);
			swap(l->pre, r->pre);
			l++;
			r--;
		}
	}
	if (l < sr)
		Sort(l, sr);
	if (r > sl)
		Sort(sl, r);
}
*/

std::vector<WordCount> wordcount(std::string filePath) {
	reserve();

	readData(filePath);

//	cout << "Read: " << getTime() << endl;

	processBitsets();
	processCounters();

	vector<pair<int, StringData> > dvec;
	dvec.reserve(sz(allData) + 1000500);

	const int zlo = 100000;
	vector<StringData> co[zlo];

	rep(i, sz(allData)) {
		if (allCount[i] == 1) {
			addSingle(allData[i]);
			continue;
		}

		if (allCount[i] < zlo) {
			co[allCount[i]].pb(allData[i]);
			continue;
		}

		dvec.pb(allCount[i], allData[i]);
	}

	for (int i = 0; i < sz(easy); i++) {
		if (easy[i] == 1) {
			string tmp = restore(i);
			addSingle(createStringData(tmp));
		}
		if (easy[i] > 1 && easy[i] < zlo) {
			string tmp = restore(i);
			co[easy[i]].pb(createStringData(tmp));
		}
		if (easy[i] >= zlo) {
			string tmp = restore(i);
			dvec.pb(easy[i], createStringData(tmp));
		}
	}

//	cout << "Prepare: " << getTime() << ' ' << dvec.capacity() << endl;

	int sum = sz(dvec);

	sort(all(dvec), f);

	rrep(i, zlo)
		sort(all(co[i]), f1);

//	cout << "Sort 1: " << getTime() << endl;

	rep(i, 256 * 256) {
		if (!sz(single[i]))
			continue;
		sum += sz(single[i]);
		sort(all(single[i]), f1);
//		Sort(&single[i][0], &single[i][0] + sz(single[i]) - 1);
	}

//	cout << "Sort 2: " << getTime() << endl;

	vector<WordCount> mvec;
	mvec.reserve(sum);

	for (auto &o : dvec) {
		mvec.pb(WordCount{o.fi, &allBytes[o.se.pos]});
	}

	rrep(i, zlo)
	for (auto &s : co[i]) {
		mvec.pb(WordCount{i, &allBytes[s.pos]});
	}

	rep(i, 256 * 256)
	for (auto &s : single[i]) {
		mvec.pb(WordCount{1, &allBytes[s.pos]});
	}

//	cout << "Final: " << getTime() << endl;

	return mvec;
}

// end

#else
// Baseline solution.
// Do not change it - you can use for quickly checking speedups
// of your solution agains the baseline, see check_speedup.py
std::vector<WordCount> wordcount(std::string filePath) {
  std::unordered_map<std::string, int> m;
  m.max_load_factor(0.5);

  std::vector<WordCount> mvec;

  std::ifstream inFile{filePath};
  if (!inFile) {
    std::cerr << "Invalid input file: " << filePath << "\n";
    return mvec;
  }

  std::string s;
  while (inFile >> s)
    m[s]++;

  mvec.reserve(m.size());
  for (auto &p : m)
    mvec.emplace_back(WordCount{p.second, move(p.first)});

  std::sort(mvec.begin(), mvec.end(), std::greater<WordCount>());
  return mvec;
}
#endif