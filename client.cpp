#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <sstream>

#define BUFLEN 256

using namespace std;

// Mesaj de eroare
void error(char *msg)
{
	perror(msg);
	exit(0);
}

void split_string(string str, string& com, string& param1, string& param2)
{
	string token;
	istringstream iss(str);
	int i = 0;
	while ( getline(iss, token, ' ') )
	{
		switch(i)
		{
		case 0:
			com = token;
			i++;
			break;
		case 1:
			param1 = token;
			i++;
			break;
		case 2:
			param2 = token;
			i++;
			break;
		default:
			break;
		}
	}
	cout << "Split-ui str : ["<<str<<"] in com: ["<<com<<"], param1: ["<<param1<<"] si param2: ["<<param2<<"]\n";
}

// Parsare comanda
void parse_command(char *buffer)
{
	string comanda (buffer);
	string com, param1, param2;

	comanda = comanda.substr(0,comanda.find_last_of("\n"));
	split_string(comanda, com, param1, param2);

	// Comanda "listclients"
	if (comanda.compare("listclients") == 0)
	{
		if (param1.compare("") != 0)
		{
			fprintf(stderr, "Wrong command. Usage: listclients\n");
			return ;
		}
		fprintf(stderr, "Am primit coomanda listclients\n");
		return;
	}

	// Comanda "infoclient nume_client"
	if (com.compare("infoclient") == 0)
	{
		if (param1.compare("") == 0 || param2.compare("") != 0)
		{
			fprintf(stderr, "Wrong command. Usage: infoclient nume_client\n");
			return ;
		}
		fprintf(stderr, "Am primit comanda infoclient pentru ");
		cerr << param1 << endl;
		return;
	}

	// Comanda "message nume_client mesaj"
	if (com.compare("message") == 0)
	{
		if (param1.compare("") == 0 || param2.compare("") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: message nume_client mesaj\n");
			return ;
		}
		fprintf(stderr, "Am primit comanda message pentru clientul ");
		cerr << param1 << "cu mesajul: {" << param2 << "}\n";
		return;
	}

	// Comanda "sharefile nume_fisier"
	if (com.compare("sharefile") == 0)
	{
		if (param1.compare("") == 0 || param2.compare("") != 0)
		{
			fprintf(stderr, "Wrong command. Usage: sharefile nume_fisier\n");
			return ;
		}
		fprintf(stderr, "Am primit comanda sharefile pentru fisierul ");
		cerr << param1 << "\n";
		return;
	}

	// Comanda "unsharefile nume_fisier"
	if (com.compare("unsharefile") == 0)
	{
		if (param1.compare("") == 0 || param2.compare("") != 0)
		{
			fprintf(stderr, "Wrong command. Usage: unsharefile nume_fisier\n");
			return ;
		}
		fprintf(stderr, "Am primit comanda unsharefile pentru fisierul ");
		cerr << param1 << "\n";
		return;
	}

	// Comanda "getshare nume_client"
	if (com.compare("getshare") == 0)
	{
		if (param1.compare("") == 0 || param2.compare("") != 0)
		{
			fprintf(stderr, "Wrong command. Usage: getshare nume_client\n");
			return ;
		}
		fprintf(stderr, "Am primit comanda getshare pentru clientul ");
		cerr << param1 << "\n";
		return;
	}

	// Comanda "getfile nume_client nume_fisier"
	if (com.compare("getfile") == 0)
	{
		if (param1.compare("") == 0 || param2.compare("") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: getfile nume_client nume_fisier\n");
			return ;
		}
		fprintf(stderr, "Am primit comanda getfile pentru clientul ");
		cerr << param1 << " si fisierul " << param2 << "\n";
		return;
	}

	// Comanda "quit"
	if (com.compare("quit") == 0)
	{
		fprintf(stderr, "Am primit comanda quit\n");
		return;
	}

	fprintf(stderr, "Wrong command. Usage: command [param1] [param2]\n");
	return;

}

int main(int argc, char *argv[])
{
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	fd_set read_fds;	//multimea de citire folosita in select()
	fd_set tmp_fds;	//multime folosita temporar
	int fdmax;		//valoare maxima file descriptor din multimea read_fds

	char buffer[BUFLEN];

	// Usage error
	if (argc < 4) {
		fprintf(stderr,"Usage %s client_name server_address server_port\n", argv[0]);
		exit(0);
	}

	//golim multimea de descriptori de citire (read_fds) si multimea (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error((char *)"ERROR opening socket at client");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	inet_aton(argv[2], &serv_addr.sin_addr);


	if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
		error((char *)"ERROR connecting to server");

	while(1){
		//citesc de la tastatura
		memset(buffer, 0 , BUFLEN);
		fgets(buffer, BUFLEN-1, stdin);

		parse_command(buffer);

		//trimit mesaj la server
		n = send(sockfd,buffer,strlen(buffer), 0);
		if (n < 0)
			 error((char *)"ERROR writing to socket");

	}
	return 0;
}


