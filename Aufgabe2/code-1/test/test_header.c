#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *file = fopen("prog1.bin", "rb");
    if (!file) {
        fprintf(stderr, "Cannot open file\n");
        return 1;
    }
    
    unsigned int header[4];
    if (fread(header, sizeof(unsigned int), 4, file) != 4) {
        fprintf(stderr, "Cannot read header\n");
        return 1;
    }
    
    printf("Header[0]: 0x%08x\n", header[0]);
    printf("Header[1]: 0x%08x\n", header[1]);
    printf("Header[2]: 0x%08x\n", header[2]);
    printf("Header[3]: 0x%08x\n", header[3]);
    
    fclose(file);
    return 0;
}
