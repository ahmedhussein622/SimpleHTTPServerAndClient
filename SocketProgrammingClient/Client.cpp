#include <stdio.h>
#include <string>
#include <errno.h>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;
std::ifstream file;
int PORT=80;
int recv_char(char * c, int cli_socket) {
	return recv(cli_socket, c, 1, 0);
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

string to_string(int t) {
	std::ostringstream ss;
	long num = t;
	ss << num;
	return ss.str();
}

std::string get_working_path()
{
   char temp[100];
   return ( getcwd(temp, 100) ? std::string( temp ) : std::string("") );
}
string home_dirc = get_working_path();

void error_report(string s) {
	printf("ERROR %s\n", s.c_str());
	file.close();
	exit(1);
}

vector<string> split(string input,char delimiter){
    vector<string>splitted;
	string temp="";
	for(int i=0;i<input.size();i++){
        if(input.at(i)==delimiter){
        splitted.push_back(temp);
        temp="";
        }else{
        temp+=input.at(i);
        }
	}
	splitted.push_back(temp);
	return splitted;
}
int create_connection(vector<string> splitted){
    int cli_socket;
	struct sockaddr_in srv_addr;

	cli_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(cli_socket < 0)
		error_report("socket creation error");

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(splitted.size()==4?atoi(splitted[3].c_str()):PORT);
	srv_addr.sin_addr.s_addr = inet_addr(splitted[2].c_str());

	if(connect(cli_socket, (struct sockaddr *) & srv_addr, sizeof(srv_addr)) < 0) {
		error_report(strerror(errno));
	}
    return cli_socket;
}
void handle_get(int socket,string file_name,char buff[]){
    string response(buff);

    string header=response.substr(0,response.find("\r\n"));

    vector<string> splitted=split(header,' ');

    if(strcmp(splitted.at(1).c_str(),"200")==0){//file is found

    int file_size=atoi(split(response.substr(response.find("Content-Length: ")),' ').at(1).c_str());
    printf("file size: %d\n",file_size);

    int received=0;
    ofstream myfile;
    myfile.open(file_name.c_str());

    while(file_size != 0){
    char data[1000];

    received = recv(socket,  data, 1000, 0);

    for(int i=0;i<received;i++)
    myfile << data[i];

    file_size-=received;
    }
    myfile.close();


    }else if(strcmp(splitted.at(1).c_str(),"404")==0){
    //file not found
    printf("file %s not found!",file_name.c_str());
    }


}
int send_to_server(int cli_socket, char buff[], int size) {
    int sent=0;
    while(size>0){
        sent=send(cli_socket, buff, size, 0);
        buff+=sent;
        size-=sent;
    }
	return size;
}

void handle_post(int socket,string file_name,char buff[]){
    string header_resp(buff);
    vector<string> splitted=split(header_resp,' ');
    if(strcmp(splitted.at(1).c_str(),"200")==0){//start to send
	std::ifstream file;
	printf("open file %s\n", (home_dirc+file_name).c_str());
	file.open((home_dirc+file_name).c_str());
	int fileSize;
	file.seekg(0, ios::end);
	fileSize = file.tellg();
	file.seekg(0, ios::beg);
	if(fileSize != 0) {
		std::vector<char> data(fileSize, 0);
		file.read(&data[0], fileSize);
		send_to_server(socket, &data[0], fileSize);
	}

    }

}

int main() {
	file.open("commands.txt");
	std::string input;
    int cli_socket;

	while(std::getline(file,input)){
	if(input.size()==0)
	continue;
	vector<string> splitted=split(input,' ');

    int cli_socket=create_connection(splitted);

	string mesg = splitted.at(0)+" " +splitted.at(1)+" HTTP/1.1\r\n"+
            "Host: "+splitted.at(2)+"\r\n"
			"Connection: close \r\n";

    if(strcmp(splitted.at(0).c_str(),"POST")==0){
    // in case of post request i put to hussein the file size in the header
	std::ifstream file;
	printf("open file %s\n", (home_dirc+splitted.at(1)).c_str());
	file.open((home_dirc+splitted.at(1)).c_str());
	int fileSize;
	file.seekg(0, ios::end);
	fileSize = file.tellg();
	file.seekg(0, ios::beg);
	mesg+= "Content-Length: "+to_string(fileSize)+"\r\n";
    }
    mesg+="\r\n";
    //send request header to hussein
	printf("sent :%d -", send(cli_socket, mesg.c_str(), strlen(mesg.c_str()), 0));

    int t = 10000;
	char buff[t];
	int r = recv_header(buff, t, cli_socket);
	buff[r]='\0';
	printf("this is R %d %s\n", r,buff);
	vector<pair<char *, int> > v;
	char head[t];
	int head_size;


	if(strcmp(splitted.at(0).c_str(),"GET")==0){
	handle_get(cli_socket,splitted.at(1).substr(1),buff);
	}else if(strcmp(splitted.at(0).c_str(),"POST")==0){
    handle_post(cli_socket,splitted.at(1),buff);
	}else{
	printf("bad request!\n");
	}
    close(cli_socket);
	}
    file.close();
	return 0;
}



















