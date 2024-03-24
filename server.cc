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
#include <cstdint>

using namespace std;

const int BUFSIZE=10240;

string getDate(){

  time_t now = time(0);

  tm *date = localtime(&now);
  string year, month, wday, mday, time;
  year = to_string(1990 + date->tm_year);

  // get month in english - using a switch
  switch (date->tm_mon + 1){
    case 1:
      month = "Jan";
      break;
    case 2:
      month = "Feb";
      break;
    case 3:
      month = "Mar";
      break;
    case 4:
      month = "Apr";
      break;
    case 5:
      month = "May";
      break;
    case 6:
      month = "Jun";
      break;
    case 7:
      month = "Jul";
      break;
    case 8:
      month = "Aug";
      break;
    case 9:
      month = "Sep";
      break;
    case 10:
      month = "Oct";
      break;
    case 11:
      month = "Nov";
      break;
    case 12:
      month = "Dec";
      break;
  }
  
  // get day in english - using switch
  switch(date->tm_wday + 1){
    case 1:
      wday = "SUN";
      break;
    case 2:
      wday = "MON";
      break;
    case 3:
      wday = "TUE";
      break;
    case 4:
      wday = "WED";
      break;
    case 5:
      wday = "THU";
      break;
    case 6:
      wday = "FRI";
      break;
    case 7:
      wday = "SAT";
      break;
  }

  mday = to_string(date->tm_mday);
  time = to_string(date->tm_hour) + ":" + to_string(date->tm_min) + ":" + to_string(date->tm_sec);

  return wday + ", " + mday + " " + month + " " + year + " " + time;

}

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

string getContentType(string fileRequested){
	string extension = fileRequested.substr(fileRequested.find_last_of(".") + 1);
	switch (extension[0]) {
	case 'h':
		return "text/html";
	case 'c':
		return "text/css";
	case 'j':
		switch (extension[1]) {
		case 's':
			return "application/javascript";
		case 'p':
			return "image/jpeg";
		}
	case 'g':
		return "image/gif";
	case 'p':
		return "image/png";
	}
	return "text/plain";
}

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
	string fileRequested;
	string metaData;
	string page;
	fstream file;
	unsigned char fileData[BUFSIZE];
	int fileSize;



	s = MakeServerSocket(argv[1]);
	assert (s != -1);

	while (true) {
		struct sockaddr_in sa;
		int sa_len = sizeof(sa);
		request = "";
		fileRequested = "";
		metaData = "";
		page = "";
		fileSize = 0;

		// works
		cout << "Waiting..." << endl;
		int fd = accept(s, (struct sockaddr *)&sa, (unsigned int *)&sa_len);
		assert(fd != -1);
		cout << "Connection accepted" << endl;

		try {
			// read first bytes
			int len = read(fd, buf, BUFSIZE);
			if (len == -1)
				throw ("Error reading from socket");
			if (len == 0)
				throw ("Client disconnected");
			buf[len] = 0;

			// parse request for file request and save as fileRequested
			request = buf;
			int filebeginning, fileend;
			filebeginning = request.find("GET /") + 5;
			fileend = request.find("HTTP/1") - 5;
			fileRequested = request.substr(filebeginning, fileend);

			// attempt to open fileRequested
			file =  fstream(DOCUMENT_ROOT + fileRequested);
			if (file.fail()) {
				metaData = "HTTP/1.1 404 Not Found\r\n";
				metaData += "Content-Type: text/html\r\n";
				metaData += "Content-Length: 0\r\n";
				metaData += "\r\n";
				if (metaData.size() != write(fd, metaData.c_str(), metaData.size()));
					throw ("Error writing to socket");
				throw ("File not found");
			}

			// get file size in bytes
			file.seekg(0, file.end);
			fileSize = file.tellg();
			file.seekg(0, file.beg);

			// generate metadata
			metaData = "HTTP/1.1 200 OK\r\n";
			metaData += "Date: " + getDate() + "\r\n";
			metaData += "Server: CS 228 Web Server\r\n";
			metaData += "Content-Length: " + to_string(fileSize) + "\r\n";
			metaData += "Content-Type: " + getContentType(fileRequested) + "\r\n";
			metaData += "\r\n";


			// write metadata to socket
			if (metaData.size() != write(fd, metaData.c_str(), metaData.size()))
				throw ("Error writing to socket");

			// write file to socket
			while (file.good()) {
				file.read((char *)fileData, BUFSIZE);
				if (file.gcount() != write(fd, fileData, file.gcount()))
					throw ("Error writing to socket");
			}

		}
		catch (string e){
			cout << e << endl;
			goto cleanup;
		}
	     cleanup:
	        // This code happens on both error and not error
	        // Don't forget to close the file too
			file.close();
	        close(fd);
	}
}



