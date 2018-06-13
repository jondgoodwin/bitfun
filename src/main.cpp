#include <iostream>
#include <ios>
#include <string>

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
		while (1) {
			std::cout << "Query (or quit): ";
			std::string query;
			std::getline(std::cin, query);
			if (strcmp(query.c_str(), "quit") == 0)
				break;
			runner->RunQuery2(query);
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