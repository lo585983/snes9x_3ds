
#ifndef _3DSMENU_H_
#define _3DSMENU_H_

typedef struct
{
    int     ID;
    char    *Text;
} SMenuItem;


int S9xMenuSelectItem();

void S9xAddTab(char *title, SMenuItem *menuItems, int itemCount);
void S9xClearMenuTabs();

void S9xShowMessage(char *title, char *messageLine1, char *messageLine2);
void S9xShowAlert(char *title, char *messageLine1, char *messageLine2);
bool S9xShowConfirmation(char *title, char *messageLine1, char *messageLine2);


#endif