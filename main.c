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
  uint16_t format;      // = 4
  uint16_t length;      // total bytes of this subtable
  uint16_t language;    // usually 0, not important
  uint16_t segCountX2;  // 2 × segment count (number of ranges × 2)
  uint16_t searchRange; // optimization
  uint16_t entrySelector;
  uint16_t rangeShift;
  uint16_t *endCode;
  uint16_t reservedPad; // must be 0
  uint16_t *startCode;
  uint16_t *idDelta;
  uint16_t *idRangeOffset;
  uint16_t *glyphIdArray; // variable size
} Cmap_Subtable;

typedef struct {
  uint16_t version;
  uint16_t numTables;
  EncodinRecord **encodingRecords;
  uint32_t offest;
  Cmap_Subtable *subtable;
} Cmap;

void read_uint32(uint32_t *value, FILE *file) {
  fread(value, 4, 1, file);
  *value = READ_32(*value);
}

void read_uint16(uint16_t *value, FILE *file) {
  fread(value, 2, 1, file);
  *value = READ_16(*value);
}

Cmap_Subtable *parse_subtable(uint32_t offset, FILE *file) {

  fseek(file, offset, SEEK_SET);

  Cmap_Subtable *sub = malloc(sizeof(Cmap_Subtable));
  read_uint16(&(sub->format), file);
  read_uint16(&(sub->length), file);
  read_uint16(&(sub->language), file);
  read_uint16(&(sub->segCountX2), file);
  read_uint16(&(sub->searchRange), file);
  read_uint16(&(sub->entrySelector), file);
  read_uint16(&(sub->rangeShift), file);
  uint16_t arr_size = sub->segCountX2;
  sub->endCode = malloc(sizeof(uint16_t) * arr_size);
  for (int x = 0; x < arr_size; x++) {
    read_uint16(&(sub->endCode[x]), file);
  }
  read_uint16(&(sub->reservedPad), file);
  sub->startCode = malloc(sizeof(uint16_t) * arr_size);
  for (int x = 0; x < arr_size; x++) {
    read_uint16(&(sub->startCode[x]), file);
  }

  sub->idDelta = malloc(sizeof(uint16_t) * arr_size);

  for (int x = 0; x < arr_size; x++) {
    read_uint16(&(sub->idDelta[x]), file);
  }

  sub->idRangeOffset = malloc(sizeof(uint16_t) * arr_size);

  for (int x = 0; x < arr_size; x++) {
    read_uint16(&(sub->idRangeOffset[x]), file);
  }

  sub->glyphIdArray = malloc(sizeof(uint16_t) * arr_size);

  for (int x = 0; x < arr_size; x++) {
    read_uint16(&(sub->glyphIdArray[x]), file);
  }

  return sub;
}

Cmap *parse_cmap(uint32_t offset, FILE *file) {
  fseek(file, offset, SEEK_SET);

  Cmap *res = (Cmap *)malloc(sizeof(Cmap));
  read_uint16(&(res->version), file);
  read_uint16(&(res->numTables), file);
  res->offest = offset;
  res->encodingRecords = malloc(sizeof(EncodinRecord) * res->numTables);
  for (int x = 0; x < res->numTables; x++) {
    res->encodingRecords[x] = malloc(sizeof(EncodinRecord));
    read_uint16(&(res->encodingRecords[x]->platformId), file);
    read_uint16(&(res->encodingRecords[x]->encodingId), file);
    read_uint32(&(res->encodingRecords[x]->offset), file);
  }

  res->subtable =
      parse_subtable(res->offest + res->encodingRecords[2]->offset, file);
  return res;
}

TableDirectory *read_table_directory(FILE *font) {

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

void print_cmap(Cmap *cmap) {
  printf("Cmap version %u\n", cmap->version);
  printf("Cmap num tables %u\n", cmap->numTables);
  for (int x = 0; x < cmap->numTables; x++) {
    EncodinRecord *er = cmap->encodingRecords[x];
    printf("Er %d platform id = %u encodingId = %u offset = %u \n", x,
           er->platformId, er->encodingId, er->offset);
  }
}

void print_subdir(Cmap_Subtable* sub){
  printf("Subtable format: %u, length: %u ,lang: %u, segCountX2: %u, searchRange: %u entrySelector %u rangeShift: %u  reservedPad: %u \n"
  , sub->format,sub->length,sub->language,sub->segCountX2,sub->searchRange,sub->entrySelector,sub->rangeShift,sub->reservedPad);
  
  for(int x = 0; x<sub->segCountX2; x++){
    printf("Index %d:  endCode: %u startCode: %u idDelta: %u idRangeOffset: %u glyphIdArray: %u \n",x,sub->endCode[x],sub->startCode[x],sub->idDelta[x],sub->idRangeOffset[x],sub->glyphIdArray[x]);
  }
}

void free_subtable(Cmap_Subtable *subtable) {
  free(subtable->endCode);
  free(subtable->startCode);
  free(subtable->idDelta);
  free(subtable->idRangeOffset);
  free(subtable->glyphIdArray);
  free(subtable);
}

void free_cmap(Cmap *cmap) {
  for (int x = 0; x < cmap->numTables; x++) {
    free(cmap->encodingRecords[x]);
  }
  free(cmap->encodingRecords);
  free_subtable(cmap->subtable);
  free(cmap);
}

void free_td(TableDirectory *tb) {

  for (int x = 0; x < tb->numTables; x++) {
    free(tb->tableReacord[x]);
  }

  free(tb->tableReacord);
  free(tb);
}

int main(void) {

  FILE *font = fopen("Arial.ttf", "r");
  TableDirectory *tb = read_table_directory(font);
  Cmap *cmap = parse_cmap(tb->tableReacord[8]->offset, font);
  print_table_directory(tb);
  print_cmap(cmap);
  print_subdir(cmap->subtable);
  free_td(tb);
  free_subtable(cmap->subtable);
  free_cmap(cmap);
  fclose(font);
  return 0;
}