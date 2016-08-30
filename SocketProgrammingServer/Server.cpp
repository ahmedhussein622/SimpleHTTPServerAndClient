#include <bits/stdc++.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <sstream>
int PORT = 80;


using namespace std;
std::string get_working_path()
{
   char temp[100];
   return ( getcwd(temp, 100) ? std::string( temp ) : std::string("") );
}
string home_dirc = get_working_path();


int srv_socket;
struct sockaddr_in srv_addr;




void error_report(string s) {
	printf("ERROR %s\n", s.c_str());
	exit(1);
}


vector<string> split(string s, char delim) {
    vector<string> elems;
    stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
    return elems;
}

string to_string(int t) {
	std::ostringstream ss;
	long num = t;
	ss << num;
	return ss.str();
}
int recv_char(char * c, int cli_socket) {
	return recv(cli_socket, c, 1, 0);
}

int recieve_chunck(char buff[], int size, int cli_socket) {
	int t = 0;
	while(t < size && recv_char(buff + t, cli_socket))t++;
	return t;
}




int recv_header(char header[], int size, int cli_socket) {

	int t = 0;
	int r = 1;
	while(t < size && r > 0) {
		r = recv_char(header + t, cli_socket);
		if(r > 0) {
			if(t >= 3 && header[t] == '\n' && header[t - 1] == '\r' &&
					header[t - 2] == '\n' && header[t - 3] == '\r') {
				break;
			}
			t++;
		}
	}
	if(r < 0 || t >= size)
		return -1;

	return t;
}

void header_parser(char header[], int size, char head[], int * head_size, vector<pair<char*, int > > * attr) {
	int t = 1;
	while(t < size && !(header[t] == '\n' && header[t - 1] =='\r'))t++;
	*head_size = t - 1;
	memcpy(head, header, *head_size + 1);
	head[*head_size] = '\0';
	t++;


	while(t < size) {
		int z = t + 1;
		while(z < size && !(header[z] == '\n' && header[z - 1] =='\r'))z++;
		int s = z - t - 1;
		attr->push_back(make_pair((char *)malloc(s + 1), s));
		memcpy((*attr)[attr->size() - 1].first, header + t, s);
		(*attr)[attr->size() - 1].first[s] = '\0';
		t = z + 1;
	}

	attr->resize(attr->size()-1);
}

int send_to_client(int cli_socket, char buff[], int size) {
    int sent=0;
    while(size>0){
        sent=send(cli_socket, buff, size, 0);
        buff+=sent;
        size-=sent;
    }
	return size;
}

void sender(char header[], int size, vector<pair<char*, int > >  attr, int cli_socket) {

	vector<string> r = split(string(header, size), ' ');
	string page = "/special_page/index.html";
	if(r[1].length() != 1)
		page = "/home"+r[1];
	 //open the file
	std::ifstream file;
	printf("open file %s\n", (home_dirc+page).c_str());
	file.open((home_dirc+page).c_str());
	string http_respons;
	int fileSize;
	if(!file.is_open()) {
		http_respons = "HTTP/1.0 404 Not Found\r\nConnection: close\r\n";
		file.open((home_dirc+"/special_page/404.html").c_str());
	}
	else {
		http_respons = "HTTP/1.1 200 OK\r\nConnection: close\r\n";
	}
	file.seekg(0, ios::end);
	fileSize = file.tellg();
	file.seekg(0, ios::beg);
	http_respons = http_respons + "Content-Length: "+to_string(fileSize)+"\r\n\r\n";
	send_to_client(cli_socket, &http_respons[0], strlen(http_respons.c_str()));
	printf("%s \n",http_respons.c_str());
	if(fileSize != 0) {
		std::vector<char> data(fileSize, 0);
		file.read(&data[0], fileSize);
		send_to_client(cli_socket, &data[0], fileSize);
	}

}
void receiver(char request[],char header[], int size, vector<pair<char*, int > >  attr, int cli_socket) {
    string http_ok_to_send="HTTP/1.1 200 OK\r\n\r\n";
    send_to_client(cli_socket,&http_ok_to_send[0],strlen(http_ok_to_send.c_str()));
    string header_str(request);
    int file_size=atoi(split(header_str.substr(header_str.find("Content-Length: ")),' ').at(1).c_str());
    printf("file size: %d\n",file_size);
    int received=0;
    ofstream myfile;
    string file_name=split(header_str.substr(0,header_str.find("\r\n")),' ').at(1);
    myfile.open (file_name.substr(1).c_str());
    while(file_size != 0){
    char data[1000];

    received = recv(cli_socket,  data, 1000, 0);

    for(int i=0;i<received;i++)
    myfile << data[i];

    file_size-=received;
    }
    myfile.close();
}
void http_handeler(char request[],char header[], int size, vector<pair<char*, int > >  attr, int cli_socket) {
	vector<string> r = split(string(header, size), ' ');
	if(strcmp(r[0].c_str(),"GET") == 0) {
		sender(header, size, attr, cli_socket);
	}
	else if(r[0].compare("POST") == 0) {
        receiver(request,header,size,attr,cli_socket);
	}
	else {
		// bad request
	}

}


void handller(int cli_socket) {
	int t = 10000;
	char buff[t];
	int r = recv_header(buff, t, cli_socket);
	buff[r]='\0';
	printf("this is R %d %s\n", r,buff);
	vector<pair<char *, int> > v;
	char head[t];
	int head_size;
	header_parser(buff, r, head, & head_size, &v);
	http_handeler(buff,head, head_size, v, cli_socket);
	close(cli_socket);
}

void server_setup() {
	printf("start setup\n");
	srv_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(srv_socket < 0) {
		error_report("socket creation failed");
	}

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	srv_addr.sin_addr.s_addr = inet_addr("192.168.1.4");

	if(bind(srv_socket, (struct sockaddr*) & srv_addr, sizeof(srv_addr)) < 0) {
		error_report(strerror(errno));
	}

	if(listen(srv_socket, 10) < 0) {
		error_report("listen failed");
	}

}

int main() {

	server_setup();
	printf("setup is done\n");
	while(1) {
		struct sockaddr_in  cli_addr;
		unsigned int cli_len = sizeof(cli_addr);
		int cli_socket;

		cli_socket = accept(srv_socket, (struct sockaddr *)& cli_addr, & cli_len);

		if(cli_socket < 0) {
			error_report("accept failed");
		}
		printf("Handling client %s\n", inet_ntoa(cli_addr.sin_addr));
		pid_t p = fork();
		if(p == 0)
			handller(cli_socket);
	}

	close(srv_socket);
	return 0;
}






