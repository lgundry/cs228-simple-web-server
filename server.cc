// Logan Gundry
// IMPORTANT: Check main for DOCUMENT_ROOT and LOG_FILES for correct path of files on lines 167 and 168
// the log file name is access.log. you can change the name on line 220
#include <iostream>		// cout, cerr, etc
#include <stdio.h>		// perror
#include <string.h>		// bcopy
#include <netinet/in.h>		// struct sockaddr_in
#include <unistd.h>		// read, write, etc
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h> // assertions
#include <fstream>	// file streams
#include <cstdint>	// for getDate function
#include <ctime>	//for log timestamp

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
	case 'i':
		return "image/ico";
	}
	return "text/plain";
}

int main(int argc, char *argv[]) {
	int s; 			// socket descriptor
	int len;		// length of reveived data
	char buf[BUFSIZE];	// buffer in which to read
	int ret;		// return code from various system calls

	const string DOCUMENT_ROOT = "/home/lgundry/site/";
	const string LOG_FILES = "/home/lgundry/site/logs/";
	string request;
	string fileRequested;
	string metadata;
	ifstream file;
	char* fileData;
	int fileSize;

	s = MakeServerSocket(argv[1]);
	assert (s != -1);

	while (true) {
		struct sockaddr_in sa;
		int sa_len = sizeof(sa);
		request = "";
		fileRequested = "";
		metadata = "";
		fileSize = 0;

		// works
		cout << "Waiting..." << endl;
		int fd = accept(s, (struct sockaddr *)&sa, (unsigned int *)&sa_len);
		if (fd == -1) {
			throw ("Connection Error");
		}
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
			fileend = request.find("HTTP/1");

			cout << "Request: \n" << request << endl;

			// get referrer
			int refbeginning, refend;;
			refbeginning = request.find("User-Agent: ") + 12;
			refend = request.find("\n", refbeginning);
			string referrer = request.substr(refbeginning, refend - refbeginning - 1);

			// generate log in combined format
			ofstream log;
			log = ofstream(LOG_FILES + "access.log", ios::app);
			if (log.fail())
				cout << "Error opening Log file" << endl;


			//display index file if not searching anything specific
			string tempFileRequest = request.substr(filebeginning, fileend-filebeginning-1);
			if (tempFileRequest == "")
				fileRequested = "index.html";
			else {
				fileRequested = tempFileRequest;
			}

			file = ifstream(DOCUMENT_ROOT + fileRequested);
			if (!file.good()){
				cout << "Error opening file" << endl;
				file.close();
				fileRequested = "404.html";
				file = ifstream(DOCUMENT_ROOT + fileRequested);
				if (!file.good()) {
					cout << "Error opening 404 file" << endl;
					throw ("Error opening 404 file");
				}
			}
			cout << "Successfully opened file" << endl;

			// get file size in bytes
			file.seekg(0, file.end);
			fileSize = file.tellg();
			file.seekg(0, file.beg);

			cout << "File Size: " << fileSize << endl;

			

			// generate metadata
			if (fileRequested == "404.html") {
				metadata = "HTTP/1.1 404 Not Found\r\n";
				// generate log for 404 in combined format
				log << gethostbyaddr(&sa.sin_addr.s_addr, sizeof(sa.sin_addr), AF_INET)->h_name << " - - [" << "25/Mar/2024:23:27:41 -0500" << "] \"GET /" << fileRequested << " HTTP/1.1\" 404 " << fileSize << " \"" << referrer << "\"" << endl;
			}
			else {
				metadata = "HTTP/1.1 200 OK\r\n";
				// generate log for 200 in combined format
				log << gethostbyaddr(&sa.sin_addr.s_addr, sizeof(sa.sin_addr), AF_INET)->h_name << " - - [" << "25/Mar/2024:23:27:41 -0500" << "] \"GET /" << fileRequested << " HTTP/1.1\" 200 " << fileSize << " \"" << referrer << "\""<<  endl;
			}
			metadata += "Date: " + getDate() + "\r\n";
			metadata += "Server: CS 228 Web Server\r\n";
			metadata += "Content-Length: " + to_string(fileSize) + "\r\n";
			metadata += "Content-Type: " + getContentType(fileRequested) + "\r\n";
			metadata += "\r\n";

			cout << "metadata:\n" << metadata << endl;			

			//write metadata to socket
			if (metadata.size() != write(fd, metadata.c_str(), metadata.size()))
				throw ("Error writing to socket");

			cout << "metadata written to socket" << endl;

			// write file data to socket
			fileData = new char[fileSize];
			file.read(fileData, fileSize);
			if (fileSize != write(fd, fileData, fileSize))
				throw ("Error writing to socket");

			cout << "file data written to socket" << endl;
			
		}
		catch (char const* e){
			cout << "uh oh" << endl;
			cout << e << endl;
			file.close();
			close(fd);
		}
		file.close();
		close(fd);
	}
}