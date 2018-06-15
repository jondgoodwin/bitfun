/*
** ServerSocket.h -- a stream socket server
*/

int serverInit();
int serverPoll();
void serverClose();

class Socket {
public:
	Socket(int fd);
	~Socket();
	
	int Send(const void *msg, int len, int flags);
	int Recv(void *buf, int len, int flags);
	
private:
	int m_sockfd;
};
