#include <stdio.h>
//Xulong's_contribution:注释
//Xinyu's_contribution:添加新的保留字的时候记得增大NRW
//Xulong's_contribution:
#define NRW        16     // number of reserved words
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
//Xinyu's_contribution:添加新的合法字符时要增大NSYM
#define NSYM       12     // maximum number of symbols in array ssym and csym 除了字母数字和关系运算符以外的字符数量
#define MAXIDLEN   10     // length of identifiers 标识最长10个字符

#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block
#define CXMAX      500    // size of code array

#define MAXSYM     30     // maximum number of symbols  

#define STACKSIZE  1000   // maximum storage
//Xinyu's_contribution:数组
#define MAX_DIM    100    //maximum dim of array
//Xulong's_contribution:
#define MAX_LOOP   9     //for循环的最大重数

enum symtype//词法分析中不同的标识
{
	SYM_NULL,//非法?
	SYM_IDENTIFIER,//名称
	SYM_NUMBER,//数字串
	SYM_PLUS,//对应'+'
	SYM_MINUS,//对应'-'
	SYM_TIMES,//对应'*'
	SYM_SLASH,//对应'/'
	SYM_ODD,//对应'odd',判断一个expression是不是奇数
	SYM_EQU,//对应'='
	SYM_NEQ,//对应'<>'即不等于
	SYM_LES,//对应'<'
	SYM_LEQ,//对应'<='
	SYM_GTR,//对应'>'
	SYM_GEQ,//对应'>='
	SYM_LPAREN,//对应'('
	SYM_RPAREN,//对应')'
	SYM_COMMA,//对应','
	SYM_SEMICOLON,//对应';'
	SYM_PERIOD,//对应'.'
	SYM_BECOMES,//赋值,对应":=
    SYM_BEGIN,//对应'begin'保留字
	SYM_END,//对应'end'保留字
	SYM_IF,//对应'if'保留字
	SYM_THEN,//对应'then'保留字
	SYM_WHILE,//对应'while'保留字
	SYM_DO,//对应'do'保留字
	SYM_CALL,//对应'call'保留字
	SYM_CONST,//对应'const'保留字
	SYM_VAR,//对应'var'保留字
	SYM_PROCEDURE,//对应''procedure保留字
	SYM_LMIDPAREN,//Xinyu's_contribution:数组,对应`[`
	SYM_RMIDPAREN,//Xinyu's_contribution:数组,对应`]`
	SYM_PRT,//Xinyu's_contribution:print,对应保留字print
	//Xulong's_contribution:
	SYM_FOR,//对应`for`保留字
	SYM_COLON,//对应`:`
	//Tianyi's_contribution:
	SYM_SETJ,//对应'setjump'
	SYM_LONGJ,//对应'longjump'
	//Yuanhao's_contribution:
	SYM_ELSE,
};
enum idtype
{
	ID_CONSTANT, //常量类型
	ID_VARIABLE, //变量类型
	ID_PROCEDURE,//过程类型
	//Xinyu's_contribution:数组
	ID_ARRAY,//数组类型
};

enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC,
	//Xinyu's_contribution:print
	PRT,
	//Xinyu's_contribution:数组
	STA,LEA,LDA,
};

enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ,
	//Tianyi's_contribution:
	OPR_SST, OPR_LST, OPR_SAVE0, OPR_SAVE1, 
	OPR_LOAD0, OPR_LOAD1, 
};


typedef struct
{
	int f; // function code 即opcode
	int l; // level 即层次差
	int a; // displacement address 视不同opcode而不同
} instruction;//指令格式,它包括f即opcode,l即层次差,a视不同opcode而不同

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "Missing '(",
			//Xinyu's_contribution:数组的错误处理信息
/* 27 */    "Missing ']'.",
/* 28 */    "Missing '['.",
/* 29 */    "Must define the dimension size of an array.",
/* 30 */    "Missing dimension(s) in array element assignment.",
/* 31 */    "Wrong when print",
/* 32 */    "There are too many levels.",
/*33*/		"too many dimensions",
			//Xulong's_contribution:
/*34*/		"Missing 'var' in forloop.",
/*35*/		"Missing ':' in forloop.",
/*36*/		"Missing ',' or ')' in forloop, the compiler failed.",
/*37*/		"the level of loop is too great.",
			//Tianyi's_contribution:
/*38*/		"Missing '(' after setjump.",
/*39*/		"Missing ')' after setjump.",
/*40*/      "Missing '(' after longjump.",
/*41*/      "Missing ',' in longjump.",
/*42*/      "Missing ')' after longjump.",


};

//////////////////////////////////////////////////////////////////////
char ch;         // last character read 最近读的一个字符
int  sym;        // last symbol read 读取的一段一段字符串即id对应的标识符
char id[MAXIDLEN + 2]; // last identifier read 这是读取的一段字符串,不出意外的话,它要么是保留字,要么是名称
int  num;        // last number read
int  cc;         // character count,当前字符是在一行代码中的第几个字符
int  ll;         // line length,一行代码的长度
int  kk;
int  err;		//err是错误数,
int  cx;         // index of current instruction to be generated.当前是第几条指令(中间代码)
int  level = 0;//层次
int  tx = 0;//符号表的索引,它用于指向符号表中最近填入的那个符号

char line[80];//一行代码最多80个字符

//Xinyu's_contribution:
//记录数组属性
typedef struct array_attribute
{
	short address;//数组的基地址(即数组第一个元素在栈中的位置)
	int size;//数组大小
	int dim;//总维数 
	int dim_size[MAX_DIM + 1];//每一维的范围大小
} array_attribute;
array_attribute array_table[TXMAX];//数组信息表
struct array_attribute* Last_Array; //指向最后读到的数组
int  arr_tx = 0; // 当前读到的数组在数组表中的索引


//Xulong's_contribution:
//for循环控制的属性
typedef struct 
{
	short low_dx;//low的相对偏移量,low实际就是for循环声明的那个变量的初值
	short high_dx;//high的相对偏移量
	short step_dx;//step的相对偏移量
}for_loop_attribute;
for_loop_attribute for_loop_table[MAX_LOOP+1];//for循环控制表,当多重循环的时候,它们的信息就会保存在里面,它的数组索引i代表的是第i层for循环,没分析完一层for循环这个表格就会删除一层
int depth_of_loop =0;//它永远代表当前循环的层次



instruction code[CXMAX];

char* word[NRW + 1] =//这是11个保留字,word[0]存放id,用以和11个保留字比较
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while",
	//Xinyu's_contribution:
	"print",//添加保留字print
	//Xulong's_contribution:
	"for",
	//Tianyi's_contribution::
	"setjmp","longjmp",
	//Yuanhao's_contribution:
	"else",
};

int wsym[NRW + 1] =//对应word中个保留字,wsym[0]是SYM_NULL表示非法
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,
	//Xinyu's_contribution:
	SYM_PRT,//增加的print保留字
	//Xulong's_contribution:
	SYM_FOR,
	//Tianyi's_contribution::
	SYM_SETJ,SYM_LONGJ,
	//Yuanhao's_contribution:
	SYM_ELSE,
};

int ssym[NSYM + 1] =//对应csym中10个合法字符
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,
	//Xinyu's_contribution:数组 应该增加两个标识`[`和`]`
	SYM_LMIDPAREN,SYM_RMIDPAREN,
};

char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';',
	//Xinyu's_contribution:数组应该增加符号
	'[',']',
};

//Xinyu's_contribution:增加新的指令时,要增大MAXINS
#define MAXINS   12

//即opcode对应的中间代码名称
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC",
	//Xinyu's_contribution:print
	"PRT",
	//Xinyu's_contribution:数组
	"STA", "LEA", "LDA",
};

typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
} comtab;

comtab table[TXMAX];//符号表,三个成员变量:name即符号名,kind即类型,value这个只有类型是常数才会表示为常数的值,否则 它使用mask中的level和address

//对应符号表中数据的另一种类型,level和address会替换comtab中的value类型,如果类型是变量,level就是层次,address就是偏移量;如果类型是过程,那么level就是层次,address是入口地址
typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;//只对类型是变量或过程有效,层次差
	short address;//偏移量或过程入口地址
} mask;
//infile是一个全局变量,它是文件指针,它指向输入的代码文件
FILE* infile;

// EOF PL0.h
