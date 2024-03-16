#include <clang-c/Index.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <cstring>
#include <string>

using namespace std;

struct Graph
{
	map<string, set<string> > 	adj;
	map<string, string> 		moduleToFile;

	void addModule(const 	string& moduleName, const 	string& filename)
	{
		moduleToFile[moduleName] = filename;
		adj[moduleName];
	}

	void addDependency(const 	string& fromModule, const 	string& toModule)
	{
		adj[fromModule].insert(toModule);
	}

	void dfs(const 	string& moduleName, 	map<	string, bool>& visited, 	list<	string>& ordering)
	{
		visited[moduleName] = true;
		for (const auto& dep : adj[moduleName])
			if (!visited[dep])
				dfs(dep, visited, ordering);
		ordering.push_front(moduleName);
	}

	list<	string> topologicalSort()
	{
		map<string, bool> 	visited;
		list<string> 		ordering;
		for (const auto& pair : adj)
			if (!visited[pair.first])
				dfs(pair.first, visited, ordering);
		list<string> 		orderedFilenames;
		for (const auto& moduleName : ordering)
			orderedFilenames.push_back(moduleToFile[moduleName]);
		return orderedFilenames;
	}
};

void processFile(const char* filename, Graph& graph, CXIndex index)
{
//	const char* args[] = {};
	CXTranslationUnit tu = clang_parseTranslationUnit(
		index,
		filename,
//		args, sizeof(args) / sizeof(args[0]),
		0, 0,
		nullptr, 0,
		CXTranslationUnit_None
	);
	if (!tu)
	{
		cerr << "Error parsing translation unit: " << filename << 	endl;
		return ;
	}
	auto was_module_unit = false;
	auto cursor = clang_getTranslationUnitCursor(tu);
	CXToken* tokens;
	unsigned numTokens;
	CXSourceRange range = clang_getCursorExtent(cursor);
	clang_tokenize(tu, range, &tokens, &numTokens);
	string currentModule;
	for (unsigned i = 0; i < numTokens; ++i)
	{
		CXTokenKind tokenKind = clang_getTokenKind(tokens[i]);
		if (tokenKind != CXToken_Keyword && tokenKind != CXToken_Identifier)
			continue;
		auto spelling = clang_getTokenSpelling(tu, tokens[i]);
		const char* spellingCStr = clang_getCString(spelling);
		if (!strcmp(spellingCStr, "export") && (i + 2 < numTokens))
		{
			auto nextSpelling = clang_getTokenSpelling(tu, tokens[i + 1]);
			if (!strcmp(clang_getCString(nextSpelling), "module")) {
				auto moduleName = clang_getTokenSpelling(tu, tokens[i + 2]);
				currentModule = clang_getCString(moduleName);
				graph.addModule(currentModule, filename);
				was_module_unit = true;
				clang_disposeString(moduleName);
				i += 2;
			}
			clang_disposeString(nextSpelling);
		}
		else if (!strcmp(spellingCStr, "import") && (i + 1 < numTokens) && !currentModule.empty())
		{
			auto importName = clang_getTokenSpelling(tu, tokens[i + 1]);
			graph.addDependency(currentModule, clang_getCString(importName));
			clang_disposeString(importName);
			++i;
		}
		clang_disposeString(spelling);
	}
	clang_disposeTokens(tu, tokens, numTokens);
	clang_disposeTranslationUnit(tu);
	return ;
}

int main(int ac, char **av)
{
	if (ac < 2)
	{
		cerr << "Usage: " << av[0] << " <source files...>" << 	endl;
		return 1;
	}
	auto index = clang_createIndex(0, 0);
	auto dependencyGraph = Graph();
	for (auto i = 1; i < ac; ++i)
		processFile(av[i], dependencyGraph, index);
	auto orderedFilenames = dependencyGraph.topologicalSort();
	for (auto it = orderedFilenames.rbegin(); it != orderedFilenames.rend(); ++it)
		cout << *it << endl;
	clang_disposeIndex(index);
	return 0;
}
