#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// VM instruktion
#define HALT 0
#define PUSHC 1
#define ADD 2
#define SUB 3
#define MUL 4
#define DIV 5
#define MOD 6
#define RDINT 7
#define WRINT 8
#define RDCHR 9
#define WRCHR 10

#define OPCODE(i) ((i) >> 24)

// Immediate in die unteren
#define IMMEDIATE(x) ((x) & 0x00ffffff)
#define SIGN_EXTEND(i) (((i) & 0x00800000) ? ((i) | 0xFF000000) : (i))

// Datenstruktur
unsigned int prog[1024];
int size;
int stack[1024];
int sp=0;

// Instruktionen
void push(int value)
{
    if (sp >= 1024)
    {
        fprintf(stderr, "Error: stack underflow\n");
        exit(1);
    }

    stack[sp++] = value;
}

int pop(void)
{
    if (sp <= 0)
    {
        fprintf(stderr, "Error: stack underflow\n");
        exit(1);
    }

    return stack[--sp];
}

// Program
void loadProg1(void)
{
    int i = 0;
    prog[i++] = (PUSHC << 24) | IMMEDIATE(3);
    prog[i++] = (PUSHC << 24) | IMMEDIATE(4);
    prog[i++] = (ADD << 24);
    prog[i++] = (PUSHC << 24) | IMMEDIATE(10);
    prog[i++] = (PUSHC << 24) | IMMEDIATE(6);
    prog[i++] = (SUB << 24);
    prog[i++] = (MUL << 24);
    prog[i++] = (WRINT << 24);
    prog[i++] = (PUSHC << 24) | IMMEDIATE(10);
    prog[i++] = (WRCHR << 24);
    prog[i++] = (HALT << 24);
    size = i;
}

void loadProg2(void)
{
    int i = 0;
    prog[i++] = (PUSHC << 24) | IMMEDIATE(-2 & 0x00ffffff);
    prog[i++] = (RDINT << 24);
    prog[i++] = (MUL << 24);
    prog[i++] = (PUSHC << 24) | IMMEDIATE(3);
    prog[i++] = (ADD << 24);
    prog[i++] = (WRINT << 24);
    prog[i++] = (PUSHC << 24) | (IMMEDIATE('\n'));
    prog[i++] = (WRCHR << 24);
    prog[i++] = (HALT << 24);
    size = i;
}

void loadProg3(void)
{
    int i = 0;
    prog[i++] = (RDCHR << 24);
    prog[i++] = (WRINT << 24);
    prog[i++] = (PUSHC << 24) | IMMEDIATE('\n');
    prog[i++] = (WRCHR << 24);
    prog[i++] = (HALT << 24);
    size = i;
}

// Listen
void listProg(void)
{
    int i;
    for (i = 0; i < size; i++)
    {
        unsigned int instr = prog[i];
        int opcode = (instr >> 24) & 0xff;
        int imm = (int)SIGN_EXTEND(instr & 0x00ffffff);
        printf("%3d: ", i);
        switch (opcode)
        {
        case HALT:
            printf("halt\n");
            break;
        case PUSHC:
            printf("pushc %d\n", imm);
            break;
        case ADD:
            printf("add\n");
            break;
        case SUB:
            printf("sub\n");
            break;
        case MUL:
            printf("mul\n");
            break;
        case DIV:
            printf("div\n");
            break;
        case MOD:
            printf("mod\n");
            break;
        case RDINT:
            printf("rdint\n");
            break;
        case WRINT:
            printf("wrint\n");
            break;
        case RDCHR:
            printf("rdchr\n");
            break;
        case WRCHR:
            printf("wrchr\n");
            break;
        default:
            fprintf(stderr, "Error: unknown opcode %d\n",opcode);
            exit(1);
        }
    }
}

// Ausführung
void execProg(void)
{
    int pc = 0;
    int n1, n2;
    while (1)
    {
        unsigned int instr = prog[pc];
        int opcode = (instr >> 24) & 0xFF;
        int imm = (int)SIGN_EXTEND(instr & 0x00FFFFFF);
        pc++;

        switch (opcode)
        {
        case HALT:
            return;
        case PUSHC:
            push(imm);
            break;
        case ADD:
            n2 = pop();
            n1 = pop();
            push(n1 + n2);
            break;
        case SUB:
            n2 = pop();
            n1 = pop();
            push(n1 - n2);
            break;
        case MUL:
            n2 = pop();
            n1 = pop();
            push(n2 * n1);
            break;
        case DIV:
            n2 = pop();
            n1 = pop();
            if (n2 == 0)
            {
                fprintf(stderr, "Error: Division by Zero\n");
                exit(1);
            }
            push(n1 / n2);
            break;
        case MOD:
            n2 = pop();
            n1 = pop();
            if (n2 == 0)
            {
                fprintf(stderr, "Error: Modulo by Zero\n");
                exit(1);
            }
            push(n1 % n2);
            break;
        case RDINT:
            scanf("%d", &n1);
            push(n1);
            break;
        case WRINT:
            n1 = pop();
            printf("%d\n", n1);
            break;
        case RDCHR:
            n1 = getchar();
            push(n1);
            break;
        case WRCHR:
            n1 = pop();
            putchar(n1);
            break;
        default:
            fprintf(stderr, "Error: unknown opcode %d\n",opcode);
            exit(1);
        }
    }
}

int main(int argc, char *argv[])
{
    printf("Ninja Virtual Machine started\n");
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [--prog1|--prog2|--prog3|--help|--version]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--prog1") == 0)
    {
        loadProg1();
    }
    else if (strcmp(argv[1], "--prog2") == 0)
    {
        loadProg2();
    }
    else if (strcmp(argv[1], "--prog3") == 0)
    {
        loadProg3();
    }
    else if (strcmp(argv[1], "--version") == 0)
    {
        printf("Ninja Virtual Machine version 1 (compiled Apr 21 2026)\n");
        return 0;
    }
    else if (strcmp(argv[1], "--help") == 0)
    {
        printf("Usage: %s [options]\n", argv[0]);
        printf("--help      show help\n");
        printf("--version   show version\n");
        printf("--prog1     select program 1 to execute\n");
        printf("--prog2     select program 2 to execute\n");
        printf("--prog3     select program 3 to execute\n");
        return 0;
    }
    else
    {
        fprintf(stderr, "Error: unknown argument: %s\n", argv[1]);
        return 1;
    }

    listProg();
    printf("\n");
    execProg();
    printf("Ninja Virtual Machine stopped\n");

    return 0;
}

