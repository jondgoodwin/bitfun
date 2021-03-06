

#include "BitFunnel/Chunks/DocumentFilters.h"
#include "BitFunnel/Chunks/Factories.h"
#include "BitFunnel/Chunks/IChunkManifestIngestor.h"
#include "BitFunnel/Configuration/Factories.h"
#include "BitFunnel/Configuration/IStreamConfiguration.h"
#include "BitFunnel/Data/Sonnets.h"
#include "BitFunnel/Exceptions.h"   // Base class.
#include "BitFunnel/IDiagnosticStream.h"
#include "BitFunnel/Index/Factories.h"
#include "BitFunnel/Index/IIngestor.h"
#include "BitFunnel/Index/IngestChunks.h"
#include "BitFunnel/Plan/Factories.h"
#include "BitFunnel/Plan/QueryInstrumentation.h"
#include "BitFunnel/Utilities/Factories.h"
#include "BitFunnel/Utilities/ReadLines.h"
#include "BitFunnel/Utilities/Stopwatch.h"
#include "CsvTsv/Csv.h"

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

    // streammap maps stream names to their document id
    m_streammap = BitFunnel::Factories::CreateStreamConfiguration();
    m_queryEngine = BitFunnel::Factories::CreateQueryEngine(*m_index, *m_streammap);

    // Indicate we are ready
	double t = stopwatch.ElapsedTime();
	ingestor.PrintStatistics(m_output, t);
}

QueryRunner::~QueryRunner()
{
}

void QueryRunner::RunQueryDocs(std::string query, std::vector<size_t> *docs)
{
	BitFunnel::QueryInstrumentation instrumentation;

	// Parse and run the query, catching ParseError or other RecoverableError
	try
	{
		auto tree = m_queryEngine->Parse(query.c_str());
		instrumentation.FinishParsing();

		// TODO: remove diagnosticStream and replace with nullable.
		auto diagnosticStream = BitFunnel::Factories::CreateDiagnosticStream(std::cout);
		if (tree != nullptr)
		{
            m_queryEngine->Run(tree, instrumentation, m_resultsBuffer);
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
	for (auto result : m_resultsBuffer)
	{
		(*docs)[i++] = result.GetHandle().GetDocId();
	}
}
