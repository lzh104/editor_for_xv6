#include "types.h" // uint��Щ�Ķ���
#include "stat.h"  // stat�Ķ���
#include "user.h"  // ϵͳ���ú�һЩC����������
#include "fcntl.h" // O_RDONLY

#define NULL 0
#define BUFF_SIZE 256
#define MAX_ROW_N 256
#define MAX_ROW_L 255

char whitespace[] = " \t\r\n\v";

/*
����
1. �Զ������ļ�
2. �ļ������ܲ��ܲ���ɾ���ٽ�һ��
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

// β�崴���µ���
struct Row *create_row_tail(struct FText *ftext);
// �����µ�FText
struct FText *create_ftext(int fd);
// ��ӡFText
void print_ftext(struct FText *ftext);
// �ͷ�ftext
void free_ftext(struct FText *ftext);
// ��ȡ����
int get_input(char *buf, int size);
// ��ȡ���������
int get_cmd(char *buf, int size);
// ������������ļ�
int cmd_save(struct FText *ftext, char *path);
// ��������˳���ǰ�ļ�
int cmd_quit(struct FText *ftext, char *path);
// �з����������һ����εĵ�ַ(���û�з���NULL)
char *sep_cmd(char **cmd);
// �ַ���ת�����������󷵻�-1
int str2uint(char *str);
// ��õ�n��
struct Row *get_row(struct FText *ftext, int n);
// �������չʾ
int cmd_list(struct FText *ftext, char *path, int n);
// ��������ڵ�i�в���һ��
int cmd_ins(struct FText *ftext, char *path, int n);
// �������ɾ����i��
int cmd_del(struct FText *ftext, char *path, int n);
// �ڵ�i������һ����
struct Row *create_new_row(struct FText *ftext, int n);
// ɾ����i��
int del_row(struct FText *ftext, int n);

int main(int argc, char *argv[])
{
    int fd;
    uint file_no; // �ļ����

    printf(1, "welcome to use my editor ^_^ \n");

    // ����������
    if (argc == 1)
    {
        printf(1, ">>> editor: please input in the format: editor file_name\n");
        exit();
    }

    // ��ÿһ���ļ�
    for (file_no = 1; file_no < argc; file_no++)
    {
        // ���ļ�
        printf(1, ">>> opening %s ... \n", argv[file_no]);
        if ((fd = open(argv[file_no], O_RDONLY)) < 0)
        {
            printf(1, ">>> editor: cannot open %s\n", argv[file_no]);
            continue;
        }
        printf(1, ">>> finish! \n", argv[file_no]);

        // ���ļ����ݴ���FText�ṹ����
        struct FText *ftext = create_ftext(fd);
        if (!ftext)
        {
            printf(1, ">>> editor: open %s error\n", argv[file_no]);
            continue;
        }

        // ��ӡ�ı�����
        print_ftext(ftext);

        // ��������
        char buf[BUFF_SIZE];
        char *ncmd, *s;

        while (get_cmd(buf, BUFF_SIZE) >= 0)
        {
            s = buf;
            printf(1, ">>> command: %s \n", s);

            if ((ncmd = sep_cmd(&s)))
            {
                // �˳�
                if (!strcmp(ncmd, "quit"))
                {
                    printf(1, ">>> quiting... \n");
                    if (cmd_quit(ftext, argv[file_no]) > 0)
                        break;
                    printf(1, ">>> editor: quit %s error\n", argv[file_no]);
                }
                // ����
                else if (!strcmp(ncmd, "save"))
                {
                    printf(1, ">>> saving... \n");
                    if (cmd_save(ftext, argv[file_no]) < 0)
                        printf(1, ">>> editor: save %s error\n", argv[file_no]);
                }
                // չʾ
                else if (!strcmp(ncmd, "list"))
                {
                    int n = -1;
                    // ����в���
                    if ((ncmd = sep_cmd(&s)))
                    {
                        // ��ȷҳ�ŷ��طǸ�ֵ
                        n = str2uint(ncmd);
                        if (n < 0)
                        {
                            printf(1, ">>> editor: error parameter, you should try: list/list n\n");
                            continue;
                        }
                    }
                    cmd_list(ftext, argv[file_no], n);
                }
                // ����һ��
                else if (!strcmp(ncmd, "ins"))
                {
                    int n = -1;
                    // ����в���
                    if ((ncmd = sep_cmd(&s)))
                    {
                        // ��ȷҳ�ŷ��طǸ�ֵ
                        n = str2uint(ncmd);
                        if (n < 0)
                        {
                            printf(1, ">>> editor: error parameter, you should try: ins/ins n\n");
                            continue;
                        }
                    }
                    cmd_ins(ftext, argv[file_no], n);
                }
                // ɾ��һ��
                else if (!strcmp(ncmd, "del"))
                {
                    int n = -1;
                    // ����в���
                    if ((ncmd = sep_cmd(&s)))
                    {
                        // ��ȷҳ�ŷ��طǸ�ֵ
                        n = str2uint(ncmd);
                        if (n < 0)
                        {
                            printf(1, ">>> editor: error parameter, you should try: del/del n\n");
                            continue;
                        }
                    }
                    cmd_del(ftext, argv[file_no], n);
                }
                // ��Ч����
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
    // �ַ����к��з������ַ�
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

    // �����հ׷�
    while (*i && strchr(whitespace, *i))
        i++;
    // ��һ���������ʼ��ַ(������0)
    if (*i == 0)
        return NULL;
    ncmd = i;
    // �ƶ�����һ�Ͻ�����ĵ�һ���ַ�,��Ϊ'\0'
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

int cmd_del(struct FText *ftext, char *path, int n)
{
    if (n >= ftext->size)
    {
        printf(1, ">>> ins error: there are only row[0~%d] in '%d'\n", ftext->size - 1, path);
        return -1;
    }
    if (n < 0)
        n = ftext->size - 1;

    del_row(ftext, n);
    ftext->dirty = 1;
    if (n < ftext->size)
        cmd_list(ftext, path, n);
    else
        cmd_list(ftext, path, ftext->size - 1);

    return 1;
}

int cmd_ins(struct FText *ftext, char *path, int n)
{
    char buf[BUFF_SIZE];
    struct Row *row;
    int len;
    if (n >= ftext->size)
    {
        printf(1, ">>> ins error: there are only row[0~%d] in '%d'\n", ftext->size - 1, path);
        return -1;
    }
    if (n < 0)
        n = ftext->size;

    printf(1, ">>> please insert the text you want to insert: \n");
    if (n / 100)
        printf(1, "%d | ", n);
    else if (n / 10)
        printf(1, " %d | ", n);
    else
        printf(1, "  %d | ", n);

    // ����һ��
    if (get_input(buf, BUFF_SIZE) >= 0)
    {
        len = strlen(buf);
        // too long for a row
        if (len > MAX_ROW_L)
        {
            printf(1, ">>> ins error: too long \n");
            return -1;
        }
        // ����

        row = create_new_row(ftext, n);
        memmove(row->str, buf, len);
        row->l += len;

        // row->str[row->l] = 0;

        cmd_list(ftext, path, n);
    }

    ftext->dirty = 1;
    return 1;
}

int del_row(struct FText *ftext, int n)
{
    struct Row *row;

    // ���ʱ����ڵ�ǰ����к�
    if (n > ftext->size - 1)
        return -1;

    row = get_row(ftext, n);
    row->next->prev = row->prev;
    row->prev->next = row->next;
    free(row);

    ftext->size--;

    return 1;
}

struct Row *create_new_row(struct FText *ftext, int n)
{
    struct Row *row, *nrow;
    // ������ڵ�ǰ����кţ�����������һ������
    if (n >= ftext->size)
        return create_row_tail(ftext);

    row = get_row(ftext, n);
    nrow = malloc(sizeof(*nrow));
    nrow->l = 0;
    nrow->next = row;
    nrow->prev = row->prev;
    row->prev->next = nrow;
    row->prev = nrow;
    ftext->size++;
    return nrow;
}

int cmd_list(struct FText *ftext, char *path, int n)
{
    struct Row *row;
    int line_no;
    if (ftext->size == 0)
        printf(1, ">>> '%s' is empty now\n", path);

    // ȫ��չʾ
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
    // ����
    else if (n >= ftext->size)
    {
        printf(1, ">>> list: there are only row[0~%d] in '%s'\n", ftext->size - 1, path);
        return -1;
    }
    // չʾ�ֲ�
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

            get_input(buf, BUFF_SIZE);
        } while (!(buf[0] == 'y' || buf[0] == 'Y' || buf[0] == 'n' || buf[0] == 'N'));

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

    // ɾ��ԭ�ļ����½�һ���µ�ͬ���ļ�
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
    return get_input(buf, size);
}

int get_input(char *buf, int size)
{
    memset(buf, 0, size);
    gets(buf, size);
    if (buf[0] == 0)
        return -1;
    buf[strlen(buf) - 1] = 0; // ȥ���س���
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
    struct Row *row; // ���ڶ������
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
        i = 0; // buf���α�
        do
        {
            beg = i; // ��λ�¾���

            // �����Ҫ��β��һ������
            if (new_row)
            {
                row = create_row_tail(ftext);
                new_row = 0;
            }

            // ����'\n'ǰ������buf�����ݵ�rowĩβ
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

            // ������һ�䣨������'\n'��
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