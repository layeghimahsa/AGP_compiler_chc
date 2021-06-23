#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <pthread.h>
#include <assert.h>
#include <string.h>

#include "2by2sim.h"
#include "cpu.h"

//dynamic code array (starts as copy of original code)
int *runtime_code;
//cpu status array
int *cpu_status;
//used to keep track of node numbers
int list_index;
//This is the number of dead nodes (0 destinations) that were removed (needed for node_dest allignment
int nodes_removed; 
//the list of all tasks (that have more than 0 destinations)
struct AGP_node *program_APG_node_list; 
//main mutex
pthread_mutex_t mem_lock;  


//DO NOT REMOVE THE LINE BELLOW!! File may become corrupt if it is (used to write code array in)
//CODE BEGINE//
const int code[] = {//End main:
0x7fffffff,
0x0,
0x7,
0x20,
0xc,
0x0,
0x1,
0x14,
0x7fffffff,
0x1,
0xfffffffc,
0x24,
0xc,
0x1,
0x0,
0x1,
0xffffffff,
0x7fffffff,
0x1,
0xfffffffc,
0x30,
0x0,
0x1,
0x0,
0x64,
0x1,
0x0,
0x38,
0x24,
//Start main @(26):
//End func:
0x7fffffff,
0x0,
0xfffffffc,
0x20,
0x1,
0x0,
0x1,
0xc,
0x7fffffff,
0x0,
0x7,
0x20,
0xc,
0x0,
0x1,
0x8,
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
//Start func @(0):
};
int code_size = 55;
int main_addr = 26;
int main_num_nodes = 3;
int dictionary[][3] = {{26,29,3},
{0,26,3}
};
int num_dict_entries = 2;
//CODE END//
//DO NOT REMOVE THE LINE ABOVE!!//


/**
 * @brief size function
 *
 * This function is called to calculate the size of each node. (how many entries does the node have)
 * @param [in] addr is the beggining of the node.
 * @param [out] size of the node.
 * @return int
 */
int size(int addr){
	//find size
	int i = addr + 1;
	int size = 1;
	while(code[i] != NODE_BEGIN_FLAG && i < code_size){ 
		size++;
		i++;
	}
	return size;
}

/**
 * @brief find_dest_node function
 *
 * This function is called to find destination node number
 * @param [in] end the offset from top of the stack 
 * @param [out] equivalent node number given the offset
 * @return int
 */
/*int find_dest_node(int end){

	int dest = code_size - (end/4);
	int count = 0;
	for(int i = 0; i <= dest; i++){
		if(code[i] == NODE_BEGIN_FLAG){
			count++;
		}
	}
	return count;
}


*/
/**
 * @brief find_num_node function
 *
 * This function is called to find the number of nodes between two addresses
 * @param [in] begin the offset from top of the stack to start search 
 * @param [in] end the offset from top of the stack te end search 
 * @param [out] number of nodes between the given offset
 * @return int
 */
int find_num_node(int begin, int end){

	int dest = code_size - (end/4);
	int count = 0;
	//int i = code_size - (begin/4)
	for(int i = begin; i <= dest; i++){
		if(code[i] == NODE_BEGIN_FLAG){
			count++;
		}
	}
	return count;
}
struct AGP_node *create_list(int start_address){
	//find dict entry
	int num_nodes_to_make;
	int func_size;
	for(int i = 0; i<num_dict_entries; i++){
		if(dictionary[i][0] == start_address){
			num_nodes_to_make = dictionary[i][2];
			func_size = dictionary[i][1];
		}
	}

	//make that many nodes
	int i = code_size - start_address - func_size;
	int func_top = i;
	int pre_list_index = list_index;//index before adding nodes
	struct AGP_node *return_node = (struct AGP_node *)malloc(sizeof(struct AGP_node));
	struct AGP_node *making = return_node;
	while(num_nodes_to_make > 0){
		
		making->node_size = size(i);
		making->code_address = i;
		making->depend = NULL;
		making->node_num = list_index; list_index++;
		making->assigned_cpu = UNDEFINED;
		making->node_func = start_address;
		//printf("I: %d\n",i);
		for(int j=0; j<making->node_size; j++){
			//printf("code[i]: %d\n",code[i]);
			making->code[j] = code[i];
			i++;
		}
		if(making->code[4] != code_expansion){
		
			making->num_dest = making->code[(6+making->code[1])];
			//printf("Num of return nodes: %d\n",making->num_dest);

			if(making->num_dest == 0){
				making->dest = NULL;
			}else{
				
				struct Destination *dest = (struct Destination *)malloc(sizeof(struct Destination));
				struct Destination *temp = dest;

				for(int j = 1; j <= making->num_dest; j++){
					if(making->code[making->node_size-j] == -1){
						temp->node_dest = OUTPUT; //write to mem
					}else{
						temp->node_dest = pre_list_index -1 + find_num_node(func_top,(start_address*4+making->code[making->node_size-j]));
					}
					temp->cpu_dest = UNDEFINED;
					temp->next = (struct Destination *)malloc(sizeof(struct Destination));
					temp = temp->next;
				}
				//temp = NULL;
				free(temp);
				making->dest = dest;
			}
		}else{
			//code expansion setup TODO
			making->num_dest =making->code[(6+(making->code[1]*2))];
			//printf("Num of return nodes: %d\n",making->num_dest);
			
		}
		num_nodes_to_make--;
		if(num_nodes_to_make == 0){making->next = NULL;}
		else{
			making->next = (struct AGP_node *)malloc(sizeof(struct AGP_node));
			making = making->next;
		}
	}
	return return_node;
}





 
 
/*
0x7fffffff, //expansion node start
0x1, //created count, missing 1 input
0xfffffffc,
0x30, //node size
0x0, //operation (zero means expansion)
0x1, // 1 argument
0x0, //value
0x1dc, //address of x in our example (in code array)
0x1, //1 destination
0x0, // address of subgraph to expand
0x38, // address of node in subgraph to remap (address of result in our example)
0x1b4, //remaping to this (address of b in our example)


//i think this was inversed 
0x38, //remaping to this (address of b in our example) 
0x1b4,// address of node in subgraph to remap (address of result in our example)
*/


//TODO

//5. check variable if unavailable -> we have to block and continiouslt check if it's been updated
//6. find an output node to ramap it (given address of node in subgraph to remap)
void expansion(struct AGP_node *current){
	
	//1. get the address of subgraph to expand
	int sub_address = current->code[(7+(current->code[1]*2))];

	int pre_expand_index = list_index;
	
	//2. calling the generate_list function using this address
	struct AGP_node *expand_top = create_list(sub_address);
	if(expand_top == NULL){
		printf("\n\nFAILED TO EXPAND GRAPH\n\n");
		exit(0);
	}
	
	//3. connect to main list, make program_agp_node top!
	struct AGP_node *traverse = expand_top;
	while(traverse->next != NULL){traverse = traverse->next;}
	traverse->next = program_APG_node_list;
	program_APG_node_list = expand_top;
	traverse = traverse->next; //now points to old top of list (usefull for refactoring) 

	//4. refactor the given expansion
	

	//REMAPING OUTPUT
	int func_size;
	for(int i = 0; i<num_dict_entries; i++){   //get sub function dictionary info 
		if(dictionary[i][0] == sub_address){
			func_size = dictionary[i][1];
		}
	}
	int sub_code_pos = code_size - sub_address - func_size;

	struct AGP_node *node_to_change = program_APG_node_list;
	struct AGP_node *node_to_point = traverse;
        //calculate node offset from called function 
	int ntc= find_num_node(sub_code_pos, (sub_address*4+current->code[(9+(current->code[1]*2))]));
	
	for(int i = 0; i<num_dict_entries; i++){ //get calling function dictionary info
		if(dictionary[i][0] == current->node_func){
			func_size = dictionary[i][1];
		}
	}
	sub_code_pos = code_size - current->node_func - func_size;
	//calculate node offset from calling function
	int ntp= find_num_node(sub_code_pos, (current->node_func*4+current->code[(8+(current->code[1]*2))]));
	printf("ntc: %d	ntp: %d\n",ntc,ntp);
	

	for(ntc; ntc>1; ntc--){node_to_change = node_to_change->next;}
	for(ntp; ntp>1; ntp--){node_to_point = node_to_point->next;}
	
	//create destination node 
	struct Destination *dest_node = (struct Destination *)malloc(sizeof(struct Destination));
	dest_node->node_dest = node_to_point->node_num;
	if(node_to_point->assigned_cpu == UNDEFINED){dest_node->cpu_dest = UNDEFINED;}
	else{dest_node->cpu_dest = node_to_point->assigned_cpu;}
	
	if(node_to_change->dest == NULL){
		node_to_change->dest = dest_node;
	}
	else{
		struct Destination *temp = node_to_change->dest;
		while(temp->next != NULL){temp = temp->next;}
		temp = dest_node;
		node_to_change->num_dest++;
	}

	//CREATING INPUT VARIABLE REQUEST MESSAGE
	/*
				
	struct Dependables *depe = (struct Dependables *)malloc(sizeof(struct Dependables));
	struct Dependables *dep = depe;

	while(traverse != NULL){
		struct Destination *dest = traverse->dest;

		for(int i = 0; i< traverse->num_dest; i++){
			if(dest->node_dest == current->node_num){
				if(dest->cpu_dest == UNDEFINED || dest->cpu_dest == UNKNOWN){
					dep->cpu_num = temp->assigned_cpu; //cpu that has that variable
					dep->node_needed = temp->node_num; //variable name to be requested
					dep->next = (struct Dependables *)malloc(sizeof(struct Dependables));
					dep = dep->next;
				}
			}
			dest = dest->next;
		}
		traverse = traverse->next;
	}
	current->depend = depe;
	*/
}


/**
 * @brief generate_lookup_table function
 *
 * This function is called to initialize cpu connections. (specifies to which queues does the current cpu have access and can put the rsult to or get the result from)
 * @param [in] current the current cpu under initialization 
 * @param [in] Q the pointer to all queues
 * @return void
 */
void generate_lookup_table(struct CPU *current, struct Queue **Q){


	switch(current->cpu_num){
			
		case 1:
			current->look_up[0] = Q[0]; //1
			current->look_up[1] = Q[1]; //2
			current->look_up[2] = Q[2]; //3
			current->look_up[3] = Q[1]; //cant send to 4
			break;
		case 2:
			current->look_up[0] = Q[0]; 
			current->look_up[1] = Q[1];
			current->look_up[2] = Q[3]; //cant send to 3
			current->look_up[3] = Q[3]; 
			break;
		case 3:
			current->look_up[0] = Q[0];
			current->look_up[1] = Q[0]; //cant send to 2
			current->look_up[2] = Q[2];
			current->look_up[3] = Q[3]; 
			break;
		case 4:
			current->look_up[0] = Q[2]; //cant send to 1
			current->look_up[1] = Q[1];
			current->look_up[2] = Q[2];
			current->look_up[3] = Q[3]; 
			break;
		default:
			printf("shouldn't happen");
			break;
	}
}

 

/**
 * @brief refactor_destinations function
 *
 * This function is called to update or refactor destinations to be matched to node's stack rather than entire code stack
 * @param [in] current is the current node we are evaluating
 * @param [in] top is the top of the AGP nodes' list
 * @return void
 */
void refactor_destinations(struct AGP_node *current, struct AGP_node *top){
	if(current == NULL){
		printf("cant refactor a null node!!!\n");
	}else{
		struct Destination *dest_struct = current->dest; //getting the list of destinations
		for(int i = 1; i<=current->num_dest;i++){
			if(dest_struct->node_dest == OUTPUT){ //return to main mem since there are no dependants
				dest_struct->cpu_dest = OUTPUT; //main mem
			}else{
				dest_struct->node_dest -= nodes_removed; //updating node_dest in case of any removed nodes
				struct AGP_node *temp = top;
				int node_count = 1;
				
				while(node_count != dest_struct->node_dest){
					temp = temp->next; node_count++;
				}

				//if the destination isnt assigned, the current node must hold the value
				if(temp->assigned_cpu == UNDEFINED || temp->code[4] == code_expansion)
					dest_struct->cpu_dest = UNKNOWN;
				else
					dest_struct->cpu_dest = temp->assigned_cpu;
				
				//now we must change the satck destination to match the node stack rather than the full code stack
				//this is done even if the cpu isnt assinged yet
				int dest = code_size - (current->code[current->node_size-i]/4) - 1;
				int count = 0;
				while(code[dest] != NODE_BEGIN_FLAG){
					count++; dest--;
				}
				dest = (temp->node_size - count -1)*4;
				printf("DEST: %d\n", dest);
				current->code[current->node_size-i] = dest;

			}

			dest_struct = dest_struct->next;
			
		}
		//refactor_destinations(current->next, top, node_num+1);
	}
}



/**
 * @brief schedule_me function
 *
 * This function is called to schedule a new task to be run on the current cpu. (on-demand scheduling)
 * @param [in] cpu_num specifies the cpu which requests for a new task to execute it.
 * @return struct AGP_node a new node that can be executed on the current cpu. it can be a dummy node in case there are no nodes left.
 */
struct AGP_node *schedule_me(int cpu_num){

	//initial while that we will traverse through and try to find the node we want to schedule
	//count number of possible to be scheduled nodes!
	//if count == 0, create dumy cpu (temp->code[1] = 1; seb) struct that has multiple dependencies, set the cpu status cpu_status [cpu_num-1] = IDLE
	//else (we can schedule), pick the first node we run into (later, nodes which SHOULD be ran, e.g. priority or deadlines) , right now FIFO;  check if -1 ->>> it measns it is still unassigned
	//runtime refactor -> change dest to either cpu numebr or temp (means hold the value) 
	//check number of dependencies, if 0 return cpu. if has dependables, do they know their destination? if yes, you can return cpu. if No, make a list of those who have your dependables, return 
	// structure cpu
	
	
	struct AGP_node *current = program_APG_node_list;
	int unode_num = 0; //number of unscheduled nodes
	
	/*finding unscheduled nodes and store them into a new list*/
	while(current != NULL){ 
		
		if(current->assigned_cpu == UNDEFINED){
			unode_num++;
			break;
		}
		
		current = current->next;
	}
	//if there is no node to be left to be scheduled
	if(unode_num == 0){
		printf("no more nodes to assign!! sending CPU %d a dummy node\n",cpu_num);
		struct AGP_node *dummy = (struct AGP_node *)malloc(sizeof(struct AGP_node));
		dummy->assigned_cpu = cpu_num;
		dummy->code[1] = 1;
		cpu_status[cpu_num-1] = CPU_IDLE; //there are no nodes left! go to idle mode.
		return dummy;
	} else{ //there is some unassigned nodes
		if(current->code[4] == code_expansion){
			expansion(current);
			return schedule_me(cpu_num);
			
		} else{
			if(current->code[1] == 0){//if the node has no dependent
				current->assigned_cpu = cpu_num;
				refactor_destinations(current, program_APG_node_list);
				cpu_status [cpu_num-1] = CPU_UNAVAILABLE;
				struct AGP_node *return_node = (struct AGP_node *)malloc(sizeof(struct AGP_node));
				*return_node = *current;
				return return_node;
			} else{ //if the node has dependables
			
				struct AGP_node *temp = program_APG_node_list;
				
				struct Dependables *depe = (struct Dependables *)malloc(sizeof(struct Dependables));
				struct Dependables *dep = depe;

				while(temp != NULL){
					struct Destination *dest = temp->dest;

					for(int i = 0; i< temp->num_dest; i++){
						if(dest->node_dest == current->node_num){
							if(dest->cpu_dest == UNDEFINED || dest->cpu_dest == UNKNOWN){
								dep->cpu_num = temp->assigned_cpu; //cpu that has that variable
								dep->node_needed = temp->node_num; //variable name to be requested
								dep->next = (struct Dependables *)malloc(sizeof(struct Dependables));
								dep = dep->next;
							}
						}
						dest = dest->next;
					}
					temp = temp->next;
				}
				current->depend = depe;
				//return the cpu.
				current->assigned_cpu = cpu_num;
				refactor_destinations(current, program_APG_node_list);
				cpu_status [cpu_num-1] = CPU_UNAVAILABLE;
				//return copy of node, not actual node
				struct AGP_node *return_node = (struct AGP_node *)malloc(sizeof(struct AGP_node));
				*return_node = *current;
				return return_node;
				
			}
		
		}
	
	}

}


/**
 * @brief writeMem function
 *
 * This function is called to writing back the result to memory.
 * @param [in] ind the code address for writing back to memory array.
 * @param [in] val the result.
 * @return void
 */
void writeMem(int ind, int val){

	pthread_mutex_lock(&mem_lock);
	runtime_code[ind] = val;
	printf("WRITING BACK TO MEMORY...\n");
	printf("code[%d] = %d\n",ind, runtime_code[ind]);
	pthread_mutex_unlock(&mem_lock);
}


/**
 * @brief print_nodes function
 *
 * This function is just a pretty printer which prints all the AGP_nodes
 * @param [in] nodes the pointer to AGP nodes' list
 * @return void
 */
void print_nodes(struct AGP_node *nodes){
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
		if(nodes->dest != NULL){
			struct Destination *temp = nodes->dest;
			for(int i = 0; i < nodes->num_dest; i++){
				printf(" - Destination %d:\n    node dest: %d\n    cpu dest: %d\n",i,temp->node_dest, temp->cpu_dest);
				temp = temp->next;
			}
		}
		print_nodes(nodes->next);
	}
}


int main(int argc, char **argv)
{
    printf("***SIMULATION START***\n\n");


    if(argc < 2){
	printf("You must enter the number of CPU you want\nex: ./sim 4 (to run a 2x2 sim with 4 cores)\n");
	return 1;
    }

    //create mutex
    if (pthread_mutex_init(&mem_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
   
    int NUM_CPU = atoi(argv[1]);
    if(NUM_CPU < 1){
	printf("NODE NUM %d\n",NUM_CPU);
	printf("YOU MUST HAVE AT LEAST 1 CPU\n");
	return 1;
    }
    
    printf("CREATING A %dx%d SIMULATION\n",NUM_CPU/2,NUM_CPU/2);


    printf("\n\nSETTING UP ENVIRONMENT\n\n"); 


    //create array of thread id
    pthread_t thread_id[NUM_CPU];
    list_index = 1;
    nodes_removed = 0; 

    //create status array 
    cpu_status = (int *)malloc(sizeof(int) * NUM_CPU);

    for(int i = 0; i<NUM_CPU; i++){
	cpu_status[i] = CPU_AVAILABLE;
    }

    //instantiate queues for all CPUs
    struct Queue *cpu_queues[NUM_CPU];
    for(int i = 0; i<NUM_CPU; i++){
	cpu_queues[i] = createQueue();
    }

    //create cpu struct 
    struct CPU *cpus[NUM_CPU];
    for(int i = 0; i<NUM_CPU; i++){
	struct CPU *cpu_t = (struct CPU*)malloc(sizeof(struct CPU));
        cpu_t->cpu_num = i+1;
	generate_lookup_table(cpu_t, cpu_queues);
	cpus[i] = cpu_t;
    }

    //TODO: copy static code array into a new array that can be modified 
    runtime_code = (int *)malloc(sizeof(int) *code_size);
    for(int i = 0; i<code_size; i++){
	printf("code[%d]: %d\n", i ,code[i]); 
	runtime_code[i] = code[i];
    }
    printf("\n\nCREATING NODE LIST\n\n");
    //main_address is calculated as below
    //1. main_addr + main_size = 26 + 39 = 65
    // now, we have the end of main (65 in our example).
    //2. since code array starts from 0 (unlike stack), therefore we must deacrese code_size from that
    // 65-65 = 0
    
    int row_num = sizeof(dictionary)/sizeof(dictionary[0]);
    int main_size = 0;
    for(int j =0; j<row_num; j++){
    	if(dictionary[j][0] == main_addr){
    		main_size = dictionary[j][1];
    	}
    }
    
    int main_start_ind;
    main_start_ind = main_addr + main_size;
    main_start_ind = code_size - main_start_ind;
    

    program_APG_node_list = create_list(main_addr);
    print_nodes(program_APG_node_list);
    printf("\n\nEXPANDING NODE LIST\n\n");
    struct AGP_node *temp = program_APG_node_list;
    temp = temp->next;
    temp = temp->next;
    expansion(temp);
    print_nodes(program_APG_node_list);

   /* program_APG_node_list = generate_list(main_start_ind);
    print_nodes(program_APG_node_list);

    printf("\n\nSCHEDULING NODES\n\n");    
    for(int i = 0; i<NUM_CPU; i++){
	cpus[i]->node_to_execute = schedule_me(cpus[i]->cpu_num);
    }
  
    printf("\n\nREFACTORING NODE DESTINATIONS\n\n");   
    for(int i = 0; i<NUM_CPU; i++){
    	refactor_destinations(cpus[i]->node_to_execute, program_APG_node_list);
    }
    print_nodes(program_APG_node_list);

    printf("\n\nLAUNCHING THREADS!!!\n\n"); 
    for(int i = 0; i<NUM_CPU; i++){
	pthread_create(&(thread_id[cpus[i]->cpu_num-1]), NULL, &CPU_start, cpus[i]);
        if(cpus[i]->node_to_execute->node_num == DUMMY_NODE)
		cpu_status[cpus[i]->cpu_num-1] = CPU_IDLE;
	else
		cpu_status[cpus[i]->cpu_num-1] = CPU_UNAVAILABLE;
    }
    
    /***********************/
    /**** Simulation end ***/
    /***********************/

    //wait for all active cpu threads to finish
 /*   int num_cpu_idle = 0;
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
	printf("code[%d]: %d\n", i ,runtime_code[i]); 
    }
  */

    printf("\n\n***SIMULATION COMPLETE***\n\n");
    return 0;
}

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































