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
#define MAX_CLIENTS 5

using namespace std;

// Mesaj de eroare
void error(char *msg)
{
	perror(msg);
	exit(0);
}

void quit_message()
{
	fprintf(stderr, "Am primit comanda quit și închid clientul\n");
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
//	cout << "Split-ui str : ["<<str<<"] in com: ["<<com<<"], param1: ["<<param1<<"] si param2: ["<<param2<<"]\n";
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
		fprintf(stderr, "Am primit comanda listclients\n");
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
	if (comanda.compare("quit") == 0)
	{
//		fprintf(stderr, "Am primit comanda quit\n");

		quit_message();
		exit(0);
		//TODO Close connections and send message to the server for closing;
//		return;
	}

	fprintf(stderr, "Wrong command. Usage: command [param1] [param2]\n");
	return;

}

int main(int argc, char *argv[])
{
	int sockfd, i, n;
	struct sockaddr_in serv_addr, listen_addr;

	fd_set read_fds;	//multimea de citire folosita in select()
	fd_set tmp_fds;		//multime folosita temporar
	int fdmax;			//valoare maxima file descriptor din multimea read_fds

	char buffer[BUFLEN];

	// Usage error
	if (argc < 4)
	{
		fprintf(stderr,"Usage %s client_name server_address server_port\n", argv[0]);
		exit(0);
	}

	// Socket pentru conectare la server
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error((char *)"ERROR opening socket at client");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	inet_aton(argv[2], &serv_addr.sin_addr);

	if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
			error((char *)"ERROR connecting to server");

	int listen_sockfd;

	// Socket pentru ascultare conexiuni
	listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sockfd < 0)
		error((char *)"ERROR opening listening socket at client");

	memset((char *) &listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(0);			// Port asignat automat
	listen_addr.sin_addr.s_addr = INADDR_ANY;	// Adresa IP a masinii

	if (bind(listen_sockfd, (struct sockaddr *) &listen_addr, sizeof(struct sockaddr)) < 0)
		error((char *)"ERROR on binding");

	listen(listen_sockfd, MAX_CLIENTS);

	//golim multimea de descriptori de citire (read_fds) si multimea (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	//adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(listen_sockfd, &read_fds);
	fdmax = listen_sockfd;

	//adaugam stdin in multimea read_fds
	FD_SET(fileno(stdin), &read_fds);

	//main loop
	while(1)
	{
		tmp_fds = read_fds;

		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error((char *)"ERROR in select");

		for (i = 0; i <= fdmax; ++i)
		{
			if (FD_ISSET(i, &tmp_fds))
			{
				if (i == fileno(stdin) )
				{
					//citesc de la tastatura
					memset(buffer, 0 , BUFLEN);
					fgets(buffer, BUFLEN-1, stdin);

					parse_command(buffer);

					//trimit mesaj la server
					n = send(sockfd,buffer,strlen(buffer), 0);
					if (n < 0)
						 error((char *)"ERROR writing to socket");
				}
			}
		}



	}
	return 0;
}


