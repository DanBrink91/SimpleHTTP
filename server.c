#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

void error(char *msg)
{
	perror(msg);
	exit(1);
}

typedef struct{
	char* host;
	char* connection;
	char* cache_control;
	char* user_agent;
	char* accept;
	char* accept_encoding;
	char* accept_language;
} headerInfo;

enum operations
{
	GET,
	POST,
	HEAD,
	UNKNOWN
};

typedef struct{
	enum operations operation;
	char* parameter;
	char* version;
	headerInfo header;
} Request;

Request getRequestObject(char* request);
void printRequestObject(Request r);
char* handleRequest(Request r);

int main(int argc, char *agv[])
{
	int sockfd, newsockfd, portno, clilen, n;
	char buffer[1023];
	struct sockaddr_in serv_addr, cli_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	
	bzero((char *)&serv_addr, sizeof(serv_addr));
	portno = 80;

	serv_addr.sin_family = AF_INET;
	// convert to network bye order
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd, (struct sockaddr *)&serv_addr,
	 sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd, 5);
	char* response = "HTTP/1.0 200 OK\nContent-Type: text/html\nContent-Length: 38\n\n<html><body><p>hello</p></body></html>";
	while(1)
	{
		clilen = sizeof(cli_addr);
		//  block until client succesfully connects to server
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);		
		if(newsockfd < 0)
			error("ERROR on accept");

		bzero(buffer, 1023);
		n = read(newsockfd, buffer, 1023);
		if(n < 0) error("ERROR reading from socket");
		printf("%s\n", buffer);
		Request r = getRequestObject(buffer);
		printRequestObject(r);
		response = handleRequest(r);
		printf("%s\n", response);
		n = write(newsockfd, response, strlen(response));
		if (n < 0) error("ERROR writing to socket");
	}
	return 0;
}

Request getRequestObject(char* request)
{
	Request r;
	char headerKey[1024], headerValue[1024];

	char* token = strtok(request, "\n");
	char* init = token;
	//printf("Initial: %s\n\n", token);
	token = strtok(NULL, "\n");
	while(token != NULL)
	{
		//  Location of : in header
		char* colonLocation = strchr(token, ':');
		if(colonLocation==NULL)
			break;
		int char_size = strlen(token) - strlen(colonLocation);

		strncpy(headerKey, token, char_size);
		strcpy(headerValue, colonLocation+2);
		// Fill in request object
		if(strcmp(headerKey, "Host")==0)
		{
			r.header.host = (char*)malloc(sizeof(char) * (strlen(headerValue)));
			strcpy(r.header.host, headerValue);
		}
		else if(strcmp(headerKey, "Connection")==0)
		{
			r.header.connection = (char*)malloc(sizeof(char) * (strlen(headerValue)));
			strcpy(r.header.connection, headerValue);
		}
		else if(strcmp(headerKey, "Accept")==0)
		{
			r.header.accept = (char*)malloc(sizeof(char) * (strlen(headerValue)));
			strcpy(r.header.accept, headerValue);
		}
		else if(strcmp(headerKey, "User-Agent")==0)
		{
			r.header.user_agent = (char*)malloc(sizeof(char) * (strlen(headerValue))+1);
			strcpy(r.header.user_agent, headerValue);
		}
		else if(strcmp(headerKey, "Accept-Encoding")==0)
		{
			r.header.accept_encoding = (char*)malloc(sizeof(char) * (strlen(headerValue)+1));
			strcpy(r.header.accept_encoding, headerValue);
		}
		else if(strcmp(headerKey, "Accept-Language")==0)
		{
			r.header.accept_language = (char*)(malloc(sizeof(char) * (strlen(headerValue)+1)));
			strcpy(r.header.accept_language, headerValue);
		}
		//printf("Header-Key: %s\nHeader-Value:%s\n\n",headerKey, headerValue);
		token = strtok(NULL, "\n");

		bzero(headerKey, sizeof(headerKey));
		bzero(headerValue, sizeof(headerValue));
	}
	init = strtok(init, " ");
	if(strcmp(init, "GET")==0)
	{
		r.operation = GET;
	}
	else if(strcmp(init, "POST")==0)
	{
		r.operation = POST;
	}
	else if(strcmp(init, "HEAD")==0)
	{
		r.operation = HEAD;
	}
	else
	{
		r.operation = UNKNOWN;
	}
	//  Get the parameter
	init = strtok(NULL, " ");
	r.parameter = (char *)malloc(sizeof(char) * (strlen(init) + 1));
	strcpy(r.parameter, init);
	//  Get the HTTP version info
	init = strtok(NULL, " ");
	r.version = (char *)malloc(sizeof(char) * (strlen(init) + 1));
	strcpy(r.version, init);

	return r;
}

void printRequestObject(Request r)
{
	switch(r.operation)
	{
		case GET:
			printf("GET ");
			break;
		case POST:
			printf("POST ");
			break;
		case HEAD:
			printf("HEAD ");
			break;
	}
	printf("%s %s\n", r.parameter, r.version);
	//  Now print out whatever header info we have
	if(strlen(r.header.host) > 0)
	{
		printf("Host: %s\n", r.header.host);
	}
	if(strlen(r.header.connection) > 0)
	{
		printf("Connection: %s\n", r.header.connection);
	}
	if(strlen(r.header.accept) > 0)
	{
		printf("Accept: %s\n", r.header.accept);
	}
	if(strlen(r.header.user_agent) > 0)
	{
		printf("User-Agent: %s\n", r.header.user_agent);
	}
}

char* handleRequest(Request r)
{
	char* header = 0;
	char* body = 0;
	unsigned long bodyLength;
	//  Send headers

	//  Send body
	if(r.operation==GET || r.operation==POST)
	{
		//  Look for an index file
		if(strcmp(r.parameter, "/")==0)
		{
			
			return "HTTP/1.0 200 OK\nContent-Type: text/html\nContent-Length: 38\n\n<html><body><p>hello</p></body></html>";
		}
		//  Grab what they ask for
		else
		{
			FILE* file = fopen(r.parameter+1, "r");
			if(file)
			{
				
				fseek(file, 0, SEEK_END);
				bodyLength = ftell(file);
				fseek(file, 0, SEEK_SET);
				body = (char*)malloc(bodyLength);
				fread(body, 1 , bodyLength, file);
				fclose(file);
			}
			else
			{
				//  File not found.
				return "HTTP/1.0 404 Not Found";
			}
		}
	}
	printf("This part might messup.\n");
	char *response = (char*)malloc(sizeof(body) * 2);
	sprintf(response, "HTTP/1.0 200 OK\nContent=Type: text/html\nContent-Length: %lu\n\n%s", bodyLength, body);
	return response;
}