#ifndef BOX_H_
#define BOX_H_
#include<stdio.h>


typedef struct FullBoxData
    {
        unsigned char version;
        unsigned char flags[3];
        void* data;
    }Data;
typedef struct box Box;
struct box
{

    char name[4];
    unsigned long int startPos;
    unsigned long int size;

    char rank;
    Data* data;

    Box* nextBox;
    Box* beforeBox;
    Box* inBox;
    Box* outBox;

};


Box* InitBox(char* name, size_t size);
void ClearUpBox(Box* box);
void DeleteBox(Box* boxRoot);
Box* InsertNewBox(Box* box, Box* boxToInsert);
Box* InsertExistBox(Box* box, Box* boxToInsert);

//"O2I""B2A"
void ConnectTwoBox(Box* b1, Box* b2, char* mode);

void PrintBoxData(Box* box);
void PrintBox(Box* box);
//find box with its name in a big box
void FindBox(char* name, Box* box, Box* result);
Box* FindBox0(char* name, Box* box);
Box* FindBoxLink(char* name, Box* box, Box* boxToReturn);

//find the last box in the "box"
Box* FindLastBox(Box* box);
Box* FindWrapBoxCanUse(Box* box);

Data* InitBoxData();
void DeleteBoxData(Data* data);
// void ErgodicOperation(void (*OperationFn)(Box*), Box* boxToErgodic);

#endif