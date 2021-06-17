#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <pthread.h>
#include <assert.h>
#include <string.h>

#include "2by2sim.h"
#include "cpu.h"

//array of 1024 lines with 64 bit words
int cpu_generated;
int list_index = 1;

//TODO: make this changable at runtime 
int cpu_status[NUM_CPU] = {CPU_AVAILABLE,CPU_AVAILABLE,CPU_AVAILABLE,CPU_AVAILABLE};

int nodes_removed; //This is the number of dead nodes (0 destinations) that were removed (needed for node_dest allignment

struct cpu *list; //the list of tasks 

pthread_mutex_t mem_lock;  //main mem mutex


//TODO: instantiated in main 
struct Queue* cpu_queue1;
struct Queue* cpu_queue2;
struct Queue* cpu_queue3;
struct Queue* cpu_queue4;


/* MUTEX commands

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

*/

//DO NOT REMOVE THE LINE BELLOW!! File may become corrupt if it is (used to write code array in)
//CODE BEGINE//
int code[] = {//End main:
0x7fffffff,
0x0,
0x7,
0x28,
0xc,
0x0,
0x3,
0x8,
0xc,
0x7c,
0x7fffffff,
0x0,
0x7,
0x20,
0xc,
0x0,
0x1,
0x78,
0x7fffffff,
0x2,
0xfffffffc,
0x28,
0x3,
0x2,
0x0,
0x0,
0x1,
0x34,
0x7fffffff,
0x0,
0x16,
0x20,
0xc,
0x0,
0x1,
0x30,
0x7fffffff,
0x2,
0xfffffffc,
0x28,
0x3,
0x2,
0x0,
0x0,
0x1,
0xffffffff,
0x7fffffff,
0x2,
0xfffffffc,
0x28,
0x3,
0x2,
0x0,
0x0,
0x1,
0xffffffff
//Start main @(0):
};
int code_size = 56;
int dictionary[][3] = {{0,56,6}
};
//CODE END//
//DO NOT REMOVE THE LINE ABOVE!!//

int size(int addr){
	//find size
	int i = addr + 1;
	int size = 1;
	while(code[i] != 2147483647 && i < code_size){  //TODO: define as node tag or something 
		size++;
		i++;
	}
	return size;
}

int find_dest_node(int end){

	int dest = code_size - (end/4);
	int count = 0;
	for(int i = 0; i <= dest; i++){
		if(code[i] == 2147483647){
			count++;
		}
	}
	return count;
}


struct cpu *generate_list(int i){  //TODO: change cpu stuct name to AGP_node or something 

	if(i >= code_size){
		return NULL;
	} else {
		struct cpu *return_node = (struct cpu *)malloc(sizeof(struct cpu));
		return_node->node_size = size(i); 
		return_node->code_address = i;
		return_node->dependables = NULL;
		return_node->node_num = list_index;
		
		for(int j=0; j<return_node->node_size; j++){
			return_node->code[j] = code[i];
			i++;
		}
		

		return_node->num_dest = return_node->code[(6+return_node->code[1])];
	
		if(return_node->num_dest == 0){
			nodes_removed++;
			return generate_list(i);//dont create a useless node
		}else{
			
			struct DEST *dest = (struct DEST *)malloc(sizeof(struct DEST));
			struct DEST *temp = dest;

			for(int i = 1; i <= return_node->num_dest; i++){
				if(return_node->code[return_node->node_size-i] == -1){
					temp->node_dest = -99; //write to mem
				}else{
					temp->node_dest = find_dest_node(return_node->code[return_node->node_size-i]);
				}
				temp->cpu_dest = -1;
				temp->next = (struct DEST *)malloc(sizeof(struct DEST));
				temp = temp->next;
			}
			//temp = NULL;
			free(temp);
			return_node->dest = dest;
		}
		
		return_node->assigned_cpu = -1;
		list_index++;
		return_node->next = generate_list(i);
		

		return return_node;
	}
}

void generate_lookup_table(struct cpu *current){

	switch(current->assigned_cpu){
			
		case 1:
			current->look_up[0] = cpu_queue1; //1
			current->look_up[1] = cpu_queue2; //2
			current->look_up[2] = cpu_queue3; //3
			current->look_up[3] = cpu_queue2; //cant send to 4
			break;
		case 2:
			current->look_up[0] = cpu_queue1; 
			current->look_up[1] = cpu_queue2;
			current->look_up[2] = cpu_queue4; //cant send to 3
			current->look_up[3] = cpu_queue4; 
			break;
		case 3:
			current->look_up[0] = cpu_queue1;
			current->look_up[1] = cpu_queue1; //cant send to 2
			current->look_up[2] = cpu_queue3;
			current->look_up[3] = cpu_queue4; 
			break;
		case 4:
			current->look_up[0] = cpu_queue3; //cant send to 1
			current->look_up[1] = cpu_queue2;
			current->look_up[2] = cpu_queue3;
			current->look_up[3] = cpu_queue4; 
			break;
		default:
			printf("shouldn't happen");
			break;
	}
}

//this is the function that mappes nodes to cpu
//likely the root of any preformance
//first iteration is very simple and mappes in a linear fassion; node 1 -> cpu 1 , node 2 -> cpu 2 ... node n -> cpu -> n
void schedule_nodes(){
	
	int cpu_scheduled = 1;
	struct cpu *current = list;

	while(cpu_scheduled <= NUM_CPU && current != NULL){
		
		printf("NUM CPU SCHEDULED %d VS NUM CPU %d\n",cpu_scheduled,NUM_CPU);
		current->assigned_cpu = cpu_scheduled;
		cpu_scheduled = cpu_scheduled+1;
		
		//set up lookup table 
		generate_lookup_table(current);
		current = current->next;
	}
}

void refactor_destinations(struct cpu *current, struct cpu *top, int node_num ){
	if(current == NULL){

	}else if(current->assigned_cpu == -1){ //dont refactr unscheduled nodes 
		refactor_destinations(current->next, top, node_num+1);
	}else{
		struct DEST *dest_struct = current->dest; //getting the list of destinations
		for(int i = 1; i<=current->num_dest;i++){
			if(dest_struct->node_dest == -99){ //return to main mem since there are no dependants
				dest_struct->cpu_dest = -99; //main mem
			}else{
				dest_struct->node_dest -= nodes_removed; //updating node_dest in case of any removed nodes
				struct cpu *temp = (struct cpu *)malloc(sizeof(struct cpu));
				int node_count;
				if(node_num < dest_struct->node_dest){
					temp = current;
					node_count = node_num;
				}else{
					temp = top;
					node_count = 1;
				}
				
				while(node_count != dest_struct->node_dest){
					temp = temp->next; node_count++;
				}



				//if the destination isnt assigned, the current node must hold the value
				if(temp->assigned_cpu == -1)
					dest_struct->cpu_dest = TEMP_A;
				else
					dest_struct->cpu_dest = temp->assigned_cpu;
				
				//now we must change the satck destination to match the node stack rather than the full code stack
				//this is done even if the cpu isnt assinged yet
				int dest = code_size - (current->code[current->node_size-i]/4) - 1;
				printf("		DEST: %d\n",dest);
				int count = 0;
				while(code[dest] != 2147483647){
					count++; dest--;
				}
				dest = (temp->node_size - count -1)*4;
				printf("DEST: %d\n", dest);
				current->code[current->node_size-i] = dest;

			}

			dest_struct = dest_struct->next;
			
		}

	
		refactor_destinations(current->next, top, node_num+1);
	}
}


struct cpu * schedule_me(int cpu_num){

	//initial while that we will traverse through and try to find the node we want to schedule
	//count number of possible to be scheduled nodes!
	//if count == 0, create dumy cpu (temp->code[1] = 1; seb) struct that has multiple dependencies, set the cpu status cpu_status [cpu_num-1] = IDLE
	//else (we can schedule), pick the first node we run into (later, nodes which SHOULD be ran, e.g. priority or deadlines) , right now FIFO;  check if -1 ->>> it measns it is still unassigned
	//runtime refactor -> change dest to either cpu numebr or temp (means hold the value) 
	//check number of dependencies, if 0 return cpu. if has dependables, do they know their destination? if yes, you can return cpu. if No, make a list of those who have your dependables, return 
	// structure cpu
	
	
	struct cpu *current = list;
	int unode_num = 0; //number of unscheduled nodes
	
	/*finding unscheduled nodes and store them into a new list*/
	while(current != NULL){ 
		
		if(current->assigned_cpu == -1){
			unode_num++;
			break;
		}
		
		current = current->next;
	}
	//if there is no node to be left to be scheduled
	if(unode_num == 0){
		printf("no more nodes to assign!! sending CPU %d a dummy node\n",cpu_num);
		struct cpu *dummy = (struct cpu *)malloc(sizeof(struct cpu));
		dummy->assigned_cpu = cpu_num;
		dummy->code[1] = 1;
		//set up lookup table 
		generate_lookup_table(dummy);
		cpu_status [cpu_num-1] = CPU_IDLE; //there are no nodes left! go to idle mode.
		return dummy;
	} else{ //there is some unassigned nodes
		
		//runtime_refactor(); 
		
		if(current->code[1] == 0){//if the node has no dependent
			current->assigned_cpu = cpu_num;
			refactor_destinations(current, list, current->node_num);
			//create look up table
			generate_lookup_table(current);
			cpu_status [cpu_num-1] = CPU_UNAVAILABLE;
			return current;
		} else{ //if the node has dependables
		
			struct cpu *temp = list;
			
			struct depend *dep = (struct depend *)malloc(sizeof(struct depend));
			struct depend *temp_dep = dep;
			

			while(temp != NULL){
				  struct DEST *dest = temp->dest;
				  
				  for(int i = 0; i< temp->num_dest; i++){
				      if(dest->node_dest == current->node_num){
					  temp_dep->cpu_num = temp->assigned_cpu; //cpu that has that variable
					  temp_dep->node_num = temp->node_num; //variable name to be requested
					  temp_dep->next = (struct depend *)malloc(sizeof(struct depend));
					  temp_dep = temp_dep->next;
				      }
				      dest = dest->next;
				  }
				  temp = temp->next;
			}
			temp_dep = NULL;
			free(temp_dep);
			current->dependables = dep;
			//return the cpu.
			current->assigned_cpu = cpu_num;
			refactor_destinations(current, list, current->node_num);
			//set up lookup table
			generate_lookup_table(current);
			cpu_status [cpu_num-1] = CPU_UNAVAILABLE;
			return current;
			
		}
	
	}
	
	return current;

}


void writeMem(int ind, int val){

	code[ind] = val;
	printf("WRITING BACK TO MEMORY...\n");
	printf("code[%d] = %d\n",ind, code[ind]);
	printf("WRITING BACK TO MEMORY HAS FINISHED\n");
}


void print_nodes(struct cpu *nodes){
	if(nodes == NULL){

	}else{
		printf("\n\nNODE: \n");
		printf(" - CPU assigned: %d\n",nodes->assigned_cpu);
		printf(" - Node number: %d\n",nodes->node_num);
		printf(" - code main mem addr: %d\n", nodes->code_address);
		printf(" - node size: %d\n",nodes->node_size);
		printf(" - number of dest: %d\n",nodes->num_dest);
		printf(" - code:\n");
		for(int i = 0; i< nodes->node_size; i++){
			printf("    code[%d]: %d\n",i,nodes->code[i]);
		}
		struct DEST *temp = nodes->dest;
		for(int i = 0; i < nodes->num_dest; i++){
			printf(" - Destination %d:\n    node dest: %d\n    cpu dest: %d\n",i,temp->node_dest, temp->cpu_dest);
			temp = temp->next;
		}
		print_nodes(nodes->next);
	}
}


int main()
{
    printf("***SIMULATION START***\n\n");

    //create mutex
    if (pthread_mutex_init(&mem_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
   
    
    for(int i = 0; i<code_size; i++){
	printf("code[%d]: %d\n", i ,code[i]); 
    }

    nodes_removed = 0;    

    cpu_generated = 0;
    pthread_t thread_id[NUM_CPU];


    printf("\n\nCREATING NODE LIST\n\n"); 
    list = generate_list(0);
    //print_nodes(list);


    //creating cpu queues
    cpu_queue1 = createQueue();
    cpu_queue2 = createQueue();
    cpu_queue3 = createQueue();
    cpu_queue4 = createQueue();

    /***********************/
    /*** task scheduling ***/
    /***********************/
    /*
	-64:	0x7fffffff      //new node label
	-60:	0x0		//number of dependencies 
	-5c:	0x7		//value (const 7)
	-58:	0x20		//end address + 1 (next node in graph)
	-54:	0xc		//operation
	-50:	0x0             //number of arguments 
	-4c:	0x1             //expansion or not flag 
	-48:	0x8             //result destination
	-44:	0x7fffffff
	-40:	0x0
	-3c:	0x7
	-38:	0x20
	-34:	0xc
	-30:	0x0
	-2c:	0x1
	-28:	0x4
	-24:	0x7fffffff
	-20:	0x2
	-1c:	0xfffffffc
	-18:	0x24
	-14:	0x3
	-10:	0x2
	-c:	0x0
	-8:	0x0
	-4:	0x0
    */

    printf("\n\nSCHEDULING NODES\n\n");    
    //fuction that schedules nodes to cpu (currently very simple)
    //this function is likely going to determine the preformance of the whole design
    schedule_nodes();
    //print_nodes(list);
    
    printf("\n\nREFACTORING NODE DESTINATIONS\n\n");   
    refactor_destinations(list, list, 1);
    print_nodes(list);

    printf("\n\nLAUNCHING THREADS!!!\n\n"); 
    //simple thread launch since we know more core than nodes 
    struct cpu *graph = list; 
    while(cpu_generated < NUM_CPU && graph != NULL){
	pthread_create(&(thread_id[graph->assigned_cpu-1]), NULL, &CPU_start, graph);
	cpu_status[graph->assigned_cpu-1] = CPU_UNAVAILABLE;
	cpu_generated++;
	graph = graph->next;
    }
    
    
   
   if(cpu_generated < NUM_CPU){ //more cores than nodes, create idle cpus
   	    int count = 0;
	    while(cpu_generated < NUM_CPU){
		if(cpu_status[count] == CPU_AVAILABLE){
			struct cpu *temp = (struct cpu *)malloc(sizeof(struct cpu));
			temp->assigned_cpu = count+1;
			temp->code[1] = 1;
			pthread_create(&(thread_id[count]), NULL, &CPU_start, temp);
			cpu_status[count] = CPU_IDLE;
			cpu_generated++;
		}
		count++;
	    }
    }
    
    //checking cpu status, if all of them are idle, cancle threads and end simulation
    
    
    /***********************/
    /**** Simulation end ***/
    /***********************/

    //wait for all active cpu threads to finish
    int num_cpu_idle = 0;
    while(num_cpu_idle < NUM_CPU){
	num_cpu_idle = 0;
	for(int i = 0; i<NUM_CPU; i++){
		if(cpu_status[i] == CPU_IDLE)
	    		num_cpu_idle++;
        }
	
	//can do other busy work while sim continues '\/('_')\/' 
	
    }

    for(int i = 0; i<NUM_CPU; i++){
	pthread_cancel(thread_id[i]); //cancel all threads 
	pthread_join(thread_id[i], NULL); //wait for all threads to clean and cancel safely 
		
    }

    pthread_mutex_destroy(&mem_lock);
    
    
    puts("\nPRINTING CODE ARRAY\n"); // want to check if result 14 is written to memory (code array)
    for(int i = 0; i<code_size; i++){
	printf("code[%d]: %d\n", i ,code[i]); 
    }


    printf("\n\n***SIMULATION COMPLETE***\n\n");
    return 0;
}

































