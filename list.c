#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"

//insert link at the first location
struct node* insertFirstU(struct node *head, int socket, char *username) {
    
   if(findU(head,username) == NULL) {
           
       //create a link
       struct node *link = (struct node*) malloc(sizeof(struct node));

       link->socket = socket;
       strcpy(link->username,username);
       
       //point it to old first node
       link->next = head;

       //point first to new first node
       head = link;
 
   }
   else
       printf("Duplicate: %s\n", username);
   return head;
}

//find a link with given user
struct node* findU(struct node *head, char* username) {

   //start from the first link
   struct node* current = head;

   //if list is empty
   if(head == NULL) {
      return NULL;
   }

   //navigate through list
    while(strcmp(current->username, username) != 0) {
	
      //if it is last node
      if(current->next == NULL) {
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }      
	
   //if username found, return the current Link
   return current;
}

void addConnection(struct node *user, struct node *newConnection) {
    if (user->connectedCount == user->connectedCapacity) {
        // Resize the array if capacity is reached
        user->connectedCapacity *= 2;
        user->connectedUsers = realloc(user->connectedUsers, user->connectedCapacity * sizeof(struct node *));
    }
    user->connectedUsers[user->connectedCount++] = newConnection;
}

void removeConnection(struct node *user, struct node *connectionToRemove) {
    for (int i = 0; i < user->connectedCount; i++) {
        if (user->connectedUsers[i] == connectionToRemove) {
            // Shift remaining connections to fill the gap
            for (int j = i; j < user->connectedCount - 1; j++) {
                user->connectedUsers[j] = user->connectedUsers[j + 1];
            }
            user->connectedCount--;
            break;
        }
    }
}
