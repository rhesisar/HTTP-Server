#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "syntax.h"
#include "tree.h"
#include "util.h"

int
http_message(char **sp, char *s_end)
{
    Node **cur;

    if (*sp > s_end)
        return 1;

    createnode(&root, "HTTP_message", *sp, s_end - *sp + 1, NULL, NULL);
    cur = &(root->child);

    if (start_line(sp, s_end, &cur)){
        freeTree(root);
        root = NULL;
        return 1;
    }
    while (1) {
        if (header_field(sp, s_end, &cur)
        || crlf(sp, s_end, &cur)) {
            break;
        }
    }
    if (crlf(sp, s_end, &cur)){
        freeTree(root);
        root = NULL;
        return 1;
    }
    message_body(sp, s_end, &cur);
    if (*sp <= s_end){
        freeTree(root);
        root = NULL;
        return 1;
    }
    return 0;
}

int
crlf(char **sp, char *s_end, Node ***n)
{
    int i;
    const char *s;

    s = "\r\n";

    if (s_end - *sp + 1 < strlen(s))
        return 1;
    for (i = 0; i < strlen(s); i++){
        if (tolower(s[i]) != tolower((*sp)[i]))
            return 1;
    }
    createnode(*n, "CRLF", *sp, strlen(s), NULL, NULL);
    *n = &((**n)->sibling);
    *sp += strlen(s);
    return 0;
}

int
http_name(char **sp, char *s_end, Node ***n)
{
    int i;
    const char *s;

    s = "HTTP";

    if (s_end - *sp + 1 < strlen(s))
        return 1;
    for (i = 0; i < strlen(s); i++){
        if (tolower(s[i]) != tolower((*sp)[i]))
            return 1;
    }
    createnode(*n, "HTTP_name", *sp, strlen(s), NULL, NULL);
    *n = &((**n)->sibling);
    *sp += strlen(s);
    return 0;
}

int
http_version(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "HTTP_version", *sp, 0, NULL, NULL);

    cur = &((**n)->child);
    p = *sp;
    if(http_name(sp, s_end, &cur)
    || string(sp, s_end, &cur, "/")
    || digit(sp, s_end, &cur)
    || string(sp, s_end, &cur, ".")
    || digit(sp, s_end, &cur))
    {
      *sp = p;
      freeTree(**n);
      **n = NULL;
      return 1;

    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
field_content(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;
    int i;

    createnode(*n, "field_content", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    if(field_vchar(sp, s_end, &cur))
        return 1;
    p = *sp;
    i = 0;
    while(1)
    {
        if(space(sp, s_end, &cur) && htab(sp, s_end, &cur)){
            break;
        }
        i++;
    }
    if (i < 1 || field_vchar(sp, s_end, &cur))
        *sp = p;

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
field_name(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "field_name", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    if(token(sp, s_end, &cur))
        return 1;

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
field_value(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "field_value", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    while(1)
    {
      if(field_content(sp, s_end, &cur) && obs_fold(sp, s_end, &cur))
        break;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
field_vchar(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "field_vchar", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    if(vchar(sp, s_end, &cur) && obs_text(sp, s_end, &cur))
    {
      freeTree(**n);
      **n = NULL;
      return 1;
    }


    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
message_body(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "message_body", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    while (1) {
        if (octet(sp, s_end, &cur) == 1)
            break;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
method(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "method", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    p=*sp;
    if(token(sp, s_end, &cur))
    {
      *sp=p;
      freeTree(**n);
      **n=NULL;
      return 1;
    }


    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
obs_fold(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;
    int i;

    createnode(*n, "obs_fold", *sp, 0, NULL, NULL);

    cur = &((**n)->child);
    p = *sp;
    if(crlf(sp, s_end, &cur))
      return 1;
    i = 0;
    while(1)
    {

      if(space(sp, s_end, &cur) && htab(sp, s_end, &cur)){
        break;
      }
      i++;
    }

    if(i<1){
      *sp = p;
      freeTree(**n);
      **n=NULL;
      return 1;

    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
obs_text(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "obs_text", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    if (range(sp, s_end, &cur, 0x80, 0xFF)){
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
origin_form(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "origin_form", *sp, 0, NULL, NULL);

    cur = &((**n)->child);
    
    if (absolute_path(sp, s_end,&cur)) {
        freeTree(**n);
        **n=NULL;
        return 1;
    }
    p = *sp;
    if (string(sp, s_end, &cur, "?")
    || query(sp, s_end, &cur))
         *sp = p;

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
reason_phrase(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "reason_phrase", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    while(1)
    {
      if(htab(sp, s_end, &cur)
      && space(sp, s_end, &cur)
      && vchar(sp, s_end, &cur)
      && obs_text(sp, s_end, &cur))
        break;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
request_line(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "request_line", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    p = *sp;
    if (method(sp, s_end, &cur)
    || space(sp, s_end, &cur)
    || request_target(sp, s_end, &cur)
    || space(sp, s_end, &cur)
    || http_version(sp, s_end, &cur)
    || crlf(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
request_target(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "request_target", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    if (origin_form(sp,s_end,&cur)) {
        freeTree(**n);
        **n=NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
start_line(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "start_line", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    if (request_line(sp, s_end, &cur)) {
        freeTree(**n);
        **n=NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
status_code(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "status_code", *sp, 0, NULL, NULL);

    cur = &((**n)->child);
    p =*sp;
    if(digit(sp, s_end, &cur)
    || digit(sp, s_end, &cur)
    || digit(sp, s_end, &cur))
    {
      *sp = p;
      freeTree(**n);
      **n = NULL;
      return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
status_line(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "status_line", *sp, 0, NULL, NULL);

    cur = &((**n)->child);
    p=*sp;
    if(http_version(sp, s_end, &cur)
    || space(sp, s_end, &cur)
    || status_code(sp, s_end, &cur)
    || space(sp, s_end, &cur)
    || reason_phrase(sp, s_end, &cur)
    || crlf(sp, s_end, &cur))
    {
      *sp = p;
      freeTree(**n);
      **n = NULL;
      return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
absolute_path(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    int i;

    createnode(*n, "absolute_path", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    i = 0;
    while(1)
    {
      if(string(sp, s_end, &cur, "/") || segment(sp, s_end, &cur) )
      {
        break;
      }
      i++;
    }

    if(i<1){
      freeTree(**n);
      **n = NULL;
      return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
Host(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Host", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (uri_host(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }
    p = *sp;
    if (string(sp, s_end, &cur, ":")
    || port(sp, s_end, &cur))
        *sp = p;

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
uri_host(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "uri_host", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (host(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
host(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "host", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (ip_literal(sp, s_end, &cur)
    && ipv4address(sp, s_end, &cur)
    && reg_name(sp, s_end, &cur)) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
port(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "port", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    while (1) {
        if (digit(sp, s_end, &cur)) {
            break;
        }
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
ip_literal(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "IP_literal", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (string(sp, s_end, &cur, "[")) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }
    if (ipv6address(sp, s_end, &cur)
    && ipvfuture(sp, s_end, &cur)) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    if (string(sp, s_end, &cur, "]")) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
ipvfuture(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;
    int i;

    createnode(*n, "IPvFuture", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "v")) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    i = 0;
    while (1) {
        if (hexdig(sp, s_end, &cur)) {
            break;
        }
        i++;
    }
    if (i < 1){
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    if (string(sp, s_end, &cur, ".")) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    i = 0;
    while (1) {
        if (unreserved(sp, s_end, &cur)
        && sub_delims(sp, s_end, &cur)
        && string(sp, s_end, &cur, ":")) {
            break;
        }
        i++;
    }
    if (i < 1){
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
ipv6address(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p1, *p2;
    int i;

    createnode(*n, "IPv6address", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    //6(h16 ":") ls32
    p1 = *sp;
    if (h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
    || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
    || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
    || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
    || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
    || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
    || ls32(sp, s_end, &cur)){
        // "::" 5(h16 ":") ls32
        *sp = p1;
        if (string(sp, s_end, &cur, "::")
        || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
        || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
        || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
        || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
        || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
        || ls32(sp, s_end, &cur)){
            // [h16 ] "::" 4(h16 ":") ls32
            *sp = p1;
            h16(sp, s_end, &cur);
            if (string(sp, s_end, &cur, "::")
            || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
            || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
            || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
            || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
            || ls32(sp, s_end, &cur)){
                *sp = p1;
                //[h16 *1( ":" h16 ) ] "::" 3(h16 ":") ls32
                if (!h16(sp, s_end, &cur)){
                    i = 0;
                    while (1) {
                        p2 = *sp;
                        if (string(sp, s_end, &cur, ":")
                        || h16(sp, s_end, &cur)) {
                            *sp = p2;
                            break;
                        }
                        i++;
                    }
                    if (i > 1){
                        *sp = p1;
                    }
                }


                if (string(sp, s_end, &cur, "::")
                || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
                || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
                || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
                || ls32(sp, s_end, &cur)){
                    //[h16 *2( ":" h16 ) ] "::" 2(h16 ":") ls32
                    *sp = p1;
                    if (!h16(sp, s_end, &cur)){
                        i = 0;
                        while (1) {
                            p2 = *sp;
                            if (string(sp, s_end, &cur, ":")
                            || h16(sp, s_end, &cur)) {
                                *sp = p2;
                                break;
                            }
                            i++;
                        }
                        if (i > 2){
                            *sp = p1;
                        }
                    }
                    if (string(sp, s_end, &cur, "::")
                    || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
                    || h16(sp, s_end, &cur) || string(sp, s_end, &cur, ":")
                    || ls32(sp, s_end, &cur)){
                        //[h16 *3( ":" h16 ) ] "::" h16 ":" ls32
                        *sp = p1;
                        if (!h16(sp, s_end, &cur)){
                            i = 0;
                            while (1) {
                                p2 = *sp;
                                if (string(sp, s_end, &cur, ":")
                                || h16(sp, s_end, &cur)) {
                                    *sp = p2;
                                    break;
                                }
                                i++;
                            }
                            if (i > 3){
                                *sp = p1;
                            }
                        }
                        if (string(sp, s_end, &cur, "::")
                        || h16(sp, s_end, &cur)
                        || string(sp, s_end, &cur, ":")
                        || ls32(sp, s_end, &cur)){
                            //[h16 *4( ":" h16 ) ] "::"  ls32
                            *sp = p1;
                            if (!h16(sp, s_end, &cur)){
                                i = 0;
                                while (1) {
                                    p2 = *sp;
                                    if (string(sp, s_end, &cur, ":")
                                    || h16(sp, s_end, &cur)) {
                                        *sp = p2;
                                        break;
                                    }
                                    i++;
                                }
                                if (i > 4){
                                    *sp = p1;
                                }
                            }
                            if (string(sp, s_end, &cur, "::")
                            || ls32(sp, s_end, &cur)){
                                //[h16 *5( ":" h16 ) ] "::"  h16
                                *sp = p1;
                                if (!h16(sp, s_end, &cur)){
                                    i = 0;
                                    while (1) {
                                        p2 = *sp;
                                        if (string(sp, s_end, &cur, ":")
                                        || h16(sp, s_end, &cur)) {
                                            *sp = p2;
                                            break;
                                        }
                                        i++;
                                    }
                                    if (i > 5){
                                        *sp = p1;
                                    }
                                }
                                if (string(sp, s_end, &cur, "::")
                                || h16(sp, s_end, &cur)){
                                    //[h16 *6(":" h16 ) ] "::"
                                    *sp = p1;
                                    if (!h16(sp, s_end, &cur)){
                                        i = 0;
                                        while (1) {
                                            p2 = *sp;
                                            if (string(sp, s_end, &cur, ":")
                                            || h16(sp, s_end, &cur)) {
                                                *sp = p2;
                                                break;
                                            }
                                            i++;
                                        }
                                        if (i > 6){
                                            *sp = p1;
                                        }
                                    }
                                    if (string(sp, s_end, &cur, "::"))
                                    {
                                        *sp = p1;
                                        freeTree(**n);
                                        **n = NULL;
                                        return 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
h16(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;
    int i;

    createnode(*n, "h16", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    i = 0;
    p = *sp;
    while (1) {
        if (hexdig(sp, s_end, &cur)) {
            break;
        }
        i++;
    }
    if (i < 1 || i > 4){
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
ls32(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;
    createnode(*n, "ls32", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);
    
    p = *sp;
    if (h16(sp, s_end, &cur)
    || string(sp, s_end, &cur, ":")
    || h16(sp, s_end, &cur)){
        *sp = p;
        if (ipv4address(sp, s_end, &cur)){
            freeTree(**n);
            **n = NULL;
            return 1;
        }
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
ipv4address(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "IPv4address", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (dec_octet(sp, s_end, &cur)
    || string(sp, s_end, &cur, ".")
    || dec_octet(sp, s_end, &cur)
    || string(sp, s_end, &cur, ".")
    || dec_octet(sp, s_end, &cur)
    || string(sp, s_end, &cur, ".")
    || dec_octet(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
dec_octet(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "dec_octet", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "25")
    || range(sp, s_end, &cur, 0x30, 0x35)){
        *sp = p;
        if (string(sp, s_end, &cur, "2")
        || range(sp, s_end, &cur, 0x30, 0x34)
        || digit(sp, s_end, &cur)){
            *sp = p;
            if (string(sp, s_end, &cur, "1")
            || digit(sp, s_end, &cur)
            || digit(sp, s_end, &cur)){
                *sp = p;
                if (range(sp, s_end, &cur, 0x31, 0x39)
                || digit(sp, s_end, &cur)){
                    *sp = p;
                    if (digit(sp, s_end, &cur)){
                        *sp = p;
                        freeTree(**n);
                        **n = NULL;
                        return 1;
                    }
                }
            }
        }
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
reg_name(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "reg_name", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    while (1) {
        if (unreserved(sp, s_end, &cur)
        && pct_encoded(sp, s_end, &cur)
        && sub_delims(sp, s_end, &cur))
            break;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
segment(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "segment", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    while(1){
      if(pchar(sp, s_end, &cur))
        break;

    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
pchar(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "pchar", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    if(unreserved(sp, s_end, &cur) && pct_encoded(sp, s_end, &cur) && sub_delims(sp, s_end, &cur) && string(sp, s_end, &cur, ":") &&string(sp, s_end, &cur, "@"))
    {
      freeTree(**n);
      **n =NULL;
      return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
query(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "query", *sp, 0, NULL, NULL);

    cur = &((**n)->child);

    while(1) {
      if(pchar(sp, s_end, &cur)
      && string(sp, s_end, &cur, "/")
      && string(sp, s_end, &cur, "?"))
        break;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
pct_encoded(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "pct_encoded", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "%")
    || hexdig(sp, s_end, &cur)
    || hexdig(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
unreserved(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "unreserved", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (alpha(sp, s_end, &cur)
    && digit(sp, s_end, &cur)
    && string(sp, s_end, &cur, "-")
    && string(sp, s_end, &cur, ".")
    && string(sp, s_end, &cur, "_")
    && string(sp, s_end, &cur, "~")) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
sub_delims(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "sub_delims", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (string(sp, s_end, &cur, "!")
    && string(sp, s_end, &cur, "$")
    && string(sp, s_end, &cur, "&")
    && string(sp, s_end, &cur, "\'")
    && string(sp, s_end, &cur, "(")
    && string(sp, s_end, &cur, ")")
    && string(sp, s_end, &cur, "*")
    && string(sp, s_end, &cur, "+")
    && string(sp, s_end, &cur, ",")
    && string(sp, s_end, &cur, ";")
    && string(sp, s_end, &cur, "=")) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    
    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
bws(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "BWS", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (ows(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
connection(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p1, *p2, *p3;

    createnode(*n, "Connection", *sp, 0, NULL, NULL);
    
    cur = &((**n)->child);

    p1 = *sp;
    while (1) {
        p2 = *sp;
        if (string(sp, s_end, &cur, ",")
        || ows(sp, s_end, &cur)) {
            *sp = p2;
            break;
        }
    }
    if (connection_option(sp, s_end, &cur)){
        *sp = p1;
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    while (1) {
        p2 = *sp;
        if (ows(sp, s_end, &cur)
        || string(sp, s_end, &cur, ",")) {
            *sp = p2;
            break;
        }
        p3 = *sp;
        if (ows(sp, s_end, &cur)
        || connection_option(sp, s_end, &cur)){
            *sp = p3;
        }
    }
    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
connection_option(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "connection_option", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (token(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
content_length(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;
    int i;

    createnode(*n, "Content-length", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    i = 0;
    p = *sp;
    while (1) {
        if (digit(sp, s_end, &cur)) {
            break;
        }
        i++;
    }
    if (i < 1){
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
ows(char **sp, char *s_end, Node ***n)
{
    int i;

    for (i = 0; s_end - *sp + 1 > 0
    && ((*sp)[i] == ' ' || (*sp)[i] == '\t'); i++) {
        ;
    }
    createnode(*n, "OWS", *sp, i, NULL, NULL);
    *n = &((**n)->sibling);
    *sp += i;
    return 0;
}

int
transfer_encoding(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p1, *p2, *p3;

    createnode(*n, "Transfer_Encoding", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p1 = *sp;
    while (1) {
        p2 = *sp;
        if (string(sp, s_end, &cur, ",")
        || ows(sp, s_end, &cur)) {
            *sp = p2;
            break;
        }
    }
    if (transfer_coding(sp, s_end, &cur)) {
        *sp = p1;
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    while (1) {
        p2 = *sp;
        if (ows(sp, s_end, &cur)
        || string(sp, s_end, &cur, ",")) {
            *sp = p2;
            break;
        }
        p3 = *sp;
        if (ows(sp, s_end, &cur)
        || transfer_coding(sp, s_end, &cur))
            *sp = p3;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
qdtext(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "qdtext", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (htab(sp, s_end, &cur)
    && space(sp, s_end, &cur)
    && string(sp, s_end, &cur, "!")
    && range(sp, s_end, &cur, 0x23, 0x5B)
    && range(sp, s_end, &cur, 0x5D, 0x7E)
    && obs_text(sp, s_end, &cur)) {
        freeTree(**n);
        **n = NULL;
        return 1;
     }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
quoted_pair(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "quoted_pair", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "\\")
    || (htab(sp, s_end, &cur)
    && space(sp, s_end, &cur)
    && vchar(sp, s_end, &cur)
    && obs_text(sp, s_end, &cur))) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
quoted_string(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p1, *p2;

    createnode(*n, "quoted_string", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p1 = *sp;
    if (dquote(sp, s_end, &cur)) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    while (1) {
        p2 = *sp;
        if (qdtext(sp, s_end, &cur)
        && quoted_pair(sp, s_end, &cur)){
            *sp = p2;
            break;
        }   
    }
    if (dquote(sp, s_end, &cur)) {
        *sp = p1;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
tchar(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "tchar", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (string(sp, s_end, &cur, "!")
    && string(sp, s_end, &cur, "#")
    && string(sp, s_end, &cur, "$")
    && string(sp, s_end, &cur, "%")
    && string(sp, s_end, &cur, "&")
    && string(sp, s_end, &cur, "'")
    && string(sp, s_end, &cur, "*")
    && string(sp, s_end, &cur, "+")
    && string(sp, s_end, &cur, "-")
    && string(sp, s_end, &cur, ".")
    && string(sp, s_end, &cur, "^")
    && string(sp, s_end, &cur, "_")
    && string(sp, s_end, &cur, "`")
    && string(sp, s_end, &cur, "|")
    && string(sp, s_end, &cur, "~")
    && digit(sp, s_end, &cur)
    && alpha(sp, s_end, &cur)) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
token(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;
    int i;

    createnode(*n, "token", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    i = 0;
    p = *sp;
    while (1) {
        if (tchar(sp, s_end, &cur)) {
            break;
        }
        i++;
    }
    if (i < 1){
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
transfer_coding(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "transfer_coding", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (string(sp, s_end, &cur, "chunked")
    && string(sp, s_end, &cur, "compress")
    && string(sp, s_end, &cur, "deflate")
    && string(sp, s_end, &cur, "gzip")
    && transfer_extension(sp, s_end, &cur)) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
transfer_extension(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "transfer_extension", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (token(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }
    while (1) {
        p = *sp;
        if (ows(sp, s_end, &cur)
        || string(sp, s_end, &cur, ";")
        || ows(sp, s_end, &cur)
        || transfer_parameter(sp, s_end, &cur)) {
            *sp = p;
            break;
        }
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
transfer_parameter(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "transfer_parameter", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (token(sp, s_end, &cur)
    || bws(sp, s_end, &cur)
    || string(sp, s_end, &cur, "=")
    || bws(sp, s_end, &cur)
    || (token(sp, s_end, &cur) && quoted_string(sp, s_end, &cur))) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    
    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
content_type(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "Content_Type", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (media_type(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
expect(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "expect", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (string(sp, s_end, &cur, "100-continue")) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
media_type(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p1, *p2;

    createnode(*n, "media_type", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p1 = *sp;
    if (type(sp, s_end, &cur)
    || string(sp, s_end, &cur, "/")
    || subtype(sp, s_end, &cur)) {
        *sp = p1;
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    while (1){
        p2 = *sp;
        if (ows(sp, s_end, &cur)
        || string(sp, s_end, &cur, ";")
        || ows(sp, s_end, &cur)
        || parameter(sp, s_end, &cur)){
            *sp = p2;
            break;
        }
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
parameter(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "parameter", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (token(sp, s_end, &cur)
    || string(sp, s_end, &cur, "=")
    || (token(sp, s_end, &cur) && quoted_string(sp, s_end, &cur))) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
subtype(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "subtype", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (token(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
type(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "type", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (token(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
connection_header(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Connection_header", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "Connection")
    || string(sp, s_end, &cur, ":")
    || ows(sp, s_end, &cur)
    || connection(sp, s_end, &cur)
    || ows(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
content_length_header(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Content_Length_header", *sp, 0, NULL, NULL);
    
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "Content-Length")
    || string(sp, s_end, &cur, ":")
    || ows(sp, s_end, &cur)
    || content_length(sp, s_end, &cur)
    || ows(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
content_type_header(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Content_Type_header", *sp, 0, NULL, NULL);
    
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "Content-Type")
    || string(sp, s_end, &cur, ":")
    || ows(sp, s_end, &cur)
    || content_type(sp, s_end, &cur)
    || ows(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
transfer_encoding_header(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Transfer_Encoding_header", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "Transfer-Encoding")
    || string(sp, s_end, &cur, ":")
    || ows(sp, s_end, &cur)
    || transfer_encoding(sp, s_end, &cur)
    || ows(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
expect_header(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Expect_header", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "Expect")
    || string(sp, s_end, &cur, ":")
    || ows(sp, s_end, &cur)
    || expect(sp, s_end, &cur)
    || ows(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
host_header(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Host_header", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "Host")
    || string(sp, s_end, &cur, ":")
    || ows(sp, s_end, &cur)
    || Host(sp, s_end, &cur)
    || ows(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
cookie_pair(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "cookie_pair", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (cookie_name(sp, s_end, &cur)
    || string(sp, s_end, &cur, "=")
    || cookie_value(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
cookie_name(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "cookie_name", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (token(sp, s_end, &cur)) {
            freeTree(**n);
            **n = NULL;
            return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
cookie_value(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "cookie_value", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (dquote(sp, s_end, &cur)){
        while (1) {
            if (cookie_octet(sp, s_end, &cur)) {
                break;
            }
        }
    } else {
        while (1) {
            if (cookie_octet(sp, s_end, &cur)) {
                break;
            }
        }
        if (dquote(sp, s_end, &cur)) {
            *sp = p;
            freeTree(**n);
            **n = NULL;
            return 1;
        }
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
cookie_octet(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "cookie_value", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    if (range(sp, s_end, &cur, 0x21, 0x21)
    && range(sp, s_end, &cur, 0x23, 0x2B)
    && range(sp, s_end, &cur, 0x23, 0x2B)
    && range(sp, s_end, &cur, 0x23, 0x2B)
    && range(sp, s_end, &cur, 0x23, 0x2B)) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
cookie_header(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "Cookie_header", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p = *sp;
    if (string(sp, s_end, &cur, "Cookie")
    || ows(sp, s_end, &cur)
    || cookie_string(sp, s_end, &cur)
    || ows(sp, s_end, &cur)) {
        *sp = p;
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
cookie_string(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p1, *p2;

    createnode(*n, "cookie_string", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    p1 = *sp;
    if (cookie_pair(sp, s_end, &cur)) {
        *sp = p1;
        freeTree(**n);
        **n = NULL;
        return 1;
    }
    while (1) {
        p2 = *sp;
        if (string(sp, s_end, &cur, ";")
        || space(sp, s_end, &cur)
        || cookie_pair(sp, s_end, &cur)) {
            *sp = p2;
            break;
        }
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
header_field(char **sp, char *s_end, Node ***n)
{
    Node **cur;
    char *p;

    createnode(*n, "header_field", *sp, 0, NULL, NULL);
    
    cur = &((**n)->child);

    if (connection_header(sp, s_end, &cur)
    && content_length_header(sp, s_end, &cur)
    && content_type_header(sp, s_end, &cur)
    && cookie_header(sp, s_end, &cur)
    && transfer_encoding_header(sp, s_end, &cur)
    && expect_header(sp, s_end, &cur)
    && host_header(sp, s_end, &cur)) {
        p = *sp;
        if (field_name(sp, s_end, &cur)
        || string(sp, s_end, &cur, ":")
        || ows(sp, s_end, &cur)
        || field_value(sp, s_end, &cur)
        || ows(sp, s_end, &cur)) {
            *sp = p;
            freeTree(**n);
            **n = NULL;
            return 1;
        }
    }
    (**n)->len = (**n)->child->len;
    *n = &((**n)->sibling);
    return 0;
}

int
space(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "SP", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (string(sp, s_end, &cur, " ")) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
digit(char **sp, char *s_end, Node ***n)
{
    if (s_end - *sp + 1 < 1 || !isdigit(**sp))
        return 1;
    createnode(*n, "DIGIT", *sp, 1, NULL, NULL);
    *n = &((**n)->sibling);
    (*sp)++;
    return 0;
}

int
alpha(char **sp, char *s_end, Node ***n)
{
    if (s_end - *sp + 1 < 1 || !isalpha(**sp))
        return 1;
    createnode(*n, "ALPHA", *sp, 1, NULL, NULL);
    *n = &((**n)->sibling);
    (*sp)++;
    return 0;
}

int
dquote(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "DQUOTE", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (range(sp, s_end, &cur, 0x22, 0x22)) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
htab(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "HTAB", *sp, 0, NULL, NULL);
        
    cur = &((**n)->child);

    if (string(sp, s_end, &cur, "\t")) {
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
vchar(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "VCHAR", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    if (range(sp, s_end, &cur, 0x21, 0x7E)){
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
hexdig(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "HEXDIG", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    if (range(sp, s_end, &cur, 0x30, 0x39)
    && range(sp, s_end, &cur, 'A', 'F')
    && range(sp, s_end, &cur, 'a', 'f')){
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}

int
octet(char **sp, char *s_end, Node ***n)
{
    Node **cur;

    createnode(*n, "octet", *sp, 0, NULL, NULL);
    cur = &((**n)->child);

    if (range(sp, s_end, &cur, 0x00, 0xFF)){
        freeTree(**n);
        **n = NULL;
        return 1;
    }

    cur = &((**n)->child);
    while (*cur) {
        (**n)->len += (*cur)->len;
        cur = &((*cur)->sibling);
    }
    *n = &((**n)->sibling);
    return 0;
}


int
string(char **sp, char *s_end, Node ***n, char *s)
{
    int i;

    if (s_end - *sp + 1 < strlen(s))
        return 1;
    for (i = 0; i < strlen(s); i++){
        if (tolower(s[i]) != tolower((*sp)[i]))
            return 1;
    }
    createnode(*n, s, *sp, strlen(s), NULL, NULL);
    *n = &((**n)->sibling);
    *sp += strlen(s);
    return 0;
}

int
range(char **sp, char *s_end, Node ***n, unsigned char inf, unsigned char sup)
{
    if (s_end - *sp + 1 < 1)
        return 1;
    if ((unsigned char)**sp < inf || (unsigned char)**sp > sup)
        return 1;

    createnode(*n, "range", *sp, 1, NULL, NULL);
    *n = &((**n)->sibling);
    (*sp)++;
    return 0;
}