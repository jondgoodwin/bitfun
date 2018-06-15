

#include "BitFunnel/Chunks/DocumentFilters.h"
#include "BitFunnel/Chunks/Factories.h"
#include "BitFunnel/Chunks/IChunkManifestIngestor.h"
#include "BitFunnel/Configuration/Factories.h"
#include "BitFunnel/Data/Sonnets.h"
#include "BitFunnel/Index/Factories.h"
#include "BitFunnel/Index/IIngestor.h"
#include "BitFunnel/Index/IngestChunks.h"
#include "BitFunnel/Plan/Factories.h"
#include "BitFunnel/Plan/QueryInstrumentation.h"
#include "BitFunnel/Plan/QueryParser.h"
#include "BitFunnel/Plan/QueryRunner.h"
#include "BitFunnel/Utilities/Factories.h"
#include "BitFunnel/Utilities/ReadLines.h"
#include "BitFunnel/Utilities/Stopwatch.h"
#include "CsvTsv/Csv.h"

#include "StreamConfiguration.h"
#include "DiagnosticStream.h"

#include "QueryRunner.h"

static const size_t c_allocatorSize = 1ull << 17; 

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
	m_failOnException(false),
	m_resources(c_allocatorSize, c_allocatorSize),
	m_resultsBuffer(1000),
	m_queriesProcessed(0)
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

	m_resultsBuffer = BitFunnel::ResultsBuffer(m_index->GetIngestor().GetDocumentCount());

	if (m_cacheLineCountMode)
	{
		m_resources.EnableCacheLineCounting(*m_index);
	}

	// Indicate we are ready
	double t = stopwatch.ElapsedTime();
	ingestor.PrintStatistics(m_output, t);
}

QueryRunner::~QueryRunner()
{
}

void QueryRunner::RunQueryCount(std::string query, std::ostream& output)
{
	++m_queriesProcessed;

	// Run the query
	auto instrumentationdata =
		BitFunnel::QueryRunner::Run(query.c_str(),
			*m_index,
			m_compilerMode,
			m_cacheLineCountMode);

	// Output query results
	output << "Results for: " << query << std::endl;
	CsvTsv::CsvTableFormatter formatter(output);
	BitFunnel::QueryInstrumentation::Data::FormatHeader(formatter);
	instrumentationdata.Format(formatter);
}

void QueryRunner::RunQueryDocs(std::string query, std::vector<size_t> *docs)
{
	BitFunnel::QueryInstrumentation instrumentation;
	m_resources.Reset();

	// Parse and run the query, catching ParseError or other RecoverableError
	try
	{
		auto config = BitFunnel::Factories::CreateStreamConfiguration();
		BitFunnel::QueryParser parser(query.c_str(),
			*config,
			m_resources.GetMatchTreeAllocator());
		auto tree = parser.Parse();
		instrumentation.FinishParsing();

		// TODO: remove diagnosticStream and replace with nullable.
		auto diagnosticStream = BitFunnel::Factories::CreateDiagnosticStream(std::cout);
		if (tree != nullptr)
		{
			BitFunnel::Factories::RunQueryPlanner(*tree,
				*m_index,
				m_resources,
				*diagnosticStream,
				instrumentation,
				m_resultsBuffer,
				m_compilerMode);

			instrumentation.QuerySuceeded();
		}
	}
	catch (BitFunnel::RecoverableError e)
	{
		// Continue processing other queries, even though the current query failed.
		// The instrumentation for this query will show that it didn't succeed.
	}

	// Output query results

	auto instrumentdata = instrumentation.GetData();
	docs->resize(instrumentdata.GetMatchCount());
	size_t i = 0;
	for (auto iter = m_resultsBuffer.begin(); iter != m_resultsBuffer.end(); ++iter)
	{
		(*docs)[i++] = (*iter).GetHandle().GetDocId();
	}
}
