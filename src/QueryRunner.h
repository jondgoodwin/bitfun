

#pragma once

#include <ios>
#include <iostream>
#include "BitFunnel/BitFunnelTypes.h"       // ShardId parameter.
#include "BitFunnel/Configuration/IFileSystem.h"
#include "BitFunnel/Index/ISimpleIndex.h"   // Parameterizes std::unique_ptr.

class QueryRunner {
public:
	QueryRunner(BitFunnel::IFileSystem& fileSystem,
		        std::istream& input,
		        std::ostream& output,
		        const char *configFolder,
		        const char *manifest,
		        size_t threadCount,
		        size_t memory);
	~QueryRunner();

	void RunQuery(std::string query);

private:
	BitFunnel::IFileSystem& m_fileSystem;
	std::istream& m_input;
	std::ostream& m_output;
	const char *m_configFolder;
	const char *m_manifest;
	size_t m_threadCount;
	size_t m_memory;
	size_t m_gramSize;

	std::unique_ptr<BitFunnel::ISimpleIndex> m_index;

	bool m_cacheLineCountMode;
	bool m_compilerMode;
	bool m_failOnException;
};
