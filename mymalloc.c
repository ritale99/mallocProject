#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"
static char myblock[4096]; //Initialize the data

//Format for metadata before each allocated data in myblock
typedef struct metadata
{
	//# of bytles until next node/end of memory
	unsigned short int size;
	//# of bytes allocated with malloc
	unsigned short int used;
} metadata;
metadata* node0;

/* Malloc function
 * Scans starting from 0
 * 	if size >= bytes (if current node is big enough)
 * 		if used == 0 (if current node is open)
 * 			set used to bytes
 * 			return pointer to node + metadataSize
 * 		else if size - used >= bytes + metadataSize (if enough space to squeeze new node before next node)
 * 			new node is at currentPosition + metadataSize + used
 * 			set new metadata to have size = size - used - metadataSize
 * 			set new metadata to have used = bytes
 * 			set current metadata to have size = used
 * 			return pointer to new node + metadataSize
 * 		else (if not enough space between current and next node)
 * 			if next node, continue to next node
 * 			else (if no more space)
 * 				return NULL
 * 	else
 * 		if next node, continue to next node
 * 		else (if no more nodes)
 * 			return NULL
 */
void* mymalloc(size_t bytes, char* fileName, int line)
{
	//Setup Node0 if NULL
	if (node0 == NULL)
	{	
		//setup of metadata
		node0 = (metadata*)myblock;
		node0->used = 0;
		node0->size = 4096 - sizeof(metadata);
	}

	if (bytes == 0) //allocating 0 bytes, return NULL
	{
		printf("error: value must be at least 1\n"
			"\tFile: %s, Line: %d", fileName, line);
		return NULL; 	
	}
	//Allocating a value greater than max size also stops cases when size_t is smaller then casting
	if (bytes > (size_t)(4096 - sizeof(metadata)))
	{
		printf("error: value exceeds memory\n"
			"\tFile: %s, Line: %d", fileName, line);
		return NULL;
	}
	//Note: size_t is an unsigned decimal, cannot be negative
	
	metadata* currentNode = node0;
	while (1)
	{	//make sure size of currentNode is greater than or equal to how much is being allocated
		if (currentNode->size >= (unsigned short)bytes)
		{
			//Current node has enough space to allocate
			//when will this not be 0? 
				//when it is not the first time allocating 
			if (currentNode->used == 0) 
			{
				currentNode->used = (unsigned short)bytes;
				return (void*)currentNode + sizeof(metadata);
			}

			//Current node is used, but still has enough space
			if (currentNode->size - currentNode->used >= (unsigned short)bytes + sizeof(metadata))
			{
				unsigned short size = currentNode->size - currentNode->used - sizeof(metadata);
				unsigned short used = (unsigned short)bytes;

				//Create newNode right after used data for currentNode
				void* newNode = (void*)currentNode + sizeof(metadata) + currentNode->used;
				((metadata*)newNode)->size = size;
				((metadata*)newNode)->used = used;

				//Resize currnetNode to 
				currentNode->size = currentNode->used;
				return (void*)newNode + sizeof(metadata); //return pointer to data for newNode
			}
		}
		//Current node is too small or big enough, but too full, continue to next node

		//Next node cannot exist, no space
		if ((void*)currentNode + currentNode->size > (void*)myblock + 4096 - (unsigned short)bytes - sizeof(metadata))
		{
			printf("\terror: no more space\n"
				"\tFile: %s, Line: %d", fileName, line);
			return NULL;
		}

		//Continue to next node
		currentNode = (metadata*)((void*)currentNode + sizeof(metadata) + currentNode->size);
	}

	return NULL; //Never occurs, idk if we need to leave this here
}

/* Free function
 * backtrack from ptr to get metadata of node to be freed, Node1
 * Scan starting from 0
 * 	if next node's index is the outside of the given memory
 * 		error: User is trying to free a node that doesnt exist
 * 	if next node's index is Node1's, save current position as Node0
 * 	if next node's index is after Node1's
 * 		error: User is trying to free a node that doesnt exist
 * get node after Node1, Node2, if it is not the end of memory
 *
 * set currentNode to Node1
 * if Node2 exists and has used=0 (erase node2 and merge into node1)
 * 	set Node1.size to Node1.size + metadataSize(of Node2) + Node2.Size
 * if Node0 has used=0 (erase node1 and merge into node0)
 * 	set currentNode to Node0
 * 	set Node0.size to Node0.size + metadataSize(of Node1) + Node1.Size
 */
//Node 1 is before it's being freed
//Node 2 is the one being freed
//Node 3 is after the one being freed

void myfree(void* ptr, char* fileName, int line)
{
	//User is freeing a node before allocated space
	if (ptr < (void*)node0 + sizeof(metadata) ) {
		printf("error: %x is not inside memory\n"
			"\tFile: %s, Line: %d", ptr, fileName, line);			
		return;
	}
		
	//Node0 is NULL when malloc is not called yet
	if (node0 == NULL){ 
		printf("error: No data allocated\n"
			"\tFile: %s, Line: %d", fileName, line);
		return;
	} 

	//metadata of node to be freed 
	metadata* node2 = (metadata*)((void*)ptr - sizeof(metadata));
	metadata* currentNode = node0;
 	metadata* node1 = NULL;
	metadata* node3 = NULL; 
	
	//Find node1
	while(1) {
		if (currentNode == node2){
			break; //If node0 is node2
		}
		

		//checks if node2 is outside of the given memory
		if ((void*)currentNode + sizeof(metadata) + currentNode->size >= (void*)myblock + 4096 ){
			printf("error: %x is not inside memory\n"
				"\tFile: %s, Line: %d", ptr, fileName, line);	
			return; 
		}
	

		//if next node is node2, currentNode is node1
		if ((void*)currentNode + sizeof(metadata) + currentNode->size == node2) {
			node1 = currentNode;
			break;
		}
		
		//passed the node2	
		if ((void*)currentNode + sizeof(metadata) + currentNode-> size >(void*) node2) {
			printf("error: Cannot free unallocated space\n"
				"\tFile: %s, Line:%d", fileName, line);	
			return;
		}
	

		//move onto the next node 
		currentNode = (metadata*)((void*)currentNode + sizeof(metadata) + currentNode->size);
	}

	//get node 3 if it exists
	if((void*)node2 + sizeof(metadata) + node2->size < (void*)myblock + 4096 - sizeof(metadata)) {
		node3 = (void*)node2 + sizeof(metadata) + node2->size;
	}
	
	//if node3 is empty, merge into node2
	if (node3 != NULL && node3->used == 0) {
		node2->size = node2->size + sizeof(metadata) + node3->size;
	}

	//if node1 is empty, merge node2 into node1
	if (node1 != NULL && node1->used == 0) {
		node1->size = node1->size + sizeof(metadata) + node2->size;
	} else if (node2->used == 0) {
		printf("error: Cannot free unallocated space\n"
			"\tFile: %s, Line: %d", fileName, line);
		return;
	} else {
		node2->used=0;
	}
	return;
}
