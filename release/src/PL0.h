#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NRW 17		 // number of reserved words
#define TXMAX 500	 // length of identifier table
#define MAXNUMLEN 14 // maximum number of digits in numbers
// #define NSYM       10     // maximum number of symbols in array ssym and csym
// HB_2022-12-17
#define NSYM 14
#define MAXIDLEN 10 // length of identifiers

#define MAXADDRESS 32767 // maximum address
#define MAXLEVEL 32		 // maximum depth of nesting block
#define CXMAX 500		 // size of code array

#define MAXSYM 30 // maximum number of symbols

#define STACKSIZE 1000 // maximum storage
#define JMPMAX 4096	   // max setjump buffer size

enum symtype
{
	SYM_NULL,		// 保留位
	SYM_IDENTIFIER, // 标识符
	SYM_NUMBER,		// 数字
	SYM_PLUS,		// 加
	SYM_MINUS,		// 减
	SYM_TIMES,		// 乘
	SYM_SLASH,		// 斜杠，即除以
	SYM_ODD,		// 取模？保留字,odd
	SYM_EQU,		// 等于（条件判断符）
	SYM_NEQ,		// 不等于
	SYM_LES,		// 小于
	SYM_LEQ,		// 小于等于
	SYM_GTR,		// 大于
	SYM_GEQ,		// 大于等于
	SYM_LPAREN,		// 左括号
	SYM_RPAREN,		// 右括号
	SYM_COMMA,		// 逗号
	SYM_SEMICOLON,	// 分号
	SYM_PERIOD,		// 句号，准确来说是句点，例如C语言中指明结构体成员的符号
	SYM_BECOMES,	// 等于（赋值符）
	SYM_BEGIN,		// 保留字，begin
	SYM_END,		// 保留字，end
	SYM_IF,			// 保留字，if
	SYM_THEN,		// 保留字，then
	SYM_ELSE,		// kimagure 2022/12/25
	SYM_WHILE,		// 保留字，while
	SYM_DO,			// 保留字，do
	SYM_CALL,		// 保留字，call
	SYM_CONST,		// 常数/保留字,const
	SYM_VAR,		// 变量/保留字,var
	SYM_PROCEDURE,	// 过程/保留字,procedure

	// HB_2022-12-17
	SYM_LSQBRAC, // 左方括号，"["
	SYM_RSQBRAC, // 右方括号，"]"
	SYM_LBRAC,	 // 左大括号，"{"
	SYM_RBRAC,	 // 右大括号，"}"
	SYM_PRINT,	 // 用于打印

	// mahiru 2022-12-25
	SYM_SET_JUMP,  // setjump()
	SYM_LONG_JUMP, // longjmp()

	// jk
	SYM_RANDOM,
	SYM_FOR,  // 用于for
	SYM_COLON // 用语for的:
};

// 标识符类型：常数、变量、过程、数组
enum idtype
{
	ID_CONSTANT,
	ID_VARIABLE,
	ID_PROCEDURE,

	// HB_2022-12-17
	ID_ARRAY
};

// 翻译后的汇编指令操作码的属性值
enum opcode
{
	LIT,
	OPR,
	LOD,
	STO,
	CAL,
	INT,
	JMP,
	JPC,
	PRT,
	RDM,
	LODA,
	STOA,
	SET_JUMP,
	LONG_JUMP,
};

// 汇编指令OPR的第三个参数，运算类型
enum oprcode
{
	OPR_RET,
	OPR_NEG,
	OPR_ADD,
	OPR_MIN,
	OPR_MUL,
	OPR_DIV,
	OPR_ODD,
	OPR_EQU,
	OPR_NEQ,
	OPR_LES,
	OPR_LEQ,
	OPR_GTR,
	OPR_GEQ
};

// 指令数据结构
typedef struct
{
	int f; // 操作码
	int l; // 层次差
	int a; // 偏移地址
} instruction;

//////////////////////////////////////////////////////////////////////
// 报错提示信息
char *err_msg[] =
	{
		/*  0 */ "",
		/*  1 */ "Found ':=' when expecting '='.",
		/*  2 */ "There must be a number to follow '='.",
		/*  3 */ "There must be an '=' to follow the identifier.",
		/*  4 */ "There must be an identifier to follow 'const', 'var', or 'procedure'.",
		/*  5 */ "Missing ',' or ';'.",
		/*  6 */ "Incorrect procedure name.",
		/*  7 */ "Statement expected.",
		/*  8 */ "Follow the statement is an incorrect symbol.",
		/*  9 */ "'.' expected.",
		/* 10 */ "';' expected.",
		/* 11 */ "Undeclared identifier.",
		/* 12 */ "Illegal assignment.",
		/* 13 */ "':=' expected.",
		/* 14 */ "There must be an identifier to follow the 'call'.",
		/* 15 */ "Only a procedure can be called.",
		/* 16 */ "'then' expected.",
		/* 17 */ "';' or 'end' expected.",
		/* 18 */ "'do' expected.",
		/* 19 */ "Incorrect symbol.",
		/* 20 */ "Relative operators expected.",
		/* 21 */ "Procedure identifier can not be in an expression.",
		/* 22 */ "Missing ')'.",
		/* 23 */ "The symbol can not be followed by a factor.",
		/* 24 */ "The symbol can not be as the beginning of an expression.",
		/* 25 */ "The number is too great.",
		/* 26 */ "",
		/* 27 */ "",
		/* 28 */ "",
		/* 29 */ "",
		/* 30 */ "",
		/* 31 */ "",
		/* 32 */ "There are too many levels.",

		// HB 2022-12-17
		/* 33 */ "The size of a dimension expected.",
		/* 34 */ "An array must be a var type.",
		/* 35 */ "An array must be initialized with a brace-enclosed initializer.",
		/* 36 */ "The subscript of a dimension exopected.",
		/* 37 */ "Missing ']'.",
		/* 38 */ "Not a allowed data type for now.",
		/* 39 */ "Declaration CANNOT be done with virables.",
		/* 40 */ "Array initialization failed.",
		/* 41 */ "Expression expected.",
		/* 42 */ "Missing '}'.",
		/* 43 */ "',' expected.",
		/* 44 */ "CANNOT convert '<brace-enclosed initializer list>' to 'var' in initialization.",
		/* 45 */ "Too many initializers.",
		/* 46 */ "CANNOT decalare a procedure in statement.",
		/* 47 */ "',' or '}' expected.",
		// mahiru 2022-12-25
		/* 48 */ "Missing '(' after setjmp",
		/* 49 */ "Missing ')' after setjmp",
		/* 50 */ "Missing '(' after longjmp",
		/* 51 */ "Missing ',' after longjmp",
		/* 52 */ "Missing ')' after longjmp",
		// jk 2022-12-27
		/* 53 */ "Missing (",
		/* 54 */ "Missing var in for statement",
		/* 55 */ "Missing : in for statement"};

//////////////////////////////////////////////////////////////////////
char ch;			   // last character read
int sym;			   // last symbol read
int sym_old;		   // 处理if then 无else情况
char id[MAXIDLEN + 1]; // last identifier read
int num;			   // last number read
int cc;				   // character count
int ll;				   // line length
int kk;
int err;		// 错误号
int cx;			// index of current instruction to be generated.
int level = 0;	// 层次
int tx = 0;		// 符号表表项编号（下标）
int ifelse = 0; // 用于else
char line[80];	// 读入的代码行

instruction code[CXMAX]; // 代码段

// 保留字
char *word[NRW + 1] =
	{
		"", /* place holder */
		"begin", "call", "const", "do", "end", "if",
		"odd", "procedure", "then", "var", "while",
		"print",
		// mahiru 2022-12-25
		"setjmp", "longjmp", "else",
		// jk
		"random", "for"};

// 保留字的属性值（用于区分是哪一个保留字）
int wsym[NRW + 1] =
	{
		SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
		SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,
		SYM_PRINT,
		// mahiru 2022-12-25
		SYM_SET_JUMP, SYM_LONG_JUMP, SYM_ELSE,
		// jk
		SYM_RANDOM, SYM_FOR};

// 终结符的属性值（用于区分是哪一个运算符，包括分号）
int ssym[NSYM + 1] =
	{
		SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
		SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,

		// HB_2022-12-17
		SYM_LSQBRAC, SYM_RSQBRAC,
		SYM_LBRAC, SYM_RBRAC};

// 终结符本身的ASCII
char csym[NSYM + 1] =
	{
		' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';',
		// HB_2022-12-17
		'[', ']',
		'{', '}'};

#define MAXINS 14

// 汇编指令的操作码
char *mnemonic[MAXINS] =
	{
		"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC", "PRT", "RDM", "LODA", "STOA", "SJP", "LJP" // added: setjump longjump
};

// 符号表表项数据结构
typedef struct
{
	char name[MAXIDLEN + 1];
	int kind;
	int value;
} comtab;

// 符号表
comtab table[TXMAX];

// 符号表变量、过程类型表项数据结构
// 原本表项的value空间被均分为level和address两部分空间
typedef struct
{
	char name[MAXIDLEN + 1];
	int kind;
	short level;
	short address;
} mask;

// setjump&longjump添加
typedef struct
{
	// setjump保存结构体
	// 理论上pc需要15bit，top和base分别需要10bit，略难
	// 助教说无所谓
	int pc;
	int top;
	int base;
} size_jmp_buf;

size_jmp_buf jmp_buf[JMPMAX];

// 输入文件
FILE *infile;

// HB_2022-12-18
// 数组的符号表管理
// 数组元素存入符号表，但数组名等附加属性统一用另一张表进行管理
// 这是因为pl0中没有定义复杂的数据类型，因此不方便使用类型的附加属性进行管理
#define MAXDIM 256 // ISO 14882, B.2
#define ARXMAX 500 // 数组附加属性表的最大条目数，命名意义为：ARRAY_INDEX_MAX，简称ARXMAX
typedef struct ARRAY_TABLE
{
	// 地址属性的类型设置成short是为了尽量跟mask保持一致
	short head_address;	  // 数组的首地址
	int dim;			  // 数组的维度（从0开始计数）
	int dim_size[MAXDIM]; // 数组各个维度的值
	int size;			  // 数组元素的总个数
} ARRAY_TABLE;
ARRAY_TABLE arytb[ARXMAX]; // 记录数组附加属性的表
ARRAY_TABLE last_ex_amk;   // 用来记录最后一个读取到的数组的附加属性
// 符号表表现的数组表项数据结构
typedef struct ARRAY_MASK
{
	char name[MAXIDLEN + 1];
	int kind;
	short level;
	short AT_address; // 附加属性在附加属性表中的下标，命名意义为：ARRAY_TABLE_ADDRESS，简称AT_address
} ARRAY_MASK;

int atx = 0;		// 附加属性表下标，命名意义为：ARRAY_TABLE_INDEX，简称atx，始终指向表中第一个空闲的位置
int dim;			/// 当前正在分析的维度
int flag_dim_0 = 0; // 标志首个维度的方括号之间的表达式是否为空
#define DIM_BACKFILL MAXADDRESS + 1
int gen_OK = 0;				  // 判断当前是否需要生成代码
int single_step = 0;		  // 单步调试模式
int pos = 0;				  // 数组初始化时存放数值的偏移量
int vol = 0;				  // 当前维度的容量，比如a[2][2][2]由高到低的三个维度容量分别为4，2，1
int init_num = 0;			  // 最高维度的数目，用于回填
int init = 0;				  // 用于帮助gen区分生成的代码存到何处
int cx_init;				  // 记载初始化操作的代码存储器下标
instruction code_init[CXMAX]; // 记载初始化操作的代码存储器
int statement_init = 0;
// EOF PL0.h
