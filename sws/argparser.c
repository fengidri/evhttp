/**
 *   author       :   丁雪峰
 *   time         :   2016-07-29 08:53:33
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "sws.h"



struct _sws_arg{
    const char *arg;
    void *value;
    void *type;
    const char *help;
    struct _sws_arg *next;
};

typedef struct _sws_arg sws_arg;
typedef int (*sws_ap_handle)(void *value, const char *arg);


static sws_arg arg_head;


void sws_ap_opt(const char *arg, void *value, void *type, const char *help)
{
    sws_arg *A;
    sws_arg *B;
    A = malloc(sizeof(*A));
    A->next = NULL;
    A->arg = arg;
    A->value = value;
    A->type = type;
    A->help = help;

    B = &arg_head;
    while (B->next) B = B->next;

    B->next = A;
}


void sws_ap_pos(const char *arg, void *value, void *type, const char *help)
{

}


static int sws_handler(int argc, const char **argv, int i)
{
    sws_arg *A;
    const char *p;
    p = argv[i];

    A = arg_head.next;
    while (A){
        if (!strcmp(p, A->arg))
        {
            goto found;
        }
        A = A->next;
    }
    return -1;// not found;

found:
    if (SWS_AP_BOOL == A->type){
        *(int*)A->value = ~*(int *)A->value;
        return i;
    }

    ++i;
    if (i >= argc) return -2; //execpt a value

    switch((long)A->type)
    {
        case (long)SWS_AP_INT:
            *(int*)A->value = atoi(argv[i]);
            break;

        case (long)SWS_AP_DOUBLE:
            *(double*)A->value = atof(argv[i]);
            break;

        case (long)SWS_AP_STRING:
            *(const char **)A->value = argv[i];
            break;

        default:
            ((sws_ap_handle)A->type)(A->value, argv[i]);
    }
    return i;
}


static const char *convert_type_to_name(sws_arg *A)
{
    switch((long)A->type)
    {
        case (long)SWS_AP_BOOL:
            return "";

        case (long)SWS_AP_INT:
            return "<int>";

        case (long)SWS_AP_DOUBLE:
            return "<double>";
            break;

        default:
            return "<str>";
    }
}


static void call_help_print(int argc, const char **argv)
{
    sws_arg *A;
    const char *help;
    int optl = 0;
    const char *opt;

    A = arg_head.next;

    while (A){
        if (optl < strlen(A->arg))
        {
            optl = strlen(A->arg);
        }
        A = A->next;
    }

    A = arg_head.next;

    printf("Usage: %s [-h]\n\n", argv[0]);
    printf("optional arguments:\n");
    while (A){
        help = A->help ? A->help : "";

        opt = "";

        printf("  %-*s %-5s %s\n", optl, A->arg, convert_type_to_name(A), A->help);
        A = A->next;
    }

}


int sws_ap(int argc, const char **argv)
{
    int i, ii;
    int help = 0;

    sws_ap_bool("-h", &help, "print this help info");
    sws_ap_bool("--help", &help, "print this help info");

    i = 1;
    while(i < argc)
    {
        ii = sws_handler(argc, argv, i);
        if (-1 == ii)
        {
            printf("invalid option: `%s'\n", argv[i]);
            return -1;
        }
        if (-2 == ii)
        {
            printf("option: `%s' except arg\n", argv[i]);
            return -1;
        }
        i = ii + 1;
    }

    if (help)
    {
        call_help_print(argc, argv);
        return -1;
    }



    return 0;
}



