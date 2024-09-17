#include <stdio.h>
#include <stdlib.h>

// type identifiers of RegExp
#define CONCAT 1
#define KLEENE 2
#define TERMINAL 3
#define ALTERNATION 4
#define INTERVAL 5

#define MAX_STATE 100
#define MAX_STACK 300
#define MAX_Terminal 96 // printable characters are from 1 to 95, 0 for epsilon
#define EPSILON (0 + 31)
#define INDEX(c) ((c) - 31)

// define the structures of regular expression tree and NFA
typedef struct
{
    char type;
    void *data;
} RegExp;

typedef struct
{
    RegExp *left;
    RegExp *right;
} Concat;

typedef struct
{
    RegExp *exp;
} Kleene;

typedef struct
{
    char terminal;
} Terminal;

typedef struct
{
    RegExp *left;
    RegExp *right;
} Alternation;

typedef struct
{
    char start;
    char end;
} Interval;

typedef struct ExpList ExpList;
typedef struct CharList CharList;

struct ExpList
{
    char nextstate;
    RegExp *exp;
    ExpList *next;
};

struct CharList
{
    char c;
    CharList *next;
};

// initialize the regular expression
RegExp *concat(RegExp *left, RegExp *right)
{
    Concat *c = (Concat *)calloc(sizeof(Concat), 1);
    c->left = left;
    c->right = right;
    RegExp *r = (RegExp *)calloc(sizeof(RegExp), 1);
    r->type = CONCAT;
    r->data = (void *)c;
    return r;
}

RegExp *kleene(RegExp *exp)
{
    Kleene *k = (Kleene *)calloc(sizeof(Kleene), 1);
    k->exp = exp;
    RegExp *r = (RegExp *)calloc(sizeof(RegExp), 1);
    r->type = KLEENE;
    r->data = (void *)k;
    return r;
}

RegExp *terminal(char t)
{
    Terminal *term = (Terminal *)calloc(sizeof(Terminal), 1);
    term->terminal = t;
    RegExp *r = (RegExp *)calloc(sizeof(RegExp), 1);
    r->type = TERMINAL;
    r->data = (void *)term;
    return r;
}

RegExp *alternation(RegExp *left, RegExp *right)
{
    Alternation *a = (Alternation *)calloc(sizeof(Alternation), 1);
    a->left = left;
    a->right = right;
    RegExp *r = (RegExp *)calloc(sizeof(RegExp), 1);
    r->type = ALTERNATION;
    r->data = (void *)a;
    return r;
}

RegExp *interval(char start, char end)
{
    Interval *i = (Interval *)calloc(sizeof(Interval), 1);
    i->start = start;
    i->end = end;
    RegExp *r = (RegExp *)calloc(sizeof(RegExp), 1);
    r->type = INTERVAL;
    r->data = (void *)i;
    return r;
}

// recursively free the memory of regular expression tree
void freeRegExp(RegExp *r)
{
    if (r->type == CONCAT)
    {
        freeRegExp(((Concat *)r->data)->left);
        freeRegExp(((Concat *)r->data)->right);
        free(r->data);
    }
    else if (r->type == KLEENE)
    {
        freeRegExp(((Kleene *)r->data)->exp);
        free(r->data);
    }
    else if (r->type == ALTERNATION)
    {
        freeRegExp(((Alternation *)r->data)->left);
        freeRegExp(((Alternation *)r->data)->right);
        free(r->data);
    }
    else
    {
        free(r->data);
    }
}

void freeExpNFA(ExpList **p)
{
    ExpList *q, *next;
    for (int i = 0; i < MAX_STATE; i++)
    {
        q = p[i];
        while (q)
        {
            free(q->exp);
            next = q->next;
            free(q);
            q = next;
        }
    }
}

void freeCharNFA(CharList ***p)
{
    CharList *q, *next;
    for (int i = 0; i < MAX_STATE; i++)
    {
        if (p[i])
        {
            for (int j = 0; j < MAX_Terminal; j++)
            {
                q = p[i][j];
                while (q)
                {
                    next = q->next;
                    free(q);
                    q = next;
                }
            }
            free(p[i]);
        }
    }
}

// print the regular expression tree
void printRegExp(RegExp *r)
{
    if (!r)
    {
        printf("NULL");
        return;
    }
    if (r->type == CONCAT)
    {
        printf("(");
        printRegExp(((Concat *)r->data)->left);
        printRegExp(((Concat *)r->data)->right);
        printf(")");
    }
    else if (r->type == KLEENE)
    {
        printf("(");
        printRegExp(((Kleene *)r->data)->exp);
        printf(")*");
    }
    else if (r->type == ALTERNATION)
    {
        printf("(");
        printRegExp(((Alternation *)r->data)->left);
        printf(" | ");
        printRegExp(((Alternation *)r->data)->right);
        printf(")");
    }
    else if (r->type == TERMINAL)
    {
        printf("%c", ((Terminal *)r->data)->terminal);
    }
    else if (r->type == INTERVAL)
    {
        printf("[%c-%c]", ((Interval *)r->data)->start, ((Interval *)r->data)->end);
    }
}

ExpList *allocExpList()
{
    ExpList *p = (ExpList *)calloc(sizeof(ExpList), 1);
    if (!p)
    {
        printf("Error: memory allocation failed\n");
        exit(1);
    }
    return p;
}

CharList *allocCharList()
{
    CharList *p = (CharList *)calloc(sizeof(CharList), 1);
    if (!p)
    {
        printf("Error: memory allocation failed\n");
        exit(1);
    }
    return p;
}

int newstate(void **NFA, int currentState)
{
    int i = currentState;
    while (NFA[i] != NULL)
    {
        i++;
        if (i == MAX_STATE)
        {
            printf("Error: too many states, max state is %d\n", MAX_STATE);
            exit(1);
        }
    }
    return i;
}

void addCharList(CharList *p, int nextstate)
{
    while (p->next && p->c != nextstate)
    {
        p = p->next;
    }
    if (p->c == nextstate)
    {
        return;
    }
    else
    {
        p->next = allocCharList();
        p = p->next;
        p->c = nextstate;
    }
}

void addTerminal(CharList ***CharNFA, char c, int state, int nextstate)
{
    int i = state;
    if (!CharNFA[state])
    {
        CharNFA[state] = (CharList **)calloc(sizeof(CharList *), MAX_Terminal);
        if (!CharNFA[state])
        {
            printf("Error: memory allocation failed\n");
            exit(1);
        }
    }
    CharList *q = CharNFA[state][INDEX(c)];
    if (!q)
    {
        q = allocCharList();
        q->c = nextstate;
        CharNFA[state][INDEX(c)] = q;
    }
    else
    {
        addCharList(q, nextstate);
    }
}

int compile(ExpList **ExpNFA, CharList ***CharNFA)
{
    int state = 1;
    ExpList *p;
    int type, nextstate, i, news;
    RegExp *left, *right, *exp;
    ExpList *q;
    char c, start, end;

    while (ExpNFA[state])
    {
        // epsilon identity transition is added to the beginning of the list
        addTerminal(CharNFA, EPSILON, state, state);

        while (ExpNFA[state])
        {
            p = ExpNFA[state];
            type = p->exp->type;
            if (type == CONCAT)
            {
                left = ((Concat *)p->exp->data)->left;
                right = ((Concat *)p->exp->data)->right;
                nextstate = p->nextstate;
                i = state;
                news = newstate((void **)ExpNFA, state);
                if (!news)
                {
                    exit(1);
                }
                p->nextstate = news;
                p->exp = left;
                ExpNFA[news] = allocExpList();
                ExpNFA[news]->exp = right;
                ExpNFA[news]->nextstate = nextstate;
            }

            else if (type == KLEENE)
            {
                exp = ((Kleene *)p->exp->data)->exp;
                nextstate = p->nextstate;
                i = state;
                p->exp = exp;
                addTerminal(CharNFA, EPSILON, state, nextstate);
                p->nextstate = state;
            }

            else if (type == TERMINAL)
            {
                c = ((Terminal *)p->exp->data)->terminal;
                nextstate = p->nextstate;
                addTerminal(CharNFA, c, state, nextstate);
                ExpNFA[state] = p->next;
                free(p);
            }

            else if (type == ALTERNATION)
            {
                left = ((Alternation *)p->exp->data)->left;
                right = ((Alternation *)p->exp->data)->right;
                nextstate = p->nextstate;
                q = p;
                while (q->next)
                {
                    q = q->next;
                }
                q->next = allocExpList();
                q = q->next;
                q->exp = right;
                q->nextstate = nextstate;
                p->exp = left;
            }
            else if (type == INTERVAL)
            {
                start = ((Interval *)p->exp->data)->start;
                end = ((Interval *)p->exp->data)->end;
                nextstate = p->nextstate;
                for (c = start; c <= end; c++)
                {
                    addTerminal(CharNFA, c, state, nextstate);
                }
                ExpNFA[state] = p->next;
                free(p);
            }
        }
        state++;
    }
}

int printNFA(CharList ***CharNFA)
{
    for (int i = 0; i < MAX_STATE; i++)
    {
        if (CharNFA[i])
        {
            printf("state %d\n", i);
            for (int j = 0; j < MAX_Terminal; j++)
            {
                CharList *p = CharNFA[i][j];
                if (p)
                {
                    if (j == 0)
                    {
                        printf(" ε: ");
                    }
                    else
                    {
                        printf("  %c: ", j + 31);
                    }
                    while (p)
                    {
                        printf("%d ", p->c);
                        p = p->next;
                    }
                    printf("\n");
                }
            }
        }
    }
}

typedef struct
{
    char *remainder;
    CharList *NFA;
} stateFrame;

int runNFA(CharList ***CharNFA, char *input)
{
    stateFrame *stack = calloc(sizeof(stateFrame), MAX_STACK);
    if (!stack)
    {
        printf("Error: memory allocation failed\n");
        return -1;
    }
    int sp = 0;
    stack[0].remainder = input;
    stack[0].NFA = CharNFA[1][0];
    char c;
    char *p;
    int state;
    CharList *q;
    while (1)
    {
        if (!stack[sp].NFA && sp == 0)
        {
            return 0;
        }
        else if (!stack[sp].NFA)
        {
            sp--;
            continue;
        }
        state = stack[sp].NFA->c;
        if (!state)
        {
            return 1;
        }
        p = stack[sp].remainder;
        stack[sp].NFA = stack[sp].NFA->next;
        sp++;
        if (sp == MAX_STACK - 1)
        {
            printf("Error: runNFA stack overflow\n");
            exit(1);
        }
        if (!(sp % 2))
        {
            if (!*p)
            {
                for (q = CharNFA[state][0]; q; q = q->next)
                {
                    if (q->c == 0)
                        return 1;
                }
                sp--;
                continue;
            }
            stack[sp].remainder = p;
            stack[sp].NFA = CharNFA[state][0];
        }
        else
        {
            stack[sp].remainder = p + 1;
            c = *p;
            stack[sp].NFA = CharNFA[state][INDEX(c)];
        }
    }
}

// add epsilon transitions to create epsilon closure
// it will help a lot when run the NFA
void refineEpsilon(CharList ***CharNFA)
{
    CharList **stack = (CharList **)calloc(sizeof(CharList *), MAX_STACK);
    if (!stack)
    {
        printf("Error: memory allocation failed\n");
        exit(1);
    }
    CharList *p;
    int sp;
    for (int i = 1; CharNFA[i]; i++)
    {
        p = CharNFA[i][0]->next;
        if (!p)
        {
            continue;
        }
        stack[0] = p;
        sp = 0;
        char c;
        while (1)
        {
            if ((!stack[sp]) && (sp == 0))
            {
                break;
            }
            else if (!stack[sp])
            {
                sp--;
            }
            c = stack[sp]->c;
            stack[sp] = stack[sp]->next;
            if (!c)
            {
                continue;
            }
            addCharList(p, c);
            if (stack[sp])
            {
                sp++;
                if (sp == MAX_STACK - 1)
                {
                    printf("Error: refineEpsilon stack overflow\n");
                    exit(1);
                }
            }
            stack[sp] = CharNFA[c][0]->next;
        }
    }
}

void test(CharList ***CharNFA, char *input, char *message)
{
    if (runNFA(CharNFA, input))
    {
        printf("\nYes, \"%s\" is %s.\n", input, message);
    }
    else
    {
        printf("\nNo, \"%s\" is not %s.\n", input, message);
    }
}

int main()
{
    RegExp *r;
    // ([0 − 9][0 − 9]∗.[0 − 9][0 − 9]∗)|(.[0 − 9][0 − 9]∗)
    r = alternation(
        concat(
            concat(
                concat(interval('0', '9'), kleene(interval('0', '9'))),
                terminal('.')),
            concat(
                interval('0', '9'),
                kleene(interval('0', '9')))),
        concat(
            terminal('.'),
            concat(
                interval('0', '9'),
                kleene(interval('0', '9')))));

    printRegExp(r);
    printf("\n");

    ExpList **ExpNFA = calloc(sizeof(ExpList *), MAX_STATE);
    CharList ***CharNFA = calloc(sizeof(CharList **), MAX_STATE);

    // state 1 is the start state, state 0 is the final state
    // currently, r is the only one edge
    ExpNFA[1] = allocExpList();
    ExpNFA[1]->nextstate = 0;
    ExpNFA[1]->exp = r;

    compile(ExpNFA, CharNFA);
    refineEpsilon(CharNFA);
    test(CharNFA, "3.1415926", "a rational number");
    test(CharNFA, "a rational nember", "a rational number");

    freeRegExp(r);
    freeExpNFA(ExpNFA);
    freeCharNFA(CharNFA);
    free(CharNFA);
    free(ExpNFA);
    ExpNFA = calloc(sizeof(ExpList *), MAX_STATE);
    CharNFA = calloc(sizeof(CharList **), MAX_STATE);

    //[A-Z][a-z]*((, [a-z][a-z]*)|( [a-z][a-z]*)))*(.|?)
    r =
        concat(
            concat(
                concat(
                    interval('A', 'Z'),
                    kleene(interval('a', 'z'))),
                kleene(
                    alternation(
                        concat(terminal(','),
                               concat(terminal(' '),
                                      concat(interval('a', 'z'), kleene(interval('a', 'z'))))),
                        concat(terminal(' '),
                               concat(interval('a', 'z'), kleene(interval('a', 'z'))))))),
            alternation(terminal('.'), terminal('?')));

    printRegExp(r);
    printf("\n");

    ExpNFA[1] = allocExpList();
    ExpNFA[1]->nextstate = 0;
    ExpNFA[1]->exp = r;

    compile(ExpNFA, CharNFA);
    refineEpsilon(CharNFA);
    test(CharNFA, "Hello, world.", "a sentence");
    test(CharNFA, "Hello, world", "a sentence");
    test(CharNFA, "Hello world.", "a sentence");
    test(CharNFA, "Hello world", "a sentence");
    test(CharNFA, "Hello, world?", "a sentence");
    test(CharNFA, "Hello, World?", "a sentence");
    test(CharNFA, "Yes, is a sentence.", "a sentence");

    return 0;
}