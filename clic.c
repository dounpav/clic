/*
 * clic.c
 *
 * Author: Pavel (dounpav)
 */

#include"stack.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<errno.h>
#include<ctype.h>

#define ERR_SUCC	 (0)
#define ERR_SYNTAX	(-1)
#define ERR_MATH	(-2)
#define ERR_MALLOC	(-3)

static Stack_t out_stack = {NULL, 0};
static Stack_t op_stack = {NULL, 0};
static Stack_t tmp_stack = {NULL, 0};

inline static long double add(long double a, long double b)
{
	return a + b;
}

inline static long double sub(long double a, long double b)
{
	return a - b;
}

inline static long double mul(long double a, long double b)
{
	return a * b;
}

inline static long double divide(long double a, long double b)
{
	return a / b;
}

static void free_stack(Stack_t *stack)
{
	Token_t *tok = NULL;

	while (stack->size != 0) {
		tok = stack_pop(stack);
		if (tok) free(tok);
	}
}

static int string_to_longdouble(const char *str, long double *value)
{
	int ret = 0;
	long double tmp;
	char *endp = NULL;
	errno = 0;

	tmp = strtold(str, &endp);
	if (errno == ERANGE) {
		errno = 0;
		ret = ERR_MATH;
	}
	else if (*endp != '\0') {
		ret = ERR_SYNTAX;
	}
	else if (endp == str) {
		ret = ERR_SYNTAX;
	}
	else {
		*value = tmp;
	}

	return ret;
}

static bool search_operator(TokenType_t type)
{
	bool found = false;
	Token_t *tok;

	while (op_stack.size != 0) {

		tok = stack_pop(&op_stack);
		if (tok->type == type) {
			found = true;
			break;
		}
		stack_push(&out_stack, tok);
	}

	return found;
}

static int parse_operator(char operator, Token_t **token)
{
	int ret = 0;
	int prec = -1;

	TokenType_t type;
	Token_t *tmp;

	switch(operator)
	{
		case '+':
			type = TOKEN_TYPE_OP_ADD;
			prec = 2;
			break;

		case '-':
			type = TOKEN_TYPE_OP_SUB;
			prec = 2;
			break;

		case '*':
			type = TOKEN_TYPE_OP_MUL;
			prec = 3;
			break;

		case '/':
			type = TOKEN_TYPE_OP_DIV;
			prec = 3;
			break;

		case '(':
			type = TOKEN_TYPE_OP_LPAR;
			prec = -1;
			break;

		case ')':
			type = TOKEN_TYPE_OP_RPAR;
			prec = -1;

			/* search for left paranthesis if not found return
			 * syntax error*/
			if (search_operator(TOKEN_TYPE_OP_LPAR) == false) {
				ret = ERR_SYNTAX;
			}
			break;

		default:
			ret = ERR_SYNTAX;
			break;
	}

	if (ret == ERR_SUCC) {

		tmp = malloc(sizeof(Token_t));
		if (tmp) {
			tmp->type = type;
			tmp->op = operator;
			tmp->precedence = prec;
			*token = tmp;
		}
		else {
			ret = ERR_MALLOC;
		}
	}

	return ret;
}

static int parse_expression(const char *expr)
{
	int ret = 0;
	long double value = 0;
	char *str = NULL;
	bool num_part = false;
	int i = 0;

	Token_t *prev;
	Token_t *token;

	while (expr[i] != '\0') {

		if (isdigit(expr[i]) || expr[i] == '.') {

			/*
			 * If the number is a part of a larger number append
			 * the number to the string. If not, allocate new
			 * string for the new number
			 */
			if (num_part) {
				str = realloc(str, strlen(str) + 2);
				strncat(str, &expr[i], 1);
			}
			else {
				str = malloc(sizeof(char) + 1);
				if (str) {
					*str = expr[i];
					*(str + 1) = '\0';
					num_part = true;
				}
				else {
					ret = ERR_MALLOC;
					break;
				}
			}
		}
		/* Do not allow whitespaces */
		else if (isspace(expr[i])) {
			ret = ERR_SYNTAX;
			goto parse_exit;
		}
		else {
			/*
			 * If the previous char/string was a number then
			 * allocate and push token with that value to the
			 * output stack.
			 */
			if (num_part) {

				if (string_to_longdouble(str, &value) == 0) {

					token = malloc(sizeof(Token_t));
					if (token) {
						token->val = value;
						token->type = TOKEN_TYPE_NUM;
						stack_push(&out_stack, token);

						prev = token;
						num_part = false;
						free(str);
					}
					else {
						ret = ERR_MALLOC;
						free(str);
						goto parse_exit;
					}
				}
				else {
					ret = ERR_MATH;
					free(str);
					goto parse_exit;
				}
			}

			if (parse_operator(expr[i], &token) == -1) {
				ret = ERR_SYNTAX;
				goto parse_exit;
			}

			/*
			 * If the token is right parenthesis, ignore it and
			 * move to the next character, since we already
			 * processes this token in parse_operator function
			 */
			if (token->type == TOKEN_TYPE_OP_RPAR) {
				free(token);
				goto next_char;
			}

			/*
			 * if the token is left parenthesis, then just push the
			 * token to the operator stack and skip all of the
			 * procedures normally done for the normal operators.
			 */
			else if (token->type == TOKEN_TYPE_OP_LPAR) {
				stack_push(&op_stack, token);
				goto next_char;
			}
			else {
				// do nothing
			}

			if (op_stack.size == 0) {

				/*
				 * If the output stack is empty and the
				 * operator token is subtraction or addition,
				 * then create and push a 'dummy' zero value
				 * token. This is needed for correct
				 * calculation of ngative numbers.
				 */
				if (out_stack.size == 0 &&
						(token->type == TOKEN_TYPE_OP_SUB ||
						token->type == TOKEN_TYPE_OP_ADD))
				{
					Token_t *zero = malloc(sizeof(Token_t));
					if (zero) {
						zero->type = TOKEN_TYPE_NUM;
						zero->val = 0;
						stack_push(&out_stack, zero);
					}
					else {
						ret = ERR_MALLOC;
						goto parse_exit;
					}
				}
				stack_push(&op_stack, token);
			}
			else {
				/*
				 * If previous token was left parenthesis and
				 * current token is either addition or
				 * subtraction operator, allocate and push
				 * dummy zero token to the output stack
				 */
				if (prev->type == TOKEN_TYPE_OP_LPAR &&
						(token->type == TOKEN_TYPE_OP_SUB ||
						 token->type == TOKEN_TYPE_OP_ADD))
				{
					Token_t *zero = malloc(sizeof(Token_t));
					if (zero) {
						zero->type = TOKEN_TYPE_NUM;
						zero->val = 0;
						stack_push(&out_stack, zero);
					}
					else {
						ret = ERR_MALLOC;
						goto parse_exit;
					}
				}

				/*
				 * If curent operator's precedence is lower than
				 * the one's on top of the stack, pop operator
				 * stack and transfer popped operators to
				 * output stack until we find operator whose
				 * precedence is lower than current operator
				 * token's precedence.
				 */
				Token_t *top = op_stack.top;
				while (token->precedence <= top->precedence) {

					top = stack_pop(&op_stack);
					stack_push(&out_stack, top);
					top = op_stack.top;

					if (op_stack.size == 0) {
						break;
					}
				}
				stack_push(&op_stack, token);
			}
		}
next_char:
		prev = token;
		/*move to the next character*/
		i++;
	}

	/*
	 * If the last token was a number, allocate and push token with that
	 * value to the ouput stack
	 */
	if (num_part) {

		if (string_to_longdouble(str, &value) == 0) {

			token = malloc(sizeof(Token_t));
			if (token) {
				token->val = value;
				token->type = TOKEN_TYPE_NUM;
				stack_push(&out_stack, token);
				free(str);
			}
			else {
				ret = ERR_MALLOC;
			}
		}
	}

	/* If there are any remaining operators in the operator stack, push
	 * them to the output stack */
	while (op_stack.size != 0) {
		token = stack_pop(&op_stack);
		if (token) {
			stack_push(&out_stack, token);
		}
	}

parse_exit:

	return ret;
}


static int calculate(Token_t *tokens[], long double *res)
{
	int ret = ERR_SUCC;

	switch(tokens[0]->type){

		case TOKEN_TYPE_OP_ADD:
			*res = add(tokens[2]->val, tokens[1]->val);
			break;

		case TOKEN_TYPE_OP_SUB:
			*res = sub(tokens[2]->val, tokens[1]->val);
			break;

		case TOKEN_TYPE_OP_MUL:
			*res = mul(tokens[2]->val, tokens[1]->val);
			break;

		case TOKEN_TYPE_OP_DIV:
			if (tokens[2]->val == 0 || tokens[1]->val == 0) {
				ret = ERR_MATH;
			}
			else {
				*res = divide(tokens[2]->val, tokens[1]->val);
			}
			break;

		default:
			ret = ERR_SYNTAX;
			break;
	}

	return ret;
}

static int evaluate_expression(long double *result)
{
	long double res = 0;
	int ret = 0;
	Token_t *token;
	Token_t *tokens[3] = {0};


	/* There should be a single token left in the output stack.
	 * That token should contain the result*/
	while (out_stack.size != 1) {

		/*
		 * We first collect three tokens from the stack. After that we
		 * examine their contens. If they are valid, we proceed to
		 * calculate the value.
		 */
		for (int i = 0; i < 3; i++ ) {

			tokens[i] = stack_pop(&out_stack);
			if (tokens[i] == NULL) {

				ret = ERR_SYNTAX;
				free(tokens[0]);
				free(tokens[1]);
				free(tokens[2]);
				goto eval_exit;
			}
		}

		/*
		 * For successfull calculation we need two numbers and one
		 * operator. Otherwise push the first token to tmp stack and
		 * restack the remaining tokens back to output stack.
		 */
		if (tokens[0]->type != TOKEN_TYPE_NUM &&
				tokens[1]->type == TOKEN_TYPE_NUM &&
				tokens[2]->type == TOKEN_TYPE_NUM)
		{
			ret = calculate(tokens, &res);
			if (ret == ERR_SUCC) {

				/*
				 * Reuse one token for the result and push it
				 * back to the outputstack. If there are any
				 * tokens left in temp stack restack then to
				 * output stack.
				 */
				tokens[0]->val = res;
				tokens[0]->type = TOKEN_TYPE_NUM;
				stack_push(&out_stack, tokens[0]);

				while (tmp_stack.size != 0) {
					token = stack_pop(&tmp_stack);
					stack_push(&out_stack, token);
				}
				free(tokens[1]);
				free(tokens[2]);
			}
			else {
				free(tokens[0]);
				free(tokens[1]);
				free(tokens[2]);
				break;
			}
		}
		else{
			stack_push(&tmp_stack, tokens[0]);
			stack_push(&out_stack, tokens[2]);
			stack_push(&out_stack, tokens[1]);
		}

	}
	if (ret == 0) {
		token = stack_pop(&out_stack);
		*result = token->val;
		free(token);
	}

eval_exit:

	return ret;

}

static void print_error(int err)
{
	switch(err) {

		case ERR_SYNTAX:
			fprintf(stderr, "Syntax Error\n");
			break;
		case ERR_MATH:
			fprintf(stderr, "Math Error\n");
			break;
		case ERR_MALLOC:
			fprintf(stderr, "malloc() failed");
			break;
		default:
			fprintf(stderr, "Unknown Error");
			break;
	}
}

int main(void)
{
	char *expr = NULL;
	int ret = 0;
	size_t len = 0;
	long double result = 0;

	while (true) {

		printf(">>> ");
		ret = getline(&expr, &len, stdin);
		if (ret < 0) {
			fprintf(stderr, "Error reading the expression");
			free(expr);
			exit(EXIT_FAILURE);
		}
		/* get rid of newline character*/
		expr[ret - 1] = 0;

		if (strlen(expr) > 0) {

			if (strcmp(expr, "exit") == 0) break;

			ret = parse_expression(expr);
			if (ret == ERR_SUCC) {

				ret = evaluate_expression(&result);
				if (ret == ERR_SUCC) {
					fprintf(stdout, "= %Lf\n", result);
				}
				else {
					print_error(ret);
				}
			}
			else {
				print_error(ret);
			}

			free_stack(&out_stack);
			free_stack(&op_stack);
			free_stack(&tmp_stack);
		}
	}
	free(expr);
}

