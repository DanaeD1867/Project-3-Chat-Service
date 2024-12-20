
#include "server.h"
#include "list.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/***
Authors: Danae Dunlap, Khandie Anijah-Obi, Le'ale Addis
***/


#define DEFAULT_ROOM "Lobby"

// USE THESE LOCKS AND COUNTER TO SYNCHRONIZE
extern int numReaders;
extern pthread_mutex_t rw_lock;
extern pthread_mutex_t mutex;

extern struct node *head;
extern struct room *rooms; 
extern char *server_MOTD;

struct MessageArgs{
  int socket; 
  char message[1024];
}; 

//Helper functions for chat room commands
struct node *findUserBySocket(struct node *head, int socket); 
void handleCreateRoom(char *roomName, char *buffer, int client); 
void handleJoinRoom(char *roomName, int client, char *buffer); 
void handleListRooms(char *buffer, int client); 
void handleListUsers(char *buffer, int client); 
void handleLeaveRoom(int client, char *roomName); 
void handleConnect(int client, char *targetUser); 
void handleDisconnect(int client, char *targetUser); 
void handleLogin(int client, char *username); 
int insertRoom(const char *roomName);  
void *sendMessage(void *args);


/*
 *Main thread for each client.  Receives all messages
 *and passes the data off to the correct function.  Receives
 *a pointer to the file descriptor for the socket the thread
 *should listen on
 */

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

void *client_receive(void *ptr) {
   int client = *(int *) ptr;  // socket
  
   int received, i;
   char buffer[MAXBUFF], sbuffer[MAXBUFF];  //data buffer of 2K  
   char tmpbuf[MAXBUFF];  //data temp buffer of 1K  
   char cmd[MAXBUFF], username[20];
   char *arguments[80];

   struct node *currentUser;
    
   send(client  , server_MOTD , strlen(server_MOTD) , 0 ); // Send Welcome Message of the Day.

   // Creating the guest user name
  
   sprintf(username,"guest%d", client);
   pthread_mutex_lock(&rw_lock); 
   head = insertFirstU(head,client ,username);
   pthread_mutex_unlock(&rw_lock);
   
   // Add the GUEST to the DEFAULT ROOM (i.e. Lobby)

   while (1) {
       
      if ((received = read(client , buffer, MAXBUFF)) != 0) {
      
            buffer[received] = '\0'; 
            strcpy(cmd, buffer);  
            strcpy(sbuffer, buffer);
         
            /////////////////////////////////////////////////////
            // we got some data from a client

            // 1. Tokenize the input in buf (split it on whitespace)

            // get the first token 

             arguments[0] = strtok(cmd, delimiters);

            // walk through other tokens 

             i = 0;
             while( arguments[i] != NULL ) {
                arguments[++i] = strtok(NULL, delimiters); 
                strcpy(arguments[i-1], trimwhitespace(arguments[i-1]));
             } 

             // Arg[0] = command
             // Arg[1] = user or room
             
             /////////////////////////////////////////////////////
             // 2. Execute command: TODO

            if(strcmp(arguments[0], "create") == 0)
            {
              handleCreateRoom(arguments[1], buffer, client); 
            }
            else if (strcmp(arguments[0], "join") == 0)
            {
               handleJoinRoom(arguments[1], client, buffer); 
            }
            else if (strcmp(arguments[0], "leave") == 0)
            {
              handleLeaveRoom(client, arguments[1]); 
            } 
            else if (strcmp(arguments[0], "connect") == 0)
            {
                handleConnect(client, arguments[1]);
            }
            else if (strcmp(arguments[0], "disconnect") == 0)
            {             
              handleDisconnect(client, arguments[1]); 
            }                  
            else if (strcmp(arguments[0], "rooms") == 0)
            {
                handleListRooms(sbuffer,client);                             
            }   
            else if (strcmp(arguments[0], "users") == 0)
            {
                handleListUsers(sbuffer, client); 
            }                           
            else if (strcmp(arguments[0], "login") == 0)
            {
                handleLogin(client, arguments[1]); 
            } 
            else if (strcmp(arguments[0], "help") == 0 )
            {
                sprintf(buffer, "login <username> - \"login with username\" \ncreate <room> - \"create a room\" \njoin <room> - \"join a room\" \nleave <room> - \"leave a room\" \nusers - \"list all users\" \nrooms -  \"list all rooms\" \nconnect <user> - \"connect to user\" \nexit - \"exit chat\" \n");
                send(client , buffer , strlen(buffer) , 0 ); // send back to client 
            }
            else if (strcmp(arguments[0], "exit") == 0 || strcmp(arguments[0], "logout") == 0)
            {
    
             //Remove the initiating user from all rooms and direct connections, then close the socket descriptor.

                pthread_mutex_lock(&rw_lock);
                printf("Starting user removal process for socket: %d\n", client);
               struct node *current = head; 
                struct node *prev = NULL; 

                while (current != NULL) {
                    printf("Checking socket: %d\n", current->socket);
                    if (current->socket == client) {
                        if (prev == NULL) {
                            printf("Removing head node.\n");
                            head = current->next; 
                        } else {
                              printf("Removing middle node.\n");
                              prev->next = current->next; 
                          }
                          free(current); 
                          printf("User successfully removed\n");
                          pthread_mutex_unlock(&rw_lock);
                      }
                      prev = current;
                      current = current->next;
                  }

                  printf("Socket not found in list.\n");
                  pthread_mutex_unlock(&rw_lock);
                  printf("Client socket successfully closed!\n");
                  fflush(stdout); 
                  close(client);
                  break; 
                  
            }                         
            else { 
                 /////////////////////////////////////////////////////////////
                 // 3. sending a message
           
                 // send a message in the following format followed by the promt chat> to the appropriate receipients based on rooms, DMs
                 // ::[userfrom]> <message>
            
                 
                char senderUsername[50] = "Unknown";
                struct node *senderNode = NULL; 

                 currentUser = head;
                 while(currentUser != NULL){
                  if(client == currentUser->socket){
                    senderNode = currentUser; 
                    strcpy(senderUsername, currentUser->username); 
                    break; 
                  }
                  currentUser = currentUser->next; 
                 }

                if(senderNode == NULL){
                  fprintf(stderr, "Error: Sender not found in user list.\n");
                  return NULL;
                }

                 sprintf(tmpbuf,"\n::%s> %s\nchat>", senderUsername, sbuffer);
                 strcpy(sbuffer, tmpbuf);

                 int i; 
                 for(i = 0; i < senderNode->connectedCount; i++) {
                  struct node *connectedUsers = senderNode->connectedUsers[i]; 
                  pthread_t sendThread;  
                  struct MessageArgs *msgArgs = malloc(sizeof(struct MessageArgs)); 
                  if (msgArgs == NULL) {
                      perror("Failed to allocate memory for message arguments");
                      continue;
                  }

                  msgArgs->socket = connectedUsers->socket; 
                  strcpy(msgArgs->message, sbuffer);
                  strcpy(buffer, "\nchat>");
                  send(client, buffer, strlen(buffer), 0); 
                      
                       
                  // Create a thread for sending the message
                      if (pthread_create(&sendThread, NULL, sendMessage, msgArgs) != 0) {
                          perror("Failed to create thread for message sending");
                          free(msgArgs);
                      }else{
                        pthread_detach(sendThread);
                      }
                      
                }

              }
 
         memset(buffer, 0, sizeof(1024));
      }
   }
   return NULL;
}

int insertRoom(const char *roomName){
  struct room *newRoom = malloc(sizeof(struct room));
  strcpy(newRoom->roomName, roomName);  
  newRoom->next = rooms; 
  newRoom->userList = NULL; 
  rooms = newRoom; 
  printf("Successfully inserted room %s into rooms linked list.\n", newRoom->roomName);
  return 1; 
}

struct room *findRoom(struct room *rooms, const char *roomName){
  struct room *current = rooms; 
  while(current != NULL){
    if(strcmp(current->roomName, roomName) == 0){
      printf("Found room %s\n", current->roomName); 
      return current; 
    }
    current = current->next;
  }
  printf("Couldn't find room %s\n", roomName); 
  return NULL; 
}


void handleCreateRoom(char *roomName, char *buffer, int client){
  printf("Attemptig to create room\n"); 
  pthread_mutex_lock(&rw_lock);

  struct room *foundRoom = findRoom(rooms, roomName); 
  if(foundRoom != NULL){
    printf("Room %s already exists.\n", roomName); 
    sprintf(buffer, "Room '%s' already exist. \n", roomName); 
  } else{
    printf("Room %s not found. Creating...\n", roomName); 
    if(insertRoom(roomName)){
      sprintf(buffer, "Room '%s' created successfully\nchat>", roomName); 
    }else{
      sprintf(buffer, "Error: Failed to create room '%s'\nchat>", roomName); 
    }
  }

  pthread_mutex_unlock(&rw_lock); 
}

void handleJoinRoom(char *roomName, int client, char *buffer){
  pthread_mutex_lock(&rw_lock);
  struct room *room = findRoom(rooms, roomName); 
  struct node *user = findUserBySocket(head, client); 

  if(room == NULL){
    sprintf(buffer, "Room '%s' does not exist.\nchat>", roomName); 
    printf("Couldn't find room '%s'\n", roomName); 
  }else{
    if(user->roomCount == user->roomCapacity){
      user->roomCapacity = (user->roomCapacity == 0) ? 2: user->roomCapacity * 2; 
      user->rooms = realloc(user->rooms, user->roomCapacity * sizeof(char *)); 
      if(!user->rooms){
        perror("Failed to allocate memory for rooms"); 
        pthread_mutex_unlock(&rw_lock); 
        return; 
      }
    }
    user->rooms[user->roomCount] = strdup(roomName); 
    user->roomCount++; 
    sprintf(buffer, "You joined the room '%s'.\nchat>", roomName); 
    printf("Successfully joined user '%s' to room '%s'\n", user->username, roomName);
  }
  pthread_mutex_unlock(&rw_lock); 
  send(client,buffer,strlen(buffer),0); 
}

void handleLeaveRoom(int client, char *roomName){
  pthread_mutex_lock(&rw_lock);
  struct node *user = findUserBySocket(head, client); 
  char buffer[MAXBUFF]; 
  sprintf(buffer, "Left room %s.\nchat>", roomName);
  int found = 0;
  int i; 
  for(i = 0; i < user->roomCount; i++) {
      if (strcmp(user->rooms[i], roomName) == 0) {
          free(user->rooms[i]);
          for (int j = i; j < user->roomCount - 1; j++) {
          user->rooms[j] = user->rooms[j + 1];
          }
        user->roomCount--;
        found = 1; 
        break;
    }
  }

  if (found) {
    sprintf(buffer, "You have successfully left the room '%s'.\nchat>", roomName);
    printf("User '%s' left the room '%s'\n", user->username, roomName);
  } else {
    sprintf(buffer, "You are not in the room '%s'.\nchat>", roomName);
    printf("User '%s' tried to leave a non-joined room '%s'\n", user->username, roomName);
  }
  pthread_mutex_unlock(&rw_lock); 
  send(client, buffer, strlen(buffer), 0);  
}

void handleListRooms(char *buffer, int client){
  pthread_mutex_lock(&rw_lock); 
  struct room *current = rooms; 
  strcpy(buffer, "Available rooms: \n"); 

  while(current != NULL){
    strcat(buffer, current->roomName); 
    strcat(buffer, "\n");
    current = current->next;  
  }

  strcat(buffer, "chat>");
  pthread_mutex_unlock(&rw_lock); 
  send(client, buffer, strlen(buffer), 0); 
}

void *sendMessage(void *args){
  struct MessageArgs *msgArgs = (struct MessageArgs *)args;
  send(msgArgs->socket, msgArgs->message, strlen(msgArgs->message), 0);
  free(msgArgs);  
  return NULL; 
}

void handleListUsers(char *buffer, int client){
  pthread_mutex_lock(&rw_lock);
  struct node *current = head; 
  struct node *currentUser = head;

  while(current != NULL){
    if(current->socket == client){
      currentUser = current; 
      break; 
    }
    current = current->next; 
  } 

  if(currentUser == NULL){
    strcpy(buffer, "Error: User not found.\nchat>"); 
    pthread_mutex_unlock(&rw_lock); 
    send(client, buffer, strlen(buffer), 0); 
    return; 
  }

  strcpy(buffer, "Connected users:\n"); 
  int i; 
  for(i = 0; i < currentUser->connectedCount; i++){
    struct node *connectedUser = currentUser->connectedUsers[i]; 
    strcat(buffer, connectedUser->username); 
    strcat(buffer, "\n");  
  } 

  strcat(buffer, "chat>");
  pthread_mutex_unlock(&rw_lock); 
  send(client, buffer, strlen(buffer), 0); 
}



void updateRoomUser(struct room *room, int client, char *newUsername){
  struct node *roomUser = room->userList; 
  while(roomUser != NULL){
    if(roomUser->socket == client){
      strcpy(roomUser->username, newUsername); 
      break; 
    }
    roomUser = roomUser->next; 
  }

}

void handleLogin(int client, char *newUsername){
  pthread_mutex_lock(&rw_lock); 
  struct node *current = findUserBySocket(head, client);

  if(current == NULL){
    pthread_mutex_unlock(&rw_lock); 
    char errorMsg[MAXBUFF] = "Error: Unable to find user.\nchat>";
    send(client, errorMsg, strlen(errorMsg), 0); 
    return; 
  } 

  if(findU(head, newUsername) != NULL){
    pthread_mutex_unlock(&rw_lock); 
    char errorMsg[MAXBUFF] = "Error: username already taken.\nchat>"; 
    send(client, errorMsg, strlen(errorMsg), 0); 
  }

  char oldName = *current->username; 
  strcpy(current->username, newUsername); 

  struct room *roomPtr = rooms; 
  while(roomPtr != NULL){
    updateRoomUser(roomPtr, client, newUsername); 
    roomPtr = roomPtr->next; 
  }

  char successMsg[MAXBUFF]; 
  sprintf(successMsg, "Welcome, %s! Your username has been updated.\nchat>", newUsername); 
  printf("Successfully updated %d's username to %s \n", oldName, newUsername);
  pthread_mutex_unlock(&rw_lock);
  send(client, successMsg, strlen(successMsg), 0); 

}


void handleConnect(int client, char *targetUser) {
    pthread_mutex_lock(&rw_lock);
    char buffer[MAXBUFF];

    // Find the client and target user nodes
    struct node *clientNode = NULL, *targetNode = NULL;
    struct node *current = head;

    while (current != NULL) {
        if (current->socket == client) {
            clientNode = current;
        }
        if (strcmp(current->username, targetUser) == 0) {
            targetNode = current;
        }
        if (clientNode && targetNode) break;  // Exit early if both are found
        current = current->next;
    }

    if (clientNode == NULL) {
        sprintf(buffer, "Error: Client not found.\nchat>");
        send(client, buffer, strlen(buffer), 0);
        pthread_mutex_unlock(&rw_lock);
        return;
    }

    if (targetNode == NULL) {
        sprintf(buffer, "Error: User %s not found.\nchat>", targetUser);
        send(client, buffer, strlen(buffer), 0);
        pthread_mutex_unlock(&rw_lock);
        return;
    }

    // Add the connection for both users
    addConnection(clientNode, targetNode);
    addConnection(targetNode, clientNode);

    sprintf(buffer, "Connected to %s.\nchat>", targetUser);
    send(client, buffer, strlen(buffer), 0);

    printf("Successfully connected %s and %s.\n", clientNode->username, targetNode->username);
    pthread_mutex_unlock(&rw_lock);
}

void handleDisconnect(int client, char *targetUser) {
    pthread_mutex_lock(&rw_lock);
    char buffer[MAXBUFF];

    // Find the client and target user nodes
    struct node *clientNode = NULL, *targetNode = NULL;
    struct node *current = head;

    while (current != NULL) {
        if (current->socket == client) {
            clientNode = current;
        }
        if (strcmp(current->username, targetUser) == 0) {
            targetNode = current;
        }
        if (clientNode && targetNode) break;  // Exit early if both are found
        current = current->next;
    }

    if (clientNode == NULL) {
        sprintf(buffer, "Error: Client not found.\nchat>");
        send(client, buffer, strlen(buffer), 0);
        pthread_mutex_unlock(&rw_lock);
        return;
    }

    if (targetNode == NULL) {
        sprintf(buffer, "Error: User %s not found.\nchat>", targetUser);
        send(client, buffer, strlen(buffer), 0);
        pthread_mutex_unlock(&rw_lock);
        return;
    }

    // Remove the connection for both users
    removeConnection(clientNode, targetNode);
    removeConnection(targetNode, clientNode);

    sprintf(buffer, "Disconnected from %s.\nchat>", targetUser);
    send(client, buffer, strlen(buffer), 0);

    printf("Successfully disconnected %s and %s.\n", clientNode->username, targetNode->username);
    pthread_mutex_unlock(&rw_lock);
}



struct node *findUserBySocket(struct node *head, int socket){
  struct node *current = head; 
  while(current != NULL){
    if(current->socket == socket){
      return current; 
    }
    current = current->next; 
  }
  return NULL; 
}