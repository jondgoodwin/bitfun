#include <iostream>
#include <ios>
#include <string>
#include <vector>

#include "BitFunnel/Configuration/Factories.h"
#include "BitFunnel/Configuration/IFileSystem.h"
#include "QueryRunner.h"

#ifndef _WIN32
#include "ServerSocket.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

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

auto prompt = "Query (or quit): ";
#define QUERYBUFLEN 8192
char querybuf[QUERYBUFLEN];
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
		
		while(1)
		{
			int new_fd = serverPoll();
			if (!fork()) { // this is the child process
				serverClose(); // child doesn't need the listener
				{
					char outbuf[1024];
					auto socket = Socket(new_fd);
					socket.BuildBuffer(outbuf, 1024);
					auto docs = new std::vector<size_t>;
					while (1)
					{
				    	socket.Send(prompt);
					    socket.GetLine(querybuf, QUERYBUFLEN);
		             	if (strlen(querybuf)==0 || strcmp(querybuf, "quit") == 0)
			             	break;
						runner->RunQueryDocs(querybuf, docs);
						socket.Add(docs->size());
						socket.Add(" matches\r\n");
						for (size_t i = 0; i < docs->size(); ++i)
						{
							socket.Add("Sonnet ");
							socket.Add((*docs)[i]);
							socket.Add("\r\n");
						}
						socket.Send();
					}
				}
				exit(0);
			}
			close(new_fd);  // parent doesn't need this
		}
	}
#endif
}