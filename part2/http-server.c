#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

static void die(const char *s) { perror(s); exit(1); }


int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: %s <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
        exit(1);
    }
	
	unsigned short server_port = atoi(argv[1]);
	char *web_root = argv[2];
	char *host = argv[3];
	if (!strcmp(host, "localhost")){
		host = "127.0.0.1";
	}
	unsigned short mdb_port = atoi(argv[4]);
	


	//------------------------(b)----------------------------
    // Create a socket for connection to mdb-lookup-server
	
	int mdb_sock; // socket descriptor
    if ((mdb_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        die("socket failed");

    // Construct a server address structure

    struct sockaddr_in servaddr2;
    memset(&servaddr2, 0, sizeof(servaddr2)); // must zero out the structure
    servaddr2.sin_family      = AF_INET;
    servaddr2.sin_addr.s_addr = inet_addr(host);
    servaddr2.sin_port        = htons(mdb_port); // must be in network byte order

    // Establish a TCP connection to the server 

    if (connect(mdb_sock, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0)
        die("connect failed");
	
	//------------------------------------------------------
	


	// Create a listening socket (also called server socket) 

    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");
    
    // Construct local address structure
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    servaddr.sin_port = htons(server_port);

    // Bind to the local address

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("bind failed");

    // Start listening for incoming connections

    if (listen(servsock, 5 /* queue size for connection requests */ ) < 0)
        die("listen failed");

    int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr;

	while (1){
				
		// Accept an incoming connection

        clntlen = sizeof(clntaddr); // initialize the in-out parameter

        if ((clntsock = accept(servsock,(struct sockaddr *) &clntaddr, &clntlen)) < 0){
            die("accept failed");
		}
		
		// Get stuff

	    FILE *request = fdopen(clntsock, "rb");

	    // Process the first line
		
		char firstLine[1000];
		fgets(firstLine, 1000, request);
		//printf("TEST FIRST LINE: %s", firstLine);
		//fclose(request);
		// Parse the request line

		char *token_separators = "\t \r\n"; // tab, space, new line
		char *method = strtok(firstLine, token_separators);
		char *requestURI = strtok(NULL, token_separators);
		char *httpVersion = strtok(NULL, token_separators);
		
		// Handle exceptions

		if (method == NULL || requestURI  == NULL || httpVersion == NULL){
			fclose(request);
			close(clntsock);
			continue;
		}

		char response[1000];

		//printf("TEST TOKEN: [%s] -- [%s] -- [%s]\n", method, requestURI, httpVersion);
				
		// Check GET
		if (!strcmp(method, "GET")){
			
		} else {
			//printf("GET Err\n");
			printf("%s \"%s %s %s\" 501 Not Implemented\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
			sprintf(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html>\r\n<body>\r\n<h1>\r\n501 Not Implemented</h1>\r\n</body>\r\n</html>\r\n");
			send(clntsock, response, strlen(response), 0);
			fclose(request);
			close(clntsock);
			continue;
		}
		
		// Check URI

		if (requestURI[0] == '/' && !strstr(requestURI, "/..")){
			
		} else {
			//printf("URI Err\n");
			printf("%s \"%s %s %s\" 400 Bad Request\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
			sprintf(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html>\r\n<body>\r\n<h1>\r\n400 Bad Request\r\n</h1>\r\n</body>\r\n</html>\r\n");
            send(clntsock, response, strlen(response), 0);
			fclose(request);
			close(clntsock);
			continue;
		}
		
		// Check HTTP 1.0

        if (!strcmp(httpVersion, "HTTP/1.1") || !strcmp(httpVersion, "HTTP/1.0")){
          	
        } else {
            //printf("HTTP Ver Err\n");
			printf("%s \"%s %s %s\" 400 Bad Request\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
            sprintf(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html>\r\n<body>\r\n<h1>\r\n400 Bad Request\r\n</h1>\r\n</body>\r\n</html>\r\n");
            send(clntsock, response, strlen(response), 0);
			fclose(request);
			close(clntsock);
			continue;
        }
		
		//------------------------------b---------------------------
		// Check if mdb-lookup or not
		//printf("URI: %s %d\n", requestURI, !strcmp(requestURI, "/mdb-lookup"));
		
				
		if (strcmp(requestURI, "/mdb-lookup") == 0){
			// Send the success message
			sprintf(response, "HTTP/1.0 200 OK\r\n\r\n<html><body>\r\n<h1>mdb-lookup</h1>\r\n<p>\r\n<form method=GET action=/mdb-lookup>\r\nlookup: <input type=text name=key>\r\n<input type=submit>\r\n</form>\r\n<p>\r\n</body></html>\r\n");
			//printf("%s\n", response);
			send(clntsock, response, strlen(response), 0);
			
			// Print out the success message
			printf("%s \"%s %s %s\" 200 OK\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);

			fclose(request);
			close(clntsock);
			continue;
		}
		
		// Get result
		
		char *temp = strstr(requestURI, "/mdb-lookup?key=");
		if (temp != NULL){
			// Send the key to mdb server
			char *key = requestURI + 16 * sizeof(char);
			printf("looking up [%s]: %s \"%s %s %s\" 200 OK\n", key, inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
			send(mdb_sock, key, strlen(key), 0);
			send(mdb_sock, "\n", 1, 0);

			// Send the start
			sprintf(response, "HTTP/1.0 200 OK\r\n\r\n<html><body>\r\n<h1>mdb-lookup</h1>\r\n<p>\r\n<form method=GET action=/mdb-lookup>\r\nlookup: <input type=text name=key>\r\n<input type=submit>\r\n</form>\r\n<p>\r\n<p><table border>\r\n");
			send(clntsock, response, strlen(response), 0);
			// Send the content
			FILE *result = fdopen(mdb_sock, "rb");
			char buffer[1000];
			int i = 0;
			fgets(buffer, 1000, result);
			while (buffer[0] != '\n'){
				//printf("i: %d", i);
				char content[1000];
				if ((i % 2) == 0){
					sprintf(content, "<tr><td>%s\r\n", buffer);
				} else {
					sprintf(content, "<tr><td bgcolor=yellow>%s\r\n", buffer);
				}
				//printf("%s", content);
				send(clntsock, content, strlen(content), 0);
				fgets(buffer, 1000, result);
				i++;
			}
			// Send the end 
			char ending[27];
			sprintf(ending, "</table>\r\n</body></html>\r\n");
			send(clntsock, ending, strlen(ending),0);

			// Cleanup
			fclose(request);
			close(clntsock);
            continue;	
		} 
		
		//------------------------------------------------------------


		
		// Check the request file path

		char filePath[1000];
		sprintf(filePath, "%s%s", web_root, requestURI);
		
		struct stat st;
		if (stat(filePath, &st) == 0) { // Path exists
			int dir = S_ISDIR(st.st_mode); // Is a dir?
			int reg = S_ISREG(st.st_mode); // Is a reg?
			if (dir == 1 && reg == 0){ // If the path is a directory
				printf("TEST LAST CHAR: [%c]\n", requestURI[strlen(requestURI)-1]);
				if (requestURI[strlen(requestURI)-1] != '/'){ // If the path is directory but does not have '/' at the end
        			printf("%s \"%s %s %s\" 501 Not Implemented\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
            		sprintf(response, "HTTP/1.0 501 Not Implemented\r\n\r\n<html>\r\n<body>\r\n<h1>\r\n501 Not Implemented\r\n</h1>\r\n</body>\r\n</html>\r\n");
            		send(clntsock, response, strlen(response), 0);
            		close(clntsock);
					continue;
				} else { // If the path is directory and has '/' at the end
					sprintf(filePath, "%s%sindex.html", web_root, requestURI); // Add index.html to the end

					FILE * file;
					file = fopen(filePath, "r");
		
					// Send the success message
					sprintf(response, "HTTP/1.0 200 OK\r\n\r\n");
					send(clntsock, response, strlen(response), 0);

					// Print out the success message
					printf("%s \"%s %s %s\" 200 OK\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);

					// Send the file
					char buffer[4096];
					int i;
    				while (1){
        				i = fread(buffer, 1, 4096, file);
						send(clntsock, buffer, i, 0);
						//printf("sob: %d", sizeof(buffer));
        				//fwrite(buffer, 1, i, fp);
        				if (i < 4096) {
							sprintf(buffer, "\r\n");
							send(clntsock, buffer, strlen(buffer), 0);
							break;
						}
					}
				// Close the file
				fclose(file);

				}
			} else if (dir == 0 && reg == 1){ // If the path is a file
				FILE * file;
				file = fopen(filePath, "r");
		
				// Send the success message
				sprintf(response, "HTTP/1.0 200 OK\r\n\r\n");
				send(clntsock, response, strlen(response), 0);

				// Print out the success message
				printf("%s \"%s\" 200 OK\n", inet_ntoa(clntaddr.sin_addr), firstLine);

				// Send the file
				char buffer[4096];
				int i;
    			while (1){
        			i = fread(buffer, 1, 4096, file);
					send(clntsock, buffer, i, 0);
					//printf("sob: %d", sizeof(buffer));
        			//fwrite(buffer, 1, i, fp);
        			if (i < 4096) {
						sprintf(buffer, "\r\n");
						send(clntsock, buffer, strlen(buffer), 0);
						break;
					}
				}
				// Close the file
				fclose(file);
			} else{ // Neither file or dir, bad request
				printf("%s \"%s %s %s\" 400 Bad Request\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
            	sprintf(response, "HTTP/1.0 400 Bad Request\r\n\r\n<html>\r\n<body>\r\n<h1>\r\n400 Bad Request\r\n</h1>\r\n</body>\r\n</html>\r\n");
            	send(clntsock, response, strlen(response), 0);
				//send(clntsock, "\n", 1, 0);
				char result[100];
				fread(result, 1, 100, 0);
				printf("Result: %s\n", result);
				fclose(request);
				close(clntsock);
				continue;
			}
		} else { // Path does not exist, 404
			printf("%s \"%s %s %s\" 404 Not Found\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
            sprintf(response, "HTTP/1.0 404 Not Found\r\n\r\n<html>\r\n<body>\r\n<h1>\r\n404 Not Found\r\n</h1>\r\n</body>\r\n</html>\r\n");
			send(clntsock, response, strlen(response), 0);
			fclose(request);
            close(clntsock);
			continue;
		}

		

		//Clean up
		fclose(request);
		close(clntsock);
	}

	//Clean up
	close(mdb_sock);
}
