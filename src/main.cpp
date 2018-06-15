#include <iostream>
#include <ios>
#include <string>
#include <vector>

#include "BitFunnel/Configuration/Factories.h"
#include "BitFunnel/Configuration/IFileSystem.h"
#include "QueryRunner.h"

#ifndef _WIN32
int serverInit();
int serverPoll();
#endif

int main(int argc, char *argv[]) {
	bool server = (argc > 1 && strcmp(argv[1],"server") == 0);

	// Initialize QueryRunner
	auto fileSystem = BitFunnel::Factories::CreateFileSystem();
	auto runner = new QueryRunner(*fileSystem,
		                          std::cin, std::cout, 
		                          "config",
		                          "manifest.txt",
								  1,
		                          2000000000);

	// Handle queries via console input/output 
	if (!server) {
		auto docs = new std::vector<size_t>;
		while (1) {
			std::cout << "Query (or quit): ";
			std::string query;
			std::getline(std::cin, query);
			if (strcmp(query.c_str(), "quit") == 0)
				break;
			runner->RunQueryDocs(query, docs);
			std::cout << docs->size() << " matches for: '" << query
				<< "'" << std::endl;
			for (size_t i = 0; i < docs->size(); ++i)
			{
				std::cout << "Sonnet " << (*docs)[i] << std::endl;
			}
		}
	}

#ifndef _WIN32	
	// Handle queries via Internet sockets
	else {
		serverInit();
		serverPoll();
	}
#endif
}