#include"Mp4info.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"box.h"

int main()
{
    //read size - read type - read data
    FILE* fp;
    Box usingBox;
    Box* mp4Box; 
    Box* tmp, *tmp2;

    if((fp = fopen(FILEPATH, "rb")) == NULL) 
    {
        exit(-1);
        printf("cant open");
    }
    printf("open !!\n");
    
    fseek(fp, 0L, SEEK_END);
    mp4Box = InitBox("mp4b", ftell(fp));
    rewind(fp);

    ReadInfoIntoBox(fp, mp4Box);
    ReadDataIntoBox(fp, mp4Box);
    printf("===========================\n");
    // PrintBox(mp4Box);
    printf("===========================\n");

    tmp = InitBox("trak", 0);
    FindBoxLink("trak", mp4Box, tmp);

    Box* stbl = tmp->inBox->inBox->nextBox->nextBox->inBox->nextBox->nextBox->inBox->nextBox->nextBox;
    // PrintBox(stbl);

    Box* stsd = stbl->inBox;
    Box* stts = stsd->nextBox;
    Box* stss = stts->nextBox;
    Box* ctts = stss->nextBox;
    Box* stsc = ctts->nextBox;
    Box* stsz = stsc->nextBox;
    Box* stco = stsz->nextBox;
    // PrintBox(stsz);
    DataPareser(fp, "stsd", stsd);
    PrintBox(mp4Box);

    // FILE* fp_wr = fopen("out.sample", "w");
    // fseek(fp, 0x1fee1, SEEK_SET);
    // void* tmpf = (void*)malloc(19766L);
    // fread(tmpf, 19766L, 1, fp);
    // fwrite(tmpf, 19766L, 1, fp_wr);
    // free(tmpf);

    fclose(fp);
    DeleteBox(mp4Box);

    return 0;
}
void ReadDataIntoBox0(FILE* fp, Box* box)
{
    Data* data;

    if(box->inBox != NULL) ReadDataIntoBox0(fp, box->inBox);
    if(box->nextBox != NULL) ReadDataIntoBox0(fp, box->nextBox);

    if(box->inBox == NULL && strncmp(box->name, "mdat", 4))
    {
        fseek(fp, box->startPos + 8L, SEEK_SET);
        data = (Data*)malloc(sizeof(Data));
        data->data = (unsigned char*)malloc(box->size - 12L);
        fread(&(data->version), sizeof(char), 1, fp);
        //printf("version : %x, flags: %hhn", data->version, data->flags);
        fread(&(data->flags), sizeof(char), 3, fp);
        RevertBigEndingChar(data->flags, 3);
        if(box->size >= 12L)
        fread(data->data, box->size - 12l, 1, fp);

        box->data = data;
    }
}
void ReadDataIntoBox(FILE* fp, Box* box)
{
    ReadDataIntoBox0(fp, box);
    rewind(fp);
}
//read information into box from fp
void ReadInfoIntoBox(FILE* fp,  Box* rootBox)
{
    Box * upBox, * tmp;
    Box* usingBox;
    usingBox = InitBox("0000", 0);

    upBox = rootBox;
    char containerBox;
    while(ftell(fp) <= rootBox->size)
    {
        
        if(!ReadBoxInfo(fp, usingBox)) break;
        
        tmp = InsertNewBox(usingBox, upBox);

        containerBox = IsContainerBox(fp, ftell(fp), usingBox->size);

        if(containerBox > 0) 
        {
            fseek(fp, 8L, SEEK_CUR);
            upBox = tmp;
        }
        else fseek(fp, usingBox->size, SEEK_CUR);

        if(ftell(fp) >= upBox->startPos + upBox->size)
        {
            upBox = FindWrapBoxCanUse(upBox);
        }
    }

    DeleteBox(usingBox);
    rewind(fp);
}


//to read the basic info of the box
int ReadBoxInfo(FILE* fp, Box* box)
{
    unsigned char tmp[4];

    if(fread(tmp, sizeof(char), 4, fp) < 4) return 0;
    box->size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
    fread(tmp, sizeof(char), 4, fp);
    strncpy(box->name, tmp, 4);
    fseek(fp, -8L, SEEK_CUR);
    box->startPos = ftell(fp);

    return 1;
}

char IsFullBox(FILE* fp, size_t boxStartPos, size_t boxSize)
{
    if(!IsContainerBox(fp, boxStartPos, boxSize)) return 1;
    else return 0;
}
//this is to analyse whether it is a container box and return the count of contained box
char IsContainerBox(FILE* fp, size_t boxStartPos, size_t boxSize)
{//FILE* fp must be set at the start of box
    char result;
    unsigned long size;
    unsigned char tmp[4];

    

    fseek(fp, 4L, SEEK_CUR);
    fread(tmp, sizeof(char), 4, fp);
    if(!strncmp(tmp, "mdat", 4)) 
    {
        fseek(fp, boxStartPos, SEEK_SET);
        return 0;
    }
    while(ftell(fp) < boxStartPos + boxSize)
    {
        fread(tmp, sizeof(char), 4, fp);
        size = (long)tmp[0] << 24 | (long)tmp[1] << 16 | (long)tmp[2] << 8 | (long)tmp[3];
        fseek(fp, -4L, SEEK_CUR);
        fseek(fp, size, SEEK_CUR);
        result++;
        if(size == 0) {result = 0; break;}
    }
    if(ftell(fp) != boxStartPos + boxSize || boxSize == 8L || size == 0) result = 0;

    fseek(fp, boxStartPos, SEEK_SET);
    
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

//change BigEnd char to number
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

void DataPareser(FILE* fp, char* name, Box* box)
{
    unsigned char entry_count[4];
    uint32 entryCount;
    unsigned char tmp[4];
    unsigned char* tmp2;

    if(!strncmp(name, "stsd", 4))
    {
        avcC* avccTmp = (avcC*)malloc(sizeof(avcC));
        avc1* avc1Tmp = (avc1*)malloc(sizeof(avc1));
        Box* exchange;

        fseek(fp, box->startPos + 12L, SEEK_SET);
        fread(tmp, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(tmp, 4);

        stsd* sample = (stsd*)malloc(sizeof(stsd));
        sample->sample_count = entryCount;


        fread(tmp, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(tmp, 4);
        
        exchange = InsertExistBox(InitBox("avc1", (size_t)entryCount), box);
        exchange->startPos = ftell(fp) - 4L;
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
        exchange->startPos = ftell(fp) - 4L;

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

        // printf("%d",((avc1*)(box->inBox->data->data))->height);
        
    };
    if(!strncmp(name, "stts", 4))
    {
        fseek(fp, box->startPos + 12L, SEEK_SET);
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
    };
    if(!strncmp(name, "stss", 4))
    {
        fseek(fp, box->startPos + 12L, SEEK_SET);
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
    };
    if(!strncmp(name, "ctts", 4))
    {
        fseek(fp, box->startPos + 12L, SEEK_SET);
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
    if(!strncmp(name, "stsc", 4))
    {
        fseek(fp, box->startPos + 12L, SEEK_SET);
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
            printf("first chunk ID: %d, sample per chunk: %d, sample_description:%d",sample[i].first_chunk, sample[i].sample_perchunk, sample[i].sample_description_index);
        }
        box->data->data = sample;
    };
    if(!strncmp(name, "stsz", 4))
    {
        fseek(fp, box->startPos + 12L, SEEK_SET);
        stsz* sampleMultiple;
        stsz sample;

        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_size = ChangeCharARToNumber(tmp, 4);

        fread(tmp, sizeof(char), 4L, fp);
        sample.sample_count = ChangeCharARToNumber(tmp, 4);

        if(sample.sample_size == 0)
        {
            sampleMultiple = (stsz*)malloc(sizeof(stsz)*sampleMultiple->sample_count);
            
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
            printf("%d sample have same size: %d", sample.sample_count, sample.sample_size);
            box->data->data = &sample;  
        }
    };
    if(!strncmp(name, "stco", 4))
    {
        // printf("box size : %ld\n", box->startPos);
        fseek(fp, box->startPos + 12L, SEEK_SET);
        fread(entry_count, sizeof(char), 4L, fp);
        entryCount = ChangeCharARToNumber(entry_count, 4);
        //printf("%d\n", entryCount);
        //printf("chunk offset list count :%d\n", entryCount);
        stco* chunk = (stco*)malloc(box->size - 16L);

        for (size_t i = 0; i < entryCount; i++)
        {
            fread(tmp, sizeof(char), 4, fp);
            chunk[i].chunk_offset = ChangeCharARToNumber(tmp, 4);
            printf("chunk ID: %ld, chunk offset: 0x%10X\n", i, chunk[i].chunk_offset);
        }

        box->data->data = chunk;
    };
}

void fread_m(void* data, FILE* fp, int isString, size_t length)
{
    fread(data, sizeof(char), length, fp);
    // printf("address of data : %p sizeof data : %ld\n", data, length);

    if(isString) RevertBigEndingChar(data, length);
    else ChangeCharARToNumber(data, length);
}