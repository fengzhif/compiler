<!--
 * Topic: 
 * Highlight:
 *  Shawn Jia  in University of sci&tech of China
 * @LastEditTime: 2022-12-08 11:25:36
-->
# Report

for_statement -> for ( var ID : (low, up, step)) statement
| for ( var ID : (low, up)) statement

1. 读到for标识时,我们让循环层次加1,表示进入下一重循环,由于我们可以预料到存在3个变量:`low`和`up`和`step`,我们生成代码`(INT,0,3)`表示扩大局部空间,并且在循环控制信息表项中记录它们的偏移:`low_dx`,`high_dx`,`step_dx`,

```c
depth_of_loop++;//表示循环层次加1
//因为我们知道for循环中一定有这3个连续存储的值,那就实现把它们的偏移记下来
for_loop_table[depth_of_loop].low_dx=dx;
for_loop_table[depth_of_loop].high_dx=dx+1;
for_loop_table[depth_of_loop].step_dx=dx+2;
gen(INT,0,3);
```

2. 读到ID时,**这个ID就是for循环声明的变量**,它的初值是`low`,我们直接在符号表中添加这一变量`enter(VARIABLE)`,由于这个ID可能和外部的ID发生重名,**我修改了enter和position函数,使之可以识别for循环中的变量**。

```c
if(depth_of_loop!=0)
{
    char temp[MAXIDLEN+2]={(char)depth_of_loop+'0','\0'};
    strcpy(id,strncat(temp,id,MAXIDLEN));//我们给id加上新的标记使它的第一个字符代表for循环层次
    printf("%s\n",id);
}
```

>在enter函数中,如果当前处于for循环中,我们添加的表项的名称,一定就是 **循环内部声明的变量** ,那么用以区分,我们给它的名字加个数字标志(即在名字前面加一个当前所在的循环层次),这样就不会重名因为正常情况下,我们不允许id以数字开头

```c
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
```

>在position函数中,也要区分是否在for循环内部,如果在循环内部,那就优先考虑以数字标志开头的表项,如果发现不存在这个表项,那再去掉数字标志后再去寻找(即此时我们找的名称可能不是for声明的变量)

实际上,上面这一设计就能实现循环内部名称隐藏外部名称的功能,例如我们允许以下语法

```shell
var i;
begin
  for(var i:(1,10))
    for(var i:(10,0,-3))
        for(var i:(0,9))
end.
```

>上面的4个重名id,`i`,我们在不同for层次中可以区分

1. 读到low时,进行expression分析,然后生成代码`(STO,0,low_dx)`,表示把栈顶元素赋值给`low`
2. 读到high时,进行expression分析,然后生成代码`(STO,0,high_dx)`,表示把栈顶元素赋值给`high`
3. 读到step时,进行expression分析,然后生成代码`(STO,0,step_dx)`,表示把栈顶元素赋值给`step_dx`,若不存在step,那就生成`(LIT,0,1)`和`(STO,0,step_dx)`,表示step的值是1
4. 在做statement分析之前,我们生成如下代码

```c
cx1=cx;//for循环跳回地址
gen(LOD,0,for_loop_table[depth_of_loop].low_dx);//把var id的值压栈
gen(LOD,0,for_loop_table[depth_of_loop].high_dx);//把high的值压栈
gen(OPR,0,OPR_LEQ);//判断id是不是小于high
cx2=cx;
gen(JPC,0,0);//跳出for循环,等待回填
```

>这里我们要保存跳回地址,还要等待后续回填跳出地址

5. 做statement分析

6. statement分析后,生成如下代码

```
gen(LOD,0,for_loop_table[depth_of_loop].low_dx);//栈顶为low的值
gen(LOD,0,for_loop_table[depth_of_loop].step_dx);//栈顶为step的值
gen(OPR,0,OPR_ADD);//将low和step相加
gen(STO,0,for_loop_table[depth_of_loop].low_dx);//将结果放回low中
gen(JMP,0,cx1);//for循环跳回
code[cx2].a=cx;//回填跳出for循环地址
```

>这里让low加上step,然后跳回去

7. 做完for循环后,我们需要做3件事:把分配空间删除,将我们在for中声明的名称删除,将循环层次减1

```c
int index=position(id_in_loop);
if(index==0)
{
    printf("error cant find id in loop\n");
    exit(0);
}
gen(INT,0,-3);//把分配空间释放
tx--;
dx=dx-3;
depth_of_loop--;//循环层次减一
```