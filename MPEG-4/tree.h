#ifndef TREE_H__
#define TREE_H__
#include <stdbool.h>
#include <stdlib.h>
// tree:
// left is child, right is brother
// (bitree) 

typedef struct box_data__       // common box's data
{
    unsigned char version;      //
    unsigned char flags[3];
    int entrycount;             // the count of the entity of the data (just some box)
    void* full_data;            // "full box" 's data
}box_data;
typedef struct box__
{
    char name[4];               // box 's  name
    long pos_start;             // start position at file
    long size;                  // box's size
    box_data* common_data;      // 1char version, 3chars flags and a void pointer
}box;

// elem means the data of select type that the node has;
typedef box tree_elem;
typedef struct tree_node__ tree_node;
struct tree_node__
{

    int depth; // the node's depth
    tree_elem item;// the node's data

    int index; // the index of the node insert the tree
    
    tree_node* l_child;
    tree_node* r_brother;

};
typedef struct tree__
{
    int count;
    tree_node* head;
}tree;
tree* InitTree();
void DeleteTree(tree* t);
void Tree_FindItem(tree* t);
// return the node that be inserted
tree_node* Tree_InsertItem(tree_node* tn, tree_elem* item, bool isChild);
// first root
void Tree_Traverse_ROOT1(tree* t, bool(*f)(tree_node* n));
// middle root
void Tree_Traverse_ROOT2(tree* t, bool(*f)(tree_node* n));


tree_node* Tree_GetRoot(tree* t);


bool FindTreeNode_AddressEqual(tree_node* tofind, tree_node* cur_visiting);
void FindNode(  tree* t, tree_node* tofind, \
                bool (*f)(tree_node* tofind, tree_node* cur_visiting), \
                tree_node** finded\
);
// in tree* t, find the 1st Node that meat the function "f" , some may need a node value "tofind",
// return the pre node of the finded node with "pre"
// for the function, "true" means satisfying the condition
void FindNodeParent(    tree* t, tree_node* tofind, \
                        bool (*f)(tree_node* tofind, tree_node* cur_visiting), \
                        tree_node** tn_pre\
);



//---------------------------------------------------------------
// seqstack  for treenode
//
typedef tree_node* stack_elem;
typedef struct seqstack__
{
    stack_elem* data;
    
    int depth_use;
    int depth_max;
}seqstack;
seqstack* InitSeqStack();
void DeleteSeqStack(seqstack* s);
void Push(seqstack* s, stack_elem e);
void Pop(seqstack* s, stack_elem* e);
void Top(seqstack* s, stack_elem* e);
bool IsEmpty(seqstack* s);
bool IsFull(seqstack* s);
void SetZero(seqstack* s);
//---------------------------------------------------------------
// it will change the value of "N"
#define TraverseNode(N, f) \
seqstack* s = InitSeqStack();\
Push(s, N);\
N = N->l_child;\
while(!IsEmpty(s))\
{\
    if(N)\
    {\
        Push(s, N);\
        f;\
        N = N->l_child;\
    }\
    else\
    {\
        Pop(s, &N);\
        N = N->r_brother;\
    }\
}\
DeleteSeqStack(s);
#endif