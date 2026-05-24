#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Aufgabe 02

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
#define PUSHG 11
#define POPG 12
#define ASF 13
#define RSF 14
#define PUSHL 15
#define POPL 16
#define EQ 17
#define NE 18
#define LT 19
#define LE 20
#define GT 21
#define GE 22
#define JMP 23
#define BRF 24
#define BRT 25

#define OPCODE(i) ((i) >> 24)

// Immediate in die unteren
#define IMMEDIATE(x) ((x) & 0x00ffffff)
#define SIGN_EXTEND(i) (((i) & 0x00800000) ? ((i) | 0xFF000000) : (i))

unsigned int *prog;
int progSize;
int dataSize = 0;
int stack[1024];
int sp = 0;
int data[256];
int fp = 0;
int breakpoint = -1;
int pc = 0;
int halted = 0; //HALT Variable

// Instruktionen
void push(int value) {
    if (sp >= 1024) {
        fprintf(stderr, "Error: stack overflow\n");
        exit(1);
    }

    stack[sp++] = value;
}

int pop(void) {
    if (sp <= 0) {
        fprintf(stderr, "Error: stack underflow\n");
        exit(1);
    }

    return stack[--sp];
}

/* unsigned int swap32(unsigned int x)
{
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
} */

void loadBinaryCode(char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(1);
    }

    unsigned int header[4];

    if (fread(header, sizeof(unsigned int), 4, file) != 4) {
        fprintf(stderr, "Error: could not read header\n");
        exit(1);
    }

    if (header[0] != 0x46424a4e) {
        fprintf(stderr, "Error: wrong file format\n");
        exit(1);
    }

    progSize = header[2];
    dataSize = header[3];

    prog = (unsigned int *)malloc(progSize * sizeof(unsigned int));
    if (prog == NULL) {
        fprintf(stderr, "Error: Could not malloc memory for %s.\n", filename);
        exit(1);
    }
    
    size_t bytesRead = fread(prog, sizeof(unsigned int), progSize, file);

    if (bytesRead == 0) {
        fprintf(stderr, "Error. Could not read from file %s\n", filename);
        exit(1);
    }

    progSize = (int) bytesRead;

    fclose(file);
}

void listProg(void) {
    for (int i = 0; i < progSize; i++) {
        // printf("Instruction %d: 0x%08x\n", i, prog[i]);

        unsigned int instr = prog[i];
        int opcode = OPCODE(instr);
        int imm = SIGN_EXTEND(IMMEDIATE(instr));
        printf("%04d:   ", i);

        switch (opcode) {
            case 0: 
                printf("halt\n"); 
                break;
            case 1:
                printf("pushc   %d\n", imm);
                break;
            case 2:
                printf("add\n");
                break;
            case 3:
                printf("sub\n");
                break;
            case 4:
                printf("mul\n");
                break;
            case 5:
                printf("div\n");
                break;
            case 6:
                printf("mod\n");
                break;
            case 7:
            printf("rdint\n");
            break;
            case 8:
            printf("wrint\n");
            break;
            case 9:
                printf("rdchr\n");
                break;
            case 10:
                printf("wrchr\n");
                break;
            case 11:
                printf("pushg   %d\n", imm);
                break;
            case 12: 
                printf("popg    %d\n", imm);
                break;
            case 13:
                printf("asf     %d\n", imm);
                break;
            case 14:
                printf("rsf\n");
                break;
            case 15:
                printf("pushl   %d\n", imm);
                break;
            case 16:
                printf("popl    %d\n", imm);
                break;
            case 17:
                printf("eq\n");
                break;
            case 18:
                printf("ne\n");
                break;
            case 19:
                printf("lt\n");
                break;
            case 20:
                printf("le\n");
                break;
            case 21:
                printf("gt\n");
                break;
            case 22:
                printf("ge\n");
                break;
            case 23:
                printf("jmp     %d\n", imm);
                break;
            case 24:
                printf("brf     %d\n", imm);
                break;
            case 25:
                printf("brt     %d\n", imm);
                break;

            default:
                fprintf(stderr, "Error: unknown opcode %d\n",opcode);
                exit(1);
        }
    }
    printf("        --- end of code ---\n");
}

void inspStack(void) {
    printf("Stack: \n");
    printf("sp = %d, fp = %d\n", sp, fp);

    if (sp == 0) {
        printf("-- empty stack\n");
        return;
    }

    for (int i= sp - 1; i >= 0; i--) {
        printf("%04d: %d", i, stack[i]);
        if (i== fp)
        {
            printf("    <-- fp");
        }

        if (i == sp - 1) 
        {
            printf("    <-- sp");
        }
        printf("\n");
    }

    printf("-- End of Stack\n");
}

void inspData(void) {
    printf("Global Data: \n");
    for (int i=0; i < 256; i++) {
        printf("%04d: %d\n", i, data[i]);
    }
}

// 
void execInstr(void) {
    unsigned int instr = prog[pc];
    int opcode = (instr >> 24) & 0xFF;
    int imm = (int)SIGN_EXTEND(instr & 0x00FFFFFF);
    pc++;
    int n1, n2;
    
    switch (opcode) {
        case HALT:
            halted = 1;
            printf("HALT reached\n");
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
        {
            char buffer[256];
            printf("input> ");
            fgets(buffer, sizeof(buffer), stdin);
            n1 = atoi(buffer);
            push(n1);
            break;
        }
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
        case PUSHG:
            push(data[imm]);
            break;
        case POPG:
            data[imm] = pop();
            break;
        case ASF:
            push(fp);
            fp = sp;
            sp = sp + imm;
            break;
        case RSF:
            sp = fp;
            fp = pop();
            break;
        case PUSHL:
            push(stack[fp + imm]);
            break;
        case POPL:
            stack[fp + imm] = pop();
            break;
        case EQ:
            n2 = pop();
            n1 = pop();
            push(n1 == n2);
            break;
        case NE:
            n2 = pop();
            n1 = pop();
            push(n1 != n2);
            break;
        case LT:
            n2 = pop();
            n1 = pop();
            push(n1 < n2);
            break;
        case LE:
            n2 = pop(); 
            n1=pop();
            push(n1 <= n2);
            break;
        case GT:
            n2=pop();
            n1=pop();
            push(n1 > n2);
            break;
        case GE:
            n2= pop();
            n1= pop();
            push(n1 >= n2);
            break;
        case JMP:
            pc = imm;
            break;
        case BRF:
            n1 = pop();
            if (!n1) { pc = imm;}
            break;
        case BRT:
            n1 = pop();
            if (n1) {pc = imm;}
            break;
    }
}

void execProg(void) {
    while (pc < progSize && !halted) {
        if (pc == breakpoint) {
            printf("Breakpoint reached at %d\n", pc);
            return;
        }

        execInstr();

        // printf("pc=%d opcode=%d imm=%d\n", pc-1, opcode, imm);
    }
}

void printCurrentInstr(void) {

    if (pc >= progSize) {
        printf("Program finished\n");
        return;
    }

    unsigned int instr = prog[pc];
    int opcode = OPCODE(instr);
    int imm = SIGN_EXTEND(IMMEDIATE(instr));

    printf("%04d:   ", pc);

    switch(opcode) {
        case HALT:
            printf("halt");
            break;

        case PUSHC:
            printf("pushc   %d", imm);
            break;

        case RDINT:
            printf("rdint");
            break;

        case WRINT:
            printf("wrint");
            break;

        case PUSHG:
            printf("pushg   %d", imm);
            break;

        case POPG:
            printf("popg    %d", imm);
            break;

        case ADD:
            printf("add");
            break;

        case SUB:
            printf("sub");
            break;

        case MUL:
            printf("mul");
            break;

        case DIV:
            printf("div");
            break;

        case MOD:
            printf("mod");
            break;

        case EQ:
            printf("eq");
            break;

        case NE:
            printf("ne");
            break;

        case LT:
            printf("lt");
            break;

        case LE:
            printf("le");
            break;

        case GT:
            printf("gt");
            break;

        case GE:
            printf("ge");
            break;

        case JMP:
            printf("jmp     %d", imm);
            break;

        case BRF:
            printf("brf     %d", imm);
            break;

        case BRT:
            printf("brt     %d", imm);
            break;

        default:
            printf("unknown");
    }

    printf("\n");
}

// Debugging Methode
void debugger(void) {
    char command[256];

    printCurrentInstr();
    while (1)
    {
        printf("DEBUG: inspect, list, breakpoint, step, run, quit?\n");
        if (fgets(command, sizeof(command), stdin)) {
            command[strcspn(command, "\n")] = '\0';
            if(strncmp(command, "list", 1) == 0) {
                listProg();
            } else if (strncmp(command, "run", 1) == 0) {
                execProg();
                if (pc >= progSize || halted) {
                    printf("Ninja Virtual Machine stopped\n");
                    break;
                }
            } else if (strncmp(command, "quit", 1) == 0) {
                break;
            } else if(strncmp(command, "inspect", 1) == 0) {
                char inspectCmd[256];

                printf("DEBUG: inspect which, stack or data?\n");

                if (fgets(inspectCmd, sizeof(inspectCmd), stdin)) {
                    inspectCmd[strcspn(inspectCmd, "\n")] = '\0';
                    if (strncmp(inspectCmd, "stack", 1) == 0) {
                        inspStack();
                    } else if (strncmp(inspectCmd, "data", 1) == 0) {
                        inspData();
                    }
                }
            } else if(strncmp(command, "step", 1) == 0) {
                if (!halted && pc < progSize) {
                    execInstr();
                    printCurrentInstr();
                }
            } else if (strncmp(command, "break", 1) == 0) {
                char input[256];

                if (breakpoint == -1) {
                    printf("DEBUG [breakpoint]: cleared\n");
                } else {
                    printf("DEBUG [breakpoint]: set at %d\n", breakpoint);
                }

                printf("DEBUG [breakpoint]: address to set, -1 to clear, <ret> for no change?\n");

                if (fgets(input, sizeof(input), stdin)) {
                    if(strcmp(input, "\n") != 0) {
                        breakpoint = atoi(input);

                        if (breakpoint == -1) {
                            printf("DEBUG [breakpoint]: now cleared\n");
                        } else {
                            printf("DEBUG [breakpoint]: now set at %d\n", breakpoint);
                        }
                    }
                }
            }
        } else {
            printf("Error reading commands\n");
            break;
        }
    }
}


int main(int argc, char *argv[]) {
    int debug = 0;
    char *filename;
    /* if (argc != 2)
    {
        fprintf(stderr, "Error: no code file specified\n");
        return 1;
    } */

    if(argc == 3 && strcmp(argv[2], "--debug") == 0) {
        debug = 1;
        filename = argv[1];
    } else if (argc == 2)
    {
        filename = argv[1];
    } else {
        fprintf(stderr, "Error: no code file specified\n");
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        // Momentanes Datum
        time_t now = time(NULL);
        time(&now);
        printf("Ninja Virtual Machine version 1.3 (compiled on %s)\n", ctime(&now));
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [--debug] <codefile>\n", argv[0]);
        printf("Options:\n");
        printf("  --debug   Enable debug mode (lists instructions and starts debugger)\n");
        printf("  --version Show version information\n");
        printf("  --help    Show this help message\n");
        return 0;
    }

    loadBinaryCode(filename);

    printf("DEBUG: file '%s' loaded (code size = %d, data size = %d)\n", filename, progSize, dataSize);
    printf("Ninja Virtual Machine started\n");

    if (debug) {
        debugger();
    } else {
        execProg();
        printf("Ninja Virtual Machine stopped\n");
    }

    free(prog);

    return 0;
}

