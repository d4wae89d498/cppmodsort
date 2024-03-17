#include <clang-c/Index.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <cstring>
#include <string>

#ifndef DBG
# define DBG 0
#endif

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

	list<string> topologicalSort()
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

int processFile(const char* filename, Graph& graph, CXIndex index)
{
	CXTranslationUnit tu = clang_parseTranslationUnit(
		index,
		filename,
		0, 0,//		args, sizeof(args) / sizeof(args[0]),
		nullptr, 0,
		CXTranslationUnit_None
	);
	if (!tu)
	{
		cerr << "Error parsing translation unit: " << filename << 	endl;
		return 1;
	}
	auto wasModuleUnit = false;
	auto cursor = clang_getTranslationUnitCursor(tu);
	CXToken* tokens;
	unsigned numTokens;
	CXSourceRange range = clang_getCursorExtent(cursor);
	clang_tokenize(tu, range, &tokens, &numTokens);
	string currentModule;
	for (unsigned i = 0; i < numTokens; ++i)
	{
		CXTokenKind tokenKind = clang_getTokenKind(tokens[i]);
		if (tokenKind != CXToken_Keyword && tokenKind != CXToken_Identifier && tokenKind != CXToken_Punctuation)
			continue;
		auto spelling = clang_getTokenSpelling(tu, tokens[i]);
		auto spellingCStr = clang_getCString(spelling);
		string currentToken = string(spellingCStr);

		if (currentToken == "export" && (i + 2 < numTokens))
		{
			auto nextSpelling = clang_getTokenSpelling(tu, tokens[i + 1]);
			if (string(clang_getCString(nextSpelling)) == "module")
			{
				currentModule.clear();
				i += 2;
				for (; i < numTokens; ++i)
				{
					auto partSpelling = clang_getTokenSpelling(tu, tokens[i]);
					auto partCStr = clang_getCString(partSpelling);
					if (clang_getTokenKind(tokens[i]) == CXToken_Punctuation && string(partCStr) == ".")
					{
						currentModule += ".";
					}
					else if (clang_getTokenKind(tokens[i]) == CXToken_Punctuation && string(partCStr) == ":")
					{
						currentModule += ":";
					}
					else if (clang_getTokenKind(tokens[i]) == CXToken_Identifier)
					{
						if (!currentModule.empty())
							currentModule += string(partCStr);
						else
							currentModule = string(partCStr);
					}
					else
					{
						break;
					}
					clang_disposeString(partSpelling);
				}
#if DBG
				cerr << filename << " has module " << currentModule << endl;
#endif
				graph.addModule(currentModule, filename);
				wasModuleUnit = true;
				i -= 1;
			}
			clang_disposeString(nextSpelling);
		}
		else if (currentToken == "import" && (i + 1 < numTokens) && !currentModule.empty())
		{
			string importName;
			i += 1;
			for (; i < numTokens; ++i)
			{
				auto partSpelling = clang_getTokenSpelling(tu, tokens[i]);
				auto partCStr = clang_getCString(partSpelling);
				if (clang_getTokenKind(tokens[i]) == CXToken_Punctuation && string(partCStr) == ".")
				{
					importName += ".";
				}
				else if (clang_getTokenKind(tokens[i]) == CXToken_Punctuation && string(partCStr) == ":")
				{
					importName += ":";
				}
				else if (clang_getTokenKind(tokens[i]) == CXToken_Identifier)
				{
					if (!importName.empty())
						importName += string(partCStr);
					else
						importName = string(partCStr);
				}
				else
				{
					break;
				}
				clang_disposeString(partSpelling);
			}
			if (importName.c_str()[0] == ':')
			{
				importName = string(currentModule.find(":") != string::npos ? currentModule.substr(0, currentModule.find(":")) : currentModule) + importName;
			}
#if DBG
			cerr << "import from " << currentModule << " to " << importName << endl;
#endif
			graph.addDependency(currentModule, importName);
			i -= 1;
		}
		clang_disposeString(spelling);

	}
	if (!wasModuleUnit)
	{
		cerr << "Error: " << filename << " was not a module interface." << 	endl;
		return 1;
	}
	clang_disposeTokens(tu, tokens, numTokens);
	clang_disposeTranslationUnit(tu);
	return 0;
}

int main(int ac, char **av)
{
	if (ac < 2)
	{
		cerr << "Usage: " << av[0] << " <source files...>" << 	endl;
		return 1;
	}
	int exit_code = 0;
	auto index = clang_createIndex(0, 0);
	auto dependencyGraph = Graph();
	for (auto i = 1; i < ac; ++i)
		if (processFile(av[i], dependencyGraph, index))
		{
			exit_code = 1;
			break;
		}
//	cerr << "____" << endl;
	if (!exit_code)
	{
		auto orderedFilenames = dependencyGraph.topologicalSort();
		for (auto it = orderedFilenames.rbegin(); it != orderedFilenames.rend(); ++it)
			cout << *it << endl;
		clang_disposeIndex(index);
	}
	return exit_code;
}
