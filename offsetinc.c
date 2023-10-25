#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//test this code

const int JUMP_VALUE = 0x1E;
const int CALL_VALUE = 0x4;
const int IF_VALUE = 0x1F;
const int MVT_VALUE = 0x64;
const int END_VALUE = 0x2;

char *comReference = "ref";
char *coms[1058];
int comCt = 0;
int argBits[1058];

/*int scriptOffsets[20];
int scriptCt = 0;
int functionOffsets[100];
int functionCt = 0;*/

int offsetsToInc[200];
int offsetCt = 0;
unsigned char *buf;
int flen;

int cmdStartOffset;

// command line tool. used to change offsets passed to jump/call/if/movement commands.
// consider: you insert 8 bytes of commands into the script file. but now a bunch of the offsets before your inserted commands are off by 8 bytes. oh no.
// - you can change all the offsets in the file. or you can change just the offsets before a certain location in the file
// 3 required arguments
//1. name of script file to modify  2. stop location  3. number to increment the offsets by
// options?
//in a057_304 everything before 0x8f3 must be incremented by (some amount?)
//i added 6b001700 6b001800 to insert two plasma guards for ghetsis
// so increase by 8 bytes
int sumBytes(int arr[], int arrlen, int startAtByte) {
    int sum = 0;
    int startIndex = startAtByte;
    for (int i = startIndex, j = 0; i < arrlen; i++, j++) {
        sum += arr[i] << (8 * j);
    }
    return sum;
}

int compare(const void *p1, const void *p2) {
    return *(int *)p1 - *(int *)p2;
}
int removeSortedDuplicates(int arr[], int size) { // return new element count
    int i, j;
    for (i = 0, j = 1; j < size;) {
        if (arr[i] == arr[j]) {
            j++;
            continue;
        }
        i++;
        if (j != (i + 1)) {
            arr[i] = arr[j];
        }
        j++;
    }
    return i + 1;
}

void increaseOffset(int offset, int amount) {
    int newOffset = amount;
    for (int i = 0; i < 4; i++) newOffset += (buf[offset + i] << 8 * i);
    //printf("0x%X ", newOffset);

    for (int i = 0; i < 4; i++) {
        buf[offset + i] = newOffset & 0xff;
        printf("0x%x ", newOffset & 0xff);
        newOffset >>= 8;
    }
}

int scanBodyOffsets(int startOffset, int stopOffset) { // 1: unknown command
    // extremely broken
    puts("scanOffsets start");
    int offset = startOffset;
        while (offset < stopOffset) {
            int val = buf[offset] + (buf[offset + 1] << 8);
            /*if (val == END_VALUE) {
                break;
            } else */
            if (!(val < comCt)) {
                printf("Reached unknown command at 0x%X", offset);
                return 1;
            }
            offset += 2; // skip the command
            int argct = argBits[val] / 8;
            if (val == JUMP_VALUE || val == CALL_VALUE) {
                int args[argct];
                for (int i = 0; i < argct; i++) args[i] = buf[offset + i];
                int fset = sumBytes(args, argct, 0) + offset;
                /*functionOffsets[functionCt] = fset;
                functionCt++;*/
                if (fset > stopOffset) { //346, 
                    offsetsToInc[offsetCt] = offset;
                    offsetCt++;
                }
            } else if (val == IF_VALUE) {
                int args[argct];
                for (int i = 0; i < argct; i++) args[i] = buf[offset + i];
                int fset = sumBytes(args, argct, 1) + offset;
                /*functionOffsets[functionCt] = fset;
                functionCt++;*/
                if (fset >= stopOffset) { // edit this?
                    offsetsToInc[offsetCt] = offset + 1;
                    offsetCt++;
                }
            } else if (val == MVT_VALUE) {
             int args[argct];
                for (int i = 0; i < argct; i++) args[i] = buf[offset + i];
                int fset = sumBytes(args, argct, 2) + offset;
                if (fset > stopOffset) { // edit this
                    offsetsToInc[offsetCt] = offset + 2;
                    offsetCt++;
                }
            }
            offset += argBits[val] / 8; // skip the arguments
        }
    return 0;
}
    
    

/*int getStartOffset() {
    int currentPosition = 0;
    while (1) {
        int nextScriptOffset = buf[currentPosition] + (buf[currentPosition + 1] << 8);
        //printf("0x%X\n", nextScriptOffset);
        if (currentPosition > flen) {
            return -1;
        } else if (nextScriptOffset == 0xFD13) {
            return currentPosition + 2;
        } else {
            currentPosition += 2;
        }
    }
}*/

int main(int argc, char **argv) {
    //printf("%u\n", sizeof(int));
    if (argc == 2 && strcmp(argv[1], "-h") == 0 || argc < 4) {
        puts("tool for increasing the jump/if/call/movement offsets in a pokemon bw script file. arguments are, in order:\n[script file] [stop offset in hexadecimal] [amount to increase by in hexadecimal]. Output will be written to a new file.");
        return 0;
    }
    
    int stopOffset = (int) strtol(argv[2], NULL, 16);
    int incBy = (int) strtol(argv[3], NULL, 16);
    
    printf("Increase every offset before 0x%x by 0x%x in the file %s? Output will be written to a new file. (y/n) ", stopOffset, incBy, argv[1]);
    char answer;
    scanf("%c", &answer);
    if (answer == 'y' || answer == 'Y') {
        puts("ok");
    } else {
        puts("exiting");
        return -1;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
      perror(argv[1]);
      return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    flen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    buf = malloc(flen * sizeof(char));
    fread(buf, flen, 1, fp);
    fclose(fp);
    // read the file into buf
    puts("read file into buffer");

    if (stopOffset > flen) {
        perror("stop offset is greater than the length of the file");
        return 1;
    } //check if incBy is too large?

    FILE *ref = fopen(comReference, "r");
    if (ref == NULL) {
        printf("file '%s' not found. is it in a different directory from the executable?", comReference);
        return 1;
    }
    
    char temp[25];
    for (int i = 0; i < 1058 && fscanf(ref, "%24s %d", temp, &argBits[i]) != EOF; i++) {
        coms[i] = malloc(strlen(temp) + 1);
        strcpy(coms[i], temp);
        comCt++;
        // free these later
        //printf("%s %d\n", coms[i], argBits[i]);
    }
    fclose(ref);
    puts("read command names");
    // calls
    int currentOffset = 0;
    while (1) {
        int nextScriptOffset = buf[currentOffset] + (buf[currentOffset + 1] << 8);
        //printf("0x%X", nextScriptOffset);
        if (nextScriptOffset == 0xFD13) {
            currentOffset += 2;
            cmdStartOffset = currentOffset;
            puts("found cmd start");
            break;
        } else if (nextScriptOffset > flen) {
            puts("invalid script offset found(bad header), exiting");
            return 2;
        } else if (currentOffset > flen) {
            puts("invalid position(bad header), exiting");
            return 3;
        } else {
            nextScriptOffset += (buf[currentOffset + 2] << 16) + (buf[currentOffset + 3] << 24) + currentOffset;
            if (nextScriptOffset > stopOffset) {
                offsetsToInc[offsetCt] = currentOffset;
                offsetCt++;
            }
            currentOffset += 4;
        }

        /*nextScriptOffset += (buf[currentOffset + 2] << 16) + (buf[currentOffset + 3] << 24);
        currentOffset += 4;
        nextScriptOffset += currentOffset;

        scriptOffsets[scriptCt] = nextScriptOffset;
        scriptCt++;*/
    }
    
    if (cmdStartOffset <= 0) perror("script file has a bad header");
    
    scanBodyOffsets(cmdStartOffset, stopOffset);
    //need to get the offsets
    qsort(offsetsToInc, offsetCt, sizeof(int), compare);
    offsetCt = removeSortedDuplicates(offsetsToInc, offsetCt);
    for (int i = 0; i < offsetCt; i++) {
        printf("offset at 0x%X\n", offsetsToInc[i]);
    }
    // now actually edit the offsets

    for (int i = 0; i < offsetCt; i++) {
        increaseOffset(offsetsToInc[i], incBy);
    }
    puts("increased the offsets");
    //return -2;
    
    char modFile[strlen(argv[1]) + 6];
    strcpy(modFile, argv[1]);
    strcat(modFile, ".n\0");
    //printf("%s", modFile);
    /*for (int i = 0; i < flen; i++) {
        printf("%X ", buf[i]);
    }*/
    fp = fopen(modFile, "wb");
    fwrite(buf, flen, 1, fp);
    fclose(fp);
    
    free(buf);
    for (int i = 0; i < 1058; i++) free(coms[i]); // free the command name strings
    return 0;
}
