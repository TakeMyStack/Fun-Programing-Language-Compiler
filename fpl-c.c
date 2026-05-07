#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//enumeration des types de jetons
typedef enum {TOK_NUMBER, TOK_IDENTIFIER, TOK_ASSIGN, TOK_ADD, TOK_SUB, TOK_DIV, TOK_MUL, TOK_BEG_PAR, TOK_END_PAR, TOK_ARROW, TOK_END_DECLARE, TOK_END_FUNCTION, TOK_END_PROGRAM} TokenType;

//création de la structure pour les tokens -> {type, valeur}
typedef struct {
	TokenType type;
	char value[32];
} Tokens;

// Définition de la structure d'un arbre de base
typedef struct Ast {
	TokenType type;
	struct Ast *right;
	struct Ast *left;
	char value[32];
} Ast;

// C'est ici que l'on définit une deuxieme structure d'arbre pour l'assembleur
typedef enum {ADD, SUB, CDQ, IDIV, DIV, IMUL, MUL, LEAVE, MOV, PUSH, RET} Operation;
typedef enum {NO_REG, RBP_VAR, RBP, RSP, RAX, RDI, RSI, RCX, RDX} Register;

typedef struct {
	Operation op;
	Register reg;
	char value[32];
} Ast_Asm;

//prototype->déclarer proprement les fonctions et les initialiser
Tokens *lexer(char *text);
Ast *parser(Tokens *token_mem);
Ast *parse_function(Tokens *token_mem, Ast *Ast_token, int idx);
Ast *parse_operation(Tokens *token_mem, Ast *Ast_token, int idx);
Ast_Asm *code_gen_fin(Tokens *token_mem, Ast *Ast_token, int idx);
Ast_Asm *code_gen_func(Tokens *token_mem, Ast *Ast_token, int idx);
Ast_Asm *code_gen_op(Tokens *token_mem, Ast *Ast_token, int idx);
Ast_Asm *code_gen_end_func(Tokens *token_mem, Ast *Ast_token, int idx);
void code_emission(Ast_Asm *Asm_tok);

// variable globale pour code_emission
static int idx_code_emission = 0;
// flag : 1 = on est dans une fonction, 0 = on n'en est pas
static int in_function = 0;

// phase d'identification et de classification des jetons
Tokens *lexer(char *text) {
	size_t len = strlen(text);
	int idx = 0;
	Tokens *token_mem = malloc(sizeof(Tokens) * len);

	while (*text != '\0') {
		if (strncmp(text, "->", 2) == 0) {
			token_mem[idx].type = TOK_ARROW;
			strcpy(token_mem[idx].value, "->");
			text += 2;
			idx++;
		} else if (isalpha(*text)) {
			int len = 0;
			token_mem[idx].type = TOK_IDENTIFIER;
			while (isalpha(*text) && *text != ' ' && *text != '\n' && *text != '\t') {
				token_mem[idx].value[len++] = *text;
				text++;
			}
			token_mem[idx].value[len] = '\0';
			idx++;
		} else if (isdigit(*text)) {
			int len = 0;
			token_mem[idx].type = TOK_NUMBER;
			while (isdigit(*text) && *text != ' ' && *text != '\n' && *text != '\t') {
				token_mem[idx].value[len++] = *text;
				text++;
			}
			token_mem[idx].value[len] = '\0';
			idx++;
		} else switch (*text) {
			case ':': token_mem[idx].type = TOK_ASSIGN;      strcpy(token_mem[idx].value, ":"); text++; idx++; break;
			case '+': token_mem[idx].type = TOK_ADD;         strcpy(token_mem[idx].value, "+"); text++; idx++; break;
			case '-': token_mem[idx].type = TOK_SUB;         strcpy(token_mem[idx].value, "-"); text++; idx++; break;
			case '/': token_mem[idx].type = TOK_DIV;         strcpy(token_mem[idx].value, "/"); text++; idx++; break;
			case '*': token_mem[idx].type = TOK_MUL;         strcpy(token_mem[idx].value, "*"); text++; idx++; break;
			case '(': token_mem[idx].type = TOK_BEG_PAR;     strcpy(token_mem[idx].value, "("); text++; idx++; break;
			case ')': token_mem[idx].type = TOK_END_PAR;     strcpy(token_mem[idx].value, ")"); text++; idx++; break;
			case '.': token_mem[idx].type = TOK_END_DECLARE; strcpy(token_mem[idx].value, "."); text++; idx++; break;
			case '$': token_mem[idx].type = TOK_END_FUNCTION; strcpy(token_mem[idx].value, "$"); text++; idx++; break;
			default: text++; break;
		}
	}
	// On ajoute un token de fin afin d'identifier la fin de la chaine
	token_mem[idx].type = TOK_END_PROGRAM;
	strcpy(token_mem[idx].value, "");
	printf("\nok pour le lexer\n");
	return token_mem;
}

// Fonction de parsing générale
Ast *parser(Tokens *token_mem) {
	Ast *Ast_token = malloc(sizeof(Ast));
	int idx = 0;

	// initialisation du fichier asm avec l'en-tête NASM
	FILE *f = fopen("out.asm", "w");
	fprintf(f, "section .note.GNU-stack noalloc noexec nowrite progbits\n");
	fprintf(f, "section .text\n\n");
	fclose(f);

	parse_function(token_mem, Ast_token, idx);
	return Ast_token;
}

Ast *parse_function(Tokens *token_mem, Ast *Ast_token, int idx) {
	/*Regle gramaticale d'une fonction: chaine de caractère (sans mots-clès) -> parenthèse ouverte -> parenthèse fermée -> fléche
	* par exemple: test()->
	* si on a rien (valeur vide '\0') alors on passe à la fonction code_gen_fin
	*/
	if (token_mem[idx].type == TOK_END_PROGRAM) {
		// vérifie qu'on a bien fermé la dernière fonction avec $
		if (in_function) {
			printf("vous n'avez pas déclarer '$' pour marquer la fin de la dernière fonction\n");
			exit(1);
		}
		Ast_token->type = token_mem[idx].type;
		strcpy(Ast_token->value, token_mem[idx].value);
		Ast_token->left = NULL;
		Ast_token->right = NULL;
		code_gen_fin(token_mem, Ast_token, idx);

	} else if (token_mem[idx].type == TOK_END_FUNCTION) {
		Ast_token->type = token_mem[idx].type;
		strcpy(Ast_token->value, token_mem[idx].value);
		Ast_token->left = NULL;
		Ast_token->right = NULL;
		idx++;
		code_gen_end_func(token_mem, Ast_token, idx);

	} else if (token_mem[idx].type == TOK_IDENTIFIER) {
		Ast_token->type = token_mem[idx].type;
		strcpy(Ast_token->value, token_mem[idx].value);
		Ast_token->left = NULL;
		Ast_token->right = NULL;
		idx++;

		if (token_mem[idx].type == TOK_BEG_PAR) {
			Ast_token->left = malloc(sizeof(Ast));
			Ast_token->left->type = token_mem[idx].type;
			strcpy(Ast_token->left->value, token_mem[idx].value);
			Ast_token->left->left = NULL;
			Ast_token->left->right = NULL;
			idx++;

			if (token_mem[idx].type == TOK_END_PAR) {
				Ast_token->right = malloc(sizeof(Ast));
				Ast_token->right->type = token_mem[idx].type;
				strcpy(Ast_token->right->value, token_mem[idx].value);
				Ast_token->right->left = NULL;
				Ast_token->right->right = NULL;
				idx++;

				if (token_mem[idx].type == TOK_ARROW) {
					// avant de déclarer une nouvelle fonction, il faut avoir fermé l'ancienne avec $
					if (in_function) {
						printf("vous n'avez pas déclarer '$' pour marquer la fin de la précédente fonction\n");
						exit(1);
					}
					Ast_token->right->right = malloc(sizeof(Ast));
					Ast_token->right->right->type = token_mem[idx].type;
					strcpy(Ast_token->right->right->value, token_mem[idx].value);
					Ast_token->right->right->left = NULL;
					Ast_token->right->right->right = NULL;
					idx++;
					// code_gen_func
					code_gen_func(token_mem, Ast_token, idx);

				} else {
					printf("pas de '->' pour terminer la fonction\n");
					exit(1);
				}
			} else {
				printf("pas de ')' pour fermer la parenthèse\n");
				exit(1);
			}
		} else {
			--idx;
			parse_operation(token_mem, Ast_token, idx);
		}
	}
	return Ast_token;
}

Ast_Asm *code_gen_fin(Tokens *token_mem, Ast *Ast_token, int idx) {
	Ast_Asm *Asm_tok = calloc(128, sizeof(Ast_Asm));
	// code_gen_fin ne fait rien - c'est code_gen_end_func qui ferme les fonctions avec $
	return Asm_tok;
}

// code_gen_func
Ast_Asm *code_gen_func(Tokens *token_mem, Ast *Ast_token, int idx) {
	Ast_Asm *Asm_tok = calloc(128, sizeof(Ast_Asm));

	// marque qu'on est dans une fonction
	in_function = 1;

	// écriture du label NASM : global + nom:
	FILE *f = fopen("out.asm", "a");
	fwrite("global ", 1, 7, f);
	fwrite(Ast_token->value, 1, strlen(Ast_token->value), f);
	fwrite("\n", 1, 1, f);
	fwrite(Ast_token->value, 1, strlen(Ast_token->value), f);
	fwrite(":\n", 1, 2, f);
	fclose(f);

	Asm_tok[0].op = PUSH;
	Asm_tok[0].reg = RBP;
	strcpy(Asm_tok[0].value, "");
	code_emission(Asm_tok);

	Asm_tok[0].op = MOV;
	Asm_tok[0].reg = RBP;
	Asm_tok[1].reg = RSP;
	strcpy(Asm_tok[0].value, "");
	code_emission(Asm_tok);

	// Recursivité du parser
	parse_function(token_mem, Ast_token, idx);
	return Asm_tok;
}

// code_gen_end_func
Ast_Asm *code_gen_end_func(Tokens *token_mem, Ast *Ast_token, int idx) {
	// vérifie qu'on ferme une fonction en cours
	if (!in_function) {
		printf("'$' trouvé hors d'une fonction\n");
		exit(1);
	}
	
	Ast_Asm *Asm_tok = calloc(128, sizeof(Ast_Asm));

	// ferme la fonction avec leave et ret
	Asm_tok[0].op = LEAVE;
	Asm_tok[0].reg = NO_REG;
	strcpy(Asm_tok[0].value, "");
	code_emission(Asm_tok);

	Asm_tok[0].op = RET;
	Asm_tok[0].reg = NO_REG;
	strcpy(Asm_tok[0].value, "");
	code_emission(Asm_tok);

	// reset : on a fermé la fonction
	in_function = 0;
	idx_code_emission = 0;

	// ligne vide de séparation entre les fonctions
	FILE *f = fopen("out.asm", "a");
	fprintf(f, "\n");
	fclose(f);

	// Recursivité du parser pour la fonction suivante
	parse_function(token_mem, Ast_token, idx);
	return Asm_tok;
}

Ast *parse_operation(Tokens *token_mem, Ast *Ast_token, int idx) {
	/* 2 Regles gramaticale pour une operation :
	* soit chaine de caractère (sans mots-clès) -> deux points assignateur -> valeur / nombre -> point
	* soit chaine de caractère (sasn mots-clès) -> deux points assignateur -> valeur / nombre -> operateur -> valeur / nombre -> point
	*
	* par exemple:
	* a: 1.
	* a: 1+1.
	* b: 2-1.
	* nombre_negatif: -2.
	* division: 4/2.
	*
	* Attention à la division, il n'est pas possible de diviser par 0
	*/

	if (token_mem[idx].type == TOK_END_FUNCTION) {
		Ast_token->type = token_mem[idx].type;
		strcpy(Ast_token->value, token_mem[idx].value);
		Ast_token->left = NULL;
		Ast_token->right = NULL;
		idx++;
		code_gen_end_func(token_mem, Ast_token, idx);

	} else if (token_mem[idx].type == TOK_IDENTIFIER) {
		Ast_token->type = token_mem[idx].type;
		strcpy(Ast_token->value, token_mem[idx].value);
		Ast_token->right = NULL;
		Ast_token->left = NULL;
		idx++;

		if (token_mem[idx].type == TOK_ASSIGN) {
			Ast_token->left = malloc(sizeof(Ast));
			Ast_token->left->type = token_mem[idx].type;
			strcpy(Ast_token->left->value, token_mem[idx].value);
			Ast_token->left->left = NULL;
			Ast_token->left->right = NULL;
			idx++;

			if (token_mem[idx].type == TOK_NUMBER) {
				Ast_token->right = malloc(sizeof(Ast));
				Ast_token->right->type = token_mem[idx].type;
				strcpy(Ast_token->right->value, token_mem[idx].value);
				Ast_token->right->left = NULL;
				Ast_token->right->right = NULL;
				idx++;

				if (token_mem[idx].type == TOK_END_DECLARE) {
					Ast_token->left->left = malloc(sizeof(Ast));
					Ast_token->left->left->type = token_mem[idx].type;
					strcpy(Ast_token->left->left->value, token_mem[idx].value);
					Ast_token->left->left->left = NULL;
					Ast_token->left->left->right = NULL;
					idx++;
					// code_gen_op
					code_gen_op(token_mem, Ast_token, idx);

				// operateur d'addition
				} else if (token_mem[idx].type == TOK_ADD) {
					Ast_token->left->left = malloc(sizeof(Ast));
					Ast_token->left->left->type = token_mem[idx].type;
					strcpy(Ast_token->left->left->value, token_mem[idx].value);
					Ast_token->left->left->left = NULL;
					Ast_token->left->left->right = NULL;
					idx++;

					if (token_mem[idx].type == TOK_NUMBER) {
						Ast_token->right->right = malloc(sizeof(Ast));
						Ast_token->right->right->type = token_mem[idx].type;
						strcpy(Ast_token->right->right->value, token_mem[idx].value);
						Ast_token->right->right->left = NULL;
						Ast_token->right->right->right = NULL;
						idx++;

						// fin declaration
						if (token_mem[idx].type == TOK_END_DECLARE) {
							Ast_token->right->right->left = malloc(sizeof(Ast));
							Ast_token->right->right->left->type = token_mem[idx].type;
							strcpy(Ast_token->right->right->left->value, token_mem[idx].value);
							Ast_token->right->right->left->left = NULL;
							Ast_token->right->right->left->right = NULL;
							idx++;
							// code_gen_op
							code_gen_op(token_mem, Ast_token, idx);

						} else {
							printf("pas de '.' pour terminer l'operation\n");
						}
					} else {
						printf("pas de nombre pour continuer l'operation\n");
						exit(1);
					}

				// operateur de soustraction
				} else if (token_mem[idx].type == TOK_SUB) {
					Ast_token->left->left = malloc(sizeof(Ast));
					Ast_token->left->left->type = token_mem[idx].type;
					strcpy(Ast_token->left->left->value, token_mem[idx].value);
					Ast_token->left->left->left = NULL;
					Ast_token->left->left->right = NULL;
					idx++;

					if (token_mem[idx].type == TOK_NUMBER) {
						Ast_token->right->right = malloc(sizeof(Ast));
						Ast_token->right->right->type = token_mem[idx].type;
						strcpy(Ast_token->right->right->value, token_mem[idx].value);
						Ast_token->right->right->left = NULL;
						Ast_token->right->right->right = NULL;
						idx++;

						// fin declaration
						if (token_mem[idx].type == TOK_END_DECLARE) {
							Ast_token->right->right->left = malloc(sizeof(Ast));
							Ast_token->right->right->left->type = token_mem[idx].type;
							strcpy(Ast_token->right->right->left->value, token_mem[idx].value);
							Ast_token->right->right->left->left = NULL;
							Ast_token->right->right->left->right = NULL;
							idx++;
							// code_gen_op
							code_gen_op(token_mem, Ast_token, idx);

						} else {
							printf("pas de '.' pour terminer l'operation\n");
						}
					} else {
						printf("pas de nombre pour continuer l'operation\n");
						exit(1);
					}

				// operateur de multiplication
				} else if (token_mem[idx].type == TOK_MUL) {
					Ast_token->left->left = malloc(sizeof(Ast));
					Ast_token->left->left->type = token_mem[idx].type;
					strcpy(Ast_token->left->left->value, token_mem[idx].value);
					Ast_token->left->left->left = NULL;
					Ast_token->left->left->right = NULL;
					idx++;

					if (token_mem[idx].type == TOK_NUMBER) {
						Ast_token->right->right = malloc(sizeof(Ast));
						Ast_token->right->right->type = token_mem[idx].type;
						strcpy(Ast_token->right->right->value, token_mem[idx].value);
						Ast_token->right->right->left = NULL;
						Ast_token->right->right->right = NULL;
						idx++;

						// fin declaration
						if (token_mem[idx].type == TOK_END_DECLARE) {
							Ast_token->right->right->left = malloc(sizeof(Ast));
							Ast_token->right->right->left->type = token_mem[idx].type;
							strcpy(Ast_token->right->right->left->value, token_mem[idx].value);
							Ast_token->right->right->left->left = NULL;
							Ast_token->right->right->left->right = NULL;
							idx++;
							// code_gen_op
							code_gen_op(token_mem, Ast_token, idx);

						} else {
							printf("pas de '.' pour terminer l'operation\n");
						}
					} else {
						printf("pas de nombre pour continuer l'operation\n");
						exit(1);
					}

				// operateur de division
				} else if (token_mem[idx].type == TOK_DIV) {
					Ast_token->left->left = malloc(sizeof(Ast));
					Ast_token->left->left->type = token_mem[idx].type;
					strcpy(Ast_token->left->left->value, token_mem[idx].value);
					Ast_token->left->left->left = NULL;
					Ast_token->left->left->right = NULL;
					idx++;

					if (token_mem[idx].type == TOK_NUMBER) {
						if (strcmp(token_mem[idx].value, "0") == 0) {
							printf("on ne peut pas diviser par 0 ou par un nombre null\n");
							exit(1);
						}
						Ast_token->right->right = malloc(sizeof(Ast));
						Ast_token->right->right->type = token_mem[idx].type;
						strcpy(Ast_token->right->right->value, token_mem[idx].value);
						Ast_token->right->right->left = NULL;
						Ast_token->right->right->right = NULL;
						idx++;

						// fin declaration
						if (token_mem[idx].type == TOK_END_DECLARE) {
							Ast_token->right->right->left = malloc(sizeof(Ast));
							Ast_token->right->right->left->type = token_mem[idx].type;
							strcpy(Ast_token->right->right->left->value, token_mem[idx].value);
							Ast_token->right->right->left->left = NULL;
							Ast_token->right->right->left->right = NULL;
							idx++;
							// code_gen_op
							code_gen_op(token_mem, Ast_token, idx);

						} else {
							printf("pas de '.' pour terminer l'operation\n");
						}
					} else {
						printf("pas de nombre pour continuer l'operation\n");
						exit(1);
					}
				} else {
					printf("pas d'operateur +,-,/,* ou de '.' pour continuer/terminer l'operation\n");
					exit(1);
				}
			} else {
				printf("pas de nombre ou de valeur à assigner\n");
				exit(1);
			}
		} else {
			printf("pas de ':' pour assigner une valeur ou une operation");
			exit(1);
		}
	}
	return Ast_token;
}

// code_gen_op
Ast_Asm *code_gen_op(Tokens *token_mem, Ast *Ast_token, int idx) {
	Ast_Asm *Asm_tok = calloc(128, sizeof(Ast_Asm));

	// declaration de la variable
	if (Ast_token->type == TOK_IDENTIFIER && Ast_token->left->type == TOK_ASSIGN) {
		Asm_tok->op = MOV;
		Asm_tok->reg = RBP_VAR;
		strcpy(Asm_tok->value, Ast_token->right->value);

		// code_emission
		code_emission(Asm_tok);

		// addition
		if (Ast_token->left->left->type == TOK_ADD) {
			Asm_tok->op = ADD;
			Asm_tok->reg = RBP_VAR;
			strcpy(Asm_tok->value, Ast_token->right->right->value);

			// code_emission
			code_emission(Asm_tok);

			// Recursivité du parser
			parse_function(token_mem, Ast_token, idx);
		}

		// substraction
		else if (Ast_token->left->left->type == TOK_SUB) {
			Asm_tok->op = SUB;
			Asm_tok->reg = RBP_VAR;
			strcpy(Asm_tok->value, Ast_token->right->right->value);

			// code_emission
			code_emission(Asm_tok);

			// Recursivité du parser
			parse_function(token_mem, Ast_token, idx);
		}

		// multiplication
		else if (Ast_token->left->left->type == TOK_MUL) {
			Asm_tok->op = IMUL;
			Asm_tok[0].reg = RBP_VAR;
			Asm_tok[1].reg = RBP_VAR;
			strcpy(Asm_tok->value, Ast_token->right->right->value);

			// code_emission
			code_emission(Asm_tok);

			// Recursivité du parser
			parse_function(token_mem, Ast_token, idx);
		}

		// division
		else if (Ast_token->left->left->type == TOK_DIV) {
			Asm_tok->op = MOV;
			Asm_tok[0].reg = RAX;
			Asm_tok[1].reg = RBP_VAR;
			strcpy(Asm_tok->value, "");
			// code_emission
			code_emission(Asm_tok);

			Asm_tok->op = CDQ;
			Asm_tok->reg = NO_REG;
			strcpy(Asm_tok->value, "");
			// code_emission
			code_emission(Asm_tok);

			Asm_tok->op = MOV;
			Asm_tok->reg = RCX;
			strcpy(Asm_tok->value, Ast_token->right->right->value);
			// code_emission
			code_emission(Asm_tok);

			Asm_tok->op = IDIV;
			Asm_tok->reg = RCX;
			strcpy(Asm_tok->value, "");
			// code_emission
			code_emission(Asm_tok);

			// Recursivité du parser
			parse_function(token_mem, Ast_token, idx);
		} else {
			// Recursivité du parser
			parse_function(token_mem, Ast_token, idx);
		}
	}
	return Asm_tok;
}

void code_emission(Ast_Asm *Asm_tok) {
	int i;
	FILE *file = fopen("out.asm", "a");

	// indentation
	fwrite("    ", 1, 4, file);

	switch(Asm_tok->op) {
		case MOV:   fwrite("mov ",  1, 4, file); break;
		case ADD:   fwrite("add ",  1, 4, file); break;
		case SUB:   fwrite("sub ",  1, 4, file); break;
		case CDQ:   fwrite("cdq",   1, 3, file); break;
		case IDIV:  fwrite("idiv ", 1, 5, file); break;
		case DIV:   fwrite("div ",  1, 4, file); break;
		case IMUL:  fwrite("imul ", 1, 5, file); break;
		case MUL:   fwrite("mul ",  1, 4, file); break;
		case LEAVE: fwrite("leave", 1, 5, file); break;
		case RET:   fwrite("ret",   1, 3, file); break;
		case PUSH:  fwrite("push ", 1, 5, file); break;
		default: break;
	}

	for (i = 0; i < 2; i++) {
		if (Asm_tok[i].reg == NO_REG) continue;

		if (i == 1 && Asm_tok[0].reg != NO_REG) fwrite(", ", 1, 2, file);

		if (Asm_tok[i].reg == RBP_VAR) {
			// on n'alloue un nouveau slot que lors d'une déclaration (MOV)
			if (Asm_tok->op == MOV && i == 0) idx_code_emission += 4;
			char charValue[128];

			// syntaxe NASM : dword [rbp-X]
			fwrite("dword [rbp-", 1, 11, file);
			sprintf(charValue, "%d", idx_code_emission);
			fwrite(charValue, 1, strlen(charValue), file);
			fwrite("]", 1, 1, file);

		} else switch (Asm_tok[i].reg) {
			case RBP: fwrite("rbp", 1, 3, file); break;
			case RSP: fwrite("rsp", 1, 3, file); break;
			case RAX: fwrite("rax", 1, 3, file); break;
			case RDI: fwrite("rdi", 1, 3, file); break;
			case RSI: fwrite("rsi", 1, 3, file); break;
			case RCX: fwrite("rcx", 1, 3, file); break;
			case RDX: fwrite("rdx", 1, 3, file); break;
			default: break;
		}
	}

	if (Asm_tok->value[0] != '\0') {
		int has_reg = (Asm_tok[0].reg != NO_REG || Asm_tok[1].reg != NO_REG);
		fwrite(has_reg ? ", " : "", 1, has_reg ? 2 : 0, file);
		fwrite(Asm_tok->value, 1, strlen(Asm_tok->value), file);
	}

	fwrite("\n", 1, 1, file);
	fclose(file);
}

int main() {
	char *text = "main()->a:1+1.$ test()->a:2-1.$";
	Tokens *token_mem = lexer(text);
	parser(token_mem);

	free(token_mem);
	return 0;
}
