#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <vector>

#define MAX_CLIENTS 5
#define BUFLEN 256
#define NAMESZ 100
#define CMDSZ 100

using namespace std;

int sockfd, newsockfd;
int fdmax;		//valoare maxima file descriptor din multimea read_fds

fd_set read_fds;	//multimea de citire folosita in select()
fd_set tmp_fds;	//multime folosita temporar

vector<string> clienti;

void error(char *msg)
{
	perror(msg);
	exit(1);
}

void parse_command(char *buffer)
{
	if (strcmp(buffer, "status\n") == 0)
	{
		fprintf(stderr, "Am primit comanda status\n");
		return;
	}
	if (strcmp(buffer, "quit\n") == 0)
	{
		fprintf(stderr, "Am primit comanda quit și închid server-ul\n");

		for (int i = 0; i <= fdmax; ++i)
			if (FD_ISSET(i, &tmp_fds))
			{
				close(i);
				FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul
			}

		exit(0);
		//return;
	}

	fprintf(stderr, "Wrong command. Usage: \"status\" or \"quit\"\n");
	return;
}

void parse_message(char *buffer, char comanda[CMDSZ])
{
	char nume[NAMESZ];
	int port;

	if ( strcmp(comanda, "connect") == 0)
	{
		cerr << "CMD {" << comanda << "}\n";

		sscanf(buffer, "%*s %s %d", nume, &port);
		cerr << "Nume: " << nume << " port: " << port << endl;

		string name(nume);
		clienti.push_back(name);
		cerr << "CLIENTI: ";
		for (unsigned int i = 0; i < clienti.size(); ++i)
			cerr << clienti[i] << " ";
		cerr << endl;
	}

	if ( strcmp(comanda, "listclients") == 0 )
	{
		cerr << "CMD {" << comanda << "}\n";

		sscanf(buffer, "%*s %s", nume);
		cerr << "nume: " << nume << endl;

		memset(buffer, 0, BUFLEN);
		cerr << "CLIENTI: ";
		for (unsigned int i = 0; i < clienti.size(); ++i)
			cerr << clienti[i] << " ";
		cerr << endl;

//		cerr << "BUFFER-CLIENTI: " << buffer << endl;
		//TODO put into buffer, send to client for printing
		// and treat case for "quit", to remove client from vector
	}


}

int main(int argc, char *argv[])
{
	int portno, clilen;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i;

	if (argc < 2)
	{
		fprintf(stderr,"Usage : %s port\n", argv[0]);
		exit(1);
	}

	//golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error((char *)"ERROR opening socket at server");

	portno = atoi(argv[1]);

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
		error((char *)"ERROR on binding");

	listen(sockfd, MAX_CLIENTS);

	//adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	//adaugam stdin in multimea read_fds
	FD_SET(fileno(stdin), &read_fds);

	// main loop
	while (1)
	{

		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error((char *)"ERROR in select");

		for(i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &tmp_fds))
			{

				if (i == fileno(stdin))
				{
					// citesc de la tastatură o comandă
					memset(buffer, 0 , BUFLEN);
					fgets(buffer, BUFLEN-1, stdin);

					parse_command(buffer);
				}

				else if (i == sockfd)
				{
					// a venit ceva pe socketul de ascultare = o nouă conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen)) == -1)
					{
						error((char *)"ERROR in accept");
					}
					else
					{
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax)
						{
							fdmax = newsockfd;
						}
					}
					fprintf(stderr, "Noua conexiune de la %s, port %d, socket_client %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}

				else
				{
					// am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiunea serverului: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0)
					{
						if (n == 0)
						{
							//conexiunea s-a inchis
							printf("selectserver: socket %d hung up\n", i);
						} else
						{
							error((char *)"ERROR in recv");
						}
						close(i);
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul
					}

					else
					{ //recv intoarce >0
						fprintf (stderr, "Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);

						char comanda[CMDSZ];
						sscanf(buffer,"%s %*s", comanda);
						cout << "COMANDA: " << comanda << endl;

						parse_message(buffer, comanda);
					}
				}
			}
		}
	}


	close(sockfd);

	return 0;
}
