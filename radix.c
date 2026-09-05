#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

static int g_argc = 0;
static char** g_argv = NULL;

static void die(const char* msg) {
	fprintf(stderr, "runtime error: %s\n", msg);
	exit(EXIT_FAILURE);
}

static void dief(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "runtime error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAILURE);
}

typedef enum {
	T_ID,
	T_NUM,
	T_BOOL,
	T_STR,
	T_LET,
	T_CONST,
	T_IF,
	T_ELSE,
	T_ELIF,
	T_WHILE,
	T_FUNC,
	T_ARROW,
	T_OUTN,
	T_SEMI,
	T_LBRACE,
	T_RBRACE,
	T_LP,
	T_RP,
	T_PLUS,
	T_MINUS,
	T_STAR,
	T_SLASH,
	T_PERCENT,
	T_GT,
	T_GEQ,
	T_EQEQ,
	T_LEQ,
	T_LT,
	T_NEQ,
	T_ANDAND,
	T_OROR,
	T_COMMA,
	T_BANG,
	T_EQ,
	T_EOF
} Tok;

typedef struct {
	Tok t;
	int ival;
	char* sval;
} Token;

typedef struct {
	Token* data;
	size_t n, cap;
} TokVec;

static void tv_init(TokVec* v) {
	v->data=NULL;
	v->n=0;
	v->cap=0;
}

static void tv_push(TokVec* v, Token tk) {
	if(v->n==v->cap) {
		v->cap = v->cap? v->cap*2 : 64;
		v->data = (Token*)realloc(v->data, v->cap*sizeof(Token));
	}
	v->data[v->n++] = tk;
}

static void tv_free(TokVec* v) {
	for(size_t i=0; i<v->n; i++) {
		if(v->data[i].t==T_ID || v->data[i].t==T_BOOL || v->data[i].t==T_STR) {
			free(v->data[i].sval);
		}
	}
	free(v->data);
}

static bool isalpha_(char c) {
	return (c=='_') || isalpha((unsigned char)c);
}

static bool isalnum_(char c) {
	return (c=='_') || isalnum((unsigned char)c);
}

static void tokenize(const char* src, TokVec* out) {
	tv_init(out);
	size_t i=0, n=strlen(src);
	while(i<n) {
		char c=src[i];
		if(isspace((unsigned char)c)) {
			i++;
			continue;
		}
		if(c=='"') {
			i++;
			size_t cap=64, len=0;
			char* s=(char*)malloc(cap);
			while(i<n && src[i]!='"') {
				char ch=src[i++];
				if(ch=='\\' && i<n) {
					char esc=src[i++];
					switch(esc) {
					case 'n': ch='\n'; break;
					case 't': ch='\t'; break;
					case 'r': ch='\r'; break;
					case '\\': ch='\\'; break;
					case '"': ch='"'; break;
					case '0': ch='\0'; break;
					default: ch=esc; break;
					}
				}
				if(len+1>=cap) {
					cap*=2;
					s=(char*)realloc(s, cap);
				}
				s[len++]=ch;
			}
			if(i>=n) die("unterminated string literal");
			i++;
			s[len]='\0';
			Token tk={0};
			tk.t=T_STR;
			tk.sval=s;
			tv_push(out, tk);
			continue;
		}
		if(isdigit((unsigned char)c)) {
			long v=0;
			while(i<n && isdigit((unsigned char)src[i])) {
				v = v*10 + (src[i]-'0');
				i++;
			}
			Token tk = { .t=T_NUM, .ival=(int)v };
			tv_push(out, tk);
			continue;
		}
		if(isalpha_(c)) {
			size_t s=i;
			i++;
			while(i<n && isalnum_(src[i])) i++;
			size_t len=i-s;
			char* id=(char*)malloc(len+1);
			memcpy(id, src+s, len);
			id[len]='\0';
			Tok t = T_ID;
			if(strcmp(id,"var")==0) {
				t=T_LET;
			} else if(strcmp(id,"const")==0) {
				t=T_CONST;
			} else if(strcmp(id,"if")==0) {
				t=T_IF;
			} else if(strcmp(id,"elif")==0) {
				t=T_ELIF;
			} else if(strcmp(id,"else")==0) {
				t=T_ELSE;
			} else if(strcmp(id,"while")==0) {
				t=T_WHILE;
			} else if(strcmp(id,"func")==0) {
				t=T_FUNC;
			} else if(strcmp(id,"true")==0 || strcmp(id,"false")==0) {
				t=T_BOOL;
			} else if(strcmp(id,"outn")==0) {
				t=T_OUTN;
			}
			Token tk= {0};
			tk.t=t;
			if(t==T_ID || t==T_BOOL) {
				tk.sval=id;
			} else {
				free(id);
			}
			tv_push(out, tk);
			continue;
		}
		if(c=='(') {
			tv_push(out, (Token){.t=T_LP});
			i++;
			continue;
		}
		if(c==')') {
			tv_push(out, (Token){.t=T_RP});
			i++;
			continue;
		}
		if(c=='{') {
			tv_push(out, (Token){.t=T_LBRACE});
			i++;
			continue;
		}
		if(c=='}') {
			tv_push(out, (Token){.t=T_RBRACE});
			i++;
			continue;
		}
		if(c==';') {
			tv_push(out, (Token){.t=T_SEMI});
			i++;
			continue;
		}
		if(c==',') {
			tv_push(out, (Token){.t=T_COMMA});
			i++;
			continue;
		}
		if(c=='+') {
			tv_push(out, (Token){.t=T_PLUS});
			i++;
			continue;
		}
		if(c=='-') {
			tv_push(out, (Token){.t=T_MINUS});
			i++;
			continue;
		}
		if(c=='*') {
			tv_push(out, (Token){.t=T_STAR});
			i++;
			continue;
		}
		if(c=='/') {
			if(i+1<n && src[i+1]=='/') {
				i+=2;
				while(i<n && src[i]!='\n') i++;
				continue;
			}
			tv_push(out, (Token){.t=T_SLASH});
			i++;
			continue;
		}
		if(c=='%') {
			tv_push(out, (Token){.t=T_PERCENT});
			i++;
			continue;
		}
		if(c=='!') {
			if(i+1<n && src[i+1]=='=') {
				tv_push(out, (Token){.t=T_NEQ});
				i+=2;
			} else {
				tv_push(out, (Token){.t=T_BANG});
				i++;
			}
			continue;
		}
		if(c=='=') {
			if(i+1<n && src[i+1]=='=') {
				tv_push(out, (Token){.t=T_EQEQ});
				i+=2;
			} else if(i+1<n && src[i+1]=='>') {
				tv_push(out, (Token){.t=T_ARROW});
				i+=2;
			} else {
				tv_push(out, (Token){.t=T_EQ});
				i++;
			}
			continue;
		}
		if(c=='<') {
			if(i+1<n && src[i+1]=='=') {
				tv_push(out, (Token){.t=T_LEQ});
				i+=2;
			} else {
				tv_push(out, (Token){.t=T_LT});
				i++;
			}
			continue;
		}
		if(c=='>') {
			if(i+1<n && src[i+1]=='=') {
				tv_push(out, (Token){.t=T_GEQ});
				i+=2;
			} else {
				tv_push(out, (Token){.t=T_GT});
				i++;
			}
			continue;
		}
		if(c=='&' && i+1<n && src[i+1]=='&') {
			tv_push(out, (Token){.t=T_ANDAND});
			i+=2;
			continue;
		}
		if(c=='|' && i+1<n && src[i+1]=='|') {
			tv_push(out, (Token){.t=T_OROR});
			i+=2;
			continue;
		}
		dief("unexpected character '%c' in input", c);
	}
	tv_push(out, (Token){.t=T_EOF});
}

typedef struct AST AST;

typedef enum {
	A_ID,
	A_NUM,
	A_BOOL,
	A_STR,
	A_LET,
	A_ASSIGN,
	A_BIN,
	A_UN,
	A_BLOCK,
	A_IFELSE,
	A_WHILE,
	A_FUNC_LIT,
	A_CALL,
	A_BUILTIN,
	A_SEQ
} ATag;

typedef enum { U_NEG, U_NOT } UOp;
typedef enum {
	B_ADD,
	B_SUB,
	B_MUL,
	B_DIV,
	B_MOD,
	B_LT,
	B_LE,
	B_GT,
	B_GE,
	B_EQ,
	B_NE,
	B_AND,
	B_OR
} BOp;

typedef struct {
	char* name;
	bool constant;
} IdNode;

typedef struct {
	AST* id;
	AST* expr;
	bool constant;
} LetNode;

typedef struct {
	AST* id;
	AST* expr;
} AssignNode;

typedef struct {
	BOp op;
	AST* left;
	AST* right;
} BinNode;

typedef struct {
	UOp op;
	AST* expr;
} UnNode;

typedef struct {
	AST* left;
	AST* right;
} SeqNode;

typedef struct {
	AST** conds;
	AST** bodies;
	size_t n;
	AST* elseBody;
} IfNode;

typedef struct {
	AST* cond;
	AST* body;
} WhileNode;

typedef struct {
	AST** params;
	size_t nparams;
	AST* body;
} FuncNode;

typedef enum { BUILTIN_OUTN } Builtin;

typedef struct {
	Builtin bi;
	AST** args;
	size_t nargs;
} BuiltinNode;

struct AST {
	ATag tag;
	union {
		IdNode id;
		int num;
		bool boolean;
		char* str;
		LetNode var_;
		AssignNode asn;
		BinNode bin;
		UnNode un;
		SeqNode seq;
		IfNode iff;
		WhileNode wh;
		FuncNode fn;
		struct {
			AST* callee;
			AST** args;
			size_t nargs;
		} call;
		BuiltinNode builtin;
		struct {
			AST* expr;
		} block;
	};
};

static AST* mk(AST a) {
	AST* p=(AST*)malloc(sizeof(AST));
	*p=a;
	return p;
}

static AST* mk_num(int v) {
	return mk((AST){.tag=A_NUM, .num=v});
}

static AST* mk_bool(bool b) {
	return mk((AST){.tag=A_BOOL, .boolean=b});
}

static AST* mk_str(const char* s) {
	AST* a=mk((AST){.tag=A_STR});
	a->str=strdup(s);
	return a;
}

static AST* mk_id(const char* s, bool c) {
	AST* a=mk((AST){.tag=A_ID});
	a->id.name=strdup(s);
	a->id.constant=c;
	return a;
}

typedef struct {
	TokVec* toks;
	size_t i;
} Parser;

static Token* P_peek(Parser* p) {
	return &p->toks->data[p->i];
}

static bool P_check(Parser* p, Tok t) {
	return P_peek(p)->t==t;
}

static bool P_is(Parser* p, Tok t) {
	if(P_check(p,t)) {
		p->i++;
		return true;
	}
	return false;
}

static Token* P_adv(Parser* p) {
	return &p->toks->data[p->i++];
}

static void P_consume(Parser* p, Tok t, const char* msg) {
	if(!P_check(p,t)) dief("parse error: %s", msg);
	p->i++;
}

static AST* parse_expr(Parser* p); static AST* parse_stmt(Parser* p); static AST* parse_block(Parser* p);

static int precedence(Tok t) {
	switch(t) {
	case T_OROR:
		return 1;
	case T_ANDAND:
		return 2;
	case T_EQEQ:
	case T_NEQ:
		return 3;
	case T_LT:
	case T_LEQ:
	case T_GT:
	case T_GEQ:
		return 4;
	case T_PLUS:
	case T_MINUS:
		return 5;
	case T_STAR:
	case T_SLASH:
	case T_PERCENT:
		return 6;
	default:
		return 0;
	}
}

static BOp tok_to_binop(Tok t) {
	switch(t) {
	case T_PLUS:
		return B_ADD;
	case T_MINUS:
		return B_SUB;
	case T_STAR:
		return B_MUL;
	case T_SLASH:
		return B_DIV;
	case T_PERCENT:
		return B_MOD;
	case T_LT:
		return B_LT;
	case T_LEQ:
		return B_LE;
	case T_GT:
		return B_GT;
	case T_GEQ:
		return B_GE;
	case T_EQEQ:
		return B_EQ;
	case T_NEQ:
		return B_NE;
	case T_ANDAND:
		return B_AND;
	case T_OROR:
		return B_OR;
	default:
		die("internal: unknown binop token");
	}
}

static AST* parse_primary(Parser* p) {
	AST* base=NULL;
	if(P_is(p,T_LP)) {
		AST* e=parse_expr(p);
		P_consume(p,T_RP,"expected ')'");
		base=e;
	} else if(P_is(p,T_FUNC)) {
		P_consume(p,T_LP,"expected '(' after func");
		AST** params=NULL;
		size_t np=0;
		if(!P_check(p,T_RP)) {
			do {
				if(!P_check(p,T_ID)) die("expected parameter identifier");
				Token* tk=P_adv(p);
				params=(AST**)realloc(params,(np+1)*sizeof(AST*));
				params[np++]=mk_id(tk->sval,false);
			} while(P_is(p,T_COMMA));
		}
		P_consume(p,T_RP,"expected ')'");
		P_consume(p,T_ARROW,"expected '=>'");
		AST* body=NULL;
		if(P_check(p,T_LBRACE)) body=parse_block(p);
		else body=parse_expr(p);
		AST a= {.tag=A_FUNC_LIT};
		a.fn.params=params;
		a.fn.nparams=np;
		a.fn.body=body;
		base=mk(a);
	} else if(P_check(p,T_NUM)) {
		int v=P_adv(p)->ival;
		return mk_num(v);
	} else if(P_check(p,T_BOOL)) {
		bool b=strcmp(P_adv(p)->sval,"true")==0;
		return mk_bool(b);
	} else if(P_check(p,T_STR)) {
		Token* tk=P_adv(p);
		return mk_str(tk->sval);
	} else if(P_check(p,T_ID)) {
		Token* id=P_adv(p);
		base=mk_id(id->sval,false);
	} else if(P_is(p,T_OUTN)) {
		if(P_is(p,T_LP)) {
			AST* arg=parse_expr(p);
			P_consume(p,T_RP,"expected ')'");
			AST a= {.tag=A_BUILTIN};
			a.builtin.bi=BUILTIN_OUTN;
			a.builtin.args=(AST**)malloc(sizeof(AST*));
			a.builtin.args[0]=arg;
			a.builtin.nargs=1;
			return mk(a);
		}
		base=mk_id("outn",false);
	} else {
		die("unexpected token in primary");
		return NULL;
	}
	while(P_is(p,T_LP)) {
		AST** args=NULL;
		size_t na=0;
		if(!P_check(p,T_RP)) {
			do {
				AST* arg=parse_expr(p);
				args=(AST**)realloc(args,(na+1)*sizeof(AST*));
				args[na++]=arg;
			} while(P_is(p,T_COMMA));
		}
		P_consume(p,T_RP,"expected ')'");
		AST a= {.tag=A_CALL};
		a.call.callee=base;
		a.call.args=args;
		a.call.nargs=na;
		base=mk(a);
	}
	return base;
}

static AST* parse_unary(Parser* p) {
	if(P_is(p,T_BANG)) {
		AST* e=parse_unary(p);
		return mk((AST) {
			.tag=A_UN, .un= {.op=U_NOT, .expr=e}
		});
	}
	if(P_is(p,T_MINUS)) {
		AST* e=parse_unary(p);
		return mk((AST) {
			.tag=A_UN, .un= {.op=U_NEG, .expr=e}
		});
	}
	return parse_primary(p);
}

static AST* parse_bin_rhs(Parser* p, int min_prec, AST* lhs) {
	for(;;) {
		Tok t=P_peek(p)->t;
		int prec=precedence(t);
		if(prec<min_prec) break;
		P_adv(p);
		AST* rhs=parse_unary(p);
		Tok t2=P_peek(p)->t;
		int prec2=precedence(t2);
		if(prec2>prec) {
			rhs = parse_bin_rhs(p, prec+1, rhs);
		}
		BOp op = tok_to_binop(t);
		lhs = mk((AST) {
			.tag=A_BIN, .bin= {.op=op, .left=lhs, .right=rhs}
		});
	}
	return lhs;
}

static AST* parse_expr(Parser* p) {
	AST* lhs=parse_unary(p);
	return parse_bin_rhs(p, 1, lhs);
}

static AST* parse_if(Parser* p) {
	AST** conds=NULL;
	AST** bodies=NULL;
	size_t n=0;
	P_consume(p,T_LP,"expected '(' after if");
	AST* c=parse_expr(p);
	P_consume(p,T_RP,"expected ')'");
	AST* b=parse_block(p);
	conds=(AST**)realloc(conds,(n+1)*sizeof(AST*));
	bodies=(AST**)realloc(bodies,(n+1)*sizeof(AST*));
	conds[n]=c;
	bodies[n]=b;
	n++;
	while(P_is(p,T_ELIF)) {
		P_consume(p,T_LP,"expected '(' after elif");
		AST* cc=parse_expr(p);
		P_consume(p,T_RP,"expected ')'");
		AST* bb=parse_block(p);
		conds=(AST**)realloc(conds,(n+1)*sizeof(AST*));
		bodies=(AST**)realloc(bodies,(n+1)*sizeof(AST*));
		conds[n]=cc;
		bodies[n]=bb;
		n++;
	}
	AST* ebody=NULL;
	if(P_is(p,T_ELSE)) {
		ebody=parse_block(p);
	}
	AST a= {.tag=A_IFELSE};
	a.iff.conds=conds;
	a.iff.bodies=bodies;
	a.iff.n=n;
	a.iff.elseBody=ebody;
	return mk(a);
}

static AST* parse_while(Parser* p) {
	P_consume(p,T_LP,"expected '(' after while");
	AST* cond=parse_expr(p);
	P_consume(p,T_RP,"expected ')'");
	AST* body=parse_block(p);
	AST a= {.tag=A_WHILE};
	a.wh.cond=cond;
	a.wh.body=body;
	return mk(a);
}

static AST* parse_stmt(Parser* p) {
	if(P_is(p,T_LET) || P_is(p,T_CONST)) {
		bool isConst = p->toks->data[p->i-1].t==T_CONST;
		if(!P_check(p,T_ID)) die("expected identifier after var/const");
		Token* id=P_adv(p);
		P_consume(p,T_EQ,"expected '=' after identifier");
		AST* expr=parse_expr(p);
		P_consume(p,T_SEMI,"expected ';' after declaration");
		AST a= {.tag=A_LET};
		a.var_.id = mk_id(id->sval,isConst);
		a.var_.expr = expr;
		a.var_.constant = isConst;
		return mk(a);
	}
	if(P_check(p,T_ID) && p->toks->data[p->i+1].t==T_EQ) {
		Token* id=P_adv(p);
		P_adv(p);
		AST* expr=parse_expr(p);
		P_consume(p,T_SEMI,"expected ';' after assignment");
		AST a= {.tag=A_ASSIGN};
		a.asn.id = mk_id(id->sval,false);
		a.asn.expr = expr;
		return mk(a);
	}
	if(P_is(p,T_IF)) return parse_if(p);
	if(P_is(p,T_WHILE)) return parse_while(p);
	if(P_is(p,T_LBRACE)) {
		p->i--;
		return parse_block(p);
	}
	AST* e=parse_expr(p);
	P_consume(p,T_SEMI,"expected ';' after expression");
	return e;
}

static AST* chain_seq(AST* left, AST* right) {
	if(!left) return right;
	if(!right) return left;
	AST a= {.tag=A_SEQ};
	a.seq.left=left;
	a.seq.right=right;
	return mk(a);
}

static AST* parse_block(Parser* p) {
	P_consume(p,T_LBRACE,"expected '{'");
	AST* seq=NULL;
	while(!P_check(p,T_RBRACE) && !P_check(p,T_EOF)) {
		AST* s=parse_stmt(p);
		seq = chain_seq(seq, s);
	}
	P_consume(p,T_RBRACE,"expected '}'");
	AST a= {.tag=A_BLOCK};
	a.block.expr=seq;
	return mk(a);
}

static AST* parse_program(Parser* p) {
	if(P_check(p,T_LBRACE)) return parse_block(p);
	AST* seq=NULL;
	while(!P_check(p,T_EOF)) {
		AST* s=parse_stmt(p);
		seq = chain_seq(seq, s);
	}
	AST a= {.tag=A_BLOCK};
	a.block.expr=seq;
	return mk(a);
}

typedef enum {
	V_NULL,
	V_NUM,
	V_BOOL,
	V_FUNC,
	V_STR,
	V_BUILTIN
} VTag;

typedef enum {
	BI_OUT,
	BI_OUTN,
	BI_LEN,
	BI_SUBSTR,
	BI_CHARAT,
	BI_ORD,
	BI_CHR,
	BI_STR,
	BI_NUM,
	BI_FIND,
	BI_READFILE,
	BI_ARGV,
	BI_ARGC
} BuiltinType;

typedef struct Env Env;
typedef struct Closure Closure;

typedef struct {
	VTag tag;
	union {
		int i;
		bool b;
		Closure* fn;
		char* s;
		BuiltinType bi;
	} as;
} Val;

static Val VNull(void) {
	Val v;
	v.tag=V_NULL;
	return v;
}

static Val VNum(int x) {
	Val v;
	v.tag=V_NUM;
	v.as.i=x;
	return v;
}

static Val VBool(bool b) {
	Val v;
	v.tag=V_BOOL;
	v.as.b=b;
	return v;
}



static Val VStr(const char* s) {
	Val v;
	v.tag=V_STR;
	v.as.s=strdup(s);
	return v;
}

static Val VStr_no_copy(char* s) {
	Val v;
	v.tag=V_STR;
	v.as.s=s;
	return v;
}

static Val VBuiltin(BuiltinType b) {
	Val v;
	v.tag=V_BUILTIN;
	v.as.bi=b;
	return v;
}

typedef struct GCObj {
	struct GCObj* next;
	bool marked;
} GCObj;

typedef struct Entry {
	char* name;
	Val val;
	bool constant;
	struct Entry* next;
} Entry;

struct Env {
	Entry* head;
	Env* parent;
	GCObj gc;
};

struct Closure {
	AST* fun;
	Env* env;
	GCObj gc;
};

static Env* g_global_env = NULL;
static GCObj* gc_all_envs = NULL;
static GCObj* gc_all_closures = NULL;
static size_t gc_alloc_count = 0;
static const size_t GC_THRESHOLD = 50000;

#define MAX_GC_STACK 100000
static Env* g_call_stack[MAX_GC_STACK];
static size_t g_call_stack_top = 0;
static Val g_val_stack[MAX_GC_STACK];
static size_t g_val_stack_top = 0;

static void gc_push_env(Env* e) {
	if(g_call_stack_top < MAX_GC_STACK) g_call_stack[g_call_stack_top++] = e;
}
static void gc_pop_env(void) {
	if(g_call_stack_top > 0) g_call_stack_top--;
}
static void gc_push_val(Val v) {
	if(g_val_stack_top < MAX_GC_STACK) g_val_stack[g_val_stack_top++] = v;
}
static void gc_pop_val(void) {
	if(g_val_stack_top > 0) g_val_stack_top--;
}

static void gc_mark_env(Env* e);

static void gc_mark_closure(Closure* c) {
	if(!c || c->gc.marked) return;
	c->gc.marked = true;
	gc_mark_env(c->env);
}

static void gc_mark_val(Val v) {
	if(v.tag == V_FUNC && v.as.fn) {
		gc_mark_closure(v.as.fn);
	}
}

static void gc_mark_env(Env* e) {
	if(!e || e->gc.marked) return;
	e->gc.marked = true;
	gc_mark_env(e->parent);
	for(Entry* it = e->head; it; it = it->next) {
		gc_mark_val(it->val);
	}
}

static void gc_collect(void) {
	if(g_global_env) gc_mark_env(g_global_env);
	for(size_t i = 0; i < g_call_stack_top; i++) {
		gc_mark_env(g_call_stack[i]);
	}
	for(size_t i = 0; i < g_val_stack_top; i++) {
		gc_mark_val(g_val_stack[i]);
	}

	GCObj** curr_e = &gc_all_envs;
	while(*curr_e) {
		Env* e = (Env*)(*curr_e);
		if(!e->gc.marked) {
			*curr_e = e->gc.next;
			Entry* it = e->head;
			while(it) {
				Entry* next = it->next;
				free(it);
				it = next;
			}
			free(e);
		} else {
			e->gc.marked = false;
			curr_e = &e->gc.next;
		}
	}

	GCObj** curr_c = &gc_all_closures;
	while(*curr_c) {
		Closure* c = (Closure*)(*curr_c);
		if(!c->gc.marked) {
			*curr_c = c->gc.next;
			free(c);
		} else {
			c->gc.marked = false;
			curr_c = &c->gc.next;
		}
	}
}

static Val VFunc(AST* f, Env* e) {
	if(++gc_alloc_count >= GC_THRESHOLD) {
		gc_alloc_count = 0;
		gc_collect();
	}
	Closure* c = (Closure*)malloc(sizeof(Closure));
	c->fun = f;
	c->env = e;
	c->gc.marked = false;
	c->gc.next = gc_all_closures;
	gc_all_closures = (GCObj*)c;
	Val v;
	v.tag = V_FUNC;
	v.as.fn = c;
	return v;
}

static Env* env_new(Env* parent) {
	if(++gc_alloc_count >= GC_THRESHOLD) {
		gc_alloc_count = 0;
		gc_collect();
	}
	Env* e = (Env*)calloc(1, sizeof(Env));
	e->parent = parent;
	e->gc.marked = false;
	e->gc.next = gc_all_envs;
	gc_all_envs = (GCObj*)e;
	return e;
}

static Entry* env_find_here(Env* e, const char* name) {
	for(Entry* it = e->head; it; it = it->next) {
		if(it->name == name || strcmp(it->name, name) == 0) return it;
	}
	return NULL;
}

static Entry* env_find(Env* e, const char* name) {
	for(Env* cur = e; cur; cur = cur->parent) {
		Entry* en = env_find_here(cur, name);
		if(en) return en;
	}
	return NULL;
}

static void env_define(Env* e, const char* name, Val v, bool c) {
	Entry* en = env_find_here(e, name);
	if(en) {
		if(en->constant) dief("cannot reassign const %s", name);
		en->val = v;
		en->constant = c;
		return;
	}
	en = (Entry*)malloc(sizeof(Entry));
	en->name = (char*)name;
	en->val = v;
	en->constant = c;
	en->next = e->head;
	e->head = en;
}

static bool env_assign(Env* e, const char* name, Val v) {
	Entry* en=env_find(e,name);
	if(!en) return false;
	if(en->constant) dief("cannot assign to const %s", name);
	en->val=v;
	return true;
}

static void want_num(Val v, const char* op) {
	if(v.tag!=V_NUM) dief("operator '%s' expects number", op);
}

static void want_bool(Val v, const char* op) {
	if(v.tag!=V_BOOL) dief("operator '%s' expects boolean", op);
}

static void want_str(Val v, const char* op) {
	if(v.tag!=V_STR) dief("operator '%s' expects string", op);
}

static char* val_to_str(Val v) {
	char buf[64];
	switch(v.tag) {
	case V_NUM:
		snprintf(buf, sizeof(buf), "%d", v.as.i);
		return strdup(buf);
	case V_BOOL:
		return strdup(v.as.b ? "true" : "false");
	case V_STR:
		return strdup(v.as.s);
	case V_FUNC:
		return strdup("<function>");
	case V_BUILTIN:
		return strdup("<builtin>");
	default:
		return strdup("null");
	}
}

static char* fslurp(const char* path);

static Val call_builtin(BuiltinType bi, size_t nargs, Val* args) {
	switch(bi) {
	case BI_OUT: {
		if(nargs!=1) die("out expects 1 argument");
		char* s=val_to_str(args[0]);
		printf("%s", s);
		free(s);
		return VBool(true);
	}
	case BI_OUTN: {
		if(nargs!=1) die("outn expects 1 argument");
		char* s=val_to_str(args[0]);
		printf("%s\n", s);
		free(s);
		return VBool(true);
	}
	case BI_LEN: {
		if(nargs!=1) die("len expects 1 argument");
		if(args[0].tag!=V_STR) die("len expects string");
		return VNum((int)strlen(args[0].as.s));
	}
	case BI_CHARAT: {
		if(nargs!=2) die("char_at expects 2 arguments");
		if(args[0].tag!=V_STR) die("char_at expects string as first argument");
		if(args[1].tag!=V_NUM) die("char_at expects number as second argument");
		const char* s=args[0].as.s;
		int idx=args[1].as.i;
		int slen=(int)strlen(s);
		if(idx<0 || idx>=slen) return VStr("");
		char res[2]={s[idx], '\0'};
		return VStr(res);
	}
	case BI_ORD: {
		if(nargs<1 || nargs>2) die("ord expects 1 or 2 arguments");
		if(args[0].tag!=V_STR) die("ord expects string");
		int idx=(nargs==2)? args[1].as.i : 0;
		const char* s=args[0].as.s;
		int slen=(int)strlen(s);
		if(idx<0 || idx>=slen) return VNum(-1);
		return VNum((unsigned char)s[idx]);
	}
	case BI_CHR: {
		if(nargs!=1) die("chr expects 1 argument");
		if(args[0].tag!=V_NUM) die("chr expects number");
		char res[2]={(char)args[0].as.i, '\0'};
		return VStr(res);
	}
	case BI_SUBSTR: {
		if(nargs!=3) die("substr expects 3 arguments");
		if(args[0].tag!=V_STR) die("substr expects string as first argument");
		if(args[1].tag!=V_NUM || args[2].tag!=V_NUM) die("substr expects numbers for start and length");
		const char* s=args[0].as.s;
		int start=args[1].as.i;
		int len=args[2].as.i;
		int slen=(int)strlen(s);
		if(start<0 || start>=slen || len<=0) return VStr("");
		if(start+len>slen) len=slen-start;
		char* sub=(char*)malloc(len+1);
		memcpy(sub, s+start, len);
		sub[len]='\0';
		return VStr_no_copy(sub);
	}
	case BI_STR: {
		if(nargs!=1) die("str expects 1 argument");
		char* s=val_to_str(args[0]);
		return VStr_no_copy(s);
	}
	case BI_NUM: {
		if(nargs!=1) die("num expects 1 argument");
		if(args[0].tag==V_NUM) return args[0];
		if(args[0].tag!=V_STR) die("num expects string or number");
		return VNum(atoi(args[0].as.s));
	}
	case BI_FIND: {
		if(nargs!=2) die("find expects 2 arguments");
		if(args[0].tag!=V_STR || args[1].tag!=V_STR) die("find expects 2 strings");
		char* pos=strstr(args[0].as.s, args[1].as.s);
		if(!pos) return VNum(-1);
		return VNum((int)(pos - args[0].as.s));
	}
	case BI_READFILE: {
		if(nargs!=1) die("read_file expects 1 argument");
		if(args[0].tag!=V_STR) die("read_file expects string path");
		char* s=fslurp(args[0].as.s);
		if(!s) return VStr("");
		return VStr_no_copy(s);
	}
	case BI_ARGV: {
		if(nargs!=1) die("argv expects 1 argument");
		if(args[0].tag!=V_NUM) die("argv expects number");
		int idx=args[0].as.i;
		if(idx>=0 && idx<g_argc) {
			return VStr(g_argv[idx]);
		}
		return VStr("");
	}
	case BI_ARGC: {
		return VNum(g_argc);
	}
	}
	die("unknown builtin");
	return VNull();
}

static Val eval(AST* a, Env* env);

static Val eval_block(AST* a, Env* env) {
	if(!a || !a->block.expr) return VNull();
	AST* s=a->block.expr;
	Val last = VNull();
	switch(s->tag) {
		case A_SEQ:
		last = eval(s->seq.left, env);
		return eval(s->seq.right, env);
	default:
		return eval(s, env);
	}
}

static Val eval(AST* a, Env* env) {
	if(!a) return VNull();
	switch(a->tag) {
	case A_NUM:
		return VNum(a->num);
	case A_BOOL:
		return VBool(a->boolean);
	case A_STR:
		return VStr(a->str);
	case A_ID: {
		Entry* en=env_find(env, a->id.name);
		if(!en) dief("undefined variable %s", a->id.name);
		return en->val;
	}
	case A_LET: {
		Val v = eval(a->var_.expr, env);
		env_define(env, a->var_.id->id.name, v, a->var_.constant);
		return v;
	}
	case A_ASSIGN: {
		Val v = eval(a->asn.expr, env);
		if(!env_assign(env, a->asn.id->id.name, v))
			dief("assign to undefined variable %s", a->asn.id->id.name);
		return v;
	}
	case A_UN: {
		Val v=eval(a->un.expr, env);
		if(a->un.op==U_NEG) {
			want_num(v,"-");
			return VNum(-v.as.i);
		} else {
			want_bool(v,"!");
			return VBool(!v.as.b);
		}
	}
	case A_BIN: {
		Val L=eval(a->bin.left, env);
		if(a->bin.op==B_AND) {
			want_bool(L,"&&");
			if(!L.as.b) return VBool(false);
			Val R=eval(a->bin.right, env);
			want_bool(R,"&&");
			return VBool(L.as.b && R.as.b);
		}
		if(a->bin.op==B_OR) {
			want_bool(L,"||");
			if(L.as.b) return VBool(true);
			Val R=eval(a->bin.right, env);
			want_bool(R,"||");
			return VBool(L.as.b || R.as.b);
		}
		Val R=eval(a->bin.right, env);
		switch(a->bin.op) {
		case B_ADD: {
			if(L.tag==V_STR || R.tag==V_STR) {
				char* s1=val_to_str(L);
				char* s2=val_to_str(R);
				size_t l1=strlen(s1);
				size_t l2=strlen(s2);
				char* buf=(char*)malloc(l1+l2+1);
				memcpy(buf, s1, l1);
				memcpy(buf+l1, s2, l2);
				buf[l1+l2]='\0';
				free(s1);
				free(s2);
				return VStr_no_copy(buf);
			}
			want_num(L,"+");
			want_num(R,"+");
			return VNum(L.as.i + R.as.i);
		}
		case B_SUB:
			want_num(L,"-");
			want_num(R,"-");
			return VNum(L.as.i - R.as.i);
		case B_MUL:
			want_num(L,"*");
			want_num(R,"*");
			return VNum(L.as.i * R.as.i);
		case B_DIV:
			want_num(L,"/");
			want_num(R,"/");
			if(R.as.i==0) die("division by zero");
			return VNum(L.as.i / R.as.i);
		case B_MOD:
			want_num(L,"%");
			want_num(R,"%");
			if(R.as.i==0) die("modulus by zero");
			return VNum(L.as.i % R.as.i);
		case B_LT:
			if(L.tag==V_STR && R.tag==V_STR) return VBool(strcmp(L.as.s, R.as.s)<0);
			want_num(L,"<");
			want_num(R,"<");
			return VBool(L.as.i < R.as.i);
		case B_LE:
			if(L.tag==V_STR && R.tag==V_STR) return VBool(strcmp(L.as.s, R.as.s)<=0);
			want_num(L,"<=");
			want_num(R,"<=");
			return VBool(L.as.i <= R.as.i);
		case B_GT:
			if(L.tag==V_STR && R.tag==V_STR) return VBool(strcmp(L.as.s, R.as.s)>0);
			want_num(L,">");
			want_num(R,">");
			return VBool(L.as.i > R.as.i);
		case B_GE:
			if(L.tag==V_STR && R.tag==V_STR) return VBool(strcmp(L.as.s, R.as.s)>=0);
			want_num(L,">=");
			want_num(R,">=");
			return VBool(L.as.i >= R.as.i);
		case B_EQ:
			if(L.tag!=R.tag) return VBool(false);
			if(L.tag==V_NUM) return VBool(L.as.i==R.as.i);
			if(L.tag==V_BOOL) return VBool(L.as.b==R.as.b);
			if(L.tag==V_STR) return VBool(strcmp(L.as.s, R.as.s)==0);
			return VBool(false);
		case B_NE:
			if(L.tag!=R.tag) return VBool(true);
			if(L.tag==V_NUM) return VBool(L.as.i!=R.as.i);
			if(L.tag==V_BOOL) return VBool(L.as.b!=R.as.b);
			if(L.tag==V_STR) return VBool(strcmp(L.as.s, R.as.s)!=0);
			return VBool(true);
		default:
			die("internal bin op");
		}
	}
	case A_SEQ:
		(void)eval(a->seq.left, env);
		return eval(a->seq.right, env);
	case A_BLOCK:
		return eval_block(a, env);
	case A_IFELSE: {
		for(size_t i=0; i<a->iff.n; i++) {
			Val v=eval(a->iff.conds[i], env);
			want_bool(v,"if/elif");
			if(v.as.b) return eval(a->iff.bodies[i], env);
		}
		if(a->iff.elseBody) return eval(a->iff.elseBody, env);
		return VNull();
	}
	case A_WHILE: {
		Val last=VNull();
		for(;;) {
			Val c=eval(a->wh.cond, env);
			want_bool(c,"while");
			if(!c.as.b) break;
			last=eval(a->wh.body, env);
		}
		return last;
	}
	case A_FUNC_LIT:
		return VFunc((AST*)a, env);
	case A_CALL: {
		Val cal = eval(a->call.callee, env);
		if(cal.tag==V_BUILTIN) {
			Val* evaluated=a->call.nargs ? (Val*)malloc(a->call.nargs*sizeof(Val)) : NULL;
			for(size_t i=0; i<a->call.nargs; i++) {
				evaluated[i]=eval(a->call.args[i], env);
			}
			Val res=call_builtin(cal.as.bi, a->call.nargs, evaluated);
			if(evaluated) free(evaluated);
			return res;
		}
		if(cal.tag!=V_FUNC) die("attempt to call non-function");
		Closure* cl=cal.as.fn;
		AST* fn=cl->fun;
		size_t nparams=fn->fn.nparams;
		if(a->call.nargs!=nparams) dief("arity mismatch: expected %zu args, got %zu", nparams, a->call.nargs);
		Env* callenv = env_new(cl->env);
		gc_push_env(callenv);
		for(size_t i=0; i<nparams; i++) {
			AST* pid = fn->fn.params[i];
			Val arg = eval(a->call.args[i], env);
			env_define(callenv, pid->id.name, arg, false);
		}
		Val res = eval(fn->fn.body, callenv);
		gc_push_val(res);
		gc_pop_env();
		gc_pop_val();
		return res;
	}
	case A_BUILTIN: {
		switch(a->builtin.bi) {
		case BUILTIN_OUTN: {
			if(a->builtin.nargs!=1) die("outn expects 1 argument");
			Val v=eval(a->builtin.args[0], env);
			switch(v.tag) {
			case V_NUM:
				printf("%d\n", v.as.i);
				break;
			case V_BOOL:
				printf("%s\n", v.as.b? "true":"false");
				break;
			case V_FUNC:
				printf("<function>\n");
				break;
			case V_STR:
				printf("%s\n", v.as.s);
				break;
			case V_BUILTIN:
				printf("<builtin>\n");
				break;
			default:
				printf("null\n");
				break;
			}
			return VBool(true);
		}
		}
		die("unknown builtin");
	}
	default:
		die("not implemented ast node");
	}
	return VNull();
}

static char* fslurp(const char* path) {
	FILE* f=fopen(path,"rb");
	if(!f) return NULL;
	fseek(f,0,SEEK_END);
	long n=ftell(f);
	fseek(f,0,SEEK_SET);
	char* s=(char*)malloc(n+1);
	if(!s) {
		fclose(f);
		return NULL;
	}
	if(fread(s,1,n,f)!=(size_t)n) {
		fclose(f);
		free(s);
		return NULL;
	}
	s[n]='\0';
	fclose(f);
	return s;
}

int main(int argc, char** argv){
	g_argc = argc;
	g_argv = argv;
	char* src=NULL;
	if(argc>=2) {
		src = fslurp(argv[1]);
		if(!src) {
			fprintf(stderr,"could not read script: %s\n", argv[1]);
			return 1;
		}
	} else {
		size_t cap=4096, n=0;
		src=(char*)malloc(cap);
		int ch;
		while((ch=getchar())!=EOF) {
			if(n+1>=cap) {
				cap*=2;
				src=(char*)realloc(src,cap);
			}
			src[n++]=(char)ch;
		}
		src[n]='\0';
	}
	TokVec tv;
	tokenize(src, &tv);
	Parser P = { .toks=&tv, .i=0 };
	AST* prog = parse_program(&P);
	Env* global = env_new(NULL);
	g_global_env = global;
	env_define(global, "out", VBuiltin(BI_OUT), true);
	env_define(global, "outn", VBuiltin(BI_OUTN), true);
	env_define(global, "len", VBuiltin(BI_LEN), true);
	env_define(global, "char_at", VBuiltin(BI_CHARAT), true);
	env_define(global, "ord", VBuiltin(BI_ORD), true);
	env_define(global, "chr", VBuiltin(BI_CHR), true);
	env_define(global, "substr", VBuiltin(BI_SUBSTR), true);
	env_define(global, "str", VBuiltin(BI_STR), true);
	env_define(global, "num", VBuiltin(BI_NUM), true);
	env_define(global, "find", VBuiltin(BI_FIND), true);
	env_define(global, "read_file", VBuiltin(BI_READFILE), true);
	env_define(global, "argv", VBuiltin(BI_ARGV), true);
	env_define(global, "argc", VBuiltin(BI_ARGC), true);
	(void)eval(prog, global);
	tv_free(&tv);
	free(src);
	return 0;
}