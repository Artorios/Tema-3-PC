#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#define BUFLEN 256
#define MAX_CLIENTS 5
#define CMDSZ 100
#define NAMESZ 100

using namespace std;
int sockfd, newsockfd;
int listen_sockfd;

int fdmax;			//valoare maxima file descriptor din multimea read_fds

fd_set read_fds;	//multimea de citire folosita in select()
fd_set tmp_fds;		//multime folosita temporar

typedef struct client_
{
	string nume;	// Nume client
	int port;		// Port ascultare
	struct sockaddr_in adresa;	// Adresa clientului
}client;

vector<client> clienti;

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

void send_verify(int n)
{
	if (n < 0)
		error((char *)"ERROR writing to socket");
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

bool parse_quick_reply(char *mesaj, int sock)
{
	char buffer[BUFLEN];
	int n;
	memset(buffer, 0, BUFLEN);
	if ((n = recv(sock, buffer, sizeof(buffer), 0)) <= 0)
	{
		if (n == 0)
		{
			//conexiunea s-a inchis
			printf("client: when parsing recieved message, socket hung up\n");
		} else
		{
			error((char *)"ERROR in recv");
		}
		close(sock);
		error((char *)"Problem in connection with server. Closing client...");
	}
	else
	{
		cerr << "Reply-from-srv-BUFFER " << buffer << endl;
		char comanda[CMDSZ];
		sscanf(buffer, "%s %*s", comanda);
		cerr << "Comanda primita: {" << comanda << "}\n";

		if (strcmp(comanda, "info-message-NOTOK") == 0)
		{
			cerr << "client: Error clientul " << buffer + strlen("info-message-NOTOK ") << endl;
			return false;
		}
		if (strcmp(comanda, "info-message") == 0)
			return false;
	}
	return false;
}

// Parsare comanda de la tastatura
bool parse_command(char *buffer, char *nume)
{
	string comanda (buffer);
	string com, param1, param2;
	char bufsend[BUFLEN];

	comanda = comanda.substr(0,comanda.find_last_of("\n"));
	split_string(comanda, com, param1, param2);

	// Comanda "listclients"
	if (comanda.compare("listclients") == 0)
	{
		if (param1.compare("") != 0)
		{
			fprintf(stderr, "Wrong command. Usage: listclients\n");
			return false;
		}

		// Trimit un string de forma "listclients nume_client_initiator"
		memset(bufsend, 0, BUFLEN);
		sprintf(bufsend, "listclients %s", nume);

		int n = send(sockfd, bufsend, strlen(bufsend), 0);
		send_verify(n);

		fprintf(stderr, "client: Am trimis comanda listclients catre server\n");
		return true;
	}

	// Comanda "infoclient nume_client"
	if (com.compare("infoclient") == 0)
	{
		if ( (param1.compare("") == 0 && param2.compare("") != 0)
				|| comanda.compare("infoclient ") == 0 || comanda.compare("infoclient") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: infoclient nume_client\n");
			return false;
		}
		fprintf(stderr, "Am primit comanda infoclient pentru ");
		cerr << param1 << endl;

		// Trimit un string de forma "infoclient nume_client_initiator
		// nume_client_despre_care_se_cer_info"
		memset(bufsend, 0, BUFLEN);
		sprintf(bufsend, "infoclient %s %s", nume, param1.c_str());

		int n = send(sockfd, bufsend, strlen(bufsend), 0);
		send_verify(n);

		cerr << "client: Am trimis comanda infoclient " << param1 << " catre server\n";

		return true;
	}

	// TODO Comanda "message nume_client mesaj"
	if (com.compare("message") == 0)
	{
		if ( (param1.compare("") == 0 || param2.compare("") == 0)
				|| comanda.compare("message") == 0 || comanda.compare("message ") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: message nume_client mesaj\n");
			return false;
		}
		char mesaj[BUFLEN];
		string buf = buffer;
		string msgstr = buf.substr(com.size() + param1.size() + 2);
		//cerr << "MSGSTR {" << msgstr << "}\n";

		fprintf(stderr, "Am primit comanda message pentru clientul ");
		cerr << param1 << " \n";

//		cerr << "BUFFER-message {" << buffer << "}\n";
		msgstr.copy(mesaj, msgstr.size() - 1);
		mesaj[msgstr.size() - 1] = '\0';
		cerr << "MESAJ: {" << mesaj << "}\n";

		// Trimit la server un string de forma
		// "message nume_client_initiator nume_client mesaj"
		memset(bufsend, 0, BUFLEN);
		sprintf(bufsend, "message %s %s", nume, param1.c_str());

//		cerr << "BUFSEND-message-to-srv {" << bufsend << "}\n";

		int n = send(sockfd, bufsend, strlen(bufsend), 0);
		send_verify(n);

		cerr << "client: Am trimis comanda " << bufsend << " catre server\n";

		return parse_quick_reply(mesaj, sockfd);
	}

	// Comanda "sharefile nume_fisier"
	if (com.compare("sharefile") == 0)
	{
		if ( (param1.compare("") == 0 && param2.compare("") != 0)
				|| comanda.compare("sharefile") == 0 || comanda.compare("sharefile ") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: sharefile nume_fisier\n");
			return false;
		}
		fprintf(stderr, "Am primit comanda sharefile pentru fisierul ");
		cerr << param1 << "\n";

		ifstream fisier;
		fisier.open(param1.c_str());
		if (!fisier.is_open())
		{
			cerr << "client: Fisierul nu poate fi deschis\n";
			return false;
		}
		else
			fisier.close();

		// Trimit mesaj catre server de forma
		// "sharefile nume_client_initiator nume_fisier"
		memset(bufsend, 0, BUFLEN);
		sprintf(bufsend, "sharefile %s %s", nume, param1.c_str());

		int n = send(sockfd, bufsend, strlen(bufsend), 0);
		send_verify(n);

		cerr << "client: Am trimis comanda " << bufsend << " catre server\n";

		return true;
	}

	// Comanda "unsharefile nume_fisier"
	if (com.compare("unsharefile") == 0)
	{
		if ( (param1.compare("") == 0 && param2.compare("") != 0)
				|| comanda.compare("unsharefile") == 0 || comanda.compare("unsharefile ") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: unsharefile nume_fisier\n");
			return false;
		}
		fprintf(stderr, "Am primit comanda unsharefile pentru fisierul ");
		cerr << param1 << "\n";

		// Trimit mesaj catre server de forma
		// "unsharefile nume_client_initiator nume_fisier"
		memset(bufsend, 0, BUFLEN);
		sprintf(bufsend, "unsharefile %s %s", nume, param1.c_str());

		int n = send(sockfd, bufsend, strlen(bufsend), 0);
		send_verify(n);

		cerr << "client: Am trimis comanda " << bufsend << " catre server\n";
		return true;
	}

	// Comanda "getshare nume_client"
	if (com.compare("getshare") == 0)
	{
		if ( (param1.compare("") == 0 && param2.compare("") != 0)
				|| comanda.compare("getshare") == 0 || comanda.compare("getshare ") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: getshare nume_client\n");
			return false;
		}
		fprintf(stderr, "Am primit comanda getshare pentru clientul ");
		cerr << param1 << "\n";

		// Trimit mesaj catre server de forma
		// "getshare nume_client_initiator nume_client_interogat"
		memset(bufsend, 0, BUFLEN);
		sprintf(bufsend, "getshare %s %s", nume, param1.c_str());

		int n = send(sockfd, bufsend, strlen(bufsend), 0);
		send_verify(n);

		cerr << "client: Am trimis comanda " << bufsend << " catre server\n";
		return true;
	}

	// TODO Comanda "getfile nume_client nume_fisier"
	if (com.compare("getfile") == 0)
	{
		if ( (param1.compare("") == 0 || param2.compare("") == 0)
						|| comanda.compare("getfile") == 0 || comanda.compare("getfile ") == 0)
		{
			fprintf(stderr, "Wrong command. Usage: getfile nume_client nume_fisier\n");
			return false;
		}
		fprintf(stderr, "Am primit comanda getfile pentru clientul ");
		cerr << param1 << " si fisierul " << param2 << "\n";
		return true;
	}

	// Comanda "quit"
	if (comanda.compare("quit") == 0)
	{
		quit_message();
		close(sockfd);
		close(listen_sockfd);
		exit(0);
	}

	fprintf(stderr, "Wrong command. Usage: command [param1] [param2]\n");
	return false;

}

void parse_message(int sock)
{
	char buffer[BUFLEN];
	char comanda[CMDSZ];
	int n;
	memset(buffer, 0, BUFLEN);
	if ((n = recv(sock, buffer, sizeof(buffer), 0)) <= 0)
	{
		if (n == 0)
		{
			//conexiunea s-a inchis
			printf("client: when parsing recieved message, socket hung up\n");
		} else
		{
			error((char *)"ERROR in recv");
		}
		close(sock);
		error((char *)"Problem in connection with server. Closing client...");
	}
	else
	{
		cerr << "Recv-BUFFER " << buffer << endl;
		stringstream ss (stringstream::in | stringstream::out);
		string buf = buffer;
		ss << buf;
		ss >> comanda;
		cerr << "Comanda: " << comanda << endl;

		// Am primit lista de clienti sub forma: "clienti client1 client2..."
		if (strcmp(comanda, "clienti") == 0)
		{
			cout << "LISTA de clienti este:\n" << buffer + strlen("clienti ") << endl;
			return;
		}

		// Am primit client-inexistent ca reply la clientinfo
		if (strcmp(comanda, "client-inexistent") == 0)
		{
			cout << "Clientul interogat nu exista\n";
			return;
		}

		// Am primit informatiile despre clientul interogat
		if (strcmp(comanda, "info-client") == 0)
		{
			char nume[NAMESZ];
			char ip[NAMESZ];
			int port;
			double dif;
			sscanf(buffer, "%*s %s %d %lf %s", nume, &port, &dif, ip);
			cout << "Info despre " << nume << " cu portul " << port;
			cout << " conectat de " << dif << " secunde la ip-ul " << ip << endl;

			return;
		}

		// Am primit shared ok
		if (strcmp(comanda, "shared_OK") == 0)
		{
			cout << "client: Fișierul a fost share-uit cu succes\n";
			return;
		}

		// Am primit shared NOTOK
		if (strcmp(comanda, "shared_NOTOK") == 0)
		{
			char nume_fis[BUFLEN];
			sscanf(buffer, "%*s %s", nume_fis);
			cout << "client: Fișierul " << nume_fis << " este deja share-uit\n";
			return;
		}

		// Am primit getshare_NOTOK
		if (strcmp(comanda, "getshare_NOTOK") == 0)
		{
			char nume_client[NAMESZ];
			sscanf(buffer, "%*s %s", nume_client);
			cout << "client: Clientul " << nume_client << " nu se afla pe server\n";
			return;
		}

		// Am primit lista cu fisiere share-uite
		if (strcmp(comanda, "shared-files") == 0)
		{
			cout << "Lista de fisiere este:\n" << buffer + strlen("shared-files ") << endl;
		}
	}
}

int main(int argc, char *argv[])
{
	int i, n, accept_len;
	struct sockaddr_in serv_addr, listen_addr, accept_addr;

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

		// Socket pentru ascultare conexiuni de la alți clienți
	listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sockfd < 0)
		error((char *)"ERROR opening listening socket at client");

	memset((char *) &listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(0);			// Port asignat automat
	listen_addr.sin_addr.s_addr = INADDR_ANY;	// Adresa IP a mașinii

	if (bind(listen_sockfd, (struct sockaddr *) &listen_addr, sizeof(struct sockaddr)) < 0)
		error((char *)"ERROR on binding client's listen_socket");

	listen(listen_sockfd, MAX_CLIENTS);

	// Aflu portul pe care asculta clientul
	socklen_t listen_len = sizeof(listen_addr);
	if (getsockname(listen_sockfd, (struct sockaddr *) &listen_addr, &listen_len) == -1)
		error((char *)"ERROR getting socket name");



//	fprintf(stderr, "%s's listen port number: %d\n", argv[1],ntohs(listen_addr.sin_port));


	// Handshake cu server-ul
	memset(buffer, 0, BUFLEN);
	// connect Nume_client listen_port_lcient
	sprintf(buffer,"connect %s %d", argv[1], ntohs(listen_addr.sin_port) );
	n = send(sockfd,buffer,strlen(buffer), 0);
	if (n < 0)
		 error((char *)"ERROR writing to socket");

	memset(buffer, 0, BUFLEN);
	if ((n = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0)
	{
		if (n == 0)
		{
			//conexiunea s-a inchis
			printf("client: socket with server hung up\n");
		} else
		{
			error((char *)"ERROR in recv");
		}
		close(sockfd);
		error((char *)"Problem in connection with server. Closing client...");
	}
	else
	{
		// Am primit instiintare de disconnect
		if (strncmp(buffer, "disconnected", strlen("disconnected")) == 0)
		{
			cout << "Client " << buffer << endl;
			close(sockfd);
			exit(0);
		}

		// Altfel am primit instiintare de connected
		cout << "Client " << buffer << " successfully to server\n";
	}

	//golim multimea de descriptori de citire (read_fds) si multimea (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	//adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(listen_sockfd, &read_fds);
	fdmax = listen_sockfd;

	//adaugam stdin in multimea read_fds
	FD_SET(fileno(stdin), &read_fds);

	//adaugam file descriptor-ul (socketul pe care e conectat clientul la server) in multimea read_fds
	FD_SET(sockfd, &read_fds);

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

					if ( parse_command(buffer, argv[1]) == true )
						parse_message(sockfd);
				}

				else if (i == sockfd)
				{
					// Am primit ceva de la server
					memset(buffer, 0, BUFLEN);
					if ((n = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0)
					{
						if (n == 0)
						{
							//conexiunea s-a inchis
							printf("client: socket with server hung up\n");
						} else
						{
							error((char *)"ERROR in recv");
						}
						close(sockfd);
						close(listen_sockfd);
						cerr << "Server has quit. Closing client...\n";
						exit(0);
					}
					else
					{
						// Am primit instiintare de disconnect
						if (strncmp(buffer, "disconnected", strlen("disconnected")) == 0)
						{
							cout << "Client " << buffer << endl;
							close(sockfd);
							exit(0);
						}

						// Altfel am primit instiintare de connected
						error((char *)"ERROR n-am primit ce trebuie de la server");
					}
				}

				else if (i == listen_sockfd)
				{
					// a venit ceva pe socketul de ascultare = o nouă conexiune
					// acțiune: accept()
					accept_len = sizeof(accept_addr);
					if ((newsockfd = accept(listen_sockfd, (struct sockaddr *)&accept_addr, (socklen_t *)&accept_len)) == -1)
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
					fprintf(stderr, "Noua conexiune de la %s, port %d, socket_client %d\n", inet_ntoa(accept_addr.sin_addr), ntohs(accept_addr.sin_port), newsockfd);

				}

				else
				{
					// am primit date pe unul din socketii cu care vorbesc cu clientii
					//actiune: recv()
					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0)
					{
						if (n == 0)
						{
							//conexiunea s-a inchis
							printf("client %s: socket %d hung up\n", argv[1], i);
						} else
						{
							error((char *)"ERROR in recv la clientul care așteapta mesaj");
						}
						close(i);
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care l-am folosit
					}

					else
					{ //recv intoarce >0
						fprintf (stderr, "Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
					}
				}
			}
		}



	}

	close(listen_sockfd);
	close(sockfd);
	return 0;
}
