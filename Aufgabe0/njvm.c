#include <stdio.h>
#include <string.h>

/*
// Aufgbae 1

int main(int argc, char *
argv[])
{
    printf("Ninja Virtual Machine started\n");
    printf("Ninja Virtual Machine stopped\n");
    return 0;
}
*/

/*
// Aufgabe 02

int main(int argc, char *
argv[])
{
    for (int i = 1; i < argc; i++)
    {
        printf("Kommandozeile: %s \n", argv[i]);
    }
    printf("Ninja Virtual Machine started\n");
    printf("Ninja Virtual Machine stopped\n");
    return 0;
}
*/

// Aufgabe 03

int main(int argc, char *argv[]) 
{
    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0) {
            printf("Ninja Virtual Machine version 1 (compiled Apr 21 2026");
        }
        else if (strcmp(argv[1],"--help") == 0) {
            printf("Usage: njvm [options] \n");
            printf("--help      show help\n");
            printf("--version   show version\n");
        }
        else {
            printf("Unknown argument: %s\n", argv[1]);
            return 1;
        }
    }
    
    printf("Ninja Virtual Machine started\n");
    printf("Ninja Virtual Machine stopped\n");
    return 0;
}