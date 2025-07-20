#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define READ_32(val)                                                           \
  (((val >> 24) & 0x000000ff) | ((val >> 8) & 0x0000FF00) |                    \
   ((val << 8) & 0x00FF0000) | ((val << 24) & 0xFF000000))
#define READ_16(val) (val >> 8) | (val << 8)

typedef struct {
  uint8_t tableTag[4];
  uint32_t checkSum;
  uint32_t offset;
  uint32_t length;
} TableRecord;

typedef struct {
  uint32_t sfntVersion;
  uint16_t numTables;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;
  TableRecord **tableReacord;

} TableDirectory;

typedef struct {

    uint16_t platformId;
    uint16_t encodingId;
    uint32_t offset;
} EncodinRecord;

typedef struct {
  uint16_t version;
  uint16_t numTables;
  EncodinRecord** encodingRecords;
} Cmap;



void read_uint32(uint32_t *value, FILE *file) {
  fread(value, 4, 1, file);
  *value = READ_32(*value);
}

void read_uint16(uint16_t *value, FILE *file) {
  fread(value, 2, 1, file);
  *value = READ_16(*value);
}

Cmap* parse_cmap(uint32_t offset, FILE* file) {
    fseek(file,offset,SEEK_SET);

    Cmap* res = (Cmap*) malloc(sizeof(Cmap));
    read_uint16(&(res->version),file);
    read_uint16(&(res->numTables),file);
    res->encodingRecords = malloc(sizeof(EncodinRecord) * res->numTables);
    for(int x = 0; x<res->numTables; x++){
     res->encodingRecords[x] = malloc(sizeof(EncodinRecord));
      read_uint16(&(res->encodingRecords[x]->platformId),file);
      read_uint16(&(res->encodingRecords[x]->encodingId),file);
      read_uint32(&(res->encodingRecords[x]->offset),file);  
    }
    return res;     

}

TableDirectory *read_table_directory(FILE *font) {

  /*
  uint32	sfntVersion
  uint16	numTables
  uint16	searchRange
  uint16	entrySelector
  uint16	rangeShift */

  TableDirectory *td = (TableDirectory *)malloc(sizeof(TableDirectory));
  read_uint32(&(td->sfntVersion), font);
  read_uint16(&(td->numTables), font);
  read_uint16(&(td->searchRange), font);
  read_uint16(&(td->entrySelector), font);
  read_uint16(&(td->rangeShift), font);
  uint16_t size = td->numTables;
  td->tableReacord = (TableRecord **)malloc(sizeof(TableRecord *) * size);
  for (int x = 0; x < size; x++) {
    td->tableReacord[x] = malloc(sizeof(TableRecord));
    TableRecord *tr = td->tableReacord[x];
    fread(&(tr->tableTag[0]), 1, 1, font);
    fread(&(tr->tableTag[1]), 1, 1, font);
    fread(&(tr->tableTag[2]), 1, 1, font);
    fread(&(tr->tableTag[3]), 1, 1, font);
    read_uint32(&(tr->checkSum), font);
    read_uint32(&(tr->offset), font);
    read_uint32(&(tr->length), font);
  }
  return td;
}

void print_table_directory(TableDirectory *td) {
  printf("SFNT Version   : 0x%08X\n", td->sfntVersion);
  printf("Num Tables     : %u\n", td->numTables);
  printf("Search Range   : %u\n", td->searchRange);
  printf("Entry Selector : %u\n", td->entrySelector);
  printf("Range Shift    : %u\n", td->rangeShift);
  for (int x = 0; x < td->numTables; x++) {
    TableRecord *tr = td->tableReacord[x];
    printf("TAGS %.4s", tr->tableTag);
    printf(" CHECKSUM: %u", tr->checkSum);
    printf(" Offset %u ", tr->offset);
    printf(" Length %u \n", tr->length);
  }
}

void print_cmap(Cmap* cmap){
    printf("Cmap version %u\n",cmap->version);
    printf("Cmap num tables %u\n",cmap->numTables);
   for(int x = 0; x<cmap->numTables;x++){
    EncodinRecord* er = cmap->encodingRecords[x];
    printf("Er %d platform id = %u encodingId = %u offset = %u \n",x,er->platformId,er->encodingId,er->offset);
   }
}

int main(void) {

  FILE *font = fopen("Arial.ttf", "r");
  TableDirectory *tb = read_table_directory(font);
   Cmap *cmap = parse_cmap(tb->tableReacord[8]->offset,font);
  print_table_directory(tb);
   print_cmap(cmap);
  for (int x = 0; x < tb->numTables; x++) {
    free(tb->tableReacord[x]);
  }
for (int x = 0; x < cmap->numTables; x++) {
    free(cmap->encodingRecords[x]);
  }
  free(cmap->encodingRecords);
  free(cmap);

  free(tb->tableReacord);
  free(tb);
  fclose(font);
  return 0;
}