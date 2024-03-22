#include <iostream>		// cout, cerr, etc
#include <stdio.h>		// perror
#include <string.h>		// bcopy
#include <netinet/in.h>		// struct sockaddr_in
#include <unistd.h>		// read, write, etc
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <fstream>

using namespace std;

const int BUFSIZE=10240;

int MakeServerSocket(const char *port) {
	const int MAXNAMELEN = 255;
	const int BACKLOG = 3;	
	char localhostname[MAXNAMELEN]; // local host name
	int s; 		
	int len;
	struct sockaddr_in sa; 
	struct hostent *hp;
	struct servent *sp;
	int portnum;	
	int ret;


	hp = gethostbyname("localhost");

	sscanf(port, "%d", &portnum);
	if (portnum ==  0) {
		sp=getservbyname(port, "tcp");
		portnum = ntohs(sp->s_port);
	}
	sa.sin_port = htons(portnum);

	bcopy((char *)hp->h_addr, (char *)&sa.sin_addr, hp->h_length);
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_family = AF_INET;

	s = socket(AF_INET, SOCK_STREAM, 0);

	ret = bind(s, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0)
		perror("Bad");
	listen(s, BACKLOG);
	cout << "Waiting for connection on port " << port << endl;
	return s;
}

// string getStatus(){

// }

// string getContentType(){

// }

// string getDate(){

// }

// string getServer(){

// }

int main(int argc, char *argv[]) {


	int s; 			// socket descriptor
	int len;		// length of reveived data
	char buf[BUFSIZE];	// buffer in which to read
	int ret;		// return code from various system calls

	const string DOCUMENT_ROOT = "~/site";
	string request;
	string filename;


	string header;
	string page;



	s = MakeServerSocket(argv[1]);
	assert (s != -1);

	while (true) {
		struct sockaddr_in sa;
		int sa_len = sizeof(sa);

		// works
		cout << "Waiting..." << endl;
		int fd = accept(s, (struct sockaddr *)&sa, (unsigned int *)&sa_len);
		assert(fd != -1);
		cout << "Connection accepted" << endl;

		// read first bytes
		int len = read(fd, buf, BUFSIZE);
		if (len == -1 || len == 0) goto cleanup;
		buf[len] = 0;

		request = buf;
		int filebeginning, fileend;
		if (request[0] == 'G') {
			filebeginning = request.find("GET /") + 5;
			fileend = request.find("HTTP/1") - 5;
			filename = request.substr(filebeginning, fileend);
		}
		// Write to the web client
		header = "HTTP/1.0 200 Great Day\r\nContent-Type: text/plain\r\n";
		page = "Here is what your request was: " + filename + "\r\n";
		if (header.length() != write(fd, header.c_str(), header.length())) goto cleanup;
		if (write(fd, "\r\n", 2) == -1) goto cleanup;
		if (write(fd, page.c_str(), page.length()) == -1) goto cleanup;
		
	     cleanup:
	        // This code happens on both error and not error
	        // Don't forget to close the file too
	        close(fd);
	}
}



