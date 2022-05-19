#include "types.h" // uint那些的定义
#include "stat.h"  // stat的定义
#include "user.h"  // 系统调用和一些C函数的声明
#include "fcntl.h" // O_RDONLY

#define NULL 0
#define BUFF_SIZE 256
#define MAX_ROW_N 256
#define MAX_ROW_L 255

char whitespace[] = " \t\r\n\v";

/*
需求：
1. 自动创建文件
2. 文件保存能不能不用删了再建一个
*/

struct Row
{
    int l;
    char str[MAX_ROW_L + 1];
    struct Row *prev;
    struct Row *next;
};

struct FText
{
    int fd;
    int size;
    struct Row head;
    int dirty;
};

// 尾插创建新的行
struct Row *create_row_tail(struct FText *ftext);
// 创建新的FText
struct FText *create_ftext(int fd);
// 打印FText
void print_ftext(struct FText *ftext);
// 释放ftext
void free_ftext(struct FText *ftext);
// 获取输入的命令
int get_cmd(char *buf, int size);
// 保存文件
int cmd_save(struct FText *ftext, char *path);
// 退出当前文件
int cmd_quit(struct FText *ftext, char *path);
// 切分命令，返回下一命令段的地址(如果没有返回NULL)
char *sep_cmd(char **cmd);
// 字符串转正整数，错误返回-1
int str2uint(char *str);
// 获得第n行
struct Row *get_row(struct FText *ftext, int n);
int cmd_list(struct FText *ftext, char *path, int n);

int main(int argc, char *argv[])
{
    int fd;
    uint file_no; // 文件序号

    printf(1, "welcome to use my editor ^_^ \n");

    // 检测命令参数
    if (argc == 1)
    {
        printf(1, ">>> editor: please input in the format: editor file_name\n");
        exit();
    }

    // 对每一个文件
    for (file_no = 1; file_no < argc; file_no++)
    {
        // 打开文件
        printf(1, ">>> opening %s ... \n", argv[file_no]);
        if ((fd = open(argv[file_no], O_RDONLY)) < 0)
        {
            printf(1, ">>> editor: cannot open %s\n", argv[file_no]);
            continue;
        }
        printf(1, ">>> finish! \n", argv[file_no]);

        // 将文件内容存在FText结构体中
        struct FText *ftext = create_ftext(fd);
        if (!ftext)
        {
            printf(1, ">>> editor: open %s error\n", argv[file_no]);
            continue;
        }

        // 打印文本内容
        print_ftext(ftext);

        // 处理命令
        char buf[BUFF_SIZE];
        char *ncmd, *s;

        while (get_cmd(buf, BUFF_SIZE) >= 0)
        {
            s = buf;
            buf[strlen(s) - 1] = 0; // 去掉回车符
            printf(1, ">>> command: %s \n", s);

            if ((ncmd = sep_cmd(&s)))
            {
                // 退出
                if (!strcmp(ncmd, "quit"))
                {
                    printf(1, ">>> quiting... \n");
                    if (cmd_quit(ftext, argv[file_no]) > 0)
                        break;
                    printf(1, ">>> editor: quit %s error\n", argv[file_no]);
                }
                // 保存
                else if (!strcmp(ncmd, "save"))
                {
                    printf(1, ">>> saving... \n");
                    if (cmd_save(ftext, argv[file_no]) < 0)
                        printf(1, ">>> editor: save %s error\n", argv[file_no]);
                }
                // 展示
                else if (!strcmp(ncmd, "list"))
                {
                    printf(1, ">>> list... \n");
                    int n = -1;
                    // 如果有参数
                    if ((ncmd = sep_cmd(&s)))
                    {
                        // 正确页号返回非负值
                        n = str2uint(ncmd);
                        if (n < 0)
                        {
                            printf(1, ">>> editor: error parameter, you should try: list/list n\n");
                            continue;
                        }
                    }
                    cmd_list(ftext, argv[file_no], n);
                }
                // 无效命令
                else
                {
                    printf(1, ">>> editor: invalid command, please re-enter\n");
                }
            }
        }

        free_ftext(ftext);
    }

    exit();
}

int str2uint(char *str)
{
    int sum = 0;

    while (*str && *str >= '0' && *str <= '9')
    {
        sum *= 10;
        sum += *str - '0';
        str++;
    }
    // 字符串中含有非数字字符
    if (*str)
        return -1;

    return sum;
}

char *sep_cmd(char **cmd)
{
    char *i;
    char *ncmd;

    i = *cmd;
    if (*i == 0)
        return NULL;

    // 跳过空白符
    while (*i && strchr(whitespace, *i))
        i++;
    // 下一断命令的起始地址(可能是0)
    if (*i == 0)
        return NULL;
    ncmd = i;
    // 移动到下一断结束后的第一个字符,改为'\0'
    while (*i && !strchr(whitespace, *i))
        i++;

    if (*i)
    {
        *i = 0;
        i++;
    }
    *cmd = i;

    return ncmd;
}

struct Row *get_row(struct FText *ftext, int n)
{
    struct Row *row;
    int count;

    if (n >= ftext->size)
        return NULL;
    for (row = ftext->head.next, count = 0; count < n; row = row->next, count++)
        ;

    return row;
}

int cmd_list(struct FText *ftext, char *path, int n)
{
    struct Row *row;
    int line_no;

    // 全文展示
    if (n < 0)
    {
        line_no = 0;
        printf(1, "[top] \n");
        row = ftext->head.next;
        while (row != &ftext->head)
        {
            if (line_no / 100)
                printf(1, "%d | %s\n", line_no++, row->str);
            else if (line_no / 10)
                printf(1, " %d | %s\n", line_no++, row->str);
            else
                printf(1, "  %d | %s\n", line_no++, row->str);
            row = row->next;
        }
        printf(1, "[end] \n");
    }
    // 过长
    else if (n >= ftext->size)
    {
        printf(1, ">>> list: there are only row[0~%d] in '%d'\n", ftext->size - 1, path);
        return -1;
    }
    // 展示局部
    else
    {
        int n1 = n, n2 = n, shiftr = 2, shiftl = 2;
        if (n1 == 1)
            shiftr++;
        else if (n1 == 0)
            shiftr += 2;

        if (n2 == ftext->size - 2)
            shiftl++;
        else if (n1 == ftext->size - 1)
            shiftl += 2;

        n1 = n1 - shiftl < 0 ? 0 : n1 - shiftl;
        n2 = shiftr + n < ftext->size ? shiftr + n : ftext->size - 1;
        row = get_row(ftext, n1);
        if (n1 == 0)
            printf(1, "[top] \n");
        else
            printf(1, "...... \n");
        while (n1 <= n2)
        {
            if (n1 / 100)
                printf(1, "%d | %s\n", n1++, row->str);
            else if (n1 / 10)
                printf(1, " %d | %s\n", n1++, row->str);
            else
                printf(1, "  %d | %s\n", n1++, row->str);
            row = row->next;
        }
        if (n1 == ftext->size)
            printf(1, "[end] \n");
        else
            printf(1, "...... \n");
    }

    return 1;
}

int cmd_quit(struct FText *ftext, char *path)
{
    if (ftext->dirty)
    {
        char buf[BUFF_SIZE];
        do
        {
            printf(1, ">>> quit: %s had been change, \n", path);
            printf(1, "         do you want to save it? [y/n]   ");

            memset(buf, 0, BUFF_SIZE);
            gets(buf, BUFF_SIZE);
        } while (buf[0] == 'y' || buf[0] == 'Y' || buf[0] == 'n' || buf[0] == 'N');

        if (buf[0] == 'y' || buf[0] == 'Y')
        {
            if (cmd_save(ftext, path) < 0)
            {
                printf(1, ">>> quit: save %s error\n", path);
                return -1;
            }
        }
    }

    close(ftext->fd);
    return 1;
}

int cmd_save(struct FText *ftext, char *path)
{
    struct Row *row;
    int fd;
    char linebreak = '\n';

    if (!ftext->dirty)
        return 1;

    // 删除原文件并新建一个新的同名文件
    unlink(path);
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        printf(1, ">>> save: cannot open new file\n");
        return -1;
    }

    for (row = ftext->head.next; row != &ftext->head; row = row->next)
    {
        write(fd, row->str, row->l);
        if (row != ftext->head.prev)
            write(fd, &linebreak, 1);
    }

    ftext->dirty = 0;

    return 1;
}

int get_cmd(char *buf, int size)
{
    printf(1, "<<< ");
    memset(buf, 0, size);
    gets(buf, size);
    if (buf[0] == 0)
        return -1;
    return 0;
}

struct Row *create_row_tail(struct FText *ftext)
{
    struct Row *row;

    row = malloc(sizeof(*row));
    ftext->head.prev->next = row;
    row->prev = ftext->head.prev;
    row->next = &(ftext->head);
    ftext->head.prev = row;
    row->l = 0;
    ftext->size++;

    return row;
}

void free_ftext(struct FText *ftext)
{
    if (!ftext)
    {
        return;
    }

    struct Row *row = ftext->head.next;
    while (row != &ftext->head)
    {
        row = row->next;
        free(row->prev);
    }
    free(ftext);
}

struct FText *create_ftext(int fd)
{
    struct FText *ftext;
    struct Row *row; // 正在读入的行
    int len, i, beg, new_row;
    char buf[BUFF_SIZE];

    ftext = malloc(sizeof(*ftext));
    ftext->fd = fd;
    ftext->dirty = 0;
    ftext->size = 0;
    ftext->head.prev = &(ftext->head);
    ftext->head.next = &(ftext->head);

    new_row = 1;
    while ((len = read(fd, buf, BUFF_SIZE)) > 0)
    {
        i = 0; // buf的游标
        do
        {
            beg = i; // 定位新句首

            // 如果需要，尾插一个新行
            if (new_row)
            {
                row = create_row_tail(ftext);
                new_row = 0;
            }

            // 复制'\n'前或整个buf的内容到row末尾
            for (; i < len && buf[i] != '\n'; i++)
                ;
            if (row->l + (i - beg) > MAX_ROW_L)
            {
                printf(1, ">>> create_ftext: row %d is too long!\n", ftext->size - 1);
                free_ftext(ftext);
                return NULL;
            }

            memmove(row->str + row->l, buf + beg, (i - beg));
            row->l += (i - beg);

            // 读完了一句（读到了'\n'）
            if (i < len && buf[i] == '\n')
            {
                row->str[row->l] = 0;
                new_row = 1;
                i++;
            }
        } while (i < len);
    }
    row->str[row->l] = 0;

    return ftext;
}

void print_ftext(struct FText *ftext)
{
    struct Row *row;
    int line_no = 0;

    printf(1, ">>> [begin] \n");
    row = ftext->head.next;
    while (row != &ftext->head)
    {
        if (line_no / 100)
            printf(1, "%d | %s\n", line_no++, row->str);
        else if (line_no / 10)
            printf(1, " %d | %s\n", line_no++, row->str);
        else
            printf(1, "  %d | %s\n", line_no++, row->str);
        row = row->next;
    }
    printf(1, ">>> [end] \n");
}