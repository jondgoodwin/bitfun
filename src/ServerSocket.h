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
	
	void GetLine(char *buf, int len);
	
	void BuildBuffer(char *buffer, int len);
	void Add(const char *str);
	void Add(int nbr);
	void Send();
	void Send(const char *str);
	
private:
	int m_sockfd;
	char *m_buffer;
	int m_pos;
	int m_size;
};
