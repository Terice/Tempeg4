#include"mpeg-4.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"box.h"

Box* ParserContainer_mpeg4(FILE* fp, Box* data)
{
    // get all file's size and make root box
    fseek(fp, 0L, SEEK_END);
    strncpy(data->name, "FILE", 4);
    data->size = ftell(fp);
    rewind(fp);

    ReadInfoIntoBox(fp, data);
    PrintBox(data);
    printf("box's structure constructed\n");
    ReadDataIntoBox(fp, data);

    return data;
}

// read the box data into box
void ReadDataIntoBox0(FILE* fp, Box* box)
{
    Data* data;
    if(box->l_child != NULL) ReadDataIntoBox0(fp, box->l_child);
    if(box->r_broth != NULL) ReadDataIntoBox0(fp, box->r_broth);

    // if a box dont have child box, it is calc as a "fullbox"
    if(box->l_child == NULL && strncmp(box->name, "mdat", 4) && strncmp(box->name, "ftyp", 4) && box->size > 8L)
    {
        fseek(fp, box->pos_start + 8L, SEEK_SET);
        // malloc the box 
        data = (Data*)malloc(sizeof(Data));
        // data->data = (unsigned char*)malloc(box->size - 12L);

        // read the version and flags
        fread(&(data->version), sizeof(char), 1, fp);
        fread(&(data->flags), sizeof(char), 3, fp);
        RevertBigEndingChar(data->flags, 3);// revert the sort of flags
        
        // process the different kind of data here
        DataPareser(fp, box);
        box->data = data;
    }
}
void ReadDataIntoBox(FILE* fp, Box* box)
{
    ReadDataIntoBox0(fp, box);
    rewind(fp);
}
// fn: read information into box from fp
// re: void
// this function is used to construct the mp4's file structure
// fp will be rewind
// data is in box(box must be all file, especially the size)
void ReadInfoIntoBox(FILE* fp,  Box* rootBox)
{
    Box * upBox, * tmp;
    Box* usingBox;
    char containerBox;

    usingBox = InitBox(NULL, 0);
    upBox = rootBox;

    // compare to the file box
    while(ftell(fp) <= rootBox->size)
    {
        if(!ReadBoxInfo(fp, usingBox)) break;
        
        tmp = InsertNewBox(usingBox, upBox);

        // if it is a container box, 
        // then go forward 8 char(size + name)
        // at this container's data start position
        containerBox = IsContainerBox(fp, ftell(fp), usingBox->size);
        if(containerBox > 0) 
        {
            fseek(fp, 8L, SEEK_CUR);
            upBox = tmp;
        }
        else fseek(fp, usingBox->size, SEEK_CUR); // else skip this box
        
        // if at the container box's end, 
        // then find the container box's wrap box
        if(ftell(fp) >= upBox->pos_start + upBox->size)
        {
            upBox = FindWrapBoxCanUse(upBox);
        }
    }
    DeleteBox(usingBox);
    rewind(fp);
}


// to read the basic info of the box
// fp(data) ---->  box
int ReadBoxInfo(FILE* fp, Box* box)
{
    //read size - read type - read data
    unsigned char tmp[4];

    // size
    if(fread(tmp, sizeof(char), 4, fp) < 4) return 0;
    box->size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
    // name
    fread(tmp, sizeof(char), 4, fp);
    strncpy(box->name, tmp, 4);
    fseek(fp, -8L, SEEK_CUR);
    box->pos_start = ftell(fp);

    return 1;
}

char IsFullBox(FILE* fp, size_t boxpos_start, size_t boxSize)
{
    if(!IsContainerBox(fp, boxpos_start, boxSize)) return 1;
    else return 0;
}
// this is to analyse whether it is a container box
// return the count of contained box
// FILE* fp must be set at the start of box's start(the pos of size's start)
char IsContainerBox(FILE* fp, size_t boxpos_start, size_t boxSize)
{
    char result;
    unsigned long size;
    unsigned char tmp[4];

    

    fseek(fp, 4L, SEEK_CUR);
    fread(tmp, sizeof(char), 4, fp);
    if(!strncmp(tmp, "mdat", 4)) 
    {
        fseek(fp, boxpos_start, SEEK_SET);
        return 0;
    }
    while(ftell(fp) < boxpos_start + boxSize)
    {
        fread(tmp, sizeof(char), 4, fp);
        size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
        fseek(fp, -4L, SEEK_CUR);
        fseek(fp, size, SEEK_CUR);
        result++;
        if(size == 0) {result = 0; break;}
    }
    if(ftell(fp) != boxpos_start + boxSize || boxSize == 8L || size == 0) result = 0;

    fseek(fp, boxpos_start, SEEK_SET);
    
    return result;
}

//change BigEnd Code to normal
void RevertBigEndingChar(char* bytesToRevert, size_t length)
{
    size_t i;
    char tmp;

    i = 0;
    for(i = 0; i < length / 2; i++)
    {
        tmp = bytesToRevert[i];
        bytesToRevert[i] = bytesToRevert[length - 1 - i];
        bytesToRevert[length - 1 - i] = tmp;
    }
}
// fn: change BigEnd char to number
// re: the calc number
uint64 ChangeCharARToNumber(unsigned char* resource, size_t length)
{
    uint64 result;

    result = 0;
    RevertBigEndingChar(resource, length);
    
    for (int i = 0; i < length; i++)
    {
        //printf("%2x", resource[i]);
        result += (long)resource[i]<<(8 * i);
        //printf("result :%ld\n", result);
    }
    
    // printf("result :%ld\n", result);
    return result;
}

// fn: decode the date of the box
// note: no postion's need for the fp
// ------it will set fp at correct postion
// ------decode by the name of the box
void DataPareser(FILE* fp, Box* box)
{
    unsigned char entry_count[4];// the entry's number(in char type-----the data in file)
    uint32 entryCount; // the entry's number
    unsigned char tmp[4];
    char* name = box->name;

    // 4(size) + 4(name) + 4(version 1 + flags 3)
    // so skip forward 12L at the data's start postion
    box->data = (Data*)malloc(sizeof(Data*));
    fseek(fp, box->pos_start + 12L, SEEK_SET);
    if(!strncmp(name, "stsd", 4))
    {
        avcC* avccTmp = (avcC*)malloc(sizeof(avcC));
        avc1* avc1Tmp = (avc1*)malloc(sizeof(avc1));
        Box* exchange;

        // 4 uchar of entry's count
        fread(tmp, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(tmp, 4);

        stsd* sample = (stsd*)malloc(sizeof(stsd));
        sample->sample_count = entryCount;


        fread(tmp, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(tmp, 4);
        
        exchange = InsertExistBox(InitBox("avc1", (size_t)entryCount), box);
        exchange->pos_start = ftell(fp) - 4L;
        // PrintBox(box);

        fseek(fp, 4L, SEEK_CUR);
        fread_m(&avc1Tmp->reserved               , fp, 1, 6);
        fread_m(&avc1Tmp->data_reference_index   , fp, 0, 2);
        fread_m(&avc1Tmp->pre_defined_0          , fp, 0, 2);
        fread_m(&avc1Tmp->reserved_2             , fp, 0, 2);
        fread_m(&avc1Tmp->pre_defined            , fp, 0, 12);
        fread_m(&avc1Tmp->width                  , fp, 0, 2);
        fread_m(&avc1Tmp->height                 , fp, 0, 2);
        fread_m(&avc1Tmp->horizons_res           , fp, 0, 4);
        fread_m(&avc1Tmp->vertical_res           , fp, 0, 4);
        fread_m(&avc1Tmp->reserved_3             , fp, 0, 4);
        fread_m(&avc1Tmp->frame_count            , fp, 0, 2);
        fread_m(&avc1Tmp->compress_name          , fp, 0, 32);
        fread_m(&avc1Tmp->bit_depth              , fp, 0, 2);
        fread_m(&avc1Tmp->pre_defined_2          , fp, 0, 2);

        exchange->data = InitBoxData();
        exchange->data->data = avc1Tmp;

        fread_m(&entryCount, fp, 0, 4);
        exchange = InsertExistBox(InitBox("avcC", (size_t)entryCount), exchange);
        exchange->pos_start = ftell(fp) - 4L;

        fseek(fp, 4L, SEEK_CUR);
        fread_m(&avccTmp->configuration              , fp, 0, 1);
        fread_m(&avccTmp->avcProfileIndication       , fp, 0, 1);
        fread_m(&avccTmp->profile_compatibility      , fp, 0, 1);
        fread_m(&avccTmp->lengthSizeMinusOne         , fp, 0, 1);
        fread_m(&avccTmp->AVCLevelIndication         , fp, 0, 1);
        fread_m(&avccTmp->numOfSequenceParametersSets, fp, 0, 1);
        // printf("\nnumber of SPS sets: %8x", avccTmp->numOfSequenceParametersSets);
        avccTmp->numOfSequenceParametersSets &= (0x1f);
        // printf("\nnumber of SPS sets: %8x", avccTmp->numOfSequenceParametersSets);
        for (size_t i = 0; i < avccTmp->numOfSequenceParametersSets; i++)
        {
            fread_m(&avccTmp->sPS.sPSLength, fp, 0, 2);
            // printf("avccTmp->sPS.sPSLength %08x\n", avccTmp->sPS.sPSLength);
            avccTmp->sPS.sPSNALUnit = (char*)malloc(sizeof(char) * avccTmp->sPS.sPSLength * avccTmp->numOfSequenceParametersSets);
            fread(avccTmp->sPS.sPSNALUnit, sizeof(char), avccTmp->sPS.sPSLength * avccTmp->numOfSequenceParametersSets, fp);
            // printf("%x ", avccTmp->sPS.sPSNALUnit[i]);
        }
        fread_m(&avccTmp->numOfPictureParametersSets, fp, 0, 1);
        // printf("\nnumber of PPS sets: %d", avccTmp->numOfPictureParametersSets);
        for (size_t i = 0; i < avccTmp->numOfPictureParametersSets; i++)
        {
            fread_m(&avccTmp->pPS.pPSLength, fp, 0, 2);
            avccTmp->pPS.pPSNALUnit = (char*)malloc(sizeof(char) * avccTmp->pPS.pPSLength * avccTmp->numOfPictureParametersSets);
            fread(avccTmp->pPS.pPSNALUnit, sizeof(char), avccTmp->pPS.pPSLength * avccTmp->numOfPictureParametersSets, fp);
            // printf("%x ", avccTmp->pPS.pPSNALUnit[i]);
        }
        exchange->data = InitBoxData();
        exchange->data->data = avccTmp;
        // PrintBox(exchange);
        // printf("%d",((avc1*)(box->r_broth->data->data))->height);
        
    }
    else if(!strncmp(name, "stts", 4))
    {
        fread(entry_count, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);
        stts* sample = (stts*)malloc(sizeof(stts)*entryCount);
        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_count = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_delta = ChangeCharARToNumber(tmp, 4);
            printf("sample cout: %d, sample delta: %d \n",sample[i].sample_count,sample[i].sample_delta);
        }
        box->data->data = sample;
    }
    else if(!strncmp(name, "stss", 4))
    {
        fread(entry_count, sizeof(char), 4, fp);
        for (size_t i = 0; i < 4; i++)
        {
            printf("%x ", entry_count[i]);
            printf("\n");
        }
        entryCount = ChangeCharARToNumber(entry_count, 4);
        // printf("entryCount:%d\n", entryCount);
        stss* sample = (stss*)malloc(sizeof(stss)*entryCount);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4, fp);
            sample[i].sample_number = ChangeCharARToNumber(tmp, 4);
            printf("ID: %ld, sample_number : %d\n", i, sample[i].sample_number);
        }

        box->data->data = sample;
    }
    else if(!strncmp(name, "ctts", 4))
    {
        fread(entry_count, sizeof(char), 4, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);

        printf("entry count : %d\n", entryCount);

        ctts* sample = (ctts*)malloc(sizeof(ctts) * entryCount);
        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_count = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_offset = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);

            printf("ID: %ld, sample count: %d,\t sample offset: %d \n", i, sample[i].sample_count, sample[i].sample_offset);
        }
        

    }
    else if(!strncmp(name, "stsc", 4))
    {
        fread(entry_count, sizeof(char), 4, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);

        stsc* sample = (stsc*)malloc(sizeof(stsc)*entryCount);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].first_chunk = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_perchunk = ChangeCharARToNumber(tmp, 4);
            fread(tmp, sizeof(char), 4L, fp);
            sample[i].sample_description_index = ChangeCharARToNumber(tmp, 4);
            printf("first chunk ID: %d, sample per chunk: %d, sample_description:%d\n",sample[i].first_chunk, sample[i].sample_perchunk, sample[i].sample_description_index);
        }
        box->data->data = sample;
    }
    else if(!strncmp(name, "stsz", 4))
    {
        stsz* sampleMultiple;
        stsz sample;
        
        // sample size 
        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_size = ChangeCharARToNumber(tmp, 4);
        // sample count
        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_count = ChangeCharARToNumber(tmp, 4);

        if(sample.sample_size == 0)
        {
            sampleMultiple = (stsz*)malloc(sizeof(stsz)*sample.sample_count);
            
            for (int i = 0; i < sample.sample_count; i++)
            {
                fread(tmp, sizeof(char), 4, fp);
                sampleMultiple[i].sample_size = ChangeCharARToNumber(tmp, 4);
                sampleMultiple[i].sample_count = i;
                printf("sample count ID: %d, sample size: %d\n", i, sampleMultiple[i].sample_size);
            }
            box->data->data = sampleMultiple;
        }
        else 
        {
            printf("%d sample have same size: %d\n", sample.sample_count, sample.sample_size);
            box->data->data = &sample;  
        }
    }
    else if(!strncmp(name, "stco", 4))
    {
        fread(entry_count, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);
        printf("stco:chunk offset list count :%d\n", entryCount);
        stco* chunk = (stco*)malloc(box->size - 16L);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4, fp);
            chunk[i].chunk_offset = ChangeCharARToNumber(tmp, 4);
            printf("chunk ID: %3ld, Offset: 0x%10X\n", i, chunk[i].chunk_offset);
        }
        box->data->data = chunk;
    }
}
void DataDeleter(Box* box)
{

}
void fread_m(void* data, FILE* fp, int isString, size_t length)
{
    fread(data, sizeof(char), length, fp);
    // printf("address of data : %p sizeof data : %ld\n", data, length);

    if(isString) RevertBigEndingChar(data, length);
    else ChangeCharARToNumber(data, length);
}