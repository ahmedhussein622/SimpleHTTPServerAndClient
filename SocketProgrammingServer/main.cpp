//#include <stdio.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
//
//void reporter(char * s) {
//	printf("ERROR : %s\n", s);
//}
//
//int main(int argc, char**argv) {
//
//	int sokt;
//	sokt = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if(sokt < 0)
//		reporter("socket is wrong");
//
//	struct sockaddr_in add;
//	memset(&add, 0, sizeof(add));
//	add.sin_family = PF_INET;
//	add.sin_addr.s_addr = inet_addr("173.194.116.36");
//	add.sin_port = htons(7);
//	printf("here\n");
//	if (connect(sokt, (struct socketadd *) & add, sizeof(add)) < 0)
//		reporter("connection failed");
//	printf("here\n");
//
//	reporter("done =D");
//
//	return 0;
//}
//
//
//
//
