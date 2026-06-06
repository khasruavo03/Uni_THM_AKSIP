#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// VM Instruktion
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

#define CALL 26
#define RET 27
#define DROP 28
#define PUSHR 29
#define POPR 30
#define DUP 31

#define OPCODE(i) ((i) >> 24)
#define IMMEDIATE(x) ((x) & 0x00ffffff)
#define SIGN_EXTEND(i) (((i) & 0x00800000) ? ((i) | 0xFF000000) : (i))

// Globe Variable
unsigned int* prog; // geladene Programmcode
int progSize; 

typedef struct {
    int value;
} ObjRef;

typedef struct {
    int is_ref;
    union {
        ObjRef *ref;
        int raw;
    } value;
} StackSlot;

ObjRef *newInt(int value) {
    ObjRef *obj = malloc(sizeof(ObjRef));

    if (obj == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }

    obj->value = value;
    return obj;
}

ObjRef **data = NULL; // Globale Variable
int dataSize = 0; // Größe der globalen Datenbereich
StackSlot stack[1024]; // Stack
int sp = 0; // Stackpointer
int fp = 0; // Frame Pointer

int breakpoint = -1; // Breakpoint für Debugger
int pc = 0; // Program Counter
int halted = 0; // HALT Flag

ObjRef *rv = NULL; // Return-Value

void pushRaw(int value) {
    if (sp >= 1024) {
        fprintf(stderr, "Error: Stack overflow\n");
        exit(1);
    }
    stack[sp].is_ref = 0;
    stack[sp].value.raw = value;
    sp++;
}

void pushRef(ObjRef *ref) {
    if (sp >= 1024) {
        fprintf(stderr, "Error: Stack overflow\n");
        exit(1);
    }
    stack[sp].is_ref = 1;
    stack[sp].value.ref = ref;
    sp++;
}

void pushSlot(StackSlot slot) {
    if (sp >= 1024) {
        fprintf(stderr, "Error: Stack overflow\n");
        exit(1);
    }
    stack[sp++] = slot;
}

StackSlot popSlot(void) {
    if (sp <= 0) {
        fprintf(stderr, "Error: Stack underflow\n");
        exit(1);
    }
    return stack[--sp];
}

int popRaw(void) {
    StackSlot slot = popSlot();
    if (slot.is_ref) {
        fprintf(stderr, "Error: Expected raw integer on stack, found object reference\n");
        exit(1);
    }
    return slot.value.raw;
}

ObjRef *popRef(void) {
    StackSlot slot = popSlot();
    if (!slot.is_ref) {
        fprintf(stderr, "Error: Expected object reference on stack, found raw integer\n");
        exit(1);
    }
    return slot.value.ref;
}

// Laden der Datei
void loadBinaryCode(char *filename) {
    FILE *file =fopen(filename, "rb");

    // 
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s", filename);
        exit(1);
    }

    // Header lesen
    unsigned int header[4];
    if (fread(header, sizeof(unsigned int), 4, file) != 4) {
        fprintf(stderr, "Error: Could not read header\n");
        exit(1);
    }

    // Prüfen auf Magic
    if (header[0] != 0x46424a4e) {
        fprintf(stderr, "Error: Wrong file format\n");
        exit(1);
    }

    progSize = header[2];
    dataSize = header[3];

    data = malloc(sizeof(ObjRef *) * dataSize);
    if (data == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    for (int i = 0; i < dataSize; i++) {
        data[i] = newInt(0);
    }

    // Speicher reservieren
    prog = (unsigned int *)malloc(progSize * sizeof(unsigned int));
    if (prog == NULL) {
        fprintf(stderr, "Error: Could not malloc memory for %s.\n", filename);
        exit(1);
    }

    // Einlesen Programm
    size_t bytesRead = fread(prog, sizeof(unsigned int), progSize, file);

    if (bytesRead != (size_t) progSize) {
        fprintf(stderr, "Error: Could not read from file %s\n", filename);
        exit(1);
    }

    progSize = (int) bytesRead;

    fclose(file);
}

void listProg(void) {
    // Gibt als lesbare Assembler-Text aus
    for (int i = 0; i < progSize; i++) {
        // printf("Instruction List %d: 0x%08x\n", i, prog[i]);

        unsigned int instr = prog[i];
        int opcode = OPCODE(instr);
        int imm = SIGN_EXTEND(IMMEDIATE(instr));

        printf("%04d:", i);
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
            case 26:
                printf("call    %d\n", imm);
                break;
            case 27:
                printf("ret\n");
                break;
            case 28:
                printf("drop    %d\n", imm);
                break;
            case 29:
                printf("pushr\n");
                break;
            case 30:
                printf("popr\n");
                break;
            case 31:
                printf("dup\n");
                break;
            default:
                fprintf(stderr, "Error: unknown opcode %d\n",opcode);
                exit(1);
        }
    
    }

    printf("    --- end of code ---     ");
}

void inspStack(void) {
    printf("Stack: \n");
    printf("sp = %d, fp = %d\n", sp, fp);
    
    if (sp == 0) {
        printf("-- empty stack\n");
        return;
    }

    for (int i = sp - 1; i >= 0; i--) {
        if (stack[i].is_ref) {
            printf("%04d: obj@%p value=%d", i, (void *)stack[i].value.ref, stack[i].value.ref->value);
        } else {
            printf("%04d: raw=%d", i, stack[i].value.raw);
        }

        if (i == fp) {
            printf("    <-- fp");
        }

        if (i == sp - 1) {
            printf("    <-- sp");
        }
        printf("\n");
    }

    printf("-- End of Stack\n");
}

void inspData(void) {
    printf("Global Data: \n");
    for (int i = 0; i < dataSize; i++) {
        if (data[i] != NULL) {
            printf("%04d: obj@%p value=%d\n", i, (void *)data[i], data[i]->value);
        } else {
            printf("%04d: NULL\n", i);
        }
    }
}

void inspObject(ObjRef *obj) {
    if (obj == NULL) {
        printf("NULL pointer\n");
        return;
    }
    printf("Object at %p:\n", (void *)obj);
    printf("  value: %d\n", obj->value);
}

void inspObjectByAddr(void) {
    char buffer[256];
    unsigned long addr;

    printf("DEBUG [object]: Enter object address in hex (0x...) or decimal: ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        printf("Error reading input\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    if (strncmp(buffer, "0x", 2) == 0) {
        sscanf(buffer, "%lx", &addr);
    } else {
        sscanf(buffer, "%lu", &addr);
    }

    ObjRef *obj = (ObjRef *)addr;
    inspObject(obj);
}

// 
void execInstr(void) {
    unsigned int instr = prog[pc];
    int opcode = (instr >> 24) & 0xFF;
    int imm = (int)SIGN_EXTEND(instr & 0x00FFFFFF);
    pc++;
    ObjRef *o1, *o2;
    
    switch (opcode) {
        case HALT:
            halted = 1;
            return;
        case PUSHC:
            pushRef(newInt(imm));
            break;
        case ADD:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value + o2->value));
            break;
        case SUB:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value - o2->value));
            break;
        case MUL:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value * o2->value));
            break;
        case DIV:
            o2 = popRef();
            o1 = popRef();
            if (o2->value == 0) {
                fprintf(stderr, "Error: Division by Zero\n");
                exit(1);
            }
            pushRef(newInt(o1->value / o2->value));
            break;
        case MOD:
            o2 = popRef();
            o1 = popRef();
            if (o2->value == 0) {
                fprintf(stderr, "Error: Modulo by Zero\n");
                exit(1);
            }
            pushRef(newInt(o1->value % o2->value));
            break;
        case RDINT: {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                fprintf(stderr, "Error: Could not read input\n");
                exit(1);
            }
            int value = atoi(buffer);
            pushRef(newInt(value));
            break;
        }
        case WRINT:
            o1 = popRef();
            printf("%d", o1->value);
            break;
        case RDCHR:
            o1 = newInt(getchar());
            pushRef(o1);
            break;
        case WRCHR:
            o1 = popRef();
            putchar(o1->value);
            break;
        case PUSHG:
            if (imm < 0 || imm >= dataSize) {
                fprintf(stderr, "Error: Illegal global variable index\n");
                exit(1);
            }
            pushRef(data[imm]);
            break;
        case POPG:
            if (imm < 0 || imm >= dataSize) {
                fprintf(stderr, "Error: Illegal global variable index\n");
                exit(1);
            }
            data[imm] = popRef();
            break;
        case ASF:
            pushRaw(fp);
            fp = sp;
            if (sp + imm >= 1024) {
                fprintf(stderr, "Error: Stack Overflow\n");
                exit(1);
            }
            for (int i = sp; i < sp + imm; i++) {
                stack[i].is_ref = 0;
                stack[i].value.raw = 0;
            }
            sp += imm;
            break;
        case RSF:
            sp = fp;
            fp = popRaw();
            break;
        case PUSHL: {
            int addr = fp + imm;
            if (addr < 0 || addr >= sp) {
                fprintf(stderr, "Error: Illegal stack access\n");
                exit(1);
            }
            pushSlot(stack[addr]);
            break;
        }
        case POPL: {
            int addr = fp + imm;
            if (addr < 0 || addr >= sp) {
                fprintf(stderr, "Error: Illegal stack access\n");
                exit(1);
            }
            stack[addr] = popSlot();
            break;
        }
        case EQ:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value == o2->value));
            break;
        case NE:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value != o2->value));
            break;
        case LT:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value < o2->value));
            break;
        case LE:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value <= o2->value));
            break;
        case GT:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value > o2->value));
            break;
        case GE:
            o2 = popRef();
            o1 = popRef();
            pushRef(newInt(o1->value >= o2->value));
            break;
        case JMP:
            pc = imm;
            break;
        case BRF:
            o1 = popRef();
            if (o1->value == 0) {
                pc = imm;
            }
            break;
        case BRT:
            o1 = popRef();
            if (o1->value != 0) {
                pc = imm;
            }
            break;
        case CALL:
            pushRaw(pc);
            pc = imm;
            break;
        case RET:
            pc = popRaw();
            break;
        case DROP:
            if (sp - imm < 0) {
                fprintf(stderr, "Error: Stack underflow\n");
                exit(1);
            }
            sp = sp - imm;
            break;
        case PUSHR:
            if (rv == NULL) {
                fprintf(stderr, "Error: Return value register is empty\n");
                exit(1);
            }
            pushRef(rv);
            break;
        case POPR:
            rv = popRef();
            break;
        case DUP:
            o1 = popRef();
            pushRef(o1);
            pushRef(o1);
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
                    case 26:
                printf("call    %d", imm);
                break;
            case 27:
                printf("ret");
                break;
            case 28:
                printf("drop    %d", imm);
                break;
            case 29:
                printf("pushr");
                break;
            case 30:
                printf("popr");
                break;
            case 31:
                printf("dup");
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
        printf("DEBUG: inspect, object, list, breakpoint, step, run, quit?\n");
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
            } else if(strncmp(command, "object", 1) == 0) {
                inspObjectByAddr();
            } else if(strncmp(command, "inspect", 1) == 0) {
                char inspectCmd[256];

                printf("DEBUG: inspect what? stack, data, or object?\n");

                if (fgets(inspectCmd, sizeof(inspectCmd), stdin)) {
                    inspectCmd[strcspn(inspectCmd, "\n")] = '\0';
                    if (strncmp(inspectCmd, "stack", 1) == 0) {
                        inspStack();
                    } else if (strncmp(inspectCmd, "data", 1) == 0) {
                        inspData();
                    } else if (strncmp(inspectCmd, "object", 1) == 0) {
                        inspObjectByAddr();
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
    /* if (argc != 2)
    {
        fprintf(stderr, "Error: no code file specified\n");
        return 1;
    } */

      printf("Ninja Virtual Machine started\n");

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        // Momentanes Datum
        time_t now = time(NULL);
        time(&now);
        printf("Ninja Virtual Machine version 5 (compiled on %s)\n", ctime(&now));
        return 0;
    }

    if (argc == 2  && strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [--debug] <codefile>\n", argv[0]);
        printf("Options:\n");
        printf("  --debug   Enable debug mode (lists instructions and starts debugger)\n");
        printf("  --version Show version information\n");
        printf("  --help    Show this help message\n");
        return 0;
    }

    int debug = 0;
    char *filename;

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

    loadBinaryCode(filename);

    if (debug) {
        printf("DEBUG: file '%s' loaded (code size = %d, data size = %d)\n", filename, progSize, dataSize);
        debugger();
    } else {
        execProg();
        printf("Ninja Virtual Machine stopped\n");
    }

    free(prog);

    return 0;
}
