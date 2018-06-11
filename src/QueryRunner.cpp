

#include "BitFunnel/Chunks/DocumentFilters.h"
#include "BitFunnel/Chunks/Factories.h"
#include "BitFunnel/Chunks/IChunkManifestIngestor.h"
#include "BitFunnel/Data/Sonnets.h"
#include "BitFunnel/Index/Factories.h"
#include "BitFunnel/Index/IIngestor.h"
#include "BitFunnel/Index/IngestChunks.h"
#include "BitFunnel/Plan/QueryInstrumentation.h"
#include "BitFunnel/Plan/QueryRunner.h"
#include "BitFunnel/Utilities/ReadLines.h"
#include "BitFunnel/Utilities/Stopwatch.h"
#include "CsvTsv/Csv.h"
#include "QueryRunner.h"

QueryRunner::QueryRunner(BitFunnel::IFileSystem& fileSystem,
	                     std::istream& input,
	                     std::ostream& output,
	                     const char *configFolder,
	                     const char *manifest,
	                     size_t threadCount,
	                     size_t memory) :
	m_fileSystem(fileSystem),
	m_input(input),
	m_output(output),
	m_configFolder(configFolder),
	m_manifest(manifest),
	m_threadCount(threadCount),
	m_memory(memory),
	m_gramSize(1),
	m_index(BitFunnel::Factories::CreateSimpleIndex(fileSystem)),
	m_cacheLineCountMode(false),
	m_compilerMode(true),
	m_failOnException(false)
{
	BitFunnel::Stopwatch stopwatch;

	output << "BitFun is starting up...\n";

	// Start the index
	m_index->SetBlockAllocatorBufferSize(m_memory);
	m_index->ConfigureForServing(m_configFolder, m_gramSize, false);
	m_index->StartIndex();

	// Load/ingest the documents
	BitFunnel::IConfiguration const & configuration = m_index->GetConfiguration();
	BitFunnel::IIngestor & ingestor = m_index->GetIngestor();

	if (strcmp(m_manifest, "sonnets") == 0)
	{
		auto manifest = BitFunnel::Factories::CreateBuiltinChunkManifest(
			BitFunnel::Sonnets::chunks,
			configuration,
			ingestor,
			false);
		BitFunnel::IngestChunks(*manifest, threadCount);
	}
	else
	{
		std::vector<std::string> filePaths;
		filePaths = BitFunnel::ReadLines(m_fileSystem, m_manifest);
		BitFunnel::NopFilter filter;
		auto manifest = BitFunnel::Factories::CreateChunkManifestIngestor(
			fileSystem,
			nullptr,
			filePaths,
			configuration,
			ingestor,
			filter,
			false);
		BitFunnel::IngestChunks(*manifest, threadCount);
	}

	// Indicate we are ready
	double t = stopwatch.ElapsedTime();
	ingestor.PrintStatistics(m_output, t);
}

QueryRunner::~QueryRunner()
{
}

void QueryRunner::RunQuery(std::string query)
{
	// Run the query
	auto instrumentation =
		BitFunnel::QueryRunner::Run(query.c_str(),
			*m_index,
			m_compilerMode,
			m_cacheLineCountMode);

	// Output query results
	m_output << "Results for: " << query << std::endl;
	CsvTsv::CsvTableFormatter formatter(m_output);
	BitFunnel::QueryInstrumentation::Data::FormatHeader(formatter);
	instrumentation.Format(formatter);
}
