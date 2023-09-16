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
#include <unordered_map>
#include <cassert> 

using namespace std;

#define SERVER_PORT 2233
#define MAX_PENDING 5
#define MAX_LINE 256
#define MAX_USERS 4

struct user {
  char* user;
  char* password;
};

unordered_map<string, string> user_credentials = {
  {"bob", "password"},
  {"alice", "secretkey"},
  {"marshall", "sudormf/"},
  {"august", "notsecure"}
};


class Server {
  public:
    int num_messages;
    int current_message_idx;
    int current_message_size;
    bool authenticated;
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
      authenticated = false;
    }

    void addMotd(char* buf) {
      string message(buf);
      messages.push_back(message);
      num_messages++;
    }

    void storeMessage(char* buf) {
      addMotd(buf);
      string confirmed = "200-OK\n, message stored\n";
      char* response = const_cast<char*>(confirmed.c_str());
      memcpy(buf, response, MAX_LINE);
    }

    // if user has authenticated and logged in, return true else false
    bool validateUser(char* buf) {      
      if (getAuthenticated() == false) {
          string not_authorized = "401-\nYou are not currently logged in, login first\n";
          char* failure = const_cast<char*>(not_authorized.c_str());
          memcpy(buf, failure, MAX_LINE);
          return false;
      }
      string authorized = "200-OK\n, user logged in\n";
      char* success = const_cast<char*>(authorized.c_str());
      memcpy(buf, success, MAX_LINE);
      return true;
    }

    void buildMessage(char* buf) {
      string res_code = "200-OK\n";
      char* msg = const_cast<char*>(getMessageOfTheDay().c_str());
      char* response = const_cast<char*>(res_code.c_str());
      strcat(response, msg);
      strcpy(buf, response);
      setMessageSize(msg, response);
    }

    void shutdown(char* buf, int socket) {
      cout << "Shutdown" << endl;
      string authorized = "200-OK\nShutting server down\n";
      char* success = const_cast<char*>(authorized.c_str());
      memcpy(buf, success, MAX_LINE);
      send (socket, buf, sizeof(buf) + 1, 0);
      close(socket);
      exit(0);
    }

    void login(char* args, char* buf) {
      char* pch;
      pch = strtok(args," ");
      int counter = 0;
      vector<string> credentials;
      while (pch != NULL) {
          if (counter == 0) {
            counter++;
            pch = strtok(NULL, " ");
            continue;
          }
          else if (counter == 1) {
            // add user to beginning of credentials (len 2)
            string user(pch);
            credentials.insert(credentials.begin(), user);
          }
          else if (counter == 2) {
            // add password to end of credentials (len 2)
            string password(pch);
            credentials.push_back(password);
          }
          else {
            cout << "Too many arguments provided" << endl;
            break;
          }
          pch = strtok(NULL, " ");
          counter++;
      }

      assert(credentials.size() == 2);

      if (authenicateUser(credentials[0], credentials[1])) {
          string success_str = "200-OK\nAuthentication success";
          char* success = const_cast<char*>(success_str.c_str());
          memcpy(buf, success, MAX_LINE);
          setAuthenticated(true);
      }
      else {
          string fail_str = "410\nWrong UserID or Password\nAuthentication failed\n";
          char* failure = const_cast<char*>(fail_str.c_str());
          memcpy(buf, failure, MAX_LINE);
        }
    }

    bool authenicateUser(string user, string password) {
      if (user_credentials[user] == password) {
        return true;
      }
      else {
        return false;
      }
    }

    void logout(char* buf) {
      string res_code = "200 - OK";
      char* response = const_cast<char*>(res_code.c_str());
      memcpy(buf, response, MAX_LINE);
      setAuthenticated(false);
    }

    void quit(char* buf) {
      string res_code = "200-OK\nLogging out...\n";
      strcpy(buf, res_code.c_str());
    }

    void setMessageSize(char* msg, char* response) {
      current_message_size = (sizeof(msg)/sizeof(msg[0])) + (sizeof(response)/sizeof(response[0]));
    }

    int getMessageSize() {
    return current_message_size;
    }

    void setAuthenticated(bool flag) {
      authenticated = flag;
    }

    bool getAuthenticated() {
      return authenticated;
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
      // copy contents of buf to new buffer as tokenization below modifies the buffer
      char response[MAX_LINE] = "";
      char unmodified_buf[MAX_LINE];
      memcpy(unmodified_buf, buf, MAX_LINE);

      // strtok modifies buf by inserting terminating chars \000
      char* token = strtok(buf, " ");
      string command(token);
      if (command == "MSGGET\n") {
        motd.buildMessage(response);
        send (new_s, response, motd.getMessageSize() + 1, 0);
      }
      else if (command == "QUIT\n") {
        motd.quit(response);
        send (new_s, response, strlen(response) + 1, 0);
        exit(0);
      }
      // a new line is not expected at the end of this command since additional arguments must be provided
      else if (command == "LOGIN") {
        // newline char delimits the end of the command provided by user
        char* args = strtok(unmodified_buf, "\n");
        motd.login(args, buf);
        send (new_s, buf, strlen(buf) + 1, 0);
      }
      else if (command == "LOGOUT\n") {
        motd.logout(response);
        send (new_s, response, strlen(response) + 1, 0);
      }
      else if (command == "MSGSTORE\n") {
        // newline char delimits the end of the command provided by user
        char* args = strtok(unmodified_buf, "\n");
        bool authorized;
        motd.validateUser(buf) ? authorized = true : authorized = false;
        send (new_s, buf, strlen(buf) + 1, 0);
        if (authorized) {
          cout << "Server: waiting for user input..." << endl;
          recv (new_s, buf, sizeof(buf), 0);
          cout << "Server: received message: " << buf << endl;
          motd.storeMessage(buf);
          send (new_s, buf, strlen(buf) + 1, 0);
        }
      }
      else if (command == "SHUTDOWN\n") {
        bool authorized;
        motd.validateUser(buf) ? authorized = true : authorized = false;
        if (authorized == true) {
          motd.shutdown(buf, new_s);
        }
        cout << "in shutdown block " << buf << endl;
        send (new_s, buf, MAX_LINE, 0);
        
      }
      else {
        cout << "Server: Command not recognized" << endl;
      }
      //flush the buffer
      memset(buf, 0, MAX_LINE);
		}

		close(new_s);
    }
} 
 
