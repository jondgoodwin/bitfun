#include <iostream>
#include <ios>
#include <string>
#include <vector>

#include "BitFunnel/Configuration/Factories.h"
#include "BitFunnel/Configuration/IFileSystem.h"
#include "QueryRunner.h"

#ifndef _WIN32
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int serverInit();
int serverPoll();

// Callback for process reaper
void sigchld_handler(int /*s*/)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// Reap dead "zombie" processes
void sigchld_reaper() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}
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
		sigchld_reaper();
		std::cout << "server: waiting for connections...\n";
		serverPoll();
	}
#endif
}