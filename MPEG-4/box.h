#ifndef BOX_H_
#define BOX_H_
#include<stdio.h>

// #ifndef LINUX_TERMINAL
// #define LINUX_TERMINAL
// #endif

typedef struct FullBoxData
{
    unsigned char version;
    unsigned char flags[3];
    void* data;
    int entrycount;
}Data;
typedef struct box Box;
struct box
{

    char name[4];
    unsigned long int pos_start;
    unsigned long int size;

    char depth;
    Data* data;

    Box* r_broth;
    Box* l_child;
    Box* beforeBox;
    Box* outBox;

};

// init a box by name and size
// if need a empty box:
// name = NULL, size = 0L;
Box* InitBox(char* name, size_t size);
void ClearUpBox(Box* box);
// delete a box and free all data
void DeleteBox(Box* boxRoot);
Box* InsertNewBox(Box* box, Box* boxToInsert);
Box* InsertExistBox(Box* box, Box* boxToInsert);

//"O2I""B2A"
void ConnectTwoBox(Box* b1, Box* b2, char* mode);

void PrintBox(Box* box);

Box* FindBoxLink(char* name, Box* box, Box* boxToReturn);
//find box with its name in a big box
Box* FindBox(char* name, Box* box);
int FindBoxs(char* name, Box* box, Box* result[], int length);

//find the last box in the "box"
Box* FindLastBox(Box* box);
Box* FindWrapBoxCanUse(Box* box);

// fn: traverse operation for every node of the tree
// note; not use until now
int TraverseBox(Box* root, void (*Operation)(Box* b));

// used for full box data
Data* InitBoxData();
void DeleteBoxData(Data* data);
void PrintBoxData(Box* box);

#endif