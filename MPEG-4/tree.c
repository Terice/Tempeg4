#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


static bool FreeTreeNode(tree_node* tn);
static tree_node* MakeTreeNode(tree_elem* e);
static void DeleteNode(tree* t, tree_node* tn);
static void InsertNode(tree_node* beinsert, tree_node* toinsert, bool ischild);

static void TraverseNode_ROOT1(tree_node* t, bool (*f)(tree_node* cur_visiting));
static void TraverseNode_ROOT2(tree_node* t, bool (*f)(tree_node* cur_visiting));

tree_node* Tree_GetRoot(tree* t){return t->head ? t->head->l_child : NULL;}

tree* InitTree()
{
    tree* t = (tree*)malloc(sizeof(tree));

    tree_node* head;
    t->head = (tree_node*)malloc(sizeof(tree_node));
    t->count = 0;
    
    head = t->head;
    head->depth = 0;
    head->l_child = NULL;
    head->r_brother = NULL;

    return t;
}
void DeleteTree(tree* t)
{
    if(!t) return;//err
    if(t->head) DeleteNode(t, t->head);
    free(t);
}


void Tree_Traverse_ROOT1(tree* t, bool(*f)(tree_node* n)){return TraverseNode_ROOT1(t->head,f);};
void Tree_Traverse_ROOT2(tree* t, bool(*f)(tree_node* n)){return TraverseNode_ROOT2(t->head,f);};
tree_node* Tree_InsertItem(tree_node* tn, tree_elem* item, bool isChild)
{
    tree_node* tn_new = MakeTreeNode(item);
    InsertNode(tn, tn_new, isChild);
    return tn_new;
}


// -----------------------------------------------

// the below is the function to operation the node

void FindNode(  tree* t, tree_node* tofind,\
                bool (*f)(tree_node* tofind, tree_node* cur_visiting), \
                tree_node** finded\
)
{
    tree_node *cur,*tn;
    tn = Tree_GetRoot(t);
    
    seqstack* s = InitSeqStack();
    Push(s, tn);
    cur = tn->l_child;
    while(!IsEmpty(s))
    {
        int result;
        if(cur)
        {
            Push(s, cur); 
            result = (*f)(tofind, cur);
            if(result) break;
            cur = cur->l_child;
        }
        else
        {
            Pop(s, &cur);
            cur = cur->r_brother;
        }
    }
    DeleteSeqStack(s);
    *finded = cur;
}
// in tree* t, find the 1st Node that meat the function "f" , some may need a node value "tofind",
// return the pre node of the finded node with "pre"
// for the function, "true" means satisfying the condition
void FindNodeParent(    tree* t, tree_node* tofind, \
                        bool (*f)(tree_node* tofind, tree_node* cur_visiting), \
                        tree_node** tn_pre\
)
{
    tree_node *pre, *cur, *r;
    
    seqstack* s = InitSeqStack();SetZero(s);
    pre = NULL;
    // find the first node of this layer(the head of the list    h->h1->h2->h_find->h3)
    Push(s, t->head);
    cur = t->head->l_child;
    while(!IsEmpty(s))
    {
        int result;
        if(cur)
        {
            Push(s, cur); 
            cur = cur->l_child;
        }
        else
        {
            Top(s, &cur);
            if(cur->r_brother && cur->r_brother != r)
            {
                cur = cur->r_brother;
                Push(s, cur);
                cur = cur->l_child;
            }
            else
            {
                Pop(s, &cur);
                result = (*f)(tofind, cur);
                if(result) break;
                r = cur;
                cur = NULL;
            }
        }
    }
    Top(s, &pre);
    *tn_pre = pre;
    DeleteSeqStack(s);
}


void TraverseNode_ROOT2(tree_node* t, bool (*f)(tree_node* cur_visiting))
{    
    tree_node *cur;
    
    seqstack* s = InitSeqStack();
    Push(s, t);
    cur = t->l_child;
    while(!IsEmpty(s))
    {
        int result;
        if(cur)
        {
            Push(s, cur);
            cur = cur->l_child;
        }
        else
        {
            Pop(s, &cur);
            result = (*f)(cur);
            cur = cur->r_brother;
        }
    }
    DeleteSeqStack(s);
}
void TraverseNode_ROOT1(tree_node* t, bool (*f)(tree_node* cur_visiting))
{    
    tree_node *cur;
    
    seqstack* s = InitSeqStack();
    Push(s, t);
    cur = t->l_child;
    while(!IsEmpty(s))
    {
        int result;
        if(cur)
        {
            Push(s, cur); result = (*f)(cur);
            cur = cur->l_child;
        }
        else
        {
            Pop(s, &cur);
            cur = cur->r_brother;
        }
    }
    DeleteSeqStack(s);
}
void TraverseNode_ROOT3(tree_node* t, bool (*f)(tree_node* cur_visiting))
{    
    tree_node *cur, *r;
    
    seqstack* s = InitSeqStack();
    Push(s, t);
    r = NULL;
    cur = t->l_child;
    while(!IsEmpty(s))
    {
        int result;
        if(cur)
        {
            Push(s, cur); 
            cur = cur->l_child;
        }
        else
        {
            Top(s, &cur);
            if(cur->r_brother && cur->r_brother != r)
            {
                cur = cur->r_brother;
                Push(s, cur);
                cur = cur->l_child;
            }
            else
            {
                Pop(s, &cur);
                result = (*f)(cur);
                r = cur;
                cur = NULL;
            }
            
        }
    }
    DeleteSeqStack(s);
}
bool FindTreeNode_AddressEqual(tree_node* tofind, tree_node* cur_visiting){return tofind == cur_visiting ? true : false;}
bool FreeTreeNode(tree_node* tn){if(tn) free(tn);}
tree_node* MakeTreeNode(tree_elem* e)
{
    tree_node* tn = (tree_node*)malloc(sizeof(tree_node));
    tn->item = *e;
    return tn;
}




void InsertNode(tree_node* beinsert, tree_node* toinsert, bool ischild)
{
    tree_node* tn;
    if(ischild)
    {
        if(!beinsert->l_child) beinsert->l_child = toinsert;
        else
        {
            tn = beinsert->l_child;
            while(tn->r_brother){tn = tn->r_brother;}
            tn->r_brother = toinsert;
        }
        toinsert->depth = beinsert->depth + 1;
    }
    else
    {
        tn = beinsert;
        while(!tn->r_brother){tn = tn->r_brother;}
        toinsert->depth = beinsert->depth;
    }
}
void DeleteNode(tree* t, tree_node* tn)
{
    tree_node* pre;
    FindNodeParent(t, tn, FindTreeNode_AddressEqual, &pre);

    if(!pre) return;
    if(pre->l_child != tn) pre = pre->l_child;
    while(pre->r_brother != tn) pre = pre->r_brother;
    
    if(tn->r_brother) // reconnect the pre and tn's brother
    {
        if(pre->l_child == tn) pre->l_child = tn->r_brother;
        else pre->r_brother = tn->r_brother;
    }
    TraverseNode_ROOT1(tn, FreeTreeNode);
}



inline bool IsEmpty(seqstack* s){return s->depth_use == 0 ? true : false;}
inline bool IsFull(seqstack* s){return s->depth_use == s->depth_max ? true : false;}
static void ReAlloc(seqstack* s)
{
    stack_elem* e;
    if(s->data) 
    {
        s->depth_max *= 2;
        e = (stack_elem*)malloc(sizeof(stack_elem) * s->depth_max);
        for(int i = 0; i < s->depth_max; i++)
        {
            e[i] = s->data[i];
        }
        free(s->data);
        s->data = e;
    }
    else
        // return an err
       return;
}
#define SEQSTACK_INITSIZE 40
seqstack* InitSeqStack()
{
    seqstack* s = (seqstack*)malloc(sizeof(seqstack));
    s->data = (stack_elem*)malloc(sizeof(stack_elem) * SEQSTACK_INITSIZE);
    s->depth_use = 0;
    s->depth_max = SEQSTACK_INITSIZE;
    return s;
}
void Push(seqstack* s, stack_elem e){if(IsFull(s)) ReAlloc(s);s->data[++s->depth_use] = e;}
void Pop(seqstack* s, stack_elem* e){if(IsEmpty(s)) return;*e = s->data[s->depth_use--];}
void Top(seqstack* s, stack_elem* e){if(IsEmpty(s)) return;*e = s->data[s->depth_use];}
void DeleteSeqStack(seqstack* s){if(s->data) free(s->data);free(s);}
void SetZero(seqstack* s){s->data[0] = NULL;}