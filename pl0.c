// pl0 compiler source code
//Xulong's_contribution:注释
#pragma warning(disable:4996)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "PL0.h"
#include "set.c"
void array_access(short arr_index, int dim, symset fsys);
void expression(symset fsys);
//输入错误的索引打印错误信息，并且让err变量加一,不会结束进程
// print error message.
void error(int n)
{
	int i;

	printf("      ");
	for (i = 1; i <= cc - 1; i++)
		printf(" ");
	printf("^\n");
	printf("Error %3d: %s\n", n, err_msg[n]);
	err++;
} // error

//第一次调用时ll=cc=0,这个函数会读取输入文件中的一行代码;然后把这行代码打印出来;还把这行代码存在line中,然后ch就等于line中第一个字符;
//然后后面几次调用时,一种情况是ll=cc!=0就是相当于这一行代码读完了,那么继续读取下一行,打印,存储,ch是line第一个字符
//或者更平凡的,ll!=cc,这时就是说这一行代码还没看完,那就执行ch=line[++cc]代码,读取当前行代码的下一个字符
void getch(void)
{
	if (cc == ll)
	{
		if (feof(infile))//如果输入代码文件读取结束,就退出进程发生错误,
		{
			printf("\nPROGRAM INCOMPLETE\n");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", cx);
		while ( (!feof(infile)) // added & modified by alex 01-02-09
			    && ((ch = getc(infile)) != '\n'))//把一行读完,然后把这行代码打出来
		{
			printf("%c", ch);
			line[++ll] = ch;
		} // while
		printf("\n");
		line[++ll] = ' ';
	}
	ch = line[++cc];//cc就是字符数,ch就是这个
} // getch

//它做的是获得一个标识,它通过调用getch,读完某个字符串,ch永远是一个还未被处理的字符,sym永远是一个未被处理的标识
// gets a symbol from input stream.
void getsym(void)
{
	int i, k;
	char a[MAXIDLEN + 1];

	while (ch == ' '||ch == '\t')//将空格和换行符省略
		getch();

	if (isalpha(ch))//如果首字符是英文字母,那么它要么是保留字,要么他就是名称
	{ // symbol is a reserved word or an identifier.
		k = 0;
		do//这一循环做的事它会一直读取字母或者数字,值得遇到非字母数字,通常来说,遇到的事空格就停止,当然这个循环最多执行10次
		{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		}
		while (isalpha(ch) || isdigit(ch));
		a[k] = 0;//即字符串结束符
		strcpy(id, a);
		word[0] = id;
		i = NRW;
		while (strcmp(id, word[i--]));//直到遇到id和word[i]相等才会结束,如果id不是保留字那么出循环时i==-1,否则出循环时,i相当于是保留字的下标减1
		if (++i)//如果id是保留字,那就把保留字的标识符给他,否则我们给他SYM_IDENTIFIER标识符,即它应该是一个名称
			sym = wsym[i]; // symbol is a reserved word
		else
			sym = SYM_IDENTIFIER;   // symbol is an identifier
	}
	else if (isdigit(ch))//如果首字符是数字,那么它不出意外的就是一串number
	{ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do//一直读取直到遇到非数字,通常来说是空格或者换行符或者分号;
		{
			num = num * 10 + ch - '0';
			k++;
			getch();
		}
		while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(25);     // The number is too great.
	}
	else if (ch == ':')//如果首字符是':',那么不出意外的话它应该赋值标识符':='
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_BECOMES; // :=
			getch();
		}
		else
		{	//Xulong's_contribution:
			sym = SYM_COLON;       // 获得对单个`:`的标识符
		}
	}
	else if (ch == '>')//如果首字符是'>',那么不出意外的话,它应该是'>=','>'
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_GEQ;     // >=
			getch();
		}
		else
		{
			sym = SYM_GTR;     // >
		}
	}
	else if (ch == '<')
	{
		getch();
		if (ch == '=')
		{
			sym = SYM_LEQ;     // <=
			getch();
		}
		else if (ch == '>')
		{
			sym = SYM_NEQ;     // <>
			getch();
		}
		else
		{
			sym = SYM_LES;     // <
		}
	}
	else//如果首字符是其他的字符,不出意外的话 我们期待它是 csym中的合法字符:' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'
	{ // other tokens
		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);
		if (++i)
		{
			sym = ssym[i];
			getch();
		}
		else
		{
			printf("Fatal Error: Unknown character.\n");
			exit(1);
		}
	}
} // getsym

//通过输入:f即opcode,l即层次差,a视不同opcode而不同 三个参数来生成一条中间代码,并把中间代码放到code数组中,然后令cx++
// generates (assembles) an instruction.
void gen(int x, int y, int z)
{
	if (cx > CXMAX)
	{
		printf("Fatal Error: Program too long.\n");
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx++].a = z;
} // gen

//即错误分析语句,做一系列skip语句
//首先判断sym是否在s1中,如果不在就error报错n,将s1和s2合并后,然后会疯狂读取标识符,直到在s1或s2中发现这些标识符
// tests if error occurs and skips all symbols that do not belongs to s1 or s2.
void test(symset s1, symset s2, int n)
{
	symset s;

	if (! inset(sym, s1))
	{
		error(n);
		s = uniteset(s1, s2);
		while(! inset(sym, s))//这个while循环,会调用若干次getsym(),直到得到的标识符在s1和s2集合中
			getsym();
		destroyset(s);
	}
} // test


int dx;  // data allocation index 数据分配的(相对)起始下标,它永远指向一个待填位置,是用来填block局部变量的

// 输入类型kind,像符号表中加入表项
//enter object(constant, variable or procedre) into table.
void enter(int kind)
{
	mask* mk;

	tx++;
	//Xulong's_contribution:填表项的时候，要看自己是不是在for循环中
	if(depth_of_loop!=0)
	{
			char temp[MAXIDLEN+2]={(char)depth_of_loop+'0','\0'};
			strcpy(id,strncat(temp,id,MAXIDLEN));//我们给id加上新的标记使它的第一个字符代表for循环层次
	}
	strcpy(table[tx].name, id);
	table[tx].kind = kind;
	switch (kind)
	{
	case ID_CONSTANT:
		if (num > MAXADDRESS)
		{
			error(25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;
		break;
	case ID_VARIABLE://如果是变量,那么设置层次和偏移量
		mk = (mask*) &table[tx];
		mk->level = level;
		mk->address = dx++;
		break;
	case ID_PROCEDURE://如果是过程,那么设置层次,入口地址这里不设置,后面做block分析的时候我们再填
		mk = (mask*) &table[tx];
		mk->level = level;
		break;
	//Xinyu's_contribution:如果此时是数组变量,我们存入符号表中,address属性应该是数组在array_table中的起始下标
	case ID_ARRAY:
		mk = (mask*)&table[tx];
		mk->level = level;
		mk->address = arr_tx;//表项地址为在数组信息表中的索引
		Last_Array = &(array_table[arr_tx]);
		array_table[arr_tx].dim = 0;
		array_table[arr_tx].size = 1;
		array_table[arr_tx].dim_size[1] = 0; 
		arr_tx++;
		//这里我们无法给数组的size给出真正定义,所以我们不对dx做增大(即分配空间),等到分析完整个数组在做
		break;	
	} // switch
} // enter

//根据输入名称找到参数在符号表中的位置,返回符号表索引
// locates identifier in symbol table. 
int position(char* id)
{
	int i;
	char id_in_loop[MAXIDLEN+2];
	//Xulong's_contribution:查表时，要看自己是不是在for循环中
	if(depth_of_loop!=0)
	{
		for(int k=depth_of_loop;k>0;k--)	
		{	
			char temp[MAXIDLEN+2]={(char)k+'0','\0'};
			strcpy(id_in_loop,strncat(temp,id,MAXIDLEN));//我们给id加上新的标记使它的第一个字符代表for循环层次
			strcpy(table[0].name, id_in_loop);
			i = tx + 1;
			while (strcmp(table[--i].name, id_in_loop) != 0);
			if(i!=0)return i;
		}
	}
	i=tx+1;
	strcpy(table[0].name, id);
	i = tx + 1;
	while (strcmp(table[--i].name, id) != 0);
	return i;
} // position

//常量声明语句的语法分析:如果标识符是名称 ok,否则报错4;它期望看到下一个标识符是':='或'=';再下一个标识符必须是number;然后填表
//实际上它就是说,常量必须进行初始化,且必须初始化为数字;就是说你不用用变量给常量赋值(PL0规定常量必须最先初始化)
void constdeclaration()
{
	if (sym == SYM_IDENTIFIER)
	{
		getsym();
		if (sym == SYM_EQU || sym == SYM_BECOMES)
		{
			if (sym == SYM_BECOMES)
				error(1); // Found ':=' when expecting '='.
			getsym();
			if (sym == SYM_NUMBER)
			{
				enter(ID_CONSTANT);
				getsym();
			}
			else
			{
				error(2); // There must be a number to follow '='.
			}
		}
		else
		{
			error(3); // There must be an '=' to follow the identifier.
		}
	} else	error(4);
	 // There must be an identifier to follow 'const', 'var', or 'procedure'.
} // constdeclaration

//Xinyu's_contribution:
void dimdeclaration() {
	int i;
	if (sym == SYM_LMIDPAREN) {
		getsym();
		switch (sym)
		{
		case SYM_NUMBER:
			if (Last_Array->dim == MAX_DIM) { error(33); }//报错:维数过多
			Last_Array->dim = Last_Array->dim + 1;//总维数+1
			Last_Array->dim_size[Last_Array->dim] = num;
			Last_Array->size = Last_Array->size * num;//数组大小*最外维范围大小
			getsym();
			if (sym == SYM_RMIDPAREN) //继续分析下一维
			{ 
				getsym();
				dimdeclaration(); 
			}
			else { error(27); }//报错:missing `]`
			break;
		default:
			error(29);//缺少维度大小
		}
	}
	else {
			dx += Last_Array->size;//为数组分配空间
			Last_Array->address = dx - 1;//记录最后访问到的数组的基址		
	}
}

//Xinyu's_contribution:数组访问时的语法分析,我们需要做的是计算出该数组元素的偏移量压入栈顶
void array_access(short arr_index, int dim, symset fsys) {//dim代表正在分析的维度
	getsym();
	if (sym == SYM_LMIDPAREN) {
		gen(LIT, 0, array_table[arr_index].dim_size[dim + 1]);//将下一维size压栈
		gen(OPR, 0, OPR_MUL);//将栈顶和次栈顶数值相乘，例如对应a[10][10]，a[3][4]，在读到3时，执行的中间代码依次为0*10=0，0+3=3,3*10=30,30+4=34.
		getsym();
		expression(fsys);
		//expression中已经获取了下一个标识
		gen(OPR, 0, OPR_ADD);//加上最外维上的偏移
		array_access(arr_index, dim + 1, fsys);//访问下一维
	}
	else if (dim != array_table[arr_index].dim) { error(30); }//维度分析错误
}
//变量声明语句:它期待这个标识符必须是名称,然后填表,它包括名称,层次和偏移量
void vardeclaration(void)
{
	if (sym == SYM_IDENTIFIER)
	{
		//Xinyu's_contribution:在变量声明语句中,我们需要数组变量和非数组变量
		getsym();
		if (sym == SYM_LMIDPAREN) {
			enter(ID_ARRAY);
			dimdeclaration();
		}
		else
		{
			enter(ID_VARIABLE); 
		}
	}
	else
	{
		error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
	}
} // vardeclaration


//就是把code数字中,code[from]打印到code[to-1]
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

//以下开始语法分析,调用链为 expression->term->factor
//factor是因子,它是term的组成部分
void factor(symset fsys)
{
	void expression(symset fsys);//函数原型
	int i,arr_index;
	symset set;
	//Tianyi's_contribution:
	symset set1;
	mask* mk;
	
	test(facbegsys, fsys, 24); // The symbol can not be as the beginning of an expression.
	//上面就是说,标识符必须是factor的first集合中的某个,否则就报错,如果报错就然后照fsys和facbegsys并集进行error处理(做所谓的skip 处理)

	if (inset(sym, facbegsys))//如果标识符是在first集合中，+，-，（，id，number
	{
		if (sym == SYM_IDENTIFIER)//标识符是一个名称,那就把这个名称的值给他搞出来
		{
			if ((i = position(id)) == 0)//如果名称不在符号表中就报错
			{
				error(11); // Undeclared identifier.
			}
			else
			{
				switch (table[i].kind)
				{
				case ID_CONSTANT://如果是常量就把它的值置于栈顶
					gen(LIT, 0, table[i].value);
					getsym();
					break;
				case ID_VARIABLE://如果是变量也把他的值至于栈顶
					mk = (mask*) &table[i];
					getsym();
					if(sym == SYM_BECOMES){
						getsym();
						expression(fsys);
						gen(STO,level - mk->level, mk->address);
					}
					gen(LOD, level - mk->level, mk->address);
					break;
				case ID_PROCEDURE://如果是过程,那就报错因为过程不能出现在表达式中
					error(21); // Procedure identifier can not be in an expression.
					getsym();
					break;
				//Xinyu's_contribution:该名称是数组的名称
				case ID_ARRAY: //因子中包含数组元素
					mk = (mask*)&table[i];
					arr_index = mk->address;//即找到这个数组在数组信息表中的下标
					gen(LEA, level - mk->level, array_table[arr_index].address);//把数组的基址放在栈顶
					gen(LIT, 0, 0);//要先在栈中压入0因为array_access第一句生成代码就是栈顶两元素相乘
					set = createset(SYM_RMIDPAREN);
					array_access(arr_index, 0, set);//获取数组元素的偏移量
					gen(OPR, 0, OPR_MIN);//数组基址 - 偏移量=元素地址
					if(sym == SYM_BECOMES){
						getsym();
						expression(fsys);//计算右值 
						gen(STA, 0, 0);//将栈顶的值存入数组元素的地址(次栈顶)中
						gen(INT,0,1);//此时栈顶是元素偏移
					}
					gen(LDA, 0, 0);//将数组元素的值置于栈顶 
					break;
				} // switch
			//Chenlong's contribution
			
			}
		}
		else if (sym == SYM_NUMBER)//标识符是数字串,那就把他放到栈顶
		{
			if (num > MAXADDRESS)
			{
				error(25); // The number is too great.
				num = 0;
			}
			gen(LIT, 0, num);
			getsym();
		}
		else if (sym == SYM_LPAREN)//标识符是`(`,那么就从下一个标识符开始expression分析,注意我们给expression时给他的同步集合(error分析集合)中加入了`)`,用以错误处理
		{
			getsym();
			set = uniteset(createset(SYM_RPAREN, SYM_NULL), fsys);
			expression(set);//重新开始expression分析
			destroyset(set);//销毁同步集合
			if (sym == SYM_RPAREN)//我们期待一个expression分析完后应该存在`)`匹配,之前的`(`.如果没有就报错
			{
				getsym();
			}
			else
			{
				error(22); // Missing ')'.
			}
		}
		else if(sym == SYM_MINUS) // UMINUS,  Expr -> '-' Expr,如果是`-`,那就继续分析factor然后把栈顶元素取反
		{  
			 getsym();
			 factor(fsys);
			 gen(OPR, 0, OPR_NEG);
		}
		//Tianyi's_contribution:
		//这里和statement下情况一样，可参照那边的注释
		else if (sym == SYM_SETJ)
		{
			 getsym();
			 if (sym != SYM_LPAREN)
			 {
				error(38);
			 }
			 else
			 {
				getsym();
			 }
			 set1 = createset(SYM_RPAREN, SYM_NULL);
			 set = uniteset(set1, fsys);
			 expression(set);
			 destroyset(set1);
			 destroyset(set);
			 if (sym != SYM_RPAREN)
			 {
				error(39);
			 }
			 else
			 {
				getsym();
			 }
			 gen(OPR, 0, OPR_SAVE0);
			 gen(OPR, 0, OPR_SAVE1);
			 gen(OPR, 0, OPR_SST);
			 gen(LIT,0,0);
		}
		test(fsys, createset(SYM_LPAREN, SYM_NULL), 23);//这是错误分析处理 skip操作
	} // if
} // factor

//term是项的意思,term的组成单元是factor,每个项都是由factor和乘法除法组成的
void term(symset fsys)
{
	int mulop;
	symset set;
	
	set = uniteset(fsys, createset(SYM_TIMES, SYM_SLASH, SYM_NULL));//这个集合包括了factor的follow集合
	factor(set);
	while (sym == SYM_TIMES || sym == SYM_SLASH)//每个项都是由factor和乘法除法组成的
	{
		mulop = sym;
		getsym();
		factor(set);
		if (mulop == SYM_TIMES)
		{
			gen(OPR, 0, OPR_MUL);
		}
		else
		{
			gen(OPR, 0, OPR_DIV);
		}
	} // while
	destroyset(set);
} // term

//expression就是表达式的意思,表达式的组成单元是term,每个表达式是由term和加减法组成的,做完expression分析后,sym应该是一个一个表达式的follow集合,例如在赋值语句中是`;`,在for表达式中是`,`或者`)`
void expression(symset fsys)
{
	int addop;
	symset set;

	set = uniteset(fsys, createset(SYM_PLUS, SYM_MINUS, SYM_NULL));//他包括了term的follow集合
	
	term(set);
	while (sym == SYM_PLUS || sym == SYM_MINUS)//每个表达式是由term和加减法组成的
	{
		addop = sym;
		getsym();
		term(set);
		if (addop == SYM_PLUS)
		{
			gen(OPR, 0, OPR_ADD);
		}
		else
		{
			gen(OPR, 0, OPR_MIN);
		}
	} // while

	destroyset(set);
} // expression

//条件表达式的分析,它要么就是ODD开头的要么就由一个expression开头的
void condition(symset fsys)
{
	int relop;
	symset set;
	if (sym == SYM_ODD)//做OPR_ODD操作,即判断栈顶是不是奇数,然后压栈
	{
		getsym();
		expression(fsys);
		gen(OPR, 0, 6);
	}
	else
	{
		set = uniteset(relset, fsys);//set包括关系运算符,我们希望如果expression分析错误,它能一直skip直到遇到关系运算符
		expression(set);
		destroyset(set);
		if (! inset(sym, relset))//分析结束expression的时候,我们希望sym是关系符号,如果不是就报错
		{
			error(20);
		}
		else
		{
			relop = sym;//将关系运算符记下来
			getsym();
			expression(fsys);//继续做expression分析,此时我们的栈顶中存在两个expression值,我们根据relop做相应操作
			switch (relop)
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
		} // else
	} // else
} // condition


//语句的分析,我们希望它的first 是id call if begin while,做完statement分析后,sym中应该是一个分号,所以不管添加什么操作都要保证吸收最后的分号
void statement(symset fsys)
{
	int i, cx1, cx2,cx3,arr_index;
	int count=0;
	//Xulong's_contribution:
	char id_in_loop[MAXIDLEN+2];//记录循环中id的新名字
	symset set1, set;

	if (sym == SYM_IDENTIFIER)//如果是id开始,我们期待它是一个赋值语句
	{ // variable assignment
		mask* mk;
		if (! (i = position(id)))//id再符号表中不存在,报错
		{
			error(11); // Undeclared identifier.
		}
		//Xinyu's_contribution:要考虑赋值语句是给数组元素的赋值
		else if (table[i].kind != ID_VARIABLE&& table[i].kind != ID_ARRAY)//id不是变量或者数组变量,报错,非法赋值,i=0
		{
			error(12); // Illegal assignment.
			i = 0;
		}
		//Chenlong's contribution
		/*if (table[i].kind == ID_VARIABLE){
			factor(fsys);
		}*/
		
		if (table[i].kind == ID_VARIABLE)//variable assignment
		{

			getsym();//我们期待获得一个`:=`符号
			if (sym == SYM_BECOMES)//如我们所愿,得到一个':=',赋值语句右部,我们期望存在一个expression
			{
				getsym();
			}
			else
			{
				error(13); // ':=' expected.
			}
			expression(fsys);//expression分析
			mk = (mask*) &table[i];//这里i就是position,它是取到左值再符号表中的位置
			if (i)//这里i是看有没有error  如果没有error 就进行STO操作,即将栈顶的expression的值赋给左值
			{
				gen(STO, level - mk->level, mk->address);
			}
		}
		//Xinyu's_contribution:数组元素的赋值语句
		else if (table[i].kind == ID_ARRAY) {//数组元素的赋值
			mk = (mask*)&table[i];
			arr_index = mk->address;
			gen(LEA, level - mk->level, array_table[arr_index].address);
			gen(LIT, 0, 0);
			set1 = createset(SYM_RMIDPAREN);
			array_access(arr_index, 0, set1);//访问数组元素
			//array_access 已经获取下一个标识符
			if (sym != SYM_BECOMES) { error(13); }
			gen(OPR, 0, OPR_MIN);
			getsym();
			expression(fsys);//计算右值 
			if (i) {
				gen(STA, 0, 0);//将栈顶的值存入数组元素的地址(次栈顶)中
			}
		}
	}
	else if (sym == SYM_CALL)//标识符是call,那么我们期待这是一个调用函数语句
	{ // procedure call
		getsym();
		if (sym != SYM_IDENTIFIER)//我们期待call 后面跟着一个名称,如果不是即报错
		{
			error(14); // There must be an identifier to follow the 'call'.
		}
		else
		{
			if (! (i = position(id)))//找到名称再符号表中位置,我们期待这个名称是一个过程名称
			{
				error(11); // Undeclared identifier.
			}
			else if (table[i].kind == ID_PROCEDURE)//正如我们所愿,这个名称是过程名称,做call操作
			{
				mask* mk;
				mk = (mask*) &table[i];
				gen(CAL, level - mk->level, mk->address);//注意这里计算了层次差
			}
			else
			{
				error(15); // A constant or variable can not be called. 
			}
			getsym();
		}
	} 
	else if (sym == SYM_IF)//if开头的语句,我们期待它是 if then 语句
	{ // if statement
	//Yuanhao's_contribution:
		getsym();
		if(sym == SYM_LPAREN)
		{
			getsym();
		}
		else
		{
			error(26);
		}
		set1 = createset(SYM_RPAREN,SYM_THEN, SYM_DO, SYM_NULL);//then do
		set = uniteset(set1, fsys);
		condition(set);//对于条件表达式进行分析,如果分析失败,做skip操作直到遇到 then或者 do 或是 条件语句的first
		destroyset(set1);
		destroyset(set);
		if(sym==SYM_RPAREN)
		{
			getsym();
		}
		else
		{
			error(22);
		}
		if (sym == SYM_THEN)
		{
			getsym();
		}
		else
		{
			error(16); // 'then' expected.
		}
		cx1 = cx;//把中间代码的地址记录,用于后面回填跳转地址
		gen(JPC, 0, 0);//生成代码:条件语句得到false时跳转到 一个地址(需要回填)
		set1 = createset(SYM_ELSE,SYM_NULL);//此时statement的follow是else
		set = uniteset(set1, fsys);
		statement(set);//继续语句分析
		destroyset(set1);
		destroyset(set);
		cx2=cx;
		gen(JMP,0,0);//生成无条件跳出,待回填
		code[cx1].a = cx;	//回填跳转地址,即上面那句JPC
		int temp_ll=ll;
		int temp_cc=cc;
		char temp_ch=ch;
		int temp_sym=sym;
		FILE stored_file=*infile;
		getsym();//我们期待得到一个else
		if(sym==SYM_ELSE)//如果存在else的话,我们继续分析
		{
			getsym();
			statement(fsys);
		}
		else
		{
			ll=temp_ll;
			cc=temp_cc;
			ch=temp_ch;
			sym=temp_sym;
			(*infile)=stored_file;

		}
		code[cx2].a=cx;//回填跳出地址
	}
	else if (sym == SYM_BEGIN)//如果是begin开始的,我们期望它是一系列语句组成 然后最后有个end保留字
	{ // block
		getsym();
		set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
		set = uniteset(set1, fsys);
		statement(set);//进行语句分析
		while (sym == SYM_SEMICOLON || inset(sym, statbegsys))//如果标识符是`;`或者标识符是语句的first,我们才开始语句下一条语句分析,这里是因为它允许我们忘记`;`
		{
			if (sym == SYM_SEMICOLON)
			{
				getsym();
			}
			else//如果不是分号就说明我们忘记写分号了
			{
				error(10);
			}
			statement(set);//开始吓一跳语句分析
		} // while
		destroyset(set1);
		destroyset(set);
		if (sym == SYM_END)//语句序列分析完毕,后面必须加一个end
		{
			getsym();
		}
		else//不加end也行,会报错
		{
			error(17); // ';' or 'end' expected.
		}
	}
	else if (sym == SYM_WHILE)//我们期待一个while do语句
	{ // while statement
		cx1 = cx;//将while语句开始地址记录,用于填跳转地址
		getsym();
		set1 = createset(SYM_DO, SYM_NULL);
		set = uniteset(set1, fsys);
		condition(set);//条件语句分析
		destroyset(set1);
		destroyset(set);
		cx2 = cx;//将DO的开始地址记录,用于填跳转地址
		gen(JPC, 0, 0);//条件false 跳转,跳转地址未确定,后面会回填
		if (sym == SYM_DO)//正如所属愿来了个DO
		{
			getsym();
		}
		else
		{
			error(18); // 'do' expected.
		}
		statement(fsys);//do语句
		gen(JMP, 0, cx1);//生成无条件跳转语句,跳到while起始地址
		code[cx2].a = cx;//回填JPC指令
	}
	//Xinyu's_contribution:打印
	else if(sym==SYM_PRT)//表示进入打印
	{
		//print statement
		getsym();
		if (sym != SYM_LPAREN) { error(31); }// wrong in print
		while (1) {
			getsym();
			if (sym == SYM_RPAREN) { break; }
			else if (sym == SYM_IDENTIFIER) {
				count++;
				if ((i = position(id)) == 0) { error(11); }// Undeclared identifier.
				else if (table[i].kind != ID_VARIABLE && table[i].kind != ID_ARRAY && table[i].kind != ID_CONSTANT)
				{
					error(31);// wrong in print
				}
				else {
					if (table[i].kind == ID_VARIABLE) {
						mask* mk;
						mk = (mask*)&table[i];
						gen(LOD, level - mk->level, mk->address);//变量值置于栈顶
						getsym();
					}
					else if (table[i].kind == ID_CONSTANT) {
						gen(LIT, 0, table[i].value);//常量值置于栈顶
						getsym();
					}
					else if (table[i].kind == ID_ARRAY) {//数组元素值置于栈顶
						int arr_index;
						mask* mk;
						mk = (mask*)&table[i];
						arr_index = mk->address;
						gen(LEA, level - mk->level, array_table[arr_index].address);
						gen(LIT, 0, 0); 
						set1 = createset(SYM_RMIDPAREN);
						array_access(arr_index, 0, set1);
						gen(OPR, 0, OPR_MIN);
						gen(LDA, 0, 0);//获取数组元素值并置于栈顶 
					}
					if (sym == SYM_COMMA) { ; }
					else if (sym == SYM_RPAREN) { break; }
					else { error(28); }//wrong format
				}
			}
			else if (sym==SYM_NUMBER)//数字直接置于栈顶
			{
				gen(LIT,0,num);
				count++;
				getsym();
				if (sym == SYM_RPAREN) { break; }
			}
			else {
				error(31);//wrong in print
			}
		}
		gen(PRT, 0, count); // 打印栈顶count个值
		getsym();
	}
	//Xulong's_contribution:
	else if(sym==SYM_FOR)
	{
		if(depth_of_loop>=MAX_LOOP)
		{
			error(37);
			exit(0);
		}
		depth_of_loop++;//表示循环层次加1
		//因为我们知道for循环中一定有这3个连续存储的值,那就实现把它们的偏移记下来
		for_loop_table[depth_of_loop].low_dx=dx;
		for_loop_table[depth_of_loop].high_dx=dx+1;
		for_loop_table[depth_of_loop].step_dx=dx+2;
		gen(INT,0,3);
		getsym();
		if(sym==SYM_LPAREN)
		{
			getsym();
		}
		else
		{
			error(26);//报错缺失'('
		}
			

		if(sym==SYM_VAR)
		{
			getsym();
		}	
		else
		{
			error(34);//报错缺失'var'
		}
		
		if(sym==SYM_IDENTIFIER)//我们希望读到一个id
		{
			strcpy(id_in_loop,id);//这里要不id记录下来，方便退出循环时把这个表项删除
			enter(ID_VARIABLE);//将这一变量塞到符号表中,注意,因为我们的符号的名字最开始有个数字,所以它一定是唯一的,一般的变量的名称是不能以数字开头的
		}
		else
		{
			error(11);//id读取失败
			exit(1);
		}
		dx=dx+2;
		getsym();
		if(sym==SYM_COLON)
		{
			getsym();
		}
		else
		{
			error(35);//报错缺失':'
		}
		if(sym==SYM_LPAREN)
		{
			getsym();
		}
		else
		{
			error(26);//报错缺失'('
		}
		set1 = createset(SYM_COMMA, SYM_NULL);//因为low的后继符号中有','
		set = uniteset(set1, fsys);
		expression(set);//不出意外栈顶就是low的值,即id的初值,
		gen(STO,0,for_loop_table[depth_of_loop].low_dx);//要把栈顶元素存到变量里面
		destroyset(set1);
		destroyset(set);
		if(sym==SYM_COMMA)
		{
			getsym();
		}
		else
		{
			error(5);//报错缺失','
		}
		set1 = createset(SYM_COMMA,SYM_RPAREN, SYM_NULL);//因为high的后继符号中有','和')'
		set = uniteset(set1, fsys);
		expression(set);//不出意外栈顶就是high的值
		gen(STO,0,for_loop_table[depth_of_loop].high_dx);//要把栈顶元素存到变量里面
		destroyset(set1);
		destroyset(set);
		if(sym==SYM_RPAREN)//如果high后面之间跟着')',那就默认step是1
		{
			gen(LIT,0,1);
			gen(STO,0,for_loop_table[depth_of_loop].step_dx);//要把栈顶元素存到变量里面
			getsym();
			if(sym==SYM_RPAREN)
			{
				getsym();
			}
			else
			{
				error(22);
			}
		}
		else if(sym==SYM_COMMA)//如果high后面之间跟着',',那就要处理step了
		{
			getsym();
			set1 = createset(SYM_RPAREN, SYM_NULL);//因为step的后继符号中有')'
			set = uniteset(set1, fsys);
			expression(set);//不出意外栈顶就是step的值
			gen(STO,0,for_loop_table[depth_of_loop].step_dx);//要把栈顶元素存到变量里面
			destroyset(set1);
			destroyset(set);
			if(sym==SYM_RPAREN)//连读两个右括号
			{
				getsym();
				if(sym==SYM_RPAREN)
				{
					getsym();
				}
				else
				{
					error(22);
				}
			}
			else
			{
				error(22);
			}
		}
		else
		{
			error(36);//缺失','或')'
			exit(0);
		}

		cx1=cx;//for循环跳回地址
		gen(LOD,0,for_loop_table[depth_of_loop].low_dx);//把var id的值压栈
		gen(LOD,0,for_loop_table[depth_of_loop].high_dx);//把high的值压栈
		gen(OPR,0,OPR_LEQ);//判断id是不是小于high
		cx2=cx;
		gen(JPC,0,0);//跳出for循环,等待回填
		statement(fsys);
		gen(LOD,0,for_loop_table[depth_of_loop].low_dx);//栈顶为low的值
		gen(LOD,0,for_loop_table[depth_of_loop].step_dx);//栈顶为step的值
		gen(OPR,0,OPR_ADD);//将low和step相加
		gen(STO,0,for_loop_table[depth_of_loop].low_dx);//将结果放回low中
		gen(JMP,0,cx1);//for循环跳回
		code[cx2].a=cx;//回填跳出for循环地址
		gen(INT,0,-3);//把分配空间释放
		dx=dx-3;
		tx--;//这是把那个for循环中声明的唯一变量删除
		depth_of_loop--;//循环层次减一
		
	}
	//Tianyi's_contribution:
	else if(sym == SYM_SETJ){
		getsym();
		if(sym != SYM_LPAREN){
			error(38);
		}
		else{
			getsym();
		}
		set1 = createset(SYM_RPAREN,SYM_NULL);
		set = uniteset(set1,fsys);
		expression(set);
		destroyset(set1);
		destroyset(set);
		if(sym != SYM_RPAREN){
			error(39);
		}
		else{
			getsym();
		}
		gen(OPR,0,OPR_SAVE0);//保存栈顶值到stack[0]
		gen(OPR,0,OPR_SAVE1);//保存pc+2值到stack[1]
		gen(OPR,0,OPR_SST);//保存当前栈
		gen(LIT,0,0);//读取stack[0]到栈顶作为返回值
	}
	else if(sym == SYM_LONGJ){
		getsym();
		if (sym != SYM_LPAREN){
			error(40);
		}
		else{
			getsym();
		}
		set1 = createset(SYM_COMMA,SYM_NULL);
		set = uniteset(set1,fsys);
		expression(set);//取得第一个参数的值
		destroyset(set1);
		destroyset(set);
		if(sym != SYM_COMMA){
			error(41);
		}
		else{
			getsym();
		}
		gen(OPR,0,OPR_LOAD0);//读取stack[0]到栈顶
		gen(OPR,0,OPR_EQU);//判断是否相等
		cx1 = cx;
		gen(JPC,0,0);//若不相等则不需要回到setjump处
		gen(OPR,0,OPR_LST);//恢复栈
		set1 = createset(SYM_RPAREN,SYM_NULL);
		set = uniteset(set1,fsys);
		expression(set);//取得第二个参数的值
		if(sym != SYM_RPAREN){
			error(42);
		}
		else{
			getsym();
		}
		gen(OPR,0,OPR_LOAD1);//将stack[1]处的值填入pc
		code[cx1].a = cx;//地址回填
	}

	test(fsys, phi, 19);
} // statement
			
//程序体分析,我们希望它以 保留字const var procedure开始 或者由statement序列组成,声明语句会声明一系列量,我们要填符号表
//注意本函数中存在多个地址:1.符号表索引即符号表数组的下标,tx 2.局部变量地址(即变量存在栈中的下标),因为局部变量是放在栈中的,局部变量地址等于过程入口地址+相对偏移(dx) 3.代码地址(即是第几行中间代码) code数组的下标
void block(symset fsys)//
{
	int cx0; // initial code index 程序体起始地址
	mask* mk;
	int block_dx;//由于存在程序嵌套,我们需要保存局部变量的可填地址,它的大小也是一个block中局部空间的大小
	int savedTx;//由于存在程序嵌套,我们需要保存符号表起始索引
	symset set1, set;

	dx = 3;//我们的局部变量从栈底以上第3个地址开始填,这是因为每次进入一个block时,它的栈中,stack[base]是静态链,stack[base+1]是动态链,stack[base+2]是返回地址,所以我们只能从stack[base+3]开始存局部变量
	block_dx = dx;
	mk = (mask*) &table[tx];//程序体本身是属于一个过程的内部的,进入block函数前,我们已经在符号表中填入了procedure表项,但是我们没有填入口地址
	//这里的mk实际上就指向这block所属的procedure在符号表中的表项
	mk->address = cx;//这个cx就指向下面那个JMP指令的地址
	gen(JMP, 0, 0);//这是进入block的第一条指令,我们必须回填这个地址,这个地址是指向完声明语句后的第一个地址即begin语句开始处
	if (level > MAXLEVEL)//防止程序体层次过大,因为我们允许block 嵌套 block
	{
		error(32); // There are too many levels.
	}
	do
	{
		if (sym == SYM_CONST)//如果以const起始,我们希望看到的是常量名称,
		{ // constant declarations
			getsym();
			do
			{
				constdeclaration();//常量声明分析
				while (sym == SYM_COMMA)//如果是`,`它可以继续常量声明
				{
					getsym();
					constdeclaration();
				}
				if (sym == SYM_SEMICOLON)//如果是';',说明声明语句结束
				{
					getsym();
				}
				else
				{
					error(5); // Missing ',' or ';'.
				}
			}
			while (sym == SYM_IDENTIFIER);//do while 循环 当然了sym一定要是名称 不然无法正确进入 常量声明分析
		} // if

		if (sym == SYM_VAR)//如果以var开始,我们希望看到的是变量名称
		{ // variable declarations
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
			}
			while (sym == SYM_IDENTIFIER);
		} // if
		block_dx = dx; //save dx before handling procedure call! 我们在过程分析前一定要把数据分配的起始下标存储
		while (sym == SYM_PROCEDURE)//做过程分析,注意这里我们是while 而上面两个变量声明是if  这时因为过程声明不能像变量一样,一次性把所有的过程声明掉
		{ // procedure declarations
			getsym();
			if (sym == SYM_IDENTIFIER)//正如我们所愿,得到一个过程名称
			{
				enter(ID_PROCEDURE);//在符号表中填入过程名称,但是不填入口地址
				getsym();
			}
			else
			{
				error(4); // There must be an identifier to follow 'const', 'var', or 'procedure'.
			}


			if (sym == SYM_SEMICOLON)//声明一个过程的语句要以';'结尾
			{
				getsym();//这时我们希望下一个是字符是block的first,例如begin const var 什么的..
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}

			level++;//进入层次,需要将level增加
			savedTx = tx;//将进入嵌套程序前的初始符号表索引存起来
			set1 = createset(SYM_SEMICOLON, SYM_NULL);
			set = uniteset(set1, fsys);
			block(set);//进入程序体分析,即嵌套程序体
			destroyset(set1);
			destroyset(set);
			tx = savedTx;//恢复符号表索引,注意这里我们只恢复符号表索引,却不恢复数据相对偏移,这是因为上面调用的那个block中已经恢复了dx
			level--;//恢复层次

			if (sym == SYM_SEMICOLON)//skip掉一些列垃圾符号,继续下一个procedure声明分析
			{
				getsym();
				set1 = createset(SYM_IDENTIFIER, SYM_PROCEDURE, SYM_NULL);
				set = uniteset(statbegsys, set1);//即set是 YM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE,SYM_IDENTIFIER, SYM_PROCEDUR
				test(set, fsys, 6);
				destroyset(set1);
				destroyset(set);
			}
			else
			{
				error(5); // Missing ',' or ';'.
			}
		} // while
		//做完 (过程)声明 之后我们已经在符号表中填入了数据,我们一定要首先恢复数据可填地址dx,因为局部变量在栈中的绝对地址是过程入口地址加上dx,即dx是一个相对地址,
		//我们恢复dx是因为,我们害怕:过程声明后面还存在一个常量或者变量声明
		dx = block_dx; //restore dx after handling procedure call! 
		set1 = createset(SYM_IDENTIFIER, SYM_NULL);
		set = uniteset(statbegsys, set1);
		test(set, declbegsys, 7);
		destroyset(set1);
		destroyset(set);
	}
	while (inset(sym, declbegsys));//这里还有一层while ,注意我们存在两层while,里面那层while是处理多条过程声明的,多条过程声明后,我们还会存在其他声明语句
	//以上我们将程序体中的所有声明语句处理完成,下面开始statement分析
	//以下我们做语句分析 statement
	code[mk->address].a = cx;//就是前面那个mk,它指向过程表项,address离存放的是那条JMP的地址,JMP需要回填,我们做填入完声明指令后的第一个地址
	mk->address = cx;//我们在填过程表项的时候,故意先空着入口地址,等什么时候确定了再填,只有把所有的声明语句全部搞完了,我们才可以填入口地址,
	//这是因为PL0是一个静态编译的语言,所有的声明变量在编译阶段就确定了,调用函数的时候,是直接从begin语句开始的,不会去执行前面的声明语句.
	//这和C++有差别,C++支持动态编译,例如动态数组a[n],在编译的时候,你无法确定n是多少,就无法给他分配符号表,那你的函数的入口地址就必须从`int a[n]`声明开始
	cx0 = cx;
	gen(INT, 0, block_dx);//我们分析完声明语句以后,我们可以计算block_dx,就按照它的大小分配局部空间
	set1 = createset(SYM_SEMICOLON, SYM_END, SYM_NULL);
	set = uniteset(set1, fsys);
	statement(set);
	destroyset(set1);
	destroyset(set);
	gen(OPR, 0, OPR_RET); // return
	test(fsys, phi, 8); // test for error: Follow the statement is an incorrect symbol.
	listcode(cx0, cx);
} // block

//根据层次差取栈的基地址(静态链)
int base(int stack[], int currentLevel, int levelDiff)
{
	int b = currentLevel;
	
	while (levelDiff--)
		b = stack[b];
	return b;
} // base

//////////////////////////////////////////////////////////////////////
// interprets and executes codes.我们已经生成了中间代码(即code[..]),下一步就是执行中间代码,执行代码的本质就是说,给你一组中间代码,把他翻译成C语言再执行
void interpret()
{
	int pc;        // program counter 程序计数器,它表示当前执行的是那一行中间代码,执行完某行中间代码后pc++,
	int stack[STACKSIZE];
	int top;       // top of stack,栈顶指针,永远指向栈顶元素(存在)
	int b;         // program, base, and top-stack register,它永远指向基地址,指向静态链(反应过程嵌套关系,stack[b]表示它的父亲的静态链),相当于就是说 栈的基地址中存着它父亲过程的基地址
	instruction i; // instruction register
	//Tianyi's_contribution:
	int j;//用于保存栈时循环控制
	int saved_stack[STACKSIZE];//保存的栈
	int saved_top;//保存的栈顶
	int saved_b;//保存的基地址

	printf("Begin executing PL/0 program.\n");

	pc = 0;
	b = 3;
	//Tianyi'scontribution
	top = 5;
	stack[3] = stack[4] = stack[5] = 0;
	//stack[3]里是静态链,stack[4]是动态链,stack[5]是返回地址
	stack[0] = -1;
	stack[1] = -1;
	do//这是一个while循环,要求pc>0,当main函数执行完return后,pc=0,然后退出循环
	{
		i = code[pc++];
		switch (i.f)//根据OPCODE来翻译
		{
		case LIT://(LIT,--,常数),它做的事情就是把常数压入栈顶
			stack[++top] = i.a;
			break;
		case OPR://(OPR,--,操作类别)
			switch (i.a) // operator
			{
			case OPR_RET://return操作:(OPR,--,0),他做的事情是函数返回,pc=返回地址,注意在PL0中没有返回值这一说法
				top = b - 1;//即退栈
				pc = stack[top + 3];//pc=返回地址
				b = stack[top + 2];//根据动态链(调用关系)恢复基地址,注意这里不是静态链
				break;
			case OPR_NEG://(OPR,--,1),它做的事情是将栈顶数据变成其相反数
				stack[top] = -stack[top];
				break;
			case OPR_ADD://(OPR,--,2),他做的事情是弹出栈顶两个数据,然后进行相加,将结果压栈
				top--;
				stack[top] += stack[top + 1];
				break;
			case OPR_MIN://(OPR,--,3),他做的事情是弹出栈顶两个数据,然后进行相减,倒数第二个数是被减数,倒数第一个数是减数,然后将结果压栈(因为a-b,a总是先压栈的)
				top--;
				stack[top] -= stack[top + 1];
				break;
			case OPR_MUL://(OPR,--,4),他做的事情是弹出栈顶两个数据,然后进行相乘,将结果压栈
				top--;
				stack[top] *= stack[top + 1];
				break;
			case OPR_DIV://(OPR,--,5),他做的事情是弹出栈顶两个数据,然后进行相除,将结果压栈,这里它还会检测除数为零的exception
				top--;
				if (stack[top + 1] == 0)
				{
					fprintf(stderr, "Runtime Error: Divided by zero.\n");
					fprintf(stderr, "Program terminated.\n");
					continue;
				}
				stack[top] /= stack[top + 1];
				break;
			case OPR_ODD://(OPR,--,6),他做的事情是弹出栈顶元素,然后判断栈顶数据是奇数还是偶数,如果是奇数就把1压栈,反之压入0
				stack[top] %= 2;
				break;
			case OPR_EQU://(OPR,--,7),他做的事情是弹出两个栈顶元素,然后判断两者是否相等,如果是则压入1,反之压入0
				top--;
				stack[top] = stack[top] == stack[top + 1];
				break;
			case OPR_NEQ://(OPR,--,8),判断不等
				top--;
				stack[top] = stack[top] != stack[top + 1];
				break;
			case OPR_LES://(OPR,--,9),他做的事情是弹出两个栈顶元素,然后判断倒数第二个数据是否小于倒数第一个数据,如果是则压入1,反之压入0
				top--;
				stack[top] = stack[top] < stack[top + 1];
				break;
			case OPR_GEQ://(OPR,--,10),他做的事情是弹出两个栈顶元素,然后判断倒数第二个数据是否大于等于倒数第一个数据,如果是则压入1,反之压入0
				top--;
				stack[top] = stack[top] >= stack[top + 1];
				break;
			case OPR_GTR://(OPR,--,11),他做的事情是弹出两个栈顶元素,然后判断倒数第二个数据是否大于倒数第一个数据,如果是则压入1,反之压入0
				top--;
				stack[top] = stack[top] > stack[top + 1];
				break;
			case OPR_LEQ:////(OPR,--,11),他做的事情是弹出两个栈顶元素,然后判断倒数第二个数据是否小于等于倒数第一个数据,如果是则压入1,反之压入0
				top--;
				stack[top] = stack[top] <= stack[top + 1];
				break;
			//Tianyi's_contribution:
			case OPR_SST://(OPR,--,13),将当前的栈保存起来
				for(j = 0;j <= top;j++)saved_stack[j] = stack[j];
				saved_top = top;
				saved_b = b;
				break;
			case OPR_LST://(OPR,--,14),将栈恢复
				for(j = 0;j <= saved_top;j++)stack[j] = saved_stack[j];
				top = saved_top;
				b = saved_b;
				break;
			case OPR_SAVE0://(OPR,--,15),将当前栈顶存到stack[0]并弹出
				stack[0] = stack[top];
				top--;
				break;
			case OPR_SAVE1://(OPR,--,16),将pc+2的值存入stack[1]
				stack[1] = pc + 2;
				break;
			case OPR_LOAD0://(OPR,--,17),将stack[0]中的值压入栈顶
				top++;
				stack[top] = stack[0];
				break;
			case OPR_LOAD1://(OPR,--,18),将stack[1]中的值读入pc
				pc = stack[1];
				break;
			} // switch
			break;
		case LOD://(LOD,层次差,相对偏移即dx),它做的是将变量的值压入栈顶
			stack[++top] = stack[base(stack, b, i.l) + i.a];
			break;
		case STO://(STO,层次差,相对偏移即dx),他做的是将栈顶数据赋值给变量,然后弹栈,它会把赋值的结果打印出来
			stack[base(stack, b, i.l) + i.a] = stack[top];
			//Xulong's_contribution:翻译时只让PRT指令做打印
			//printf("%d\n", stack[top]);
			top--;
			break;
		case CAL://(CAL,层次差,代码地址),这里也存在层次差,因为过程名称和变量名称在层次模型中本质上是名称,它做的事:调用函数,然后pc变成过程的入口地址
			stack[top + 1] = base(stack, b, i.l);//填入静态链 
			// generate new block mark
			stack[top + 2] = b;//填入动态链(反应函数调用关系,即填的是它调用者的基地址)
			stack[top + 3] = pc;//填入返回地址
			b = top + 1;//这里它让b始终指向基地址
			pc = i.a;
			break;
		case INT://(INT,--,常数),它做的事开辟局部空间,即让栈顶增加一个常数
			top += i.a;
			break;
		case JMP://(JMP,--,常数),它就是让pc等于某个常数,即程序下一步执行那句中间代码
			pc = i.a;
			break;
		case JPC://(JPC,--,常数),判断栈顶与那是是不是0,如果是就跳转指令,即逻辑非跳转
			if (stack[top] == 0)
				pc = i.a;
			top--;
			break;
		//Xinyu's_contribution:
		case LEA://(LEA,层次差,相对偏移),他做的是取绝对地址压入栈顶
			stack[++top] = base(stack, b, i.l) + i.a;
			break;
		case LDA://(LDA,--,--),它做的是把栈顶数据当成地址去取数据,然后修改栈顶数据
			stack[top] = stack[stack[top]];
			break;
		case STA://(STA,--,--),它做的事把栈顶数据赋值给 以倒数第二个数据为地址的数据,然后弹栈两次
			stack[stack[top - 1]] = stack[top];
			top = top - 2;
			break;
		case PRT://(PRT,--,常数),它做的是弹出栈顶常数个数据并打印
			if (i.a == 0) { printf("\n"); }
			else {
				for (int k = i.a - 1;k >= 0;k--) {
					printf("%d ", stack[top - k]);
				}
				top = top - i.a;  
			}
			break;
		} // switch
	}
	while (pc);

	printf("End executing PL/0 program.\n");
} // interpret

//////////////////////////////////////////////////////////////////////
void main (int argc,char* argv[])
{
	FILE* hbin;//指向生成的中间代码文件
	char s[80];
	int i;
	symset set, set1, set2;
	if(argc==1)
	{
		printf("Please input source file name: "); // get file name to be compiled
		fflush(stdout);//刷新标准输出缓存
		scanf("%s", s);
	}
	else if(argc==2)
	{
		strcpy(s,argv[1]);
	}
	else
	{
		printf("too many files\n");
	}
	if ((infile = fopen(s, "r")) == NULL)
	{
		printf("File %s can't be opened.\n", s);
		exit(1);
	}//以上是读入一个文件
	phi = createset(SYM_NULL);
	relset = createset(SYM_EQU, SYM_NEQ, SYM_LES, SYM_LEQ, SYM_GTR, SYM_GEQ, SYM_NULL);
	
	// create begin symbol sets
	declbegsys = createset(SYM_CONST, SYM_VAR, SYM_PROCEDURE, SYM_NULL);
	//Xulong's_contribution:
	statbegsys = createset(SYM_BEGIN, SYM_CALL, SYM_IF, SYM_WHILE,SYM_FOR, SYM_PRT,SYM_NULL);
	//Tianyi's_contribution:
	facbegsys = createset(SYM_IDENTIFIER, SYM_NUMBER, SYM_LPAREN, SYM_MINUS, SYM_SETJ,SYM_NULL);
	//以上是一些标志集合

	err = cc = cx = ll = 0; // initialize global variables,
	ch = ' ';//最近读的一个字符
	kk = MAXIDLEN;//这句代码没用

	getsym();

	set1 = createset(SYM_PERIOD, SYM_NULL);
	set2 = uniteset(declbegsys, statbegsys);
	set = uniteset(set1, set2);//set是一堆标识符的集合:结束符,类型推断标识符,statement的first标识符
	block(set);//进入函数体分析
	destroyset(set1);
	destroyset(set2);
	destroyset(set);
	destroyset(phi);
	destroyset(relset);
	destroyset(declbegsys);
	destroyset(statbegsys);
	destroyset(facbegsys);

	if (sym != SYM_PERIOD)//我们这个函数体最后应该一个结束符
		error(9); // '.' expected.
	if (err == 0)
	{
		if((hbin = fopen("hbin.txt", "w"))==NULL)
		{
			printf("file hbin.txt cant be opened\n");
			exit(0);
		}
		for (i = 0; i < cx; i++)
			{
				if(fwrite(&code[i], sizeof(instruction), 1, hbin)!=1)
					printf("fwrite error!\n");
			}
		fclose(hbin);
	}
	if (err == 0)
		interpret();
	else
		printf("There are %d error(s) in PL/0 program.\n", err);
	listcode(0, cx);
	system("pause");
} // main

//////////////////////////////////////////////////////////////////////
// eof pl0.c
