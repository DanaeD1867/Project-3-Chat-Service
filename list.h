#ifndef LIST_H
#define LIST_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct node {
   char username[50];
   int socket;
   char currentRoom[50];
   struct node *next;

   //For connections
   struct node **connectedUsers;  
   int connectedCount;           
   int connectedCapacity; 
};

/////////////////// USERLIST //////////////////////////

//insert node at the first location
struct node* insertFirstU(struct node *head, int socket, char *username);

//find a node with given username
struct node* findU(struct node *head, char* username);
void addConnection(struct node *user, struct node *newConnection); 
void removeConnection(struct node *user, struct node *connectionToRemove); 
 
#endif // LIST_H
