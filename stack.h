#ifndef STACK_H
#define STACK_H

#include<stddef.h>

typedef enum {
	TOKEN_TYPE_UNKNOWN = -1,
	TOKEN_TYPE_NUM = 0,
	TOKEN_TYPE_OP_ADD = 1,
	TOKEN_TYPE_OP_SUB = 2,
	TOKEN_TYPE_OP_MUL = 3,
	TOKEN_TYPE_OP_DIV = 4,
	TOKEN_TYPE_OP_LPAR = 5,
	TOKEN_TYPE_OP_RPAR = 6
}TokenType_t;

typedef struct token{
	union {
		long double val;
		char op;
	};
	TokenType_t type;
	int precedence;
	struct token *next;
}Token_t;

typedef struct {
	Token_t *top;
	size_t size;
}Stack_t;

int stack_push(Stack_t *stack,  Token_t *token);
Token_t *stack_pop(Stack_t *stack);

#endif
