// pl0 compiler source code

#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "PL0.h"
// #include "set.c"
#include "set.h"

//////////////////////////////////////////////////////////////////////
// 输出错误信息
// 用一个字符串数组err_msg存储需要输出的内容，根据错误编号输出内容
void error(int n)
{
    int i;

    printf("      "); // 分隔开一定距离
    for (i = 1; i <= cc - 1; i++)
        printf(" ");                          // 定位，使得后面输出的箭头恰好指向出错的位置
    printf("^\n");                            // 用箭头指向出错的位置
    printf("Error %3d: %s\n", n, err_msg[n]); // 输出报错信息
    err++;                                    // 错误数增加
} // error

//////////////////////////////////////////////////////////////////////
// 词法分析器
void getch(void)
{
    if (cc == ll) // 已经取完一行字符，需要更新串line[]
    {
        if (feof(infile)) // 已到达文件结尾
        {
            printf("\nPROGRAM INCOMPLETE\n");
            exit(1);
        }                                                        // 代码读取完毕
        ll = cc = 0;                                             // 重置字符数量的标记信息
        printf("%5d  ", cx);                                     // 输出当前需要生成的语句编号
        while ((!feof(infile)) && ((ch = getc(infile)) != '\n')) // 按行读入
        {
            printf("%c", ch); // 回显
            line[++ll] = ch;  // 填入缓冲区，缓冲区的首个位置（下标0）保留
        }                     // while
        printf("\n");
        line[++ll] = ' '; // 缓冲区最后一个位置填入一个SPACE
    }
    ch = line[++cc]; // （当前行的字符还未取完）取下一个字符
} // getch

//////////////////////////////////////////////////////////////////////
// 从输入流中获取“标识”
void getsym(void)
{
    if (ifelse == 1)
    {
        // 先回复sym
        ifelse = 2;
        sym_old = sym;
        sym = SYM_SEMICOLON;
    }
    else if (ifelse == 2)
    {
        ifelse = 0;
        sym = sym_old;
    }
    else
    {
        int i, k;
        char a[MAXIDLEN + 1]; // 存放标识符

        while (ch == ' ' || ch == '\t') // 去除符号间或符号之前的空白字符
            getch();

        if (isalpha(ch)) // 判断开头是否为英文字母
        {                // 若是，则目标为保留字或标识符
            k = 0;
            do // 循环读入整个单词，并存放到串a[]中
            {
                if (k < MAXIDLEN)
                    a[k++] = ch;
                getch();
            } while (isalpha(ch) || isdigit(ch)); // 读入的标识符或者保留字只能由数字或字母组成
            a[k] = 0;                             // 最后一个位置手动加入串的结束标志"\0"
            strcpy(id, a);                        // 将读入的标识符或关键字存入串id[]
            word[0] = id;                         // 将id放入关键字表word的首个位置（该位置初始时是保留的）
            i = NRW;                              // 关键字数目
            while (strcmp(id, word[i--]))
                ;                     // 从关键字列表末端开始遍历，逐个比较以便确定id是否是关键字
            if (++i)                  // while循环结束时，i不为-1，说明id是保留字
                sym = wsym[i];        // 记录当前读入的保留字，这里使用wsym中的值标识是哪一个保留字（下标与通过字符标识的串数组word是对应的）
            else                      // i为-1时，说明所有保留字都不匹配，读入的id是用户定义的标识符
                sym = SYM_IDENTIFIER; // 记录标识符，并用symtype中的值标记为标识符
        }
        else if (isdigit(ch)) // 判断开头是否为数字------这正是标识符不允许以数字开头的原因
        {                     // 目标是数字序列
            k = num = 0;
            sym = SYM_NUMBER; // 记录标识符，并用symtype中的值标记
            do                // 循环读入所有数字，并以**十进制**的格式存放到num中
            {
                num = num * 10 + ch - '0';
                k++;
                getch();
            } while (isdigit(ch));
            if (k > MAXNUMLEN)
                error(25); // 数字的大小超出限制
        }
        // 下面开始识别是否是其他特殊符号
        // else if分支检测可能由两个字符组成的符号
        // else则检测其他预定义的符号
        else if (ch == ':') //:=
        {
            getch();
            if (ch == '=')
            {
                sym = SYM_BECOMES; //:=
                getch();
            }
            else
            {
                sym = SYM_COLON; // To be done: 识别到非法字符？但后面NULL多次出现在后继符号集合中...与错误处理有关？
            }
        }
        else if (ch == '>') //> , >=
        {
            getch();
            if (ch == '=')
            {
                sym = SYM_GEQ; //>=
                getch();
            }
            else
            {
                sym = SYM_GTR; //>
            }
        }
        else if (ch == '<') //< , <=
        {
            getch();
            if (ch == '=')
            {
                sym = SYM_LEQ; //<=
                getch();
            }
            // 注意，不等号为"<>"，而非"!="
            else if (ch == '>')
            {
                sym = SYM_NEQ; //<>
                getch();
            }
            else
            {
                sym = SYM_LES; // <
            }
        }
        else
        { // 其他字符
            i = NSYM;
            csym[0] = ch; // 用上面识别保留字相同的方法，识别是否为运算符
            while (csym[i--] != ch)
                ;
            if (++i) // 是运算符
            {
                sym = ssym[i];
                getch();
            }
            else // 不是运算符，直接判断为非法字符
            {
                printf("Fatal Error: Unknown character.\n");
                exit(1);
            }
        }
    }
} // getsym

//////////////////////////////////////////////////////////////////////
// 代码生成
//  生成一条指令（汇编），把三个参数 f、l、a 组装成一条目标指令并存放于 code 数组中，增加 CX 的值，CX 表示下一条即将生成的目标指令的地址。
void gen(int x, int y, int z)
{
    if (cx > CXMAX) // 代码超出长度限制
    {
        printf("Fatal Error: Program too long.\n");
        exit(1);
    }
    if (gen_OK)
        return;
    if (!init || statement_init)
    {
        code[cx].f = x;   // 操作码，如LIT,LOD,STO等
        code[cx].l = y;   // 操作数的层次差（声明点和引用点之间的静态层次差）
        code[cx++].a = z; // 操作数的偏移地址
    }
    else
    {
        code_init[cx_init].f = x;
        code_init[cx_init].l = y;
        code_init[cx_init++].a = z;
    }
    // 上述赋值完成时，cx自增，以指向下一个位置
} // gen

//////////////////////////////////////////////////////////////////////
// 错误诊断处理
// 若错误发生，则跳过部分字符，直到遇到同步符号（关键字规则）或者可以正常执行的部分（镇定规则）
//  test 过程有三个参数：
//  （1） 可允许的下一个符号集合 S1，如果当前符号不在此集合中，当即得到一个错误号；
//  （2） 另加的停止符号集合 S2，有些符号的出现，虽然无疑是错的，但它们绝对不应被忽略而跳过；
//  （3） 整数 n，表示有关错误的诊断号；
void test(symset s1, symset s2, int n)
{
    symset s;
    if (!inset(sym, s1)) // 如果当前的符号不在同步符号集中，报错，并开始应用镇定规则
    {
        if (!gen_OK)
        {
            error(n);
        }
        s = uniteset(s1, s2);  // 向同步符号集合中加入新的停止符号，并将合并后的集合存入临时集合s中
        while (!inset(sym, s)) // 如果当前符号不在扩展后的集合中，暂时跳过，继续读取后面的符号（保持镇定）
            getsym();
        // 注意，s2中的符号是镇定规则中所说的“不该被盲目忽略的符号”
        destroyset(s); // 销毁临时空间
    }
} // test

//////////////////////////////////////////////////////////////////////
// 符号表管理
int dx; // 过程的数据单元下标，用于分配变量，顺序分配，从3开始。前三个位置预留给RA,DL,SL

// 将常量、变量、过程等加入符号表
// 表项数据结构为mask，包含名字、种类、层次差、地址
// 标识符表应包含每一标识符所联系的属性；如果标识符被说明为常数，其属性值为常数值；
// 如果标识符被说明成变量，其属性就是由层次和修正量（偏移量）组成的地址；
// 如果标识符被说明为过程，其属性就是过程的入口地址及层次。
void enter(int kind)
{
    mask *mk; // 新表项
    // tx是符号表中最后一个非空表项的下表，自增之后即指向首个空闲的位置
    // HB_2022-12-18
    ARRAY_MASK *amk;            // 数组附加属性表新表项
    tx++;                       // 表项编号（下标）
    strcpy(table[tx].name, id); // 名字插入表中
    table[tx].kind = kind;      // 标记种类
    switch (kind)               // 分类填入属性
    {
    case ID_CONSTANT: // 常量，属性值为常数值
        if (num > MAXADDRESS)
        {
            error(25); // The number is too great.
            num = 0;
        } // 常数值超出大小限制
        table[tx].value = num;
        break;
    case ID_VARIABLE:            // 变量，属性是由层次和偏移组成的地址
        mk = (mask *)&table[tx]; // 为变量分配空间
        mk->level = level;       // 写入层次
        mk->address = dx++;      // 数据空间下标加一
        break;
    case ID_PROCEDURE:           // 过程
        mk = (mask *)&table[tx]; // 为过程分配空间
        mk->level = level;       // 写入层次
        // 这里为什么不填入地址呢？
        // 这是因为程序的地址将在block函数中填入符号表
        break;
    case ID_ARRAY:
        // HB_2022-12-17
        // 注意：数组名本身只是一个“标签”，本身是不占用空间的
        amk = (ARRAY_MASK *)&table[tx]; // 为数组名分配空间
        amk->level = level;             // 写入层次
        amk->AT_address = atx;          // 写入附加属性表地址
        // 下面根据last_ex_amk来填写附加属性表
        arytb[atx].dim = last_ex_amk.dim;
        for (int i = last_ex_amk.dim; i >= 0; i--)
        {
            arytb[atx].dim_size[i] = last_ex_amk.dim_size[i];
        }
        arytb[atx].head_address = dx;
        dx += last_ex_amk.size; // 为数组元素腾出空间
        arytb[atx].size = last_ex_amk.size;
        atx++;
    } // switch
} // enter

//////////////////////////////////////////////////////////////////////
// 查找符号表中的标识符
// 若存在表项，则返回在符号表中的下标（非零），否则返回0
int position(char *id)
{
    int i;
    strcpy(table[0].name, id);               // 使用与getsym函数中一样的方法，从后向前遍历查找
    i = tx + 1;                              // i当前指向符号表的第一个空余位置
    while (strcmp(table[--i].name, id) != 0) // 向前遍历，如果存在与id配对的表项，i不会降到0
        ;
    return i;
} // position

//////////////////////////////////////////////////////////////////////
// 语法分析器
// 结合文法得到的递归下降的语法分析
// 声明时处理数组的维度
void dimension(void)
{
    gen_OK = 1;                  // 告诉表达式现在开始处理的是数组声明
    int expression(symset fsys); // 内部声明expression，使得后面能够正确调用
    dim = 0;                     // 重置当前处理的维度
    int cur_dim_size;            // 当前维度的大小
    symset set;
    set = createset(SYM_RSQBRAC, SYM_NULL); // 生成一个包含右方括号的符号集，因为右括号不应当被跳过
    last_ex_amk.dim = 0;                    // 记录附加属性
    last_ex_amk.size = 0;
    int flag = 0;   // 标志某个维度方括号之间的表达式是否为空
    flag_dim_0 = 0; // 标志首个维度的方括号之间的表达式是否为空
    while (sym == SYM_LSQBRAC)
    { // 当数组的维度和初始化数据没有处理完毕时
        getsym();
        cur_dim_size = expression(set);      // 获取方括号之间的表达式的数值
        flag = cur_dim_size == DIM_BACKFILL; // 测试是否为空
        if (flag && dim)
        {              // 如果当前维度为空且并非第一个维度，报错
            error(33); //"The size of a dimension expected."
        }
        else if (flag && !dim)
        {
            flag_dim_0 = 1;
            last_ex_amk.size = 1;        // 大小暂时设置为1，等待回填
            last_ex_amk.dim_size[0] = 1; // 大小暂时设置为1，等待回填
        }
        else
        { // flag=0，此时维度的大小正常被声明
            last_ex_amk.dim_size[dim] = cur_dim_size;
            if (dim)
            {
                last_ex_amk.size *= cur_dim_size;
            }
            else
            {
                last_ex_amk.size = cur_dim_size;
            }
        }
        if (sym == SYM_RSQBRAC) // 如果读取到了右方括号括号（正确完成配对）
        {
            dim++;
            getsym(); // 继续读取下一个符号
        }
        else // 没有读取到右括号，报错
        {
            error(37); // Missing ']'.
        }
    }
    last_ex_amk.dim = --dim; // 记录维度的个数
    gen_OK = 0;              // 数组声明处理结束
    destroyset(set);         // 调用完成，销毁临时的符号集
    return;
}

// 常量声明
void constdeclaration()
{
    int expression(symset fsys);
    if (sym == SYM_IDENTIFIER) // 识别到左值为标识符
    {
        getsym();                                 // 获取下一个符号，从而做进一步的判断
        if (sym == SYM_EQU || sym == SYM_BECOMES) // 下一个符号是条件判断等于或赋值等于
        {
            if (sym == SYM_BECOMES) // 赋值的等号
                // 注意，声明中赋值使用的等号与条件判断中的等号是相同的，似乎不存在条件判断表达式
                error(1); // Found ':=' when expecting '='.
            getsym();     // 读取下一个符号
            gen_OK = 1;   // 只是为了在编译时获取表达式的数值，故不能生成代码
            symset set = createset(SYM_COMMA, SYM_SEMICOLON, SYM_NULL);
            num = expression(set);
            gen_OK = 0;
            enter(ID_CONSTANT); // 将标识符标注为常量类型并加入符号表
        }
        // HB_2022-12-18
        else if (sym == SYM_LSQBRAC)
        {              // 数组必须是var类型
            error(34); //"An array must be a var type."
        }
        else
        {
            error(3); // There must be an '=' to follow the identifier.
        }
    }
    else
        error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//////////////////////////////////////////////////////////////////////
// 数组初始化，函数按照产生式进行编写
void initializer(ARRAY_TABLE amk, int *count)
{
    ARRAY_TABLE amk_temp;
    int unit_size; // 每一片“连续存储地址”的大小，其实就是下一层的“容量”
    int enbrace_init_flag = 0;
    int *cnt = (int *)malloc(sizeof(int));
    *cnt = 0;
    void initializer_list(ARRAY_TABLE amk, int flag, int *count);
    void assignment_expression(ARRAY_TABLE amk);
    if (!(*count))
    {
        *count = 1;
    }
    else
    {
        *count = *count + 1;
    }
    if (*count > amk.size && ((dim > 0) || !flag_dim_0))
    {              // 不需要回填时，size是固定的
        error(45); //"Too many initializers."
    }
    if (inset(sym, facbegsys))
    {
        assignment_expression(amk);
        if (sym != SYM_COMMA && sym != SYM_RBRAC)
        {
            error(47); //"',' or '}' expected."
        }
    }
    else if (sym == SYM_LBRAC)
    {
        dim++;
        amk_temp.dim = amk.dim - 1;
        amk_temp.size = 1;
        for (int i = amk_temp.dim; i >= 0; i--)
        {
            amk_temp.dim_size[i] = amk.dim_size[i + 1];
            amk_temp.size *= amk_temp.dim_size[i];
        }
        if ((pos - amk.head_address) % amk_temp.size)
        {
            enbrace_init_flag = 1; // 标志是否是连续空间的第一个数据，若是则占据整个空间
        }
        int temp_flag = 0;
        int temp_mul = amk_temp.size;
        for (int i = 0; i < amk_temp.dim; i++)
        {
            temp_mul /= amk_temp.dim_size[i];
            if (!((pos - amk.head_address) % temp_mul) && temp_mul != 1)
            {
                temp_flag = 1;
            }
        }
        amk_temp.head_address = pos;
        getsym();
        initializer_list(amk_temp, enbrace_init_flag && !temp_flag, cnt);
        unit_size = amk_temp.size / amk_temp.dim_size[0];
        dim--;
        while ((pos - amk.head_address) % amk_temp.size && (!enbrace_init_flag || temp_flag))
        { // 计算是否有缺省时，以上一层的头地址作为基准
            // 缺省部分全部填入0
            *cnt = *cnt + 1;
            gen(LIT, 0, 0);
            gen(STO, 0, pos);
            pos++;
        }
        temp_flag = 0;
        if (flag_dim_0)
        {
            init_num = (pos - amk_temp.head_address) / unit_size; // 当需要回填时，将本层初始化数目填入，init_num最后一次被修改对应的一定是最后一层
        }
        free(cnt);
        if (sym == SYM_COMMA)
        {
            getsym();
        }
        if (sym == SYM_RBRAC)
        {

            getsym();
        }
        else
        {
            error(42); //"Missing '}'."
        }
    }
    else
    {
        error(41); //"Expression expected."
    }
    return;
}

void initializer_list(ARRAY_TABLE amk, int flag, int *count)
{
    void initializer_list_p(ARRAY_TABLE amk, int flag, int *count);
    initializer(amk, count);
    initializer_list_p(amk, flag, count);
    return;
}
// 下面意为initializer_list'，是用于消除直接左递归而增加的新非终结符
void initializer_list_p(ARRAY_TABLE amk, int flag, int *count)
{
    if (sym == SYM_COMMA)
    {
        if (flag)
        {
            error(44); //"CANNOT convert '<brace-enclosed initializer list>' to 'var' in initialization"
        }
        getsym();
        initializer(amk, count);
        initializer_list_p(amk, flag, count);
        return;
    }
    else
        return;
}

void assignment_expression(ARRAY_TABLE amk)
{
    int expression(symset fsys);
    symset set = createset(SYM_COMMA, SYM_RBRAC, SYM_NULL);
    expression(set); // 生成计算某个单元的初始值所需的代码
    // 执行完上面的部分，此时栈顶存有需要放入存储单元的数值
    gen(STO, 0, pos); // 注意，声明处的层次差一定是0
    pos++;            // 下一个初始化位置
    destroyset(set);
    return;
}
// 变量声明
void vardeclaration(void)
{
    init = 1;
    int expression(symset fsys);
    if (sym == SYM_IDENTIFIER) // 识别到左值为标识符
    {
        getsym(); // 根据下一个符号是否为方括号判断变量类型是简单变量还是数组变量
        if (sym == SYM_LSQBRAC)
        {
            dimension();                              // 处理维度
                                                      // 下一个符号如果不是等号，说明没有对数组进行初始化，那么首个维度不可以等待回填
            if (sym == SYM_EQU || sym == SYM_BECOMES) // 下一个符号是条件判断等于或赋值等于
            {
                if (sym == SYM_BECOMES) // 赋值的等号
                    // 注意，声明中赋值使用的等号与条件判断中的等号是相同的，似乎不存在条件判断表达式
                    error(1);          // Found ':=' when expecting '='.
                getsym();              // 读取下一个符号
                if (sym == SYM_NUMBER) // 数字
                {
                    error(35); //"An array must be initialized with a brace-enclosed initializer."
                }
                else if (sym == SYM_LBRAC) // 检测到左大括号，开始对数组进行初始化
                {
                    dim = -1;
                    ARRAY_TABLE amk_temp;
                    amk_temp.dim = last_ex_amk.dim + 1;
                    amk_temp.size = 1;
                    for (int i = amk_temp.dim; i >= 1; i--)
                    {
                        amk_temp.dim_size[i] = last_ex_amk.dim_size[i - 1];
                        amk_temp.size *= amk_temp.dim_size[i];
                    }
                    amk_temp.head_address = dx;
                    pos = dx;
                    int *count = (int *)malloc(sizeof(int));
                    *count = 0;
                    initializer(amk_temp, count);
                    if (flag_dim_0)
                    {
                        last_ex_amk.dim_size[0] = init_num;          // 回填缺省的首个维度
                        last_ex_amk.size *= last_ex_amk.dim_size[0]; // 回填数组总大小
                    }
                    free(count);
                }
                else
                {              // 模仿C语言处理的报错
                    error(40); //"Array initialization failed."
                }
            }
            else if (flag_dim_0)
            {              // 数组的首个维度在没有初始化时不能为空
                error(33); //"The size of a dimension expected."
            }
            enter(ID_ARRAY); // 将数组填入符号表
            if (statement_init)
            {
                gen(INT, 0, last_ex_amk.size);
            }
        }
        else // 普通变量声明
        {
            if (sym == SYM_EQU || sym == SYM_BECOMES) // 下一个符号是条件判断等于或赋值等于
            {
                if (sym == SYM_BECOMES) // 赋值的等号
                    // 注意，声明中赋值使用的等号与条件判断中的等号是相同的，似乎不存在条件判断表达式
                    error(1); // Found ':=' when expecting '='.
                getsym();     // 读取下一个符号
                // 变量的值只能通过指令在运行时改变
                symset set = createset(SYM_COMMA, SYM_SEMICOLON, SYM_NULL);
                expression(set); // 处理完表达式之后，栈顶应当存有用于初始化变量的数值
                destroyset(set);
                gen(STO, 0, dx); // 由于本身就是声明的位置，层次差一定为0
            }
            enter(ID_VARIABLE); // 将标识符标注为变量类型并加入符号表
            if (statement_init)
            {
                gen(INT, 0, 1);
            }
        }
    }
    else
    {
        error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
    }
    init = 0;
} // vardeclaration
//////////////////////////////////////////////////////////////////////
// 输出翻译完成的汇编代码
// 格式为： 操作码 层次差 偏移地址
void listcode(int from, int to)
{
    int i;

    printf("\n");
    for (i = from; i < to; i++)
    {
        printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
    }
    printf("\n");
} // listcode

// 引用时处理数组的维度，参数中的amk是被引用的数组
// 由于pl0中没有设置复杂的数据类型，因此每次引用必须是某个具体的数组元素，不允许引用地址
// 该函数生成用于计算偏移量的指令
// 引用时，下标可能是变量
void dimension_cite(ARRAY_MASK *amk)
{
    int expression(symset fsys); // 内部声明expression，使得后面能够正确调用
    dim = 0;                     // 重置当前处理的维度
    int cur_dim_pos;             // 当前维度的下标
    int flag = 0;                // 标志某个维度方括号之间的表达式是否为空
    symset set;
    set = createset(SYM_RSQBRAC, SYM_NULL); // 生成一个包含右方括号的符号集，因为右括号不应当被跳过
    ARRAY_TABLE atb;                        // 用来暂存数组的附加信息
    atb = arytb[amk->AT_address];
    while (sym == SYM_LSQBRAC && dim <= atb.dim)
    {
        getsym();
        if (dim)
        {                                       //// 进入这一分支时，栈顶恰好是到目前维度为止的偏移量
            gen(LIT, 0, atb.dim_size[dim]);     // atb.dim_size[dim]
            gen(OPR, 0, OPR_MUL);               // pos = pos * atb.dim_size[dim]
            cur_dim_pos = expression(set);      // 获取方括号之间的表达式的数值
            flag = cur_dim_pos == DIM_BACKFILL; // 测试是否为空
            if (flag)
            {              // 如果为空，报错
                error(36); //"The subscript of a dimension exopected."
            }
            gen(OPR, 0, OPR_ADD); // pos = pos * atb.dim_size[dim] + cur_dim_pos;
        }
        else
        {
            cur_dim_pos = expression(set);      // 获取方括号之间的表达式的数值
            flag = cur_dim_pos == DIM_BACKFILL; // 测试是否为空
            if (flag)
            {              // 如果为空，报错
                error(36); //"The subscript of a dimension exopected."
            }
        }
        if (sym == SYM_RSQBRAC) // 如果读取到了右方括号括号（正确完成配对）
        {
            dim++;
            getsym(); // 继续读取下一个符号
        }
        else // 没有读取到右括号，报错
        {
            error(37); // Missing ']'.
        }
    }
    if (dim <= atb.dim)
    {              // 因为没匹配到左方括号而结束循环，并且没有读到最后一层维度,目前无法处理
        error(38); //"Not a allowed data type for now."
    }
    gen(LIT, 0, atb.head_address); // atb.head_address
    gen(OPR, 0, OPR_ADD);          // pos += atb.head_address;
    destroyset(set);               // 调用完成，销毁临时的符号集
    return;
}

//////////////////////////////////////////////////////////////////////
// 语法分析部分图1-2中的“因子”
// 处理非终结符“因子”，若出错，则试图恢复到另一个因子的起始点附近
// HB_2022-12-18
// 修改因子、项、表达式，使得这些函数在分析时，若可以计算出常数值，即返回对应的数值。如2+3将在分析时直接返回5
int factor(symset fsys) ////fsys是后继符号集合
{
    int num_of_factor = 0; // 当前分析的因子的数值

    int expression(symset fsys); // 内部声明expression，使得后面能够正确调用
    int i;
    symset set;
    symset set1;
    ARRAY_MASK *amk;

    test(facbegsys, fsys, 24); // 如果当前读入的符号不是factor的开始符号，则会报错，否则继续检查，期间遗漏的后继符号会假想“自动填补”，使得下一个开始处理的位置是某一个因子的开头或者结尾

    if (inset(sym, facbegsys)) // 如果当前处理的符号是因子的开始符号
    {
        if (sym == SYM_IDENTIFIER) // 如果当前符号是标识符
        {
            if ((i = position(id)) == 0) // 标识符不在符号表中
            {
                error(11); // Undeclared identifier.
            }
            else // 标识符在符号表中
            {
                switch (table[i].kind) // 根据标识符的类型做不同处理，其中，访问常量和过程类型时，需要改变访问模式为mask
                {
                    mask *mk;
                case ID_CONSTANT: // 常量情况下，生成一条LIT指令，格式为(LIT,0,常量)，将常量置于栈顶
                    gen(LIT, 0, table[i].value);
                    num_of_factor = table[i].value;
                    getsym(); // 继续读取下一个符号
                    break;
                case ID_VARIABLE: // 变量情况下，生成一条LOD指令，格式为(LOD,层次差,数据地址)，将变量值置于栈顶
                    if (gen_OK)
                    {
                        error(39); //"Declaration CANNOT be done with virables."
                    }
                    mk = (mask *)&table[i];
                    getsym(); // 继续读取下一个符号
                    // 处理赋值表达式
                    if (sym == SYM_BECOMES)
                    {
                        getsym();
                        expression(fsys);
                        gen(STO, level - mk->level, mk->address);
                    }
                    gen(LOD, level - mk->level, mk->address);
                    break;
                case ID_PROCEDURE: // 过程情况下，报错，因为pl0语法中过程名在任何情况下都不能出现在表达式中
                    error(21);     // Procedure identifier can not be in an expression.
                    getsym();      // 继续读取下一个符号
                    break;
                case ID_ARRAY: // 数组
                    if (gen_OK)
                    {
                        error(39); //"Declaration CANNOT be done with virables."
                    }
                    amk = (ARRAY_MASK *)&table[i]; // 获取数组名在符号表中的位置
                    getsym();                      // 获取第一个方括号
                    dimension_cite(amk);           // 处理完维度时，已经读取了下一个符号
                    // 处理赋值表达式
                    if (sym == SYM_BECOMES)
                    {
                        getsym();
                        expression(fsys); // 计算子表达式右值
                        gen(STOA, 0, 0);  // 将栈顶的值，即子表达式右值存入左值，即次栈顶内数组元素
                        gen(INT, 0, 1);   // 恢复原栈顶(左值的偏移)
                    }
                    gen(LODA, level - amk->level, 0); // 生成一条指令，将数组元素的数值置于栈顶
                }                                     // switch
            }
        }
        else if (sym == SYM_NUMBER) // 如果当前符号是数字
        {
            if (num > MAXADDRESS) // 读取到的数字过大
            {
                error(25); // The number is too great.
                num = 0;   // 重置为0
            }
            num_of_factor = num;
            gen(LIT, 0, num); // 数字合法时，生成一条LIT指令，格式为(LIT,0,常量)，将常量置于栈顶
            getsym();         // 继续读取下一个符号
        }
        else if (sym == SYM_LPAREN) // 如果当前符号是左括号，根据pl0文档第五页的图，括号内部必须是表达式
        {
            getsym();                                              // 继续读取下一个符号
            set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys); // 生成一个包含右括号和fsys的符号集
            num_of_factor = expression(set);                       // 调用表达式过程（类比书上的递归向下分析程序）
            destroyset(set);                                       // 调用完成，销毁临时的符号集
            if (sym == SYM_RPAREN)                                 // 如果读取到了右括号（正确完成配对）
            {
                getsym(); // 继续读取下一个符号
            }
            else // 没有读取到右括号，报错
            {
                error(22); // Missing ')'.
            }
        }
        else if (sym == SYM_MINUS) // 如果读取到了减号
        // UMINUS,  Expr -> '-' Expr
        {
            getsym();                      // 继续读取下一个符号
            num_of_factor = -factor(fsys); // 根据pl0文档第5页，减号后必须是一个因子，因此调用因子函数
            gen(OPR, 0, OPR_NEG);          // 生成一条OPR指令，即算术或逻辑运算指令，格式为(OPR,0,运算类别)，这条指令是为了实现减号的作用
        }
        else if (sym == SYM_SET_JUMP)
        {
            getsym();
            if (sym == SYM_LPAREN)
            {
                getsym();
            }
            else
            {
                error(48); // missing '('
            }
            // follow
            set1 = createset(SYM_RPAREN, SYM_NULL);
            set = uniteset(set1, fsys);
            // 分析表达式
            int index = expression(set);
            // 销毁follow
            destroyset(set1);
            destroyset(set);
            if (sym == SYM_RPAREN)
            {
                getsym();
            }
            else
            {
                error(49); // missing ')'
            }
            gen(SET_JUMP, 1, index); // 使用0，1区分是否需要返回值，这涉及到是否修改栈顶
        }
        else if (sym == SYM_RANDOM)
        {
            getsym();
            if (sym != SYM_LPAREN)
            {
                error(53);
            }
            else
            {
                getsym();
            }

            if (sym == SYM_RPAREN)
            {
                getsym();
                gen(LIT, 0, 1);
                num_of_factor = rand();
                gen(RDM, 0, -1);
            }
            else
            {
                int ret = expression(uniteset(fsys, createset(SYM_RPAREN, SYM_NULL)));
                num_of_factor = rand() % ret;
                gen(RDM, 0, ret);
                getsym();
            }
        }
        test(fsys, createset(SYM_LPAREN, SYM_NULL), 23); // 匹配后继符号，若成功则说明处理正确到达尾部，否则报错并试图通过填补假想的左括号，从而从另一个因子（表达式的首个位置为项，项的首个位置为因子）的处恢复分析
    }                                                    // if
    else if (sym == SYM_RSQBRAC)
    {                                 // 方括号内部无内容
        num_of_factor = DIM_BACKFILL; // 暂时填入一个特殊数值，等待回填
    }
    return num_of_factor;
} // factor

//////////////////////////////////////////////////////////////////////
// 语法分析部分图1-2中的“项”
int term(symset fsys)
{
    int num_of_term; // 当前分析的项的数值
    int temp;        // 用于记录数值的临时变量
    int mulop;
    symset set;
    // 精心设计一个用于因子的后继符号集合，其中除了项的后继符号，还引入了项的结构中跟在因子后面的后继符号{*,/}
    set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));
    // 为什么这么设计呢？因为facter函数的末尾有差错检查，会使得处理完因子后，sym要么是后继符号，要么是(
    // 此时所处的位置要么是因子的结尾（正确处理完毕），要么是项的起始位置（假想）
    // 项的起始位置恰好是一个因子的起始位置，该因子恰好对应项中在乘号除号之后的因子，于是恢复到了可以继续分析的位置
    num_of_term = factor(set);                   // 项中第一个因子
    while (sym == SYM_TIMES || sym == SYM_SLASH) // 识别因子之后，后面的符号只能是乘号或者斜杠
    {
        mulop = sym;            // 记录上一个符号，因为读取完下一个符号之后还需要知道做什么运算
        getsym();               // 读取下一个符号，这个符号也必须是因子的一部分
        temp = factor(set);     // 用同样的方式检查因子
        if (mulop == SYM_TIMES) // 如果是乘法，则生成一条乘法指令，格式为(OPR,0,运算类别)
        {
            num_of_term *= temp;
            gen(OPR, 0, OPR_MUL);
        }
        else // 如果是除法，则生成一条除法指令，格式为(OPR,0,运算类别)
        {
            num_of_term /= temp;
            gen(OPR, 0, OPR_DIV);
        }
    }                // while
    destroyset(set); // 销毁临时生成的符号集
    return num_of_term;
} // term

//////////////////////////////////////////////////////////////////////
////语法分析部分图1-2中的“表达式”
int expression(symset fsys)
{
    int num_of_expr; // 当前分析的表达式的数值
    int temp;        // 用于记录数值的临时变量
    int addop;
    symset set;

    set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL)); // 与上面“项”过程的set是一样的设计

    num_of_expr = term(set);                    // 检查第一个项，参见pl0文档第四页的最后一张图
    while (sym == SYM_PLUS || sym == SYM_MINUS) // 后面的符号只能是加或减
    {
        addop = sym;           // 记录运算类型
        getsym();              // 读取下一个符号
        temp = term(set);      // 运算符号后必须是一个项，再次检查
        if (addop == SYM_PLUS) // 如果当前符号是加号，则生成一条汇编加法指令，格式为(OPR,0,运算类型)
        {
            num_of_expr += temp;
            gen(OPR, 0, OPR_ADD);
        }
        else // 否则生成一条汇编减法指令，格式为(OPR,0,运算类型)
        {
            num_of_expr -= temp;
            gen(OPR, 0, OPR_MIN);
        }
    }                // while
    destroyset(set); // 销毁临时生成符号集合
    return num_of_expr;
} // expression

//////////////////////////////////////////////////////////////////////
// 语法分析部分图1-2中的“条件”
void condition(symset fsys)
{
    int relop;
    symset set;

    if (sym == SYM_ODD) // 识别到保留字odd，后面必须是一个表达式
    {
        getsym();         // 获取下一个符号，这个符号必须是表达式的开始符号
        expression(fsys); // 调用表达式过程
        gen(OPR, 0, 6);   // OPR_ODD=6，生成一条ODD指令，格式为(OPR,0,运算类型)
    }
    else // 如果首个符号不是odd,那必须是一个表达式的开始符号
    {
        set = uniteset(relset, fsys); // 将条件运算符号加入后继符号集合
        // 这个设计思想与上面也是一样的，每向上归约一层，为了能正确恢复，都要加上新一层中新增的后继符号
        expression(set);         // 检查表达式
        destroyset(set);         // 销毁临时集合
        if (!inset(sym, relset)) // 下一个符号如果不是条件运算符，则报错
        {
            error(20);
        }
        else
        {
            relop = sym;      // 记录条件运算的类型
            getsym();         // 获取下一个符号，这个符号必须是表达式的开始符号
            expression(fsys); // 检查表达式
            switch (relop)    // 根据条件运算的类型生成不同的条件运算指令，格式都是(OPR,0,运算类型)
            {
            case SYM_EQU:
                gen(OPR, 0, OPR_EQU);
                break;
            case SYM_NEQ:
                gen(OPR, 0, OPR_NEQ);
                break;
            case SYM_LES:
                gen(OPR, 0, OPR_LES);
                break;
            case SYM_GEQ:
                gen(OPR, 0, OPR_GEQ);
                break;
            case SYM_GTR:
                gen(OPR, 0, OPR_GTR);
                break;
            case SYM_LEQ:
                gen(OPR, 0, OPR_LEQ);
                break;
            } // switch
        }     // else
    }         // else
} // condition

//////////////////////////////////////////////////////////////////////
// 语句
void statement(symset fsys)
{
    int i, cx1, cx2;
    symset set1, set, set_print;
    ARRAY_MASK *amk;
    set_print = createset(SYM_COMMA, SYM_RPAREN, SYM_NULL); // 用于print函数的错误恢复
    set_print = uniteset(fsys, set_print);
    statement_init = 1;
    if (sym == SYM_IDENTIFIER) // 识别到左值为标识符，开始处理变量赋值语句
    {
        mask *mk;
        if (!(i = position(id))) // 标识符不在符号表中
        {
            error(11); // Undeclared identifier.
        }
        // else if (table[i].kind != ID_VARIABLE) // 标识符在符号表中，但不是变量类型
        // {
        // 	error(12); // Illegal assignment.
        // 	i = 0;	   // 为i赋为0，使得所有错误情况i的数值都保持一致
        // }
        else if (table[i].kind == ID_VARIABLE)
        {
            getsym();               // 读取下一个符号，这个符号必须是赋值等号
            if (sym == SYM_BECOMES) // 读取到赋值等号
            {
                getsym(); // 继续读取下一个符号
            }
            else // 没取到赋值等号，报错
            {
                error(13); // ':=' expected.
            }
            expression(fsys);
            // 经过上面的处理，此时如果前面的部分合法，i是符号表中的匹配表项下标，否则i会被自动赋为0
            mk = (mask *)&table[i]; // 取存放变量属性的空间（实际上就是同一个表项，这样做是为了通过不同格式正确访问到变量的属性
            if (i)                  // 如果前面的部分合法，那么生成一条存储指令，格式为(STO,层次差,数据地址)
            {
                gen(STO, level - mk->level, mk->address); // 注意，表项中存储的是层次，但指令装载的应当是层次差
            }
        }
        else if (table[i].kind == ID_ARRAY)
        { // 开始处理数组
            getsym();
            amk = (ARRAY_MASK *)&table[i];
            dimension_cite(amk);
            if (sym == SYM_BECOMES) // 读取到赋值等号
            {
                getsym(); // 继续读取下一个符号
            }
            else // 没取到赋值等号，报错
            {
                error(13); // ':=' expected.
            }
            expression(fsys);
            if (i)
            {
                gen(STOA, level - amk->level, 0); // 生成汇编语句为数组元素赋值
            }
        }
        else
        {              // 变量和数组元素以外的数据类型不允许作为赋值语句的左值
            error(12); //"Illegal assignment."
        }
    }
    else if (sym == SYM_CALL) // 识别到保留字call，开始处理过程引用
    {
        getsym();                  // 读取下一个符号，这个符号必须是过程的标识符
        if (sym != SYM_IDENTIFIER) // 如果没有读取到标识符，报错
        {
            error(14); // There must be an identifier to follow the 'call'.
        }
        else // 识别到标识符，继续处理
        {
            if (!(i = position(id))) // 识别到的标识符不在符号表中
            {
                error(11); // Undeclared identifier.
            }
            else if (table[i].kind == ID_PROCEDURE) // 识别到过程名
            {
                mask *mk;
                mk = (mask *)&table[i];                   // 切换到变量和过程的访问方式
                gen(CAL, level - mk->level, mk->address); // 生成一条过程调用指令，格式为(CAL,层次差,程序地址)
            }
            else // 识别到的标识符的类型不为过程
            {
                error(15); // "Only a procedure can be called."
            }
            getsym(); // 继续读取下一个符号
        }
    }
    else if (sym == SYM_IF) // 识别到保留字if，开始处理条件判断语句
    {
        getsym(); // 获取下一个符号，这个符号必须是条件的开始符号
        // To be done: 为什么加入do？
        set1 = createset(SYM_THEN, SYM_DO, SYM_NULL); // 将条件的后继符号then和do加入后继符号集合
        // 这样设计可以形成后继符号被遗漏也能正常继续分析的效果，请见pl0文档13-15页
        set = uniteset(set1, fsys);
        condition(set);   // 处理条件
        destroyset(set1); // 销毁临时符号集
        destroyset(set);
        if (sym == SYM_THEN) // 下一个符号只能是保留字then
        {
            getsym(); // 如果成功识别then，则继续读取下一个符号，这个符号必须是语句的开始符号
        }
        else // 否则报错
        {
            error(16); // 'then' expected.
        }
        // 跳转中使用回填技术，先记录跳转指令的指令地址cx，接着生成一条无目标的JPC指令，等待后续回填
        cx1 = cx;
        gen(JPC, 0, 0);  // 生成一条无目标的JPC指令，格式为(JPC,0,程序地址)
        statement(fsys); // 处理语句
        if (sym == SYM_SEMICOLON)
        {
            getsym();
        }
        else
        {
            error(10); //"';' expected."
        }
        if (sym == SYM_ELSE) // 添加else
        {
            cx2 = cx;
            gen(JMP, 0, 0); // then的结尾跳过else部分
            code[cx1].a = cx;
            getsym();
            statement(fsys);
            code[cx2].a = cx;
        }
        else
        {
            ifelse = 1; // 作用：跳过下一次getsym 因为添加else后比原先多执行了一次
            code[cx1].a = cx;
            getsym(); // 恢复现场
        }
    }
    // 以下的一段else if是pl0文档第15也错误诊断处理中关于镇定规则的示例
    // 这一段的处理方法可以形成“分析时好像自动补上了遗漏的分号”的效果
    // 为什么呢？因为分析到因子的结尾时，归约到语句之后，如果没有分号，直接识别到分号之后的内容也可以恢复分析过程
    else if (sym == SYM_BEGIN) // 识别到保留字begin，开始处理复合语句（block）（语句序列）
    {
        getsym();                                           // 读取下一个符号，这个符号必须是语句的开始符号
        set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL); // 将语句的后继符号分号以及复合语句的后继符号end加入后继符号集合
        set = uniteset(set1, fsys);
        statement(set);                                        // 处理语句
        while (sym == SYM_SEMICOLON || inset(sym, statbegsys)) // 如果下一个符号是分号或者语句的开始符号
        {
            if (sym == SYM_SEMICOLON) // 如果是分号
            {
                getsym(); // 继续读取下一个符号，继续处理下一个语句
            }
            else // 如果是语句的开始符号，说明遗漏了分号
            {
                error(10); //"';' expected."
            }
            statement(set); // 处理语句
        }                   // while
        destroyset(set1);   // 销毁临时生成的符号集合
        destroyset(set);
        if (sym == SYM_END) // 识别到复合语句的结束标志，保留字end
        {
            getsym(); // 继续读取下一个符号
        }
        else // 识别错误，则可能是end被遗漏，或最后一个语句没有加上分号
        {
            error(17); // ';' or 'end' expected.
        }
    }
    else if (sym == SYM_WHILE) // 识别到保留字while，开始处理条件循环语句
    {
        cx1 = cx;                           // 这里并非需要回填技术，而是记录条件判断部分的指令地址，以便循环
        getsym();                           // 获取下一个符号，这个符号必须是条件的开始符号
        set1 = createset(SYM_DO, SYM_NULL); // 将while语句的后继符号加入后继符号集合
        set = uniteset(set1, fsys);         //
        condition(set);                     // 处理条件
        destroyset(set1);                   // 销毁临时生成的符号集合
        destroyset(set);
        cx2 = cx;          // 第二条跳转指令的地址，等待回填
        gen(JPC, 0, 0);    // 生成一条无目标的跳转指令
        if (sym == SYM_DO) // 下一个符号如果是do，则读取下一个符号
        {
            getsym(); // 读取下一个符号，这个符号必须是语句的开始符号
        }
        else // 否则do缺失，报错
        {
            error(18); // 'do' expected.
        }
        statement(fsys);  // do后的处理语句
        gen(JMP, 0, cx1); // 生成一条JMP（无条件）跳转指令，格式为(JMP,0,程序地址)，跳转回条件判断部分
        code[cx2].a = cx; // 回填跳转指令的目标地址（循环结束处）
    }
    else if (sym == SYM_FOR) // 识别到保留字 for, 开始处理 for 循环语句
    {
        getsym();
        if (sym != SYM_LPAREN)
        {
            error(53);
        }
        else
        {
            getsym();
        }

        if (sym != SYM_VAR)
        {
            error(54);
        }
        else
        {
            getsym();
        }

        int rec_pos = -1;
        if (sym != SYM_IDENTIFIER)
        {
            error(4);
        }
        else
        {
            enter(ID_VARIABLE);
            rec_pos = position(id);
            gen(INT, 0, 1);
            getsym();
        }

        if (sym != SYM_COLON)
        {
            error(55);
        }
        else
        {
            getsym();
        }

        if (sym != SYM_LPAREN)
        {
            error(53);
        }
        else
        {
            getsym();
        }

        symset set_for_1, set_for_2;
        set_for_1 = createset(SYM_COMMA, SYM_NULL);
        set_for_2 = uniteset(fsys, set_for_1);
        int low = expression(set_for_2);
        destroyset(set_for_1);
        destroyset(set_for_2);

        mask *mk = (mask *)&table[rec_pos];
        gen(STO, level - mk->level, mk->address);
        cx1 = cx;
        gen(LOD, level - mk->level, mk->address);
        getsym();

        set_for_1 = createset(SYM_COMMA, SYM_RPAREN, SYM_NULL);
        set_for_2 = uniteset(fsys, set_for_1);
        int up = expression(set_for_2);
        destroyset(set_for_1);
        destroyset(set_for_2);

        if (low < up)
        {
            gen(OPR, 0, OPR_LES);
        }
        else
        {
            gen(OPR, 0, OPR_GTR);
        }
        cx2 = cx;
        gen(JPC, 0, 0);

        int step_value = 1;
        if (sym != SYM_RPAREN)
        {
            // given step
            getsym();
            if (sym == SYM_MINUS)
            {
                getsym();
                step_value = -num;
            }
            else
            {
                step_value = num;
            }

            getsym();
        }

        getsym();
        if (sym != SYM_RPAREN)
        {
            error(22);
        }
        else
        {
            getsym();
        }

        statement(fsys);

        if (step_value >= 0)
        {
            gen(LIT, 0, step_value);
        }
        else
        {
            gen(LIT, 0, -step_value);
            gen(OPR, 0, OPR_NEG);
        }
        gen(LOD, level - mk->level, mk->address);
        gen(OPR, 0, OPR_ADD);
        gen(STO, level - mk->level, mk->address);
        gen(JMP, 0, cx1);
        code[cx2].a = cx;
        gen(INT, 0, -1);
        tx--;
        // must 恢复现场
    }
    else if (sym == SYM_PRINT)
    { // 检测到内置函数print
        getsym();
        if (sym == SYM_LPAREN)
        { // 检测到左括号
            getsym();
            if (sym == SYM_RPAREN)
            {
                gen(PRT, 0, 0); // 特殊的PRT指令，表示打印换行符
                getsym();
            }
            else
            {
                expression(set_print);
                gen(PRT, 0, 1); // 标准的PRT指令，表示打印出运行时栈顶的内容
                while (sym == SYM_COMMA)
                { // 循环处理所有需要打印的内容
                    getsym();
                    expression(set_print);
                    gen(PRT, 0, 1); // 标准的PRT指令，表示打印出运行时栈顶的内容
                }
                if (sym != SYM_RPAREN)
                {
                    error(22); //"Missing ')'."
                }
                else
                {
                    getsym();
                }
            }
        }
        else
        {              // 函数名后没有括号，此时视为函数指针，这是暂时不能处理的数据类型
            error(38); //"Not a allowed data type for now."
        }
    }
    else if (sym == SYM_SET_JUMP)
    {
        // statement情况不需要使用到setjump返回值，需要的是factor的情况
        getsym();
        if (sym == SYM_LPAREN)
        {
            getsym();
        }
        else
        {
            error(48); // missing '('
        }
        // follow
        set1 = createset(SYM_RPAREN, SYM_NULL);
        set = uniteset(set1, fsys);
        // 分析表达式
        int index = expression(set);
        // 销毁follow
        destroyset(set1);
        destroyset(set);
        if (sym == SYM_RPAREN)
        {
            getsym();
        }
        else
        {
            error(49); // missing ')'
        }
        gen(SET_JUMP, 0, index);
    }
    else if (sym == SYM_LONG_JUMP)
    {
        getsym();
        if (sym == SYM_LPAREN)
        {
            getsym();
        }
        else
        {
            error(50);
        }
        set1 = createset(SYM_COMMA, SYM_NULL);
        set = uniteset(set1, fsys);
        int index = expression(set);
        destroyset(set1);
        destroyset(set);
        if (sym == SYM_COMMA)
        {
            getsym();
        }
        else
        {
            error(51);
        }
        set1 = createset(SYM_RPAREN, SYM_NULL);
        set = uniteset(set1, fsys);
        int value = expression(set);
        destroyset(set1);
        destroyset(set);

        if (sym == SYM_RPAREN)
        {
            getsym();
        }
        else
        {
            error(52); // missing ')'
        }
        gen(LONG_JUMP, index, value);
    }
    else if (inset(sym, declbegsys))
    {
        if (sym == SYM_CONST) // 常量声明
        {
            getsym(); // 获取下一个符号
            do        // 当检测到标识符时，重复处理可能连续出现的常量声明
            {
                constdeclaration();      // 调用常量声明处理
                while (sym == SYM_COMMA) // 当检测到逗号时，说明存在多个并列的常量声明，全部处理
                {
                    getsym();
                    constdeclaration();
                }
                if (sym == SYM_SEMICOLON) // 检测到分号时，声明结束，获取下一个符号
                {
                    break;
                }
                else // 常量声明的结尾只能是逗号或分号，未检测到则报错
                {
                    error(5); // Missing ',' or ';'.
                }
            } while (sym == SYM_IDENTIFIER);
        } // if

        if (sym == SYM_VAR) // 变量声明，与常量声明的设计类似
        {
            getsym();
            do
            {
                vardeclaration();
                while (sym == SYM_COMMA)
                {
                    getsym();
                    vardeclaration();
                }
                if (sym == SYM_SEMICOLON)
                {
                    break;
                }
                else
                {
                    error(5); // Missing ',' or ';'.
                }
            } while (sym == SYM_IDENTIFIER);
        }
        if (sym == SYM_PROCEDURE) // 过程声明，直接报错
        {
            error(46); //"CANNOT decalare a procedure in statement."

        } // while
    }
    destroyset(set_print);
    statement_init = 0;
    test(fsys, phi, 19); // 处理完语句之后，测试当前符号是否是后继符号，，若是说明处理正确，若不是则报错，并运用镇定规则跳过错误部分，直到遇到可以识别的符号
} // statement

//////////////////////////////////////////////////////////////////////
void init_code_insert(int start_init, int end_init)
{
    int i;
    for (i = start_init; i <= end_init; i++)
    {
        code[cx].f = code_init[i].f;
        code[cx].l = code_init[i].l;
        code[cx].a = code_init[i].a;
        cx++;
    }
    return;
}
// 程序体
void block(symset fsys)
{
    int cx0; // initial code index
    mask *mk;
    int block_dx;
    int start_init = cx_init;
    int savedTx;
    symset set1, set;

    dx = 3;        // 前面三个位置分别保留给了RA,DL,SL
    block_dx = dx; // 保存辅助分配变量地址的指针
    // 为什么要给block_dx赋予初始值呢？
    // 因为下面的循环中对block_dx的赋值不一定会执行
    mk = (mask *)&table[tx]; // 以过程的形式访问符号表
    mk->address = cx;        // 将程序地址（需要执行的第一条代码的下标）填入符号表
    gen(JMP, 0, 0);          // 生成一条无条件跳转指令，跳转目标地址等待回填
    if (level > MAXLEVEL)    // 层次差过大，报错
    {
        error(32); // There are too many levels.
    }
    // 以下开始按pl0文档中的语法图处理
    do // 当读入的符号是声明的开始符号时
    {
        if (sym == SYM_CONST) // 常量声明
        {
            getsym(); // 获取下一个符号
            do        // 当检测到标识符时，重复处理可能连续出现的常量声明
            {
                constdeclaration();      // 调用常量声明处理
                while (sym == SYM_COMMA) // 当检测到逗号时，说明存在多个并列的常量声明，全部处理
                {
                    getsym();
                    constdeclaration();
                }
                if (sym == SYM_SEMICOLON) // 检测到分号时，声明结束，获取下一个符号
                {
                    getsym();
                }
                else // 常量声明的结尾只能是逗号或分号，未检测到则报错
                {
                    error(5); // Missing ',' or ';'.
                }
            } while (sym == SYM_IDENTIFIER);
        } // if

        if (sym == SYM_VAR) // 变量声明，与常量声明的设计类似
        {
            getsym();
            do
            {
                vardeclaration();
                while (sym == SYM_COMMA)
                {
                    getsym();
                    vardeclaration();
                }
                if (sym == SYM_SEMICOLON)
                {
                    getsym();
                }
                else
                {
                    error(5); // Missing ',' or ';'.
                }
            } while (sym == SYM_IDENTIFIER);
        }
        block_dx = dx;               // 调用过程之前保存辅助分配变量地址的指针，用于恢复现场
        while (sym == SYM_PROCEDURE) // 过程声明，处理可能连续出现的过程声明
        {
            getsym();                  // 获取下一个符号
            if (sym == SYM_IDENTIFIER) // 检测到下一个符号是标识符
            {
                enter(ID_PROCEDURE); // 将声明的新过程加入符号表
                getsym();            // 继续读取下一个符号
            }
            else // 否则报错
            {
                error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
            }

            if (sym == SYM_SEMICOLON) // 检测到分号
            {
                getsym(); // 继续获取下一个符号
            }
            else // 否则报错
            {
                error(5); // Missing ',' or ';'.
            }

            level++;      // 当前所在的层次增加1（进入更深一层的活动记录）
            savedTx = tx; // 保存当前的符号表表项下标，用于恢复现场
            // 与之前的处理是相同的思路
            // 将程序体的后继符号分号加入后继符号集合，便于差错恢复
            // 其效果类似于编译器自动填上了一个假想的分号
            set1 = createset(SYM_SEMICOLON, SYM_NULL);
            set = uniteset(set1, fsys);
            block(set);       // 处理过程声明定义的函数的函数体
            destroyset(set1); // 销毁临时生成的符号集合
            destroyset(set);
            tx = savedTx; // 恢复之前的符号表下标
            // To be done: tx是顺序使用的，如果恢复到之前的下标，如果父过程需要再使用tx，那不就会覆盖子过程的符号表表项吗？
            // To be done: 另外，这个编译器似乎并没有考虑变量重定义的问题。
            level--; // 过程声明处理完毕，回到父过程，层次减少1（回到更上一层的活动记录）

            // 跟在程序体后的分号缺失问题没有使用镇定规则
            // To be done: 为什么不填补分号丢失，反而要在检测到分号的情况下再做test?
            if (sym == SYM_SEMICOLON) // 检测到分号，这个分号应当是跟在程序体之后的分号，参见pl0文档中的语法图
            {
                getsym(); // 读取下一个符号
                // 注意，这里过程声明不属于“语句”
                set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL); // 将连续过程声明中可能出现在分号之后的终结符加入后继符号集合
                set = uniteset(statbegsys, set1);                          // 将语句的开始符号加入后继符号集合
                // ident和procedure两个终结符恢复分析的位置不同，procedure试图恢复到下一个过程声明，而ident和statbegsys试图恢复到下一个语句
                // ident对应语句中的赋值语句
                // To be done:为什么不把声明的开始符号也放进去？
                test(set, fsys, 6); //"Incorrect procedure name."
                destroyset(set1);   // 销毁临时生成的符号集合
                destroyset(set);
            }
            else // 缺少分号，直接报错
            {
                error(5); // Missing ',' or ';'.
            }
        }              // while
        dx = block_dx; // 恢复现场，回到父过程的变量空间
    } while (inset(sym, declbegsys));
    // 以下语句移到循环外侧，解除const-var-procedure的类型声明顺序限制
    //  将语句的开始符号加入后继符号集合中
    set1 = createset(SYM_IDENTIFIER, SYM_NULL);
    set = uniteset(statbegsys, set1);
    // 试图恢复到下一个语句或声明的位置
    test(set, declbegsys, 7); //"Statement expected."
    destroyset(set1);         // 销毁临时生成的变量集合
    destroyset(set);
    int end_init = cx_init;
    code[mk->address].a = cx; // 回填上面JMP指令的跳转地址
    // cx似乎无变化，有必要使用回填技术吗
    // 有，因为block会改变
    mk->address = cx; // 记录的跳转的目标地址，这才是本过程的起点，下面的“语句”就是过程的内容
    cx0 = cx;         // 起始地址标记为此时的cx
    // 此时block_dx是程序中声明的变量、常量等总共将要占用的空间
    gen(INT, 0, block_dx);                              // 在程序体的起始位置加入一条INT指令，格式为(INT,0,常量)，作用是为程序开辟空间
    init_code_insert(start_init, end_init);             // 在INT后，函数体之前，插入本函数体初始化所需的代码
    set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL); // 将语句序列的后继符号分号与end加入后继符号集合
    set = uniteset(set1, fsys);
    statement(set);   // 处理语句，对应语法图最下方的“语句”
    destroyset(set1); // 销毁临时生成的符号集合
    destroyset(set);
    gen(OPR, 0, OPR_RET); // 在程序末尾插入一条RET指令，格式为(OPR,0,运算类型)，作用是return到父过程
    // 最后，检查是否处理正确，这次没有添加追加符号，因为该程序体理论上后面不再有代码，无需按镇定规则恢复
    test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
    listcode(cx0, cx);  // 打印出为程序体生成的所有汇编代码
} // block

//////////////////////////////////////////////////////////////////////
// 根据层次差和当前层次，从栈中寻找目标所在的数据区地址
int base(int stack[], int currentLevel, int levelDiff)
{
    int b = currentLevel;

    while (levelDiff--)
        b = stack[b];
    return b;
} // base

//////////////////////////////////////////////////////////////////////
// 代码执行
//  interprets and executes codes.
void interpret()
{
    int addr; // 临时变量
    int pc;   // program counter
    int stack[STACKSIZE];
    int top;       // top of stack
    int b;         // program, base, and top-stack register
    instruction i; // instruction register

    printf("Begin executing PL/0 program.\n");

    pc = 0;
    b = 1;   // b用于存放基地址
    top = 3; // 前三个位置是留给RA,DL,SL的
    stack[1] = stack[2] = stack[3] = 0;
    do // 执行代码，直到执行完毕
    {
        i = code[pc++]; // 读取下一条指令
        switch (i.f)    // 跟据汇编操作码执行不同操作
        {
        case LIT: // 将常数置于栈顶
            stack[++top] = i.a;
            break;
        case OPR:        // 一组算术或逻辑运算
            switch (i.a) // i.a中存放着运算类型
            {
            case OPR_RET:            // RET，返回父过程，这里的操作与CAL指令对应
                top = b - 1;         // top回到调用前的位置
                pc = stack[top + 3]; // RA装填入PC寄存器
                b = stack[top + 2];  // 由DL获取调用者的基地址
                break;
            case OPR_NEG: // NEG，栈顶值取反并存于原位置
                stack[top] = -stack[top];
                break;
            case OPR_ADD: // ADD，栈顶两值相加并存于次栈顶
                top--;
                stack[top] += stack[top + 1];
                break;
            case OPR_MIN: // MIN，次栈顶值减去栈顶值并存于次栈顶
                top--;
                stack[top] -= stack[top + 1];
                break;
            case OPR_MUL: // MUL，栈顶两值相乘并存于次栈顶
                top--;
                stack[top] *= stack[top + 1];
                break;
            case OPR_DIV: // DIV，次栈顶值除以栈顶值并存于次栈顶
                top--;
                if (stack[top + 1] == 0) // 被除数为0时报错
                {
                    fprintf(stderr, "Runtime Error: Divided by zero.\n");
                    fprintf(stderr, "Program terminated.\n");
                    continue;
                }
                stack[top] /= stack[top + 1];
                break;
            case OPR_ODD: // ODD，栈顶值模2并存于原位置
                stack[top] %= 2;
                break;
            case OPR_EQU: // EQU，判断栈顶值与次栈顶值是否相等，结果存于次栈顶，是则为1，否则为0
                top--;
                stack[top] = stack[top] == stack[top + 1];
                break;
            case OPR_NEQ: // NEQ，判断栈顶值与次栈顶值是否不相等，结果存于次栈顶，是则为1，否则为0
                top--;
                stack[top] = stack[top] != stack[top + 1];
                break;
            case OPR_LES: // LES，判断次栈顶值是否小于栈顶值，结果存于次栈顶，是则为1，否则为0
                top--;
                stack[top] = stack[top] < stack[top + 1];
                break;
            case OPR_GEQ: // GEQ，判断次栈顶值是否大于等于栈顶值，结果存于次栈顶，是则为1，否则为0
                top--;
                stack[top] = stack[top] >= stack[top + 1];
                break;
            case OPR_GTR: // GTR，判断次栈顶值是否大于栈顶值，结果存于次栈顶，是则为1，否则为0
                top--;
                stack[top] = stack[top] > stack[top + 1];
                break;
            case OPR_LEQ: // LEQ，判断次栈顶值是否小于等于栈顶值，结果存于次栈顶，是则为1，否则为0
                top--;
                stack[top] = stack[top] <= stack[top + 1];
                break;
            } // switch
            break;
        case LOD: // LOD,将变量值置于栈顶
            stack[++top] = stack[base(stack, b, i.l) + i.a];
            break;
        case STO: // STO,将栈顶的值赋予某变量，并将这个值从栈中删去
            stack[base(stack, b, i.l) + i.a] = stack[top];
            // printf("%d\n", stack[top]); // To be done: 赋值需要打印吗？
            top--;
            break;
        case CAL:                                 // CAL,过程调用,这里的操作与RET指令对应
            stack[top + 1] = base(stack, b, i.l); // 新过程的SL指向对象，即指向定义者的基地址
            // generate new block mark
            stack[top + 2] = b;  // 新过程的DL的指向对象，即指向调用者的基地址
            stack[top + 3] = pc; // 保存RA
            b = top + 1;         // 从栈顶的下一个位置开始，是引用过程的空间
            pc = i.a;
            break;
        case INT:       // INT,在数据栈中分配存储空间
            top += i.a; // 直接将栈顶指针移动指定的单位（相当于腾出空间）
            // 移动栈顶指针来开辟空间有意义吗？栈指针当且仅当返回或栈中有数值时才可能减小
            //  在函数的初始状态下，腾出来的空间真的会被用到吗？
            // 可以，符号表中记录的地址相当于记录好变量等在运行时栈中的位置，这也是为什么dx和top都从3开始
            // 符号表相当于运行时栈的一比一模拟
            // 但不同的是，数组的名字会占用符号表空间，但不会占用运行时栈的空间
            break;
        case JMP:     // JMP,无条件跳转到目标地址
            pc = i.a; // 向PC中载入目标地址
            break;
        case JPC: // JPC,条件跳转，满足条件时执行跳转
            if (stack[top] == 0)
                pc = i.a;
            top--; // 同时要把栈条件判断的结果弹栈
            break;
        case PRT: // PRT指令,打印换行符,或打印栈顶内容并将该内容弹栈
            if (!i.a)
            { // 打印换行符
                printf("\n");
            }
            else
            {
                printf("%d ", stack[top--]);
            }
            break;
        case RDM:
            if (i.a <= 0)
            {
                stack[top] = rand();
            }
            else
            {
                stack[top] = rand() % i.a;
            }
            break;
        case LODA: // LODA指令,“间接读”指令LODA表示以当前栈顶单元的内容为“地址偏移”来读取相应单元的值,格式为(LODA,层次差,0),并将该值存储到原先的栈顶单元中
            addr = base(stack, b, i.l) + stack[top];
            if (addr > top) // 访问越界
            {
                fprintf(stderr, "Runtime Error: Segmentation Fault.\n");
                fprintf(stderr, "Program terminated.\n");
                continue;
            }
            stack[top] = stack[addr]; // 以栈顶单元的值作为偏移量，读取变量的值到原栈顶的位置
            break;
        case STOA: // STOA指令,“间接写”指令STOA则将位于栈顶单元的内容,存入到次栈顶单元内容所代表的栈单元里,然后弹出栈顶和次栈顶。
            addr = base(stack, b, i.l) + stack[top - 1];
            if (addr > top) // 访问越界
            {
                fprintf(stderr, "Runtime Error: Segmentation Fault.\n");
                fprintf(stderr, "Program terminated.\n");
                continue;
            }
            stack[addr] = stack[top];
            top -= 2;
            break;
        case SET_JUMP:
            jmp_buf[i.a].pc = pc;
            jmp_buf[i.a].top = top;
            jmp_buf[i.a].base = b;
            if (i.l == 1)
            {
                // 需要使用返回值，生成返回值放于栈顶 使用set调用返回值是0
                // 只有作为factor才会需要返回值，statement是不需要的
                stack[++top] = 0;
            }
            break;
        case LONG_JUMP:
            pc = jmp_buf[i.l].pc;
            top = jmp_buf[i.l].top;
            b = jmp_buf[i.a].base;
            stack[++top] = i.a; // 新的返回值
            // 和之前的情况对比就是返回值不一样了
            break;
        } // switch
        if (single_step)
        {
            for (int i = 0; i <= top; i++)
            {
                printf("stack[%d]: %d\n", i, stack[i]);
            }
            system("pause");
        }
    } while (pc);

    printf("\n");
    printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
int main()
{
    FILE *hbin;
    char s[80];
    int i;
    symset set, set1, set2;
    srand((unsigned)time(NULL));

    if ((infile = fopen("test.txt", "r")) == NULL)
    {
        printf("File %s can't be opened.\n", s);
        exit(1);
    }

    printf("Single step mode? 0 for no,otherwise yes.\n");
    scanf("%d", &single_step);

    phi = createset(SYM_NULL);
    relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);

    // create begin symbol sets
    declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
    statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE, SYM_FOR, SYM_PRINT, SYM_SET_JUMP, SYM_LONG_JUMP, SYM_NULL); // jk 2022-12-27 add for and print
    facbegsys = createset(SYM_RANDOM, SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_SET_JUMP, SYM_NULL);              // mahiru 2022-12-26 add setjmp; jk 2022-12-27 add random

    statbegsys = uniteset(statbegsys, declbegsys);

    err = cc = cx = ll = 0; // initialize global variables
    ch = ' ';
    kk = MAXIDLEN;
    cx_init = 0;

    getsym();

    // 这里后继符号集合为{空，句号}，这是根据语法图得到的
    set1 = createset(SYM_PERIOD, SYM_NULL);
    // To be done:
    // 追加的后继符号集合为声明的开始符号以及语句的开始符号，为什么？应当不需要错误恢复，因为恢复的方法已在子过程中设定
    set2 = uniteset(declbegsys, statbegsys);
    set = uniteset(set1, set2);
    block(set);

    // 执行完毕，销毁
    destroyset(set1);
    destroyset(set2);
    destroyset(set);
    destroyset(phi);
    destroyset(relset);
    destroyset(declbegsys);
    destroyset(statbegsys);
    destroyset(facbegsys);

    // 检查程序的最后一个符号
    if (sym != SYM_PERIOD) // 不是句点，报错
        error(9);          // '.' expected.
    if (err == 0)          // 当没有任何错误时，生成hbin文件
    {
        // To be done: .txt文件用fwrite写入？
        // 为什么不写入汇编代码，而是录入属性呢？是为了避免字符处理吗？
        hbin = fopen("hbin.txt", "w");
        for (i = 0; i < cx; i++)
            fwrite(&code[i], sizeof(instruction), 1, hbin);
        fclose(hbin);
    }
    // To be done: 为什么不和上面的if放在同一个大括号下？
    if (err == 0)
        interpret(); // 执行生成的汇编代码
    else             // 否则告知错误总数
        printf("There are %d error(s) in PL/0 program.\n", err);
    listcode(0, cx); // 打印生成的所有代码
    system("pause"); // 暂停，并供用户阅读
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
