#ifndef CPU_H
#define CPU_H

#define MAX_MEM 1024*64
//#include <stdbool.h>



#define code_expansion 	0
#define code_input		1
//#define code_output		2 //output isn't really an operation, it's a destination
#define code_plus 		3
#define code_times		4
#define	code_is_equal	5
#define	code_is_less	6
#define code_is_greater	7
#define	code_if			8 
#define code_else		9 
#define code_minus		10
#define code_merge		11
#define code_identity	12

#define code_output		0xFFFFFFFF	//convenient to have it set to a special value that can be tested at runtime
#define NAV			0xFFFFFFFC
//Dead operator: remove it
#define DEAD 		0xFFFFFFFF
//Can process corresponding operation and send result to destinations (all arguments resolved)
#define READY 		0


// A linked list node
struct QNode {
    int value;
    struct QNode* next;
};
  
// The queue which is a pointer to the front and rear node
struct Queue {
    int size;
    struct QNode *front, *rear;
};

struct cpu{

	int code[64]; //chunk of code
	int node_size; //actual the size of stack/code
	int code_address;
	//bool has_dependent;
	//int dependents_num; //specify the number of dependency if has any

	//int cpu_queue[4]; // each cpu has its own queue
	int connection[4]; //this is used for connection purpose between CPUs (e.g. connection[1,1,1,0] means that cpu 1 is connected to itself, cpu 2, cpu 3, but not to cpu 4)
	
	
	int assigned_cpu; //cpu the node is assinged to or currently being processed on 
	int cpu_dest; 	//destination cpu
	int dest_node;  //destination in node list (used in allocation)

	//struct cpu *cpu_source;
	//struct cpu *cpu_dest;
	struct cpu *next;
};


struct cpu_out{
	int value;
	struct Queue* cpu_queue;
	int dest;
	int addr; //destination could also be a tuppl, but we need cpu num and its stack destination address
};



void *CPU_start();
//void execute(struct memory *mem,struct cpu *CPU);
//void fetch_task();

struct QNode* newNode(int val);
struct Queue* createQueue();
void enQueue(struct Queue* q, int val);
int deQueue(struct Queue* q);



#endif
