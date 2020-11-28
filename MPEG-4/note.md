# some information
## stsd
sps & pps
## stts
## stsc
sample to chunk
example:
stsc: first chunk ID: 1, sample per chunk: 3, sample_description:1
stsc: first chunk ID: 4, sample per chunk: 1, sample_description:1
stsc: first chunk ID: 6, sample per chunk: 2, sample_description:1
stsc: first chunk ID: 8, sample per chunk: 1, sample_description:1
it means:
| Chunk ID | sample count |sample_description|
|--|--|--|
|contain left & right|how many samples in this chunk||
|1 - 3|3|1|
|4 - 5|1|1|
|6 - 7|2|1|
|8 - end|1|1|
## stsz
sample size and sample count
## stco
chunk offset
