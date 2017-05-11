// DataMiningAlgorithm.cpp : FP-growth算法
//

#include "stdafx.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <string>
#include <algorithm>

using namespace std;

const int min_sup = 2;

typedef struct FPTreeNode {
	string itemID;
	int supCount;
	vector<FPTreeNode*> children;
	FPTreeNode* link;
	FPTreeNode* parent;

	FPTreeNode(string id, int count, FPTreeNode* p) : itemID(id), supCount(count), parent(p), link(nullptr) {};
} Node;

typedef struct {
	vector<string> items;
	int supCount;
} Pattern;

void printPattern(Pattern* p)
{
	cout << "{ ";
	for each(auto i in p->items) {
		if (p->items.back() == i)
			cout << i;
		else
			cout << i << ", ";
	}
	cout << ": " << p->supCount << " }" << endl;
}

Node* containsChild(Node* n, string id)
{
	for (auto it = n->children.begin(); it != n->children.end(); it++)
	{
		if ((*it)->itemID == id)
			return *it;
	}
	return nullptr;
}

bool contains(vector<string>* v, string id)
{
	if (v == nullptr)
		return false;
	for (auto it = v->begin(); it != v->end(); it++)
	{
		if ((*it) == id)
			return true;
	}
	return false;
}

bool isSinglePath(const Node* root)
{
	if (root->children.size() == 0)
		return true;
	if (root->children.size() > 1)
		return false;
	return isSinglePath(root->children[0]);
}

void linkSameItems(Node*& s, Node*& n)
{
	if (s) {
		n->link = s->link;
		s->link = n;
	}
	else {
		s = n;
	}
}

void deleteTree(Node* root)
{
	if (root) {
		for each (Node* n in root->children)
		{
			deleteTree(n);
		}
		delete root;
	}
}

string toString(const vector<string>& v)
{
	string id;
	for each(auto i in v) {
		id += i;
	}
	return id;
}

auto generateTermList(const vector<vector<string>>& itemRecords)
{
	vector<tuple<string, int, Node*>> itemList;
	unordered_map<string, int> item2Index;
	for each (auto r in itemRecords)
	{
		for each(auto i in r) {
			if (item2Index.find(i) == item2Index.end()) {
				item2Index[i] = itemList.size();
				itemList.push_back(make_tuple(i, 1, nullptr));
			}
			else {
				get<1>(itemList[item2Index[i]])++;
			}
		}
	}
	itemList.erase(remove_if(itemList.begin(), itemList.end(), [](auto& item) { return get<1>(item) < min_sup; }), itemList.end());
	sort(itemList.begin(), itemList.end(), [](auto& a, auto& b) { return get<1>(a) > get<1>(b); });
	return itemList;
}

Node* generateFPTree(const vector<vector<string>>& itemRecords,
	vector<tuple<string, int, Node*>>& itemList,
	const unordered_map<string, int>& item2Index)
{
	Node* root = new Node("", 0, nullptr);
	int count = 1;
	for each(auto r in itemRecords) {
		r.erase(remove_if(r.begin(), r.end(), [&item2Index](auto& i) { return item2Index.find(i) == item2Index.end(); }), r.end());
		sort(r.begin(), r.end(), [&item2Index](auto& a, auto& b) { return item2Index.at(a) < item2Index.at(b); });
		Node* curr = root;
		Node* next;
		for each (auto i in r) {
			if ((next = containsChild(curr, i)) != nullptr) {
				next->supCount++;
			}
			else {
				next = new Node(i, 1, curr);
				linkSameItems(get<2>(itemList[item2Index.at(i)]), next);
				curr->children.push_back(next);
			}
			curr = next;
		}
	}
	return root;
}

void FP_growth(const vector<tuple<string, int, Node*>>& itemList, Node* root, vector<string>* alpha)
{
	if (isSinglePath(root)) {
		int combinationNum = (int)pow(2, itemList.size());
		unordered_map<string, int> no_repeatSet;
		for (int i = 1; i < combinationNum; i++) {
			int base = 1;
			vector<string> beta;
			int minSup = INT_MAX;
			for (size_t j = 0; j < itemList.size(); j++) {
				if ((i & base) == base) {
					minSup = min(minSup, get<1>(itemList[j]));
					if(!contains(alpha, get<0>(itemList[j])))
						beta.push_back(get<0>(itemList[j]));
				}
				base *= 2;
			}
			string hash = toString(beta);
			if (no_repeatSet.find(hash) != no_repeatSet.end()) //已经输出过该集合了
				continue;
			no_repeatSet[hash] = 0;
			if(alpha != nullptr)
				beta.insert(beta.end(), alpha->begin(), alpha->end());
			Pattern combined = { beta, minSup };
			printPattern(&combined);
		}
	}
	else {
		for (auto it = itemList.rbegin(); it != itemList.rend(); it++) {
			Node* n = get<2>(*it);
			vector<vector<string>> conditionPatternBase;
			while (n) {
				vector<string> l;
				Node* p = alpha ? n : n->parent;
				while (p != root) {
					l.push_back(p->itemID);
					p = p->parent;
				}
				if (l.size() > 0) {
					int count = n->supCount;
					while (count--)
						conditionPatternBase.push_back(vector<string>(l.rbegin(), l.rend()));
				}
				n = n->link;
			}
			if (conditionPatternBase.size() == 0) //如果没有条件模式基
				continue;
			vector<tuple<string, int, Node*>> treeBetaList = generateTermList(conditionPatternBase);
			unordered_map<string, int> item2Index;
			for (size_t i = 0; i < treeBetaList.size(); i++) {
				item2Index[get<0>(treeBetaList[i])] = i;
			}
			Node* treeBeta = generateFPTree(conditionPatternBase, treeBetaList, item2Index);
			vector<string> beta;
			beta.push_back(get<0>(*it));
			if (alpha)
				beta.insert(beta.end(), alpha->begin(), alpha->end());
			FP_growth(treeBetaList, treeBeta, &beta);
			deleteTree(treeBeta);
		}
	}
}

int main()
{
	//初始化交易数据
	string transactionIDs[] = { "T100", "T200" , "T300", "T400", "T500", "T600", "T700", "T800", "T900" };
	vector<vector<string>> itemRecords;
	itemRecords.push_back(vector<string>({ "I1", "I2", "I5" }));
	itemRecords.push_back(vector<string>({ "I2", "I4" }));
	itemRecords.push_back(vector<string>({ "I2", "I3" }));
	itemRecords.push_back(vector<string>({ "I1", "I2", "I4" }));
	itemRecords.push_back(vector<string>({ "I1", "I3" }));
	itemRecords.push_back(vector<string>({ "I2", "I3" }));
	itemRecords.push_back(vector<string>({ "I1", "I3" }));
	itemRecords.push_back(vector<string>({ "I1", "I2", "I3", "I5" }));
	itemRecords.push_back(vector<string>({ "I1", "I2", "I3" }));
	/*
	itemRecords.push_back(vector<string>({ "A", "C", "D" }));
	itemRecords.push_back(vector<string>({ "B", "C", "E" }));
	itemRecords.push_back(vector<string>({ "A", "B", "C", "E" }));
	itemRecords.push_back(vector<string>({ "B", "E" }));
	*/

	//扫描数据一次，生成排序好的1项集的结果表作为项头表
	vector<tuple<string, int, Node*>> itemList = generateTermList(itemRecords);
	unordered_map<string, int> item2Index;
	for (size_t i = 0; i < itemList.size(); i++) {
		item2Index[get<0>(itemList[i])] = i;
	}

	//构建FP—Tree
	Node* root = generateFPTree(itemRecords, itemList, item2Index);

	//使用项头表，从后向前计算频繁项集
	FP_growth(itemList, root, nullptr);
	deleteTree(root);

	for each(auto i in itemList) {
		Pattern p = { vector<string>({get<0>(i)}), get<1>(i) };
		printPattern(&p);
	}

    return 0;
}