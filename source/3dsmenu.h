
#ifndef _3DSMENU_H_
#define _3DSMENU_H_

typedef struct
{
    int     ID;
    char    *Text;
    int     Checked;           // -1, not a checkbox
                                // 0, unchecked
                                // 1, checked
} SMenuItem;


int S9xMenuSelectItem();

void S9xAddTab(char *title, SMenuItem *menuItems, int itemCount);
void S9xClearMenuTabs();

void S9xShowMessage(char *title, char *messageLine1, char *messageLine2);
void S9xShowAlert(char *title, char *messageLine1, char *messageLine2);
bool S9xShowConfirmation(char *title, char *messageLine1, char *messageLine2);

void S9xUncheckGroup(SMenuItem *menuItems, int itemCount, int group);
void S9xCheckItemByID(SMenuItem *menuItems, int itemCount, int id);

#endif