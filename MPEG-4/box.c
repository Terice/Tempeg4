#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"box.h"


Box* InitBox(char* name, size_t size)
{
    Box* box = (Box*)malloc(sizeof(Box));

    // give a name 
    if(name == NULL) strncpy(box->name, "NULL", 4);    
    else             strncpy(box->name, name, 4); 
    // give the init depth
    box->depth = 0;
    box->data = NULL;

    box->pos_start = 0L;
    box->size = size;

    box->data = NULL; 

    box->l_child = NULL;
    box->r_broth = NULL;

    box->beforeBox = NULL;
    //printf("init a box: %s \t %ld\n", box->name, box->size);
    return box;
}

void DeleteBox(Box* box)
{
    if(box->data != NULL) 
    {
        DeleteBoxData(box->data);
        // printf("box:%s's data is freed:   ", box->name);
    }
    if(box->r_broth != NULL) DeleteBox(box->r_broth);
    if(box->l_child != NULL) DeleteBox(box->l_child);
    // printf("%s is freed\n", box->name);
    free(box);
}
// fn: insert a copied box into box
// re: the box be inserted
// "New" means malloc a new box, 
// so all data is copied from resource box
Box* InsertNewBox(Box* box, Box* boxToInsert)
{
    Box* tmp = InitBox(box->name, box->size);
    tmp->pos_start = box->pos_start;
    if(boxToInsert->l_child == NULL) ConnectTwoBox(boxToInsert, tmp, "O2I");
    else ConnectTwoBox(FindLastBox(boxToInsert), tmp, "B2A");

    tmp->depth = boxToInsert->depth + 1;
    //printf("----box insert: %s to %s\n", box->name, boxToInsert->name);
    return tmp;
}
// fn: insert the exist pointer of box
// re: the box be inserted
// "Exist" means just add the pointer into the tree
// so shouldn't edit the resource box in other way
Box* InsertExistBox(Box* box, Box* boxToInsert)
{
    if(boxToInsert->l_child == NULL) ConnectTwoBox(boxToInsert, box, "O2I");
    else ConnectTwoBox(FindLastBox(boxToInsert), box, "B2A");
    // boxToInsert->size += box->size;
    box->depth = boxToInsert->depth + 1;
    //printf("----box insert: %s to %s\n", box->name, boxToInsert->name);
    return box;
}

//take out an box and its all child box, free space
void ClearUpBox(Box* box)
{
    if(box->l_child != NULL)
    {
        DeleteBox(box->l_child);
        box->l_child = NULL;
    }        
}


//print all box infomation
void PrintBox(Box* box)
{
    //\e[31;1m[OK]\e[0m
    int depth;
    if(box != NULL)
    {
        depth = box->depth;
        while(depth--){printf("\t");}
#ifdef LINUX_COLORED_TERMINAL
        printf("name:%4s pos_start:\e[31;1m0x%-8x\e[0m size:\e[31;1m%10ld\e[0m ", box->name, box->pos_start, box->size);
#else
        printf("name:%4s pos_start:0x%-8x size:%10ld", box->name, box->pos_start, box->size);
#endif
        PrintBoxData(box);
        printf("\n");
        if(box->l_child != NULL) PrintBox(box->l_child);
        if(box->r_broth != NULL) PrintBox(box->r_broth);
    }
}

//"O2I""B2A"
void ConnectTwoBox(Box* b1, Box* b2, char* mode)
{
    if(!strncmp(mode, "B2A", 3)){b1->r_broth = b2; b2->beforeBox = b1;}
    else if(!strncmp(mode, "O2I", 3)){b1->l_child = b2; b2->outBox = b1;}
}

//find box with its name in a big box
Box* FindBoxLink(char* name, Box* box, Box* boxToReturn)
{
    Box* tmp;
    tmp = box;

    if(tmp->l_child != NULL && strncmp(tmp->name, name, 4))  FindBoxLink(name, tmp->l_child, boxToReturn);
    if(tmp->r_broth != NULL && strncmp(tmp->name, name, 4))  FindBoxLink(name, tmp->r_broth, boxToReturn);
    if(!strncmp(tmp->name, name, 4) && boxToReturn != NULL) 
    {   
        InsertExistBox(tmp, boxToReturn);
    }
    return tmp;
}

Box* FindBox0(char* name, Box* box, Box* result)
{

    if(!strncmp(box->name, name, 4L)) result = box;

    if(result == NULL)
    {
        if(box->l_child != NULL) result = FindBox0(name, box->l_child, result);
        if(box->r_broth != NULL) result = FindBox0(name, box->r_broth, result);
    }

    return result;
}
Box* FindBox(char* name, Box* box)
{
    Box* re;

    re = NULL;
    return FindBox0(name, box, re);
}
int FindBoxs0(char* name, Box* box, Box* result[], int* i, int length)
{
    int re;

    re = 0;
    if(!strncmp(box->name , name, 4L)) 
    {
        if(*i >= length)
        {
            printf("result length is not enough\n");
            return -1;
        }; 
        result[*i] = box; 
        (*i)++;
    }
    if(box->l_child != NULL) FindBoxs0(name, box->l_child, result, i, length);
    if(box->r_broth != NULL) FindBoxs0(name, box->r_broth, result, i, length);
    return re = (*i) >= length ? -1:(*i);
}
int FindBoxs(char* name, Box* box, Box* result[], int length)
{
    int i;

    i = 0;
    return FindBoxs0(name, box, result, &i, length);
}

//find the first leaf box wrap in the "box"
Box* FindLastBox(Box* box)
{
    Box* tmp;

    if(box->l_child == NULL) return NULL;
    else 
    {
        tmp = box->l_child;
        while(tmp->r_broth != NULL){tmp = tmp->r_broth;}
        return tmp;
    }
}
// //find the first leaf box wrap in the "box"
    // Box* FindLastBox(Box* box)
    // {
    //     Box* tmp;

    //     if(box->l_child == NULL) return NULL;
    //     else 
    //     {
    //         tmp = box->l_child;
    //         while(tmp->r_broth != NULL){tmp = tmp->r_broth;}
    //         return tmp;
    //     }
    // }
//

// find whick box wrap the box && it can continue wrap
// can continue wrap means that still have space to put box
Box* FindWrapBoxCanUse(Box* box)
{
    Box* tmp;

    tmp = box;
    while(1)
    {
        while(tmp->outBox == NULL)
        {
            if(tmp->beforeBox != NULL) tmp = tmp->beforeBox;
            else return NULL;
        }
        if(box->pos_start + box->size == tmp->outBox->pos_start + tmp->outBox->size)
        {
            tmp = tmp->outBox;
        }
        else break;
    }
    return tmp->outBox;
    printf("its wrap box is %s\n", tmp->outBox->name);
}



// the before is operation
// void ErgodicOperation(void (*OperationFn)(Box*), Box* boxToErgodic)
// {
//     if(boxToErgodic->l_child != NULL) Ergodic(OperationFn,boxToErgodic->l_child);
//     if(boxToErgodic->r_broth != NULL) Ergodic(OperationFn, boxToErgodic->r_broth);

//     OperationFn(boxToErgodic);
// }

Data* InitBoxData()
{
    Data* data = (Data*)malloc(sizeof(Data));
    data->data = NULL;

    data->version = 0xF;
    strncpy(data->flags, "NUL", 3);

    return data;
}
void DeleteBoxData(Data* data)
{
    if(data->data != NULL) 
    {
        free(data->data);
        //printf("data-> freed,");
    };
    //printf("   and data is freed\n");
    free(data);
}

// print version and flags
void PrintBoxData(Box* box)
{
    if(box != NULL)
    {
        if(box->data != NULL)
        {
            printf("versiong: %d, flags: %x %x %x", box->data->version, box->data->flags[0],box->data->flags[1],box->data->flags[2]);
        }
    }
}

int TraverseBox(Box* root, void (*Operation)(Box* b))
{
    if(root == NULL) return -1;
    else
    {
        if(root->l_child != NULL) TraverseBox(root->l_child, Operation);
        if(root->r_broth != NULL) TraverseBox(root->r_broth, Operation);
        Operation(root);
    }
}