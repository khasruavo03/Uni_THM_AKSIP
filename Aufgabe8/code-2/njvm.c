#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

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

#define NEW 32
#define GETF 33
#define PUTF 34
#define NEWA 35
#define GETFA 36
#define PUTFA 37
#define GETSZ 38
#define PUSHN 39
#define REFEQ 40
#define REFNE 41

#define OPCODE(i) ((i) >> 24)
#define IMMEDIATE(x) ((x) & 0x00ffffff)
#define SIGN_EXTEND(i) (((i) & 0x00800000) ? ((i) | 0xFF000000) : (i))

#define MSB (1u << 31)
#define IS_PRIM(obj) (((obj)->size & MSB) == 0)
#define GET_SIZE(obj) ((obj)->size & ~MSB)

// Globe Variable
unsigned int* prog; // geladene Programmcode
int progSize;


void printInstructionAt(FILE *stream, int index);
extern int pc;
extern int currentInstrPc;

// Heap Objekt
typedef struct {
    unsigned int size;
    unsigned char data[1];
} *ObjRef;

// Fehlerausgabe
// Aus Skript
void error(char *fmt , ...) {
    va_list ap;
    va_start(ap , fmt );    
    printf("Error: ");
    vprintf(fmt , ap);
    printf("\n");
    va_end(ap);
    exit (1);
}

// primitives Objekt erstellen
void *newPrimObject(int datasize) {
    ObjRef obj;obj = malloc(sizeof(unsigned int) + datasize);

    if (obj == NULL)
    {
        error("Out of Memory");
        exit(1);
    }

    obj->size = datasize;
    return obj;
}

// zeiger
void *getPrimObjectDataPointer(void *obj) {
    ObjRef oo = (ObjRef)obj;
    return oo->data;
}



// Headderdateien inkludiertt
typedef void *BigObjRef;

typedef struct {
    int nd;unsigned char sign;
    unsigned char digits[1];
} Big;

typedef struct {
    BigObjRef op1;
    BigObjRef op2;
    BigObjRef res;
    BigObjRef rem;
} BIP;

BIP bip = {NULL, NULL, NULL, NULL};

#define BIG_NEGATIVE ((unsigned char)0)
#define BIG_POSITIVE ((unsigned char)1)
#define BIG_PTR(bigObjRef) ((Big *)(getPrimObjectDataPointer(bigObjRef)))
#define GET_ND(bigObjRef) (BIG_PTR(bigObjRef)->nd)
#define SET_ND(bigObjRef, val) (BIG_PTR(bigObjRef)->nd = (val))
#define GET_SIGN(bigObjRef) (BIG_PTR(bigObjRef)->sign)
#define SET_SIGN(bigObjRef, val) (BIG_PTR(bigObjRef)->sign = (val))
#define GET_DIGIT(bigObjRef, i) (BIG_PTR(bigObjRef)->digits[i])
#define SET_DIGIT(bigObjRef, i, val) (BIG_PTR(bigObjRef)->digits[i] = (val))

static BigObjRef newBig(int nd) {
    int dataSize = sizeof(int) + 1 + nd;
    BigObjRef bigObjRef = newPrimObject(dataSize);
    return bigObjRef;
}

static int bigAbsCmp(BigObjRef a, BigObjRef b) {
    int nd1 = GET_ND(a);
    int nd2 = GET_ND(b);
    int i;

    if (nd1 != nd2) {
        return nd1 - nd2;
    }

    for (i = nd1 - 1; i >= 0; --i) {
        int diff = GET_DIGIT(a, i) - GET_DIGIT(b, i);
        if (diff != 0) {
            return diff;
        }
    }
    return 0;
}

static void bigAbsAdd(BigObjRef a, BigObjRef b) {
    int nd1 = GET_ND(a);
    int nd2 = GET_ND(b);
    int nd = nd1 > nd2 ? nd1 : nd2;
    int i;unsigned int carry = 0;

    bip.res = newBig(nd + 1);
    for (i = 0; i < nd + 1; ++i) {
        SET_DIGIT(bip.res, i, 0);
    }

    for (i = 0; i < nd; ++i) {
        unsigned int da = (i < nd1) ? GET_DIGIT(a, i) : 0;
        unsigned int db = (i < nd2) ? GET_DIGIT(b, i) : 0;
        unsigned int sum = da + db + carry;
        SET_DIGIT(bip.res, i, (unsigned char)(sum & 0xFF));
        carry = sum >> 8;   
    }
    SET_DIGIT(bip.res, i, (unsigned char)carry);

    while (i > 0 && GET_DIGIT(bip.res, i - 1) == 0) {
        --i;
    }
    SET_ND(bip.res, i);
}

static void bigAbsSub(BigObjRef a, BigObjRef b) {int nd1 = GET_ND(a);int nd2 = GET_ND(b);int nd = nd1;int i;int borrow = 0;

    if (bigAbsCmp(a, b) < 0) {
        fatalError("internal bigint error");
    }

    bip.res = newBig(nd);
    for (i = 0; i < nd; ++i) {
        SET_DIGIT(bip.res, i, 0);
    }

    for (i = 0; i < nd; ++i) {
        int da = (i < nd1) ? GET_DIGIT(a, i) : 0;
        int db = (i < nd2) ? GET_DIGIT(b, i) : 0;
        int tmp = da - db - borrow;
        if (tmp < 0) {
            tmp += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }
        SET_DIGIT(bip.res, i, (unsigned char)tmp);
    }

    if (borrow != 0) {
        fatalError("internal bigint error");
    }

    while (i > 0 && GET_DIGIT(bip.res, i - 1) == 0) {
        --i;
    }
    SET_ND(bip.res, i);
}

static void bigAbsMul(BigObjRef a, BigObjRef b) {int nd1 = GET_ND(a);int nd2 = GET_ND(b);int i, j, k;unsigned int carry;

bip.res = newBig(nd1 + nd2);
for (i = 0; i < nd1 + nd2; ++i) {
    SET_DIGIT(bip.res, i, 0);
}

for (j = 0; j < nd2; ++j) {
    carry = 0;
    for (k = j, i = 0; i < nd1; ++i, ++k) {
        unsigned int aux = (unsigned int)GET_DIGIT(a, i) * (unsigned int)GET_DIGIT(b, j) +
                           (unsigned int)GET_DIGIT(bip.res, k) + carry;
        SET_DIGIT(bip.res, k, (unsigned char)(aux & 0xFF));
        carry = aux >> 8;
    }
    SET_DIGIT(bip.res, k, (unsigned char)carry);
}

i = nd1 + nd2;
while (i > 0 && GET_DIGIT(bip.res, i - 1) == 0) {
    --i;
}
SET_ND(bip.res, i);

}

static void bigAbsInc(BigObjRef a) {int i;int nd = GET_ND(a);

for (i = 0; i < nd; ++i) {
    unsigned int digit = GET_DIGIT(a, i) + 1;
    SET_DIGIT(a, i, (unsigned char)(digit & 0xFF));
    if (digit <= 0xFF) {
        return;
    }
}

if (GET_ND(a) == 0) {
    SET_ND(a, 1);
    SET_DIGIT(a, 0, 1);
    return;
}

SET_ND(a, nd + 1);
SET_DIGIT(a, nd, 1);

}

static void bigAbsDivMod(BigObjRef dividend, BigObjRef divisor, int doMod) {BigObjRef q = newBig(1);BigObjRef r = newBig(GET_ND(dividend));int i;

SET_ND(q, 0);
SET_SIGN(q, BIG_POSITIVE);
SET_ND(r, GET_ND(dividend));
SET_SIGN(r, BIG_POSITIVE);
for (i = 0; i < GET_ND(dividend); ++i) {
    SET_DIGIT(r, i, GET_DIGIT(dividend, i));
}

while (GET_ND(r) > 0 && bigAbsCmp(r, divisor) >= 0) {
    bigAbsSub(r, divisor);
    bigAbsInc(q);
}

if (doMod) {
    bip.rem = r;
} else {
    bip.res = q;
}

}

int bigSgn(void) {if (GET_ND(bip.op1) == 0) {return 0;}return GET_SIGN(bip.op1) == BIG_NEGATIVE ? -1 : 1;}

int bigCmp(void) {int cmp;if (GET_SIGN(bip.op1) != GET_SIGN(bip.op2)) {return GET_SIGN(bip.op1) == BIG_NEGATIVE ? -1 : 1;}cmp = bigAbsCmp(bip.op1, bip.op2);return GET_SIGN(bip.op1) == BIG_NEGATIVE ? -cmp : cmp;}

void bigNeg(void) {if (GET_ND(bip.op1) != 0) {SET_SIGN(bip.op1, GET_SIGN(bip.op1) == BIG_NEGATIVE ? BIG_POSITIVE : BIG_NEGATIVE);}bip.res = bip.op1;}

void bigAdd(void) {
    if (GET_ND(bip.op1) == 0) {
        bip.res = bip.op2;return;
    }
    
    if (GET_ND(bip.op2) == 0) {
        bip.res = bip.op1;
        return;
    }

    if (GET_SIGN(bip.op1) == GET_SIGN(bip.op2)) {
        bigAbsAdd(bip.op1, bip.op2);
        SET_SIGN(bip.res, GET_SIGN(bip.op1));
    } else {
        int cmp = bigAbsCmp(bip.op1, bip.op2);
        if (cmp == 0) {
            bip.res = newBig(1);
            SET_ND(bip.res, 0);
            SET_SIGN(bip.res, BIG_POSITIVE);
            return;
        }
        if (cmp > 0) {
            bigAbsSub(bip.op1, bip.op2);
            SET_SIGN(bip.res, GET_SIGN(bip.op1));
        } else {
            bigAbsSub(bip.op2, bip.op1);
            SET_SIGN(bip.res, GET_SIGN(bip.op2));
        }
    }
}

void bigSub(void) {
    if (GET_ND(bip.op2) == 0) {
        bip.res = bip.op1;
        return;
    }
    
    if (GET_SIGN(bip.op2) == BIG_NEGATIVE) {
        BigObjRef tmp = bip.op2;
        bip.op2 = bip.op1;
        bip.op1 = tmp;
    }
    
    bigNeg();
    bigAdd();
}

void bigMul(void) {if (GET_ND(bip.op1) == 0 || GET_ND(bip.op2) == 0) {bip.res = newBig(1);SET_ND(bip.res, 0);SET_SIGN(bip.res, BIG_POSITIVE);return;}

bigAbsMul(bip.op1, bip.op2);
SET_SIGN(bip.res, (GET_SIGN(bip.op1) == GET_SIGN(bip.op2)) ? BIG_POSITIVE : BIG_NEGATIVE);

}

void bigDiv(void) {if (GET_ND(bip.op2) == 0) {fatalError("division by zero");}if (GET_ND(bip.op1) == 0) {bip.res = newBig(1);SET_ND(bip.res, 0);SET_SIGN(bip.res, BIG_POSITIVE);bip.rem = newBig(1);SET_ND(bip.rem, 0);SET_SIGN(bip.rem, BIG_POSITIVE);return;}

bigAbsDivMod(bip.op1, bip.op2, 0);
SET_SIGN(bip.res, (GET_SIGN(bip.op1) == GET_SIGN(bip.op2)) ? BIG_POSITIVE : BIG_NEGATIVE);
if (GET_ND(bip.res) == 0) {
    SET_SIGN(bip.res, BIG_POSITIVE);
}

}

void bigFromInt(int n) {unsigned long long u;int nd = 0;int i;long long v = n;

if (n == 0) {
    bip.res = newBig(1);
    SET_ND(bip.res, 0);
    SET_SIGN(bip.res, BIG_POSITIVE);
    return;
}

u = (v < 0) ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v;
{
    unsigned long long tmp = u;
    while (tmp > 0) {
        ++nd;
        tmp >>= 8;
    }
}

bip.res = newBig(nd);
SET_ND(bip.res, nd);
SET_SIGN(bip.res, (n < 0) ? BIG_NEGATIVE : BIG_POSITIVE);
for (i = 0; i < nd; ++i) {
    SET_DIGIT(bip.res, i, (unsigned char)((u >> (8 * i)) & 0xFF));
}

}

int bigToInt(void) {unsigned long long value = 0;int i;int nd = GET_ND(bip.op1);int sign = GET_SIGN(bip.op1) == BIG_NEGATIVE ? -1 : 1;

for (i = nd - 1; i >= 0; --i) {
    value = (value << 8) | GET_DIGIT(bip.op1, i);
}

if (sign < 0) {
    value = (unsigned long long)(-(long long)value);
}

if (value > (unsigned long long)INT_MAX) {
    return INT_MAX;
}
if (sign < 0 && value > (unsigned long long)INT_MAX) {
    return INT_MIN;
}
return (int)value * sign;

}

void bigRead(FILE *in) {int c;int sign = 1;long long value = 0;

while ((c = fgetc(in)) != EOF && isspace((unsigned char)c)) {
}
if (c == '-') {
    sign = -1;
    c = fgetc(in);
} else if (c == '+') {
    c = fgetc(in);
}
if (c != EOF && isdigit((unsigned char)c)) {
    do {
        value = value * 10 + (c - '0');
        c = fgetc(in);
    } while (c != EOF && isdigit((unsigned char)c));
}
if (c != EOF) {
    ungetc(c, in);
}
bigFromInt((int)(sign < 0 ? -value : value));

}

void bigPrint(FILE *out) {
    fprintf(out, "%d", bigToInt());
}

void bigDump(FILE *out, BigObjRef bigObjRef) {
    int i;
    fprintf(out, "Big(nd=%d, sign=%s)", GET_ND(bigObjRef), GET_SIGN(bigObjRef) == BIG_NEGATIVE ? "-" : "+");
    for (i = 0; i < GET_ND(bigObjRef); ++i) {
        fprintf(out, " %02x", GET_DIGIT(bigObjRef, i));
    }
}


// --- //


// Meine Implementierung

// Garbage Collector 
void garbageCollect(void) {
    fatalError("GC called");
}

// Globale Variable für Heap
char *heap = NULL; // gesamter Heap
char *fromSpace = NULL; // aktive Heap-Hälfte
char *toSpace = NULL; // zweite Heap-Hälfte
char *freePtr = NULL; // nächste freie Stelle
char *heapEnd = NULL; // Ende der aktive Hälfte

size_t heapSize = 0;
size_t semiSize = 0;

// allocate
ObjRef allocate(size_t n) {
    if ((freePtr + n) > heapEnd) {
        garbageCollect();
        fatalError("Out of memory");
    }

    ObjRef obj = (ObjRef) freePtr;
    freePtr += n;

    return obj;
}


ObjRef newPrimitiveObject(int numByte) {
    int size = sizeof(unsigned int) + numByte;

    // statt malloc() -> Stop & Copy funktionieren
    ObjRef obj = allocate(size); 
    if (obj == NULL)
    {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }

    obj->size = numByte;
    return obj;
}

ObjRef newCompoundObject(int numOfRef) {
    /* ObjRef obj = malloc(sizeof(unsigned int) + numOfRef * sizeof(ObjRef)); */

    // Alternative zu malloc
    int size = sizeof(unsigned int) + numOfRef;

    // statt malloc() -> Stop & Copy funktionieren
    ObjRef obj = allocate(size); 
    if(!obj) {
        fprintf(stderr, "Error out of Memory\n");
        exit(1);
    }
    obj->size = MSB | (unsigned int) numOfRef;
    ObjRef *fields = (ObjRef *)obj->data;
    for(int i=0; i<numOfRef; i++) {
        fields[i] = NULL;
    }
    return obj;
}

typedef struct {
    int is_ref;
    union {
        ObjRef objRef;
        int number;
    } value;
} StackSlot;

// Für Aufgabe 06 muss man es mit der bigint lösen um auch mit großen Zahlen zu rechnen
ObjRef newInt(int value) {
    bigFromInt(value);
    return bip.res;
}

ObjRef *data = NULL; // Globale Variable
int dataSize = 0; // Größe der globalen Datenbereich
/* StackSlot stack[1024]; // Stack */

// Stack dynamisch mit malloc() anlegen
StackSlot *stack = NULL;
int stackSize;

int sp = 0; // Stackpointer
int fp = 0; // Frame Pointer

int breakpoint = -1; // Breakpoint für Debugger
int pc = 0; // Program Counter
int halted = 0; // HALT Flag
int currentInstrPc = -1;

ObjRef rv = NULL; // Return-Value

void printInstructionAt(FILE *stream, int index) {
    if (index < 0 || index >= progSize) {
        fprintf(stream, "<no instruction>");
        return;
    }

    unsigned int instr = prog[index];
    int opcode = OPCODE(instr);
    int imm = SIGN_EXTEND(IMMEDIATE(instr));

    fprintf(stream, "%04d:   ", index);

    switch (opcode) {
        case HALT:
            fprintf(stream, "halt");
            break;
        case PUSHC:
            fprintf(stream, "pushc   %d", imm);
            break;
        case RDINT:
            fprintf(stream, "rdint");
            break;
        case WRINT:
            fprintf(stream, "wrint");
            break;
        case PUSHG:
            fprintf(stream, "pushg   %d", imm);
            break;
        case POPG:
            fprintf(stream, "popg    %d", imm);
            break;
        case ADD:
            fprintf(stream, "add");
            break;
        case SUB:
            fprintf(stream, "sub");
            break;
        case MUL:
            fprintf(stream, "mul");
            break;
        case DIV:
            fprintf(stream, "div");
            break;
        case MOD:
            fprintf(stream, "mod");
            break;
        case EQ:
            fprintf(stream, "eq");
            break;
        case NE:
            fprintf(stream, "ne");
            break;
        case LT:
            fprintf(stream, "lt");
            break;
        case LE:
            fprintf(stream, "le");
            break;
        case GT:
            fprintf(stream, "gt");
            break;
        case GE:
            fprintf(stream, "ge");
            break;
        case JMP:
            fprintf(stream, "jmp     %d", imm);
            break;
        case BRF:
            fprintf(stream, "brf     %d", imm);
            break;
        case BRT:
            fprintf(stream, "brt     %d", imm);
            break;
        case CALL:
            fprintf(stream, "call    %d", imm);
            break;
    case RET:
        fprintf(stream, "ret");
        break;
    case DROP:
        fprintf(stream, "drop    %d", imm);
        break;
    case PUSHR:
        fprintf(stream, "pushr");
        break;
    case POPR:
        fprintf(stream, "popr");
        break;
    case DUP:
        fprintf(stream, "dup");
        break;
    case NEW:
        fprintf(stream, "new     %d", imm);
        break;
    case GETF:
        fprintf(stream, "getf    %d", imm);
        break;
    case PUTF:
        fprintf(stream, "putf    %d", imm);
        break;
    case NEWA:
        fprintf(stream, "newa");
        break;
    case GETFA:
        fprintf(stream, "getfa");
        break;
    case PUTFA:
        fprintf(stream, "putfa");
        break;
    case GETSZ:
        fprintf(stream, "getsz");
        break;
    case PUSHN:
        fprintf(stream, "pushn");
        break;
    case REFEQ:
        fprintf(stream, "refeq");
        break;
    case REFNE:
        fprintf(stream, "refne");
        break;
    default:
        fprintf(stream, "unknown");
        break;
}

}

void runtimeError(char *msg) {int idx = currentInstrPc >= 0 ? currentInstrPc : pc;fprintf(stderr, "Error at pc=%d: %s\n", idx, msg);fprintf(stderr, "  instruction: ");printInstructionAt(stderr, idx);fprintf(stderr, "\n");exit(1);}

void pushRaw(int value) {if (sp >= 1024) {runtimeError("Stack overflow");}stack[sp].is_ref = 0;stack[sp].value.number = value;sp++;}

void pushRef(ObjRef ref) {if (sp >= 1024) {runtimeError("Stack overflow");}stack[sp].is_ref = 1;stack[sp].value.objRef = ref;sp++;}

void pushSlot(StackSlot slot) {if (sp >= 1024) {runtimeError("Stack overflow");}stack[sp++] = slot;}

StackSlot popSlot(void) {if (sp <= 0) {runtimeError("Stack underflow");}return stack[--sp];}

int popRaw(void) {StackSlot slot = popSlot();if (slot.is_ref) {runtimeError("Expected raw integer on stack, found object reference");}return slot.value.number;}

ObjRef popRef(void) {StackSlot slot = popSlot();if (!slot.is_ref) {runtimeError("Expected object reference on stack, found raw integer");}return slot.value.objRef;}

// Laden der Datei
void loadBinaryCode(char *filename) {
    FILE *file = fopen(filename, "rb");

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

    progSize = (int)header[2];
    dataSize = (int)header[3];

    data = malloc(sizeof(ObjRef) * dataSize);
    if (data == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        exit(1);
    }
    for (int i = 0; i < dataSize; i++) {
        data[i] = NULL;
    }

    // Speicher reservieren
    prog = (unsigned int *)malloc(progSize * sizeof(unsigned int));
    if (prog == NULL) {
        fprintf(stderr, "Error: Could not malloc memory for %s.\n", filename);
        exit(1);
    }

    // Einlesen Programm
    size_t bytesRead = fread(prog, sizeof(unsigned int), progSize, file);

    if (bytesRead != (size_t)progSize) {
        fprintf(stderr, "Error: Could not read from file %s\n", filename);
        exit(1);
    }

    progSize = (int)bytesRead;

    fclose(file);
}

void listProg(void) {
    // Gibt als lesbare Assembler-Text aus
    for (int i = 0; i < progSize; i++) {
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
            case 32:
                printf("new     %d\n", imm);
                break;
            case 33:
                printf("getf    %d\n", imm);
                break;
            case 34:
                printf("putf    %d\n", imm);
                break;
            case 35:
                printf("newa\n");
                break;
            case 36:
                printf("getfa\n");
                break;
            case 37:
                printf("putfa\n");
                break;
            case 38:
                printf("getsz\n");
                break;
            case 39:
                printf("pushn\n");
                break;
            case 40:
                printf("refeq\n");
                break;
            case 41:
                printf("refne\n");
                break;
            default:
                fprintf(stderr, "Error: unknown opcode %d\n", opcode);
                exit(1);
        }
    }

    printf("    --- end of code ---     ");
}

void inspStack(void) {printf("Stack: \n");printf("sp = %d, fp = %d\n", sp, fp);

if (sp == 0) {
    printf("-- empty stack\n");
    return;
}

for (int i = sp - 1; i >= 0; i--) {
    if (stack[i].is_ref) {
        bip.op1 = stack[i].value.objRef;
        int val = bigToInt();
        printf("%04d: obj@%p value=%d", i, (void *)stack[i].value.objRef, val);
    } else {
        printf("%04d: number =%d", i, stack[i].value.number);
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

void inspData(void) {printf("Global Data: \n");for (int i = 0; i < dataSize; i++) {if (data[i] != NULL) {bip.op1 = data[i];int val = bigToInt();printf("%04d: obj@%p value=%d\n", i, (void *)data[i], val);} else {printf("%04d: NULL\n", i);}}}

void inspObject(ObjRef obj) {if (obj == NULL) {printf("NULL pointer\n");return;}

/*
* printf("Object at %p:\n", (void *)obj);
* bip.op1 = obj;
* printf("  value: %d\n", bigToInt());
*/

if(IS_PRIM(obj)) {
    printf("Primitive Object, size=%d\n", GET_SIZE(obj));
} else {
    printf("Compound Object,fields=%d\n", GET_SIZE(obj));
}

}

void inspObjectByAddr(void) {char buffer[256];unsigned long addr;

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

ObjRef obj = (ObjRef)addr;
inspObject(obj);

}

void execInstr(void) {
    currentInstrPc = pc;
    unsigned int instr = prog[pc];
    int opcode = (instr >> 24) & 0xFF;
    int imm = (int)SIGN_EXTEND(instr & 0x00FFFFFF);
    pc++;
    ObjRef o1, o2;

    // printf("pc=%d opcode=%d imm=%d sp=%d fp=%d\n",
        //    currentInstrPc,
        //    opcode,
        //    imm,
        //    sp,
        //    fp);

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
        bip.op1 = o1;
        bip.op2 = o2;
        bigAdd();
        pushRef(bip.res);
        break;
    case SUB:
        o2 = popRef();
        o1 = popRef();
        bip.op1 = o1;
        bip.op2 = o2;
        bigSub();
        pushRef(bip.res);
        break;
    case MUL:
        o2 = popRef();
        o1 = popRef();
        bip.op1 = o1;
        bip.op2 = o2;
        bigMul();
        pushRef(bip.res);
        break;
    case DIV:
        o2 = popRef();
        o1 = popRef();
        bip.op1 = o1;
        bip.op2 = o2;
        bigDiv();
        pushRef(bip.res);
        break;
    case MOD:
        o2 = popRef();
        o1 = popRef();
        bip.op1 = o1;
        bip.op2 = o2;
        bigDiv();
        pushRef(bip.rem);
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
        bip.op1 = o1;
        printf("%d", bigToInt());
        break;
    case RDCHR:
        o1 = newInt(getchar());
        pushRef(o1);
        break;
    case WRCHR:
        o1 = popRef();
        bip.op1 = o1;
        putchar(bigToInt());
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
            runtimeError("Stack Overflow");
        }
        for (int i = sp; i < sp + imm; i++) {
            stack[i].is_ref = 1;
            stack[i].value.objRef = NULL;
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
            printf("PUSHL: fp=%d imm=%d addr=%d sp=%d\n", fp, imm, addr, sp);
            runtimeError("Illegal stack access");
        }
        if (!stack[addr].is_ref) {
            runtimeError("PUSHL detected number in local or paramter variable");
        }
        pushSlot(stack[addr]);
        break;
    }
    case POPL: {
        int addr = fp + imm;
        if (addr < 0 || addr >= sp) {
            runtimeError("Illegal stack access");
        }
        stack[addr] = popSlot();
        break;
    }
    case EQ:
            o2 = popRef();
            o1 = popRef();
            bip.op1 = o1;
            bip.op2 = o2;
            pushRef(newInt(bigCmp() == 0));
        break;
    case NE:
            o2 = popRef();
            o1 = popRef();
            bip.op1 = o1;
            bip.op2 = o2;
            pushRef(newInt(bigCmp() != 0));
        break;
    case LT:
            o2 = popRef();
            o1 = popRef();
            bip.op1 = o1;
            bip.op2 = o2;
            pushRef(newInt(bigCmp() < 0));
        break;
    case LE:
            o2 = popRef();
            o1 = popRef();
            bip.op1 = o1;
            bip.op2 = o2;
            pushRef(newInt(bigCmp() <= 0));
        break;
    case GT:
            o2 = popRef();
            o1 = popRef();
            bip.op1 = o1;
            bip.op2 = o2;
            pushRef(newInt(bigCmp() > 0));
        break;
    case GE:
            o2 = popRef();
            o1 = popRef();
            bip.op1 = o1;
            bip.op2 = o2;
            pushRef(newInt(bigCmp() >= 0));
        break;
    case JMP:
        pc = imm;
        break;
    case BRF:
        o1 = popRef();
        bip.op1 = o1;
        if (bigToInt() == 0) {
            pc = imm;
        }
        break;
    case BRT:
        o1 = popRef();
        bip.op1 = o1;
        if (bigToInt() != 0) {
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
            runtimeError("Stack underflow");
        }
        sp = sp - imm;
        break;
    case PUSHR:
        if (rv == NULL) {
            runtimeError("Return value register is empty");
        }
        pushRef(rv);
        break;
    case POPR:
        rv = popRef();
        break;
    case DUP:
        {
            StackSlot s = popSlot();
            pushSlot(s);
            pushSlot(s);
        }
        break;
    case NEW:
        if(imm < 0) {
            runtimeError("Illegal object size");
        }
        pushRef(newCompoundObject(imm));
        break;
    case GETF:
        {
            o1 = popRef();
            if (o1 == NULL) {
                fatalError("NULL Pointer Reference");
            }
            if (imm < 0 || (unsigned) imm >= GET_SIZE(o1)) {
                fatalError("index out of range");
            }
            if(IS_PRIM(o1)) {
                fatalError("primitive Object has no fields");
            }
            ObjRef *fields = (ObjRef *)o1->data;
            pushRef(fields[imm]);
        }
        break;
    case PUTF:
        {
            ObjRef value = popRef();
            o1 = popRef();
            if (o1 == NULL) {
                fatalError("NULL Pointer Reference");
            }
            if (imm < 0 || (unsigned) imm >= GET_SIZE(o1)) {
                fatalError("index out of range");
            }
            if(IS_PRIM(o1)) {
                fatalError("primitive Object has no fields");
            }
            ObjRef *fields = (ObjRef *)o1->data;
            fields[imm] = value;
        }
        break;
    case NEWA: {
        /*{
            int n = popRaw();
            if (n < 0) {
                fatalError("illegal array size\n");
            }
            pushRef(newCompoundObject(n));
        }
        break;*/
        ObjRef sizeObj = popRef();
        bip.op1 = sizeObj;
        int n = bigToInt();

        if (n < 0) {
            fatalError("Illegal Array Size");
        }

        pushRef(newCompoundObject(n));
        break;
        }
    case GETFA:
        {
            ObjRef id_Obj = popRef();
            bip.op1 = id_Obj;
            int i = bigToInt();
            o1 = popRef();

            if(o1 == NULL) {
                fatalError("NULL pointer reference access\n");
            }
            if(i < 0 || (unsigned int) i >= GET_SIZE(o1)) {
                fatalError("index out of range");
            }
            if(IS_PRIM(o1)) {
                fatalError("primitive Object has no fields");
            }
            ObjRef *fields = (ObjRef *)o1->data;
            pushRef(fields[i]);
            break;
        }
    case PUTFA:
        {
            ObjRef value = popRef();
            ObjRef id_Obj = popRef();
            bip.op1 = id_Obj;
            int i = bigToInt();
            o1 = popRef();
            
            if (o1 == NULL) {
                fatalError("NULL Pointer Reference Access");
            }
            if(i < 0 || (unsigned int)i >= GET_SIZE(o1)) {
                fatalError("index out of range");
            }
            if(IS_PRIM(o1)) {
                fatalError("primitive Object has no fields");
            }
            ObjRef *fields = (ObjRef *)o1->data;
            fields[i] = value;
            break;
        }
    case GETSZ:
        o1 = popRef();
        if (o1 == NULL) {
            fatalError("NULL Pointer Reference Access");
        }
        pushRef(newInt(GET_SIZE(o1)));
        break;
    case PUSHN:
        pushRef(NULL);
        break;

    // Referenzvergleich
    case REFEQ:
        o2 = popRef();
        o1 = popRef();
        pushRef(newInt(o1 == o2));
        break;
    case REFNE:
        o2 = popRef();
        o1 = popRef();
        pushRef(newInt(o1 != o2));
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
    }
}

void printCurrentInstr(void) {
    if (pc >= progSize) {
        printf("Program finished\n");
        return;
    }

    printInstructionAt(stdout, pc);
    printf("\n");
}

// Debugging method
void debugger(void) {
    char command[256];

    printCurrentInstr();
    while (1) {
        printf("DEBUG: inspect, object, list, breakpoint, step, run, quit?\n");
        if (fgets(command, sizeof(command), stdin)) {
            command[strcspn(command, "\n")] = '\0';
            if (strncmp(command, "list", 1) == 0) {
                listProg();
            } else if (strncmp(command, "run", 1) == 0) {
                execProg();
                if (pc >= progSize || halted) {
                    printf("Ninja Virtual Machine stopped\n");
                    break;
                }
            } else if (strncmp(command, "quit", 1) == 0) {
                break;
            } else if (strncmp(command, "object", 1) == 0) {
                inspObjectByAddr();
            } else if (strncmp(command, "inspect", 1) == 0) {
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
            } else if (strncmp(command, "step", 1) == 0) {
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
                    if (strcmp(input, "\n") != 0) {
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

int main(int argc, char **argv) {
    printf("Ninja Virtual Machine started\n");
    
    //Stack
    // Default Anzahl an KiB
    int stack_n = 64;
    stackSize = (stack_n / 1024) / sizeof(StackSlot);
    stack = malloc(stackSize * sizeof(StackSlot));

    // Heap
    int heap_n = 8192;
    heapSize = heap_n * 1024;
    semiSize = heapSize / 2;

    heap = malloc(heapSize);
    fromSpace = heap;
    toSpace = heap + semiSize;
    freePtr = fromSpace;
    heapEnd = fromSpace + semiSize;
    

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        time_t now = time(NULL);
        printf("Ninja Virtual Machine version 5 (compiled on %s)\n", ctime(&now));
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [--debug] <codefile>\n", argv[0]);
        printf("Options:\n");
        printf("  --debug   Enable debug mode (lists instructions and starts debugger)\n");
        printf("  --version Show version information\n");
        printf("  --help    Show this help message\n");
        return 0;
    }

    int debug = 0;
    char *filename;

    if (argc == 3 && strcmp(argv[2], "--debug") == 0) {
        debug = 1;
        filename = argv[1];
    } else if (argc == 2) {
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

    free(data);

    return 0;
}