#include <stdio.h>
#include <stdlib.h>

// type identifiers of RegExp
#define CONCAT 1
#define KLEENE 2
#define TERMINAL 3
#define ALTERNATION 4
#define INTERVAL 5

#define MAX_STATE 100
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
    free(r);
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
        return 0;
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
            return 0;
        }
    }
    return i;
}

int addTerminal(CharList ***CharNFA, char c, int state, int nextstate)
{
    int i = state;
    if (!CharNFA[state])
    {
        CharNFA[state] = (CharList **)calloc(sizeof(CharList *), MAX_Terminal);
        if (!CharNFA[state])
        {
            printf("Error: memory allocation failed\n");
            return -1;
        }
    }
    CharList *q = CharNFA[state][INDEX(c)];
    if (!q)
    {
        q = (CharList *)calloc(sizeof(CharList), 1);
        if (!q)
        {
            printf("Error: memory allocation failed\n");
            return -1;
        }
        q->c = nextstate;
        CharNFA[state][INDEX(c)] = q;
    }
    else
    {
        while (q->next && q->c != nextstate)
        {
            q = q->next;
        }
        if (q->c != nextstate)
        {
            q->next = (CharList *)calloc(sizeof(CharList), 1);
            if (!q->next)
            {
                printf("Error: memory allocation failed\n");
                return -1;
            }
            q = q->next;
            q->c = nextstate;
        }
    }
    return 0;
}

int compile(ExpList **ExpNFA, CharList ***CharNFA)
{
    int state = 1;
    while (ExpNFA[state])
    {
        while (ExpNFA[state])
        {
            ExpList *p = ExpNFA[state];
            int type = p->exp->type;
            printRegExp(p->exp);
            printf("\n");
            printf("state %d\n", state);
            if (type == CONCAT)
            {
                RegExp *left = ((Concat *)p->exp->data)->left;
                RegExp *right = ((Concat *)p->exp->data)->right;
                int nextstate = p->nextstate;
                int i = state;
                int news = newstate((void **)ExpNFA, state);
                if (!news)
                {
                    return -1;
                }
                p->nextstate = news;
                p->exp = left;
                ExpNFA[news] = allocExpList();
                if (!ExpNFA[news])
                {
                    return -1;
                }
                ExpNFA[news]->exp = right;
                ExpNFA[news]->nextstate = nextstate;
            }

            else if (type == KLEENE)
            {
                RegExp *exp = ((Kleene *)p->exp->data)->exp;
                int nextstate = p->nextstate;
                int i = state;
                p->exp = exp;
                addTerminal(CharNFA, EPSILON, state, nextstate);
                p->nextstate = state;
            }

            else if (type == TERMINAL)
            {
                char c = ((Terminal *)p->exp->data)->terminal;
                int nextstate = p->nextstate;
                if (addTerminal(CharNFA, c, state, nextstate))
                {
                    return -1;
                }
                printf("current node: %p, next node: %p", p, p->next);
                ExpNFA[state] = p->next;
                free(p);
            }

            else if (type == ALTERNATION)
            {
                RegExp *left = ((Alternation *)p->exp->data)->left;
                RegExp *right = ((Alternation *)p->exp->data)->right;
                int nextstate = p->nextstate;
                ExpList *q = p;
                while (q->next)
                {
                    q = q->next;
                }
                q->next = allocExpList();
                if (!q->next)
                {
                    return -1;
                }
                q = q->next;
                q->exp = right;
                q->nextstate = nextstate;
                p->exp = left;
            }
            else if (type == INTERVAL)
            {
                char start = ((Interval *)p->exp->data)->start;
                char end = ((Interval *)p->exp->data)->end;
                int nextstate = p->nextstate;
                for (char c = start; c <= end; c++)
                {
                    if (addTerminal(CharNFA, c, state, nextstate))
                    {
                        return -1;
                    }
                }
                printf("current node: %p, next node: %p", p, p->next);
                ExpNFA[state] = p->next;
                free(p);
            }
        }
        state++;
        printf("state %d\n", state);
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

int runNFA(CharList ***CharNFA, char *input)
{
    typedef struct
    {
        CharList *states;
        char *remand;
    } Branch;
    Branch *stack = (Branch *)calloc(sizeof(Branch), 100);
    if (!stack)
    {
        printf("Error: memory allocation failed\n");
        return -1;
    }
    int state = 1;
    char c

}

int main()
{
    RegExp *r;
    // carete a regular expression tree "([0 − 9][0 − 9]∗.[0 − 9][0 − 9]∗)|(.[0 − 9][0 − 9]∗)"
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
    
    //r = alternation(terminal('a'), concat(terminal('a'),terminal('b')));

    printRegExp(r);

    //
    ExpList **ExpNFA = calloc(sizeof(ExpList *), MAX_STATE);
    CharList ***CharNFA = calloc(sizeof(CharList **), MAX_STATE);

    // state 1 is the start state, state 0 is the final state
    // currently, r is the only one edge
    ExpNFA[1] = allocExpList();
    ExpNFA[1]->nextstate = 0;
    ExpNFA[1]->exp = r;
    ExpNFA[0] = NULL;

    printf("start\n");
    compile(ExpNFA, CharNFA);
    printf("NFA\n");
    freeRegExp(r);
    printNFA(CharNFA);
    r = NULL;
    return 0;
}