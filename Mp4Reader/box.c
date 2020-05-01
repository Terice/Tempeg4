#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"box.h"

Box* InitBox(char* name, size_t size)
{
    Box* box = (Box*)malloc(sizeof(Box));

    strncpy(box->name, name, 4);
    box->nextBox = NULL;
    box->beforeBox = NULL;
    box->rank = 0;
    box->data = NULL;
    box->startPos = 0L;
    box->size = size;
    box->inBox = NULL;
    box->data = NULL;
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
    if(box->inBox != NULL) DeleteBox(box->inBox);
    if(box->nextBox != NULL) DeleteBox(box->nextBox);
    //printf("%s is freed\n", box->name);
    free(box);
}

//return the box be inserted
Box* InsertNewBox(Box* box, Box* boxToInsert)
{
    Box* tmp = InitBox(box->name, box->size);
    tmp->startPos = box->startPos;
    if(boxToInsert->inBox == NULL) ConnectTwoBox(boxToInsert, tmp, "O2I");
    else ConnectTwoBox(FindLastBox(boxToInsert), tmp, "B2A");

    tmp->rank = boxToInsert->rank + 1;
    //printf("----box insert: %s to %s\n", box->name, boxToInsert->name);
    return tmp;
}

Box* InsertExistBox(Box* box, Box* boxToInsert)
{
    if(boxToInsert->inBox == NULL) ConnectTwoBox(boxToInsert, box, "O2I");
    else ConnectTwoBox(FindLastBox(boxToInsert), box, "B2A");
    // boxToInsert->size += box->size;
    box->rank = boxToInsert->rank + 1;
    //printf("----box insert: %s to %s\n", box->name, boxToInsert->name);
    return box;
}

//take out an box and its all child box, free space
void ClearUpBox(Box* box)
{
    Box* tmp;
    if(box->inBox != NULL)
    {
        tmp = box->inBox;
        DeleteBox(tmp);
    }
}


//print all box infomation
void PrintBox(Box* box)
{
    Box* tmp;
    char rank;
    
    if(tmp != NULL)
    {
        tmp = box;
        rank = box->rank;
        while(rank--){printf("\t");}
        printf("name:%4s startPos:%10ld size:%10ld", tmp->name, tmp->startPos, tmp->size);
        PrintBoxData(tmp);
        printf("\n");
        if(tmp->inBox != NULL) PrintBox(tmp->inBox);
        if(tmp->nextBox != NULL) PrintBox(tmp->nextBox);
    }
}
//"O2I""B2A"
void ConnectTwoBox(Box* b1, Box* b2, char* mode)
{
    if(!strncmp(mode, "B2A", 3)){b1->nextBox = b2; b2->beforeBox = b1;}
    else if(!strncmp(mode, "O2I", 3)){b1->inBox = b2; b2->outBox = b1;}
}

//find box with its name in a big box
Box* FindBoxLink(char* name, Box* box, Box* boxToReturn)
{
    Box* tmp;
    tmp = box;

    if(tmp->inBox != NULL && strncmp(tmp->name, name, 4))  FindBoxLink(name, tmp->inBox, boxToReturn);
    if(tmp->nextBox != NULL && strncmp(tmp->name, name, 4))  FindBoxLink(name, tmp->nextBox, boxToReturn);
    if(!strncmp(tmp->name, name, 4) && boxToReturn != NULL) 
    {   
        InsertExistBox(tmp, boxToReturn);
    }
    return tmp;
}
void FindBox(char* name, Box* box, Box* result)
{
    result = FindBox0(name, box);
}

Box* FindBox0(char* name, Box* box)
{
    Box* result;
    Box* tmp;
    tmp = box;
    if(tmp->inBox != NULL && strncmp(tmp->name, name, 4)) FindBox(name, tmp->inBox, result);
    if(tmp->nextBox != NULL && strncmp(tmp->name, name, 4)) FindBox(name, tmp->nextBox, result);

    if(!strncmp(tmp->name, name, 4L)) 
        result = tmp;

    return result;
}

//find the first leaf box wrap in the "box"
Box* FindLastBox(Box* box)
{
    Box* tmp;

    if(box->inBox == NULL) return NULL;
    else 
    {
        tmp = box->inBox;
        while(tmp->nextBox != NULL){tmp = tmp->nextBox;}
        return tmp;
    }
}
// //find the first leaf box wrap in the "box"
    // Box* FindLastBox(Box* box)
    // {
    //     Box* tmp;

    //     if(box->inBox == NULL) return NULL;
    //     else 
    //     {
    //         tmp = box->inBox;
    //         while(tmp->nextBox != NULL){tmp = tmp->nextBox;}
    //         return tmp;
    //     }
    // }
//

//find whick box wrap the box && it can continue wrap
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
        if(box->startPos + box->size == tmp->outBox->startPos + tmp->outBox->size)
        {
            tmp = tmp->outBox;
        }
        else break;
    }

    return tmp->outBox;
    printf("its wrap box is %s\n", tmp->outBox->name);
}

void PrintBoxData(Box* box)
{
    if(box != NULL)
    {
        if(box->data != NULL && box->data->data != NULL)
        {
            printf("versiong: %d, flags: %x %x %x", box->data->version, box->data->flags[0],box->data->flags[1],box->data->flags[2]);
        }
    }
}

//the before is operation
// void ErgodicOperation(void (*OperationFn)(Box*), Box* boxToErgodic)
// {
//     if(boxToErgodic->inBox != NULL) Ergodic(OperationFn,boxToErgodic->inBox);
//     if(boxToErgodic->nextBox != NULL) Ergodic(OperationFn, boxToErgodic->nextBox);

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