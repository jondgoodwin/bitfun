#include <iostream>
#include <ios>
#include <string>

#include "BitFunnel/Configuration/Factories.h"
#include "BitFunnel/Configuration/IFileSystem.h"
#include "QueryRunner.h"

int main(int argc, char *argv[]) {
	if (argc > 1 && **argv != '\0') {
		;
	}
	
	auto fileSystem = BitFunnel::Factories::CreateFileSystem();
	auto runner = new QueryRunner(*fileSystem,
		                          std::cin, std::cout, 
		                          "config",
		                          "sonnets",
								  8,
		                          2000000000);

	while (1) {
		std::cout << "Query (or quit): ";
		std::string query;
		std::getline(std::cin, query);
		if (strcmp(query.c_str(), "quit") == 0)
			break;
		runner->RunQuery2(query);
	}

	exit(0);
}