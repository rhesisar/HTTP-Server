#ifndef _CONF_H_
#define _CONF_H_

#define PORT 8080
#ifndef SITES_FOLDER
#define SITES_FOLDER "/var/www"
#endif
#define DFLT_TARG "index.html"
#define DFLT_HOST SITE1_FR

/* Edit host files in conf.c */

enum hosts {
    SITE1_FR,
    SITE2_FR,
    WWW_TOTO_COM,
    WWW_FAKE_COM,
    N_HOSTS
};
extern char * const hosts[];

#endif
