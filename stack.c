
#include"stack.h"
#include<stdlib.h>

int stack_push(Stack_t *stack, Token_t *token)
{
	int ret = 0;

	if (stack && token) {

		/* check if the stack is empty*/
		if (stack->size == 0) {
			stack->top = token;
		}
		else {
			Token_t *old = stack->top;
			token->next = old;
			stack->top = token;
		}
		stack->size++;
	}
	else {
		ret = -1;
	}

	return ret;
}

Token_t *stack_pop(Stack_t *stack)
{
	Token_t *old_top;
	Token_t *new_top;

	if (stack) {

		if (stack->size == 0) {
			old_top = NULL;
		}
		else {
			old_top = stack->top;
			new_top = old_top->next;
			stack->top = new_top;
			stack->size--;
		}
	}
	return old_top;
}
