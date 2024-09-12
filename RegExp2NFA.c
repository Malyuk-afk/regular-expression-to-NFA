#include <stdio.h>
#include <stdlib.h>

// type identifiers of RegExp
#define CONCAT 1
#define KLEENE 2
#define TERMINAL 3
#define ALTERNATION 4
#define INTERVAL 5

#define MAX_STATE 100
#define MAX_CHAR 96 // printable characters are from 1 to 95
#define EPSILON 0
#define INDEX(c) ((c) - 31)

// define the structure of regular expression
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
    Concat *c = (Concat *)malloc(sizeof(Concat));
    c->left = left;
    c->right = right;
    RegExp *r = (RegExp *)malloc(sizeof(RegExp));
    r->type = CONCAT;
    r->data = (void *)c;
    return r;
}

RegExp *kleene(RegExp *exp)
{
    Kleene *k = (Kleene *)malloc(sizeof(Kleene));
    k->exp = exp;
    RegExp *r = (RegExp *)malloc(sizeof(RegExp));
    r->type = KLEENE;
    r->data = (void *)k;
    return r;
}

RegExp *terminal(char t)
{
    Terminal *term = (Terminal *)malloc(sizeof(Terminal));
    term->terminal = t;
    RegExp *r = (RegExp *)malloc(sizeof(RegExp));
    r->type = TERMINAL;
    r->data = (void *)term;
    return r;
}

RegExp *alternation(RegExp *left, RegExp *right)
{
    Alternation *a = (Alternation *)malloc(sizeof(Alternation));
    a->left = left;
    a->right = right;
    RegExp *r = (RegExp *)malloc(sizeof(RegExp));
    r->type = ALTERNATION;
    r->data = (void *)a;
    return r;
}

RegExp *interval(char start, char end)
{
    Interval *i = (Interval *)malloc(sizeof(Interval));
    i->start = start;
    i->end = end;
    RegExp *r = (RegExp *)malloc(sizeof(RegExp));
    r->type = INTERVAL;
    r->data = (void *)i;
    return r;
}

// recursively free the memory of regular expression
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
    free(r);
}

// print the regular expression
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

int compile(ExpList **ExpNFA, CharList **CharNFA)
{
    int i = 0;
    ExpList *p = ExpNFA[i];
    while (p)
    {
        printf("State %d: ", i);
        printRegExp(p->exp);
        printf(" -> %d\n", p->nextstate);
        p = p->next;
    }
    return 0;
}

int main()
{
    RegExp *r;
    // carete a regular expression tree "([0 − 9][0 − 9]∗.[0 − 9]∗)|(.[0 − 9][0 − 9]∗)"
    r = alternation(
        concat(
            concat(
                concat(interval('0','9'), kleene(interval('0','9'))),
                terminal('.')),
            kleene(interval('0','9'))
              ),
        concat(
            terminal('.'),
            concat(
                interval('0', '9'),
                kleene(interval('0', '9'))
                  )
              )
              );
    
    printRegExp(r);

    //
    ExpList **ExpNFA = calloc(sizeof(ExpList), MAX_STATE);
    CharList **CharNFA = calloc(sizeof(CharList), MAX_CHAR);

    // state 0 is the start state, state 1 is the final state
    // currently, r is the only one edge
    ExpNFA[0] = (ExpList *)malloc(sizeof(ExpList));
    ExpNFA[0]->nextstate = 1;
    ExpNFA[0]->exp = r;
    ExpNFA[0]->next = NULL;

    compile(ExpNFA, CharNFA);
    freeRegExp(r);
    r = NULL;
    return 0;
}