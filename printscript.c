// script printer
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int END_VALUE = 0x2;
const int JUMP_VALUE = 0x1E;
const int CALL_VALUE = 0x4;
const int IF_VALUE = 0x1F;

char *coms[1058];
int comCt = 0;
char *comReference = "ref";
int argBits[1058];
unsigned char *buf; // read entire script file into buf
long flen;

int scriptOffsets[20];
int scriptCt = 0;
int functionOffsets[100];
int functionCt = 0;
int cmdsStartOffset;

int sumBytes(int arr[], int arrlen, int ignoreFirst) {
    int sum = 0;
    int startIndex = ignoreFirst ? 1 : 0;
    for (int i = startIndex, j = 0; i < arrlen; i++, j++) {
        sum += arr[i] << (8 * j);
    }
    return sum;
}
void printScriptAtOffset(int startOffset, int saveFunctions) {
    int offset = startOffset;
    while (offset < flen) {
        int val = buf[offset] + (buf[offset + 1] << 8);
        if (val == END_VALUE) {
            printf("%s", "End.");
            break;
        }
        //if (val == 0x140) break;
        if (val < comCt) {
            //printf("val = 0x%X\n", val);
            printf("%s ", coms[val]);
        } else {
            printf("Unknown command at 0x%X", offset);
            break;
        }
        offset += 2;
        
        int argct = argBits[val] / 8;
        //the number of bytes to read^
        int args[argct];
       for (int i = 0; i < argct; i++) {
            printf("%02X ", buf[offset]);
            //printf("0x%X ", arg);
            args[i] = buf[offset];
            offset++;
        }
        
        if (val == JUMP_VALUE || val == CALL_VALUE) {
     
            int fset = sumBytes(args, argct, 0) + offset;
            printf("Function at 0x%X", fset);
            if (saveFunctions) {
                functionOffsets[functionCt] = fset;
                functionCt++;
            }
            //printf("%s", "\n");
            if (val == JUMP_VALUE) {
                break;
            }
            
        } else if (val == IF_VALUE) {
            int foffset = sumBytes(args, argct, 1) + offset;
            printf("Function at 0x%X", foffset);
            if (saveFunctions) {
                functionOffsets[functionCt] = foffset;
                functionCt++;
            }
        }
        printf("%s", "\n");
    }
}
int scanAllFunctions(int startOffset) {
    int offset = startOffset;
    while (offset < flen) {
        int cmd = buf[offset] + (buf[offset + 1] << 8);
        //printf("0%X\n", cmd);
        int argct;
        if (cmd < comCt) {
            argct = argBits[cmd] / 8;
        } else {
            printf("Unknown command at 0x%X", offset);
            return 1;
        }
        offset += 2;
        if (cmd == JUMP_VALUE || cmd == CALL_VALUE || cmd == IF_VALUE) {
            int args[argct];
            for (int i = 0; i < argct; i++) {
                args[i] = buf[offset];
                offset++;
            }
            
            int foffset = sumBytes(args, argct, (cmd == IF_VALUE) ? 1 : 0) + offset;
            if (foffset >= cmdsStartOffset && foffset < flen) {
                functionOffsets[functionCt] = foffset;
                //printf("function offset 0x%X referenced at 0x%X\n", foffset, offset);
                functionCt++;
            }
        } else {
            offset += argct;
        }
        }
    return 0;
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

int main(int argc, char **argv) {
    if (argc < 2) {
    puts("specify a file containing the script");
    return 0;
    }
    
    char temp[30];
    FILE *ref = fopen(comReference, "r");
    if (ref == NULL) {
        printf("file '%s' not found. is it in a different directory from the executable?", comReference);
        return 1;
    }
    for (int i = 0; i < 1058 && fscanf(ref, "%24s %d", temp, &argBits[i]) != EOF; i++) {
        coms[i] = malloc(strlen(temp) + 1);
        strcpy(coms[i], temp);
        comCt++;
        // free these later
        //printf("%s %d\n", coms[i], argBits[i]);
    }
    fclose(ref);
    //puts("script commands read");
    
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        perror(argv[1]);
        return 2;
    }
    fseek(fp, 0, SEEK_END);
    flen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    //printf("file length is %lu\n", flen);
    buf = malloc(flen * sizeof(char));
    fread(buf, flen, 1, fp);
    fclose(fp);
    //puts("file read into buffer");

    int currentPosition = 0;
    while (1) {
        int nextScriptOffset = buf[currentPosition] + (buf[currentPosition + 1] << 8);
        if (nextScriptOffset == 0xFD13) {
            currentPosition += 2;
            cmdsStartOffset = currentPosition;
            //puts("finished reading script offsets in header");
            break;
        } else if (nextScriptOffset > flen) {
            //puts("invalid script offset(bad file header), exiting now");
            return 2;
        } else if (currentPosition > flen) {
            //puts("invalid position(bad header), exiting now");
            return 3;
        }
        
        nextScriptOffset += (buf[currentPosition + 2] << 16) + (buf[currentPosition + 3] << 24);
        currentPosition += 4;
        nextScriptOffset += currentPosition;

        scriptOffsets[scriptCt] = nextScriptOffset;
        scriptCt++;
        //puts("found one script offset");
    }
    
    for (int i = 0; i < scriptCt; i++) {
        printf("Script #%d at offset 0x%X:\n", i, scriptOffsets[i]);
        printScriptAtOffset(scriptOffsets[i], 0); 
        printf("%s", "\n\n");
    } // printing out the scripts
    
    scanAllFunctions(cmdsStartOffset);
    qsort(functionOffsets, functionCt, sizeof(int), compare);
    functionCt = removeSortedDuplicates(functionOffsets, functionCt);
    for (int i = 0; i < functionCt; i++) {
        printf("Function at offset 0x%X:\n", functionOffsets[i]);
        printScriptAtOffset(functionOffsets[i], 0);
        printf("%s", "\n\n");
    } // printing out the functions
    
    printf("%d scripts and %d functions, file length is %d bytes\n", scriptCt, functionCt, flen);
    //printScriptAtOffset(0x88f, 0);
    //printScriptAtOffset(0x22, 0);
    free(buf);
    for (int i = 0; i < 1058; i++) free(coms[i]); // free the command name strings
    return 0;
}
