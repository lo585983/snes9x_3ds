#include <cstring>
#include <string.h>
#include <stdio.h>
#include <3ds.h>

#include "snes9x.h"
#include "port.h"

#include "3dsmenu.h"
#include "3dsgpu.h"

#define CONSOLE_WIDTH           40
#define MENU_HEIGHT             (24)
#define S9X3DS_VERSION	        "0.1" 





typedef struct
{
    SMenuItem   *MenuItems;
    char        *Title;
    int         ItemCount;
    int         FirstItemIndex;
    int         SelectedItemIndex;
} SMenuTab;


SMenuTab            menuTab[10];
int                 menuTabCount;
int                 currentMenuTab = 0;

char *S9xMenuTruncateString(char *outBuffer, char *inBuffer)
{
    memset(outBuffer, 0, CONSOLE_WIDTH);
    if (strlen(inBuffer) < CONSOLE_WIDTH - 3)
        return inBuffer;

    for (int i = 0; i < CONSOLE_WIDTH - 3; i++)
    {
        outBuffer[i] = inBuffer[i];
        if (inBuffer[i] == 0)
            break;
    }

    return outBuffer;
}

// Display the list of choices for selection
//
void S9xMenuShowItems()
{
    SMenuTab *currentTab = &menuTab[currentMenuTab];
    
    char tempBuffer[CONSOLE_WIDTH];

    consoleClear();

    for (int i = 0; i < menuTabCount; i++)
    {
        if (i == currentMenuTab)
            printf (">");
        else
            printf (" ");
        printf ("%s  ", menuTab[i].Title);
    }
    printf ("\n");

    printf ("---------------------------------------\n");
    int c = 0;
    for (int i = currentTab->FirstItemIndex; i < currentTab->ItemCount && i < currentTab->FirstItemIndex + MENU_HEIGHT; i++)
    {
        if (currentTab->SelectedItemIndex == i)
            printf ("> ");
        else
            printf ("  ");

        if (currentTab->MenuItems[i].Text != NULL)
            printf ("%s", S9xMenuTruncateString(tempBuffer, currentTab->MenuItems[i].Text));

        if (currentTab->MenuItems[i].Checked == 0)
            printf ("[ ]");
        else if (currentTab->MenuItems[i].Checked == 1)
            printf ("[X]"); 
        printf ("\n");

        c ++;
    }
    for (; c < MENU_HEIGHT; c++)
    {
        printf ("\n");
    }

    printf ("---------------------------------------\n");
    printf ("A - Ok    B - Cancel\n");
}


// Displays the menu and allows the user to select from
// a list of choices.
//
int S9xMenuSelectItem()
{    
    int framesDKeyHeld = 0;

    SMenuTab *currentTab = &menuTab[currentMenuTab];

    S9xMenuShowItems();

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;
    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (thisKeysHeld & KEY_UP || thisKeysHeld & KEY_DOWN)
            framesDKeyHeld ++;
        else
            framesDKeyHeld = 0;
        if (keysDown & KEY_B)
        {
            return -1;
        }
        if ((keysDown & KEY_RIGHT) || (keysDown & KEY_R))
        {
            currentMenuTab++;
            if (currentMenuTab >= menuTabCount)
                currentMenuTab = 0;
            currentTab = &menuTab[currentMenuTab];

            S9xMenuShowItems();
        }
        if ((keysDown & KEY_LEFT) || (keysDown & KEY_L))
        {
            currentMenuTab--;
            if (currentMenuTab < 0)
                currentMenuTab = menuTabCount - 1;
            currentTab = &menuTab[currentMenuTab];

            S9xMenuShowItems();
            
        }
        if (keysDown & KEY_START || keysDown & KEY_A)
        {
            return currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
        }
        if (keysDown & KEY_UP || ((thisKeysHeld & KEY_UP) && (framesDKeyHeld > 30) && (framesDKeyHeld % 2 == 0)))
        {
            do 
            { 
                currentTab->SelectedItemIndex--;
                if (currentTab->SelectedItemIndex < 0)
                    currentTab->SelectedItemIndex = currentTab->ItemCount - 1;
            }
            while (currentTab->MenuItems[currentTab->SelectedItemIndex].ID == -1);
            
            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + MENU_HEIGHT)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - MENU_HEIGHT + 1;

            S9xMenuShowItems();
            
        }
        if (keysDown & KEY_DOWN || ((thisKeysHeld & KEY_DOWN) && (framesDKeyHeld > 30) && (framesDKeyHeld % 2 == 0)))
        {
            do 
            { 
                currentTab->SelectedItemIndex++;
                if (currentTab->SelectedItemIndex >= currentTab->ItemCount)
                    currentTab->SelectedItemIndex = 0;
            }
            while (currentTab->MenuItems[currentTab->SelectedItemIndex].ID == -1);

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + MENU_HEIGHT)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - MENU_HEIGHT + 1;

            S9xMenuShowItems();
        }

        
        gfxFlushBuffers();
        gpu3dsTransferToScreenBuffer();
        gfxSwapBuffers();

        gspWaitForVBlank();
    }
}


void S9xAddTab(char *title, SMenuItem *menuItems, int itemCount)
{
    SMenuTab *currentTab = &menuTab[menuTabCount];
    
    currentTab->Title = title;
    currentTab->MenuItems = menuItems;
    currentTab->ItemCount = itemCount;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID != -1)
        {
            currentTab->SelectedItemIndex = i;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + MENU_HEIGHT)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - MENU_HEIGHT + 1;
            break;
        }
    }

    menuTabCount++;
}


void S9xClearMenuTabs()
{
    menuTabCount = 0;
    currentMenuTab = 0;
}


void S9xPrintCentreAligned (char *s)
{
    if (s == NULL)
    {
        printf ("\n");
        return;
    }
    int len = strlen(s);
    for (int i = 0; i < (CONSOLE_WIDTH - len) / 2; i++)
        printf (" ");
    printf ("%s\n", s);
}


void S9xShowMessage(char *title, char *messageLine1, char *messageLine2)
{
    consoleClear();
    S9xPrintCentreAligned(title);
    printf ("---------------------------------------\n");
    for (int i = 0; i < 5; i++)
        printf ("\n");
    S9xPrintCentreAligned(messageLine1);
    S9xPrintCentreAligned(messageLine2);
    for (int i = 0; i < 5; i++)
        printf ("\n");

}

void S9xShowAlert(char *title, char *messageLine1, char *messageLine2)
{
    consoleClear();
    
    S9xPrintCentreAligned(title);
    printf ("---------------------------------------\n");
    for (int i = 0; i < 5; i++)
        printf ("\n");
    S9xPrintCentreAligned(messageLine1);
    S9xPrintCentreAligned(messageLine2);
    for (int i = 0; i < 5; i++)
        printf ("\n");

    S9xPrintCentreAligned ("A - OK\n");

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;

    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_START || keysDown & KEY_A)
        {
            consoleClear();
            return;
        }
    }
}


bool S9xShowConfirmation(char *title, char *messageLine1, char *messageLine2)
{
    consoleClear();
    
    S9xPrintCentreAligned(title);
    printf ("---------------------------------------\n");
    for (int i = 0; i < 5; i++)
        printf ("\n");
    S9xPrintCentreAligned(messageLine1);
    S9xPrintCentreAligned(messageLine2);
    for (int i = 0; i < 5; i++)
        printf ("\n");

    S9xPrintCentreAligned ("A - Yes      B - No\n");

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;

    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_START || keysDown & KEY_A)
        {
            consoleClear();
            return true;
        }
        if (keysDown & KEY_B)
        {
            consoleClear();
            return false;
        }
    }   
}


void S9xUncheckGroup(SMenuItem *menuItems, int itemCount, int group)
{
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID / 1000 == group / 1000)
        {
            menuItems[i].Checked = 0;
        }

    }
}


void S9xCheckItemByID(SMenuItem *menuItems, int itemCount, int id)
{
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID == id)
        {
            menuItems[i].Checked = 1;
        }

    }
}