#ifndef INFO_H__
#define INFO_H__

typedef unsigned long int uint64;
typedef unsigned int uint32;
typedef unsigned short int uint16;

typedef struct mvhd
{
    unsigned char version = 0;//1
    unsigned char flags[3];

    uint32 creation_time;//64
    uint32 mod_time;//64
    uint32 timescale;
    uint32 duration;//64

    uint32 rate;
    uint16 volume;

    uint16 reserved;
    uint32 reserved2[2];

    uint32 matrix[9];
    uint32 pre_defined[6];
    uint32 next_track_ID;
}Mvhd;

typedef struct tkhd
{
    unsigned char version = 0;
    unsigned char flags[3];

    uint32 creation_time;//64
    uint32 mod_time;//64
    uint32 track_ID;
    uint32 reserved;
    uint32 duration;//64

    uint32 reserved2[2];
    uint16 layer;
    uint16 alternate_group;
    uint16 volume;
    uint16 reserver3;

    uint32 matrix[9];
    uint32 width;
    uint32 height;

}Tkhd;
typedef struct mdha
{
    unsigned char version = 0;
    unsigned char flags[3];

    uint32 creation_time;//64
    uint32 mode_time;//64
    uint32 timescale;
    uint32 duration;//64

    uint16 pad_language;
    uint16 pre_defined;
}Mdha;


typedef struct hdlr
{
    unsigned char version = 0;
    unsigned char flags[3];

    uint32 pre_defined;
    uint32 handler_type;
    uint32 reserved[3];
    char* name;
};
typedef struct vmhd
{
    unsigned char version = 0;
    unsigned char flags[3];

    uint16 graphicsmode;
    uint16 opcolor[3];
};
typedef struct smhd
{
    unsigned char version = 0;
    unsigned char flags[3];

    uint16 balance;
    uint16 reserved;
};

typedef struct stsd
{
    
};


//decoding time to sample box
typedef struct stts
{
    uint32 entry_count;
    //for
    uint32 sample_count;
    uint32 sample_delta;//单位微秒
}Stts;

//sync sample box------the index of key frame sample 
typedef struct stss
{
    uint32 entry_count;

    //list
    uint32 sample_number;//[entry_count];
}Stss;

//Composition time to sample box
typedef struct ctts
{
    uint32 entry_count;

    //for
    uint32 sample_cout;
    uint32 sample_offset;//vsersion == 1 this is signed
    
};

//sample to chunk
typedef struct stsc
{
    uint32 entry_count;

    //for
    uint32 first_chunk;
    uint32 samples_per_chunk;
    uint32 sample_description_index;
};

//sample size
typedef struct stsz
{
    uint32 sample_size_defined;//if(sample_size_defined == 0) then every sample have same size

    uint32 sample_count;//all sample count
    //list of every sample size
    uint32 sample_size;
};


#endif // INFO_H__