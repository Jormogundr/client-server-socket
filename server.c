/*
 * server.c
 */

#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <string>

using namespace std;

#define SERVER_PORT 2233
#define MAX_PENDING 5
#define MAX_LINE 256


class Server {
  public:
    int num_messages;
    int current_message_idx;
    int current_message_size;
    vector<string> messages {"message1", "message2", "message3"};

    // return message of the day as string
    string getMessageOfTheDay() {

      // check if we are at the end of the 0-indexed vector of messages, and wrap back to 0 as needed
      if (current_message_idx < num_messages - 1)
          current_message_idx++;
      else 
        current_message_idx = 0;

      string msg = messages[current_message_idx];
      current_message_size = msg.size();

      return msg;
    }

    // constructor
    Server() {
      current_message_idx = 0;
      num_messages = messages.size();
    }

    void buildMessage(char* buf) {
      string res_code = "200-OK\n";
      char* msg = const_cast<char*>(getMessageOfTheDay().c_str());
      char* response = const_cast<char*>(res_code.c_str());
      strcat(response, msg);
      strcpy(buf, response);
      setMessageSize(msg, response);
    }

    void quit(char* buf) {
      string res_code = "200-OK\nLogging out...";
      strcpy(buf, res_code.c_str());
    }

    void setMessageSize(char* msg, char* response) {
      current_message_size = (sizeof(msg)/sizeof(msg[0])) + (sizeof(response)/sizeof(response[0]));
    }

    int getMessageSize() {
    return current_message_size;
    }
};



int main(int argc, char **argv) {

    struct sockaddr_in sin;
    socklen_t addrlen;
    char buf[MAX_LINE];
    int len;
    int s;
    int new_s;

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (SERVER_PORT);

    /* setup passive open */
    if (( s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
    }

    // set re-usable address option so that the address is not busy when re-running the server app
    int set_reusable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &set_reusable, sizeof(int)) < 0 ) {
        perror("setsockopt");
        exit(1);
    }

    if ((bind(s, (struct sockaddr *) &sin, sizeof(sin))) < 0) {
		perror("bind");
		exit(1);
    }

    listen (s, MAX_PENDING);

    addrlen = sizeof(sin);
    cout << "The server is up, waiting for connection" << endl;

    /* wait for connection, then receive and print text */
    while (1) {
		if ((new_s = accept(s, (struct sockaddr *)&sin, &addrlen)) < 0) {
			perror("accept");
			exit(1);
		}
		cout << "new connection from " << inet_ntoa(sin.sin_addr) << endl;

     
    Server motd;
		while (len = recv(new_s, buf, sizeof(buf), 0)) {
      string command(buf);

      // it appears that we need corresponding send/receive pairs in both the client and server depending on the command
			// send (new_s, buf, strlen(buf) + 1, 0);

      char msg[MAX_LINE] = "";
      if (command == "MSGGET\n") {
        motd.buildMessage(msg);
        send (new_s, msg, motd.getMessageSize() + 1, 0);
      }
      else if (command == "QUIT\n") {
        motd.quit(msg);
        send (new_s, msg, strlen(msg), 0);
        exit(0);
      }
      else {
        cout << "Command not recognized" << endl;
      }
		}

		close(new_s);
    }
} 
 
