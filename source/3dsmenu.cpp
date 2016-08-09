#include <cstring>
#include <string.h>
#include <stdio.h>
#include <3ds.h>

#include "snes9x.h"
#include "port.h"

#include "3dsmenu.h"
#include "3dsgpu.h"
#include "3dsui.h"

#define CONSOLE_WIDTH           40
#define MENU_HEIGHT             (17)




typedef struct
{
    SMenuItem   *MenuItems;
    char        SubTitle[256];
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


void S9xShowTitleAndMessage(
    int titleForeColor, int titleBackColor,
    int mainForeColor, int mainBackColor,
    char *title, char *messageLine1, char *messageLine2, char *messageLine3, char *messageLine4)
{
    ui3dsSetColor(titleForeColor, titleBackColor);
    ui3dsDrawRect(0, 0, 320, 16);
    ui3dsDrawRect(0, 224, 320, 240);
    ui3dsDrawString(2, 2, 318, true, title);

    ui3dsSetColor(mainForeColor, mainBackColor);
    ui3dsDrawRect(0, 16, 320, 224);

    int line = 70;
    ui3dsDrawString(2, line, 318, true, messageLine1);
    ui3dsDrawString(2, line+12, 318, true, messageLine2);
    ui3dsDrawString(2, line+24, 318, true, messageLine3);
    ui3dsDrawString(2, line+36, 318, true, messageLine4);
}


// Display the list of choices for selection
//
char buffer[512];
void S9xMenuShowItems()
{
    SMenuTab *currentTab = &menuTab[currentMenuTab];
    
    char tempBuffer[CONSOLE_WIDTH];
    
    //void ui3dsDrawString(int x0, int x1, int y, bool centreAligned, char *format, ...);
    for (int i = 0; i < menuTabCount; i++)
    {
        if (i == currentMenuTab)
            ui3dsSetColor(0xffffff, 0x1565C0);
        else
            ui3dsSetColor(0x64B5F6, 0x1565C0);
        ui3dsDrawString(i*80, 2, 2+(i+1)*80, true, menuTab[i].Title);
    }

    ui3dsSetColor(0xffffff, 0x1565C0);
    ui3dsDrawString(0, 226, 320, false, "  A - Select   B - Cancel                                          SNES9x for 3DS v0.31");
    
    int line = 0;
    int maxItems = MENU_HEIGHT;
    int menuStartY = 16;

    if (currentTab->SubTitle[0])
    {
        maxItems--;
        menuStartY += 12;
        snprintf (buffer, 511, "  %s", currentTab->SubTitle);
        ui3dsSetColor(0x000000, 0x90CAF9);
        ui3dsDrawString(0, 16, 320, false, buffer);
    }

    for (int i = currentTab->FirstItemIndex; 
        i < currentTab->ItemCount && i < currentTab->FirstItemIndex + maxItems; i++)
    {
        int y = line * 12 + menuStartY;

        if (currentTab->SelectedItemIndex == i)
            ui3dsSetColor(0xffffff, 0x2196F3);
        else if (currentTab->MenuItems[i].ID == -1)
            ui3dsSetColor(0x2196F3, 0xffffff);
        else if (currentTab->MenuItems[i].Checked == 1)
            ui3dsSetColor(0x000000, 0xffffff);
        else if (currentTab->MenuItems[i].Checked == 0)
            ui3dsSetColor(0x999999, 0xffffff);
        else
            ui3dsSetColor(0x333333, 0xffffff);

        if (currentTab->MenuItems[i].Text == NULL)
            buffer[0] = 0;
        else    
            snprintf(buffer, 512, "     %s", currentTab->MenuItems[i].Text);
        ui3dsDrawString(0, y, 280, false, buffer);

        if (currentTab->MenuItems[i].Checked == 0)
            ui3dsDrawString(280, y, 320, false, "\xfe");
        else if (currentTab->MenuItems[i].Checked == 1)
            ui3dsDrawString(280, y, 320, false, "\xfd");
        else
            ui3dsDrawString(280, y, 320, false, "");

        line += 1;
    }
    ui3dsSetColor(0x333333, 0xffffff);
    for (; line < maxItems; )
    {
        int y = line * 12 + menuStartY;
        ui3dsDrawString(0, y, 380, false, NULL);

        line += 1;
    }

}


// Displays the menu and allows the user to select from
// a list of choices.
//
int S9xMenuSelectItem()
{    
    int framesDKeyHeld = 0;

    SMenuTab *currentTab = &menuTab[currentMenuTab];

    S9xShowTitleAndMessage(0xffffff, 0x1565C0, 0x333333, 0xffffff, "", "", "", "", "");
    S9xMenuShowItems();

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;
    while (aptMainLoop())
    {
        APT_AppStatus appStatus = aptGetStatus();
        if (appStatus == APP_EXITING)
            return -1;
        
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        int maxItems = MENU_HEIGHT;
        if (currentTab->SubTitle[0])
        {
            maxItems--;
        }

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
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

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
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            S9xMenuShowItems();
        }

        
        //gfxFlushBuffers();
        //gpu3dsTransferToScreenBuffer();
        //gfxSwapBuffers();

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


void S9xSetTabSubTitle(int tabIndex, char *subtitle)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    currentTab->SubTitle[0] = 0;
    if (subtitle != NULL)
        strncpy(currentTab->SubTitle, subtitle, 255);
}

void S9xSetCurrentMenuTab(int tabIndex)
{
    currentMenuTab = tabIndex;
}


void S9xClearMenuTabs()
{
    menuTabCount = 0;
    currentMenuTab = 0;
}




void S9xShowWaitingMessage(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0x2196F3, 
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "");
}


void S9xAlertSuccess(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0x43A047, 
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "A - OK");

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;

    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_A)
        {
            return;
        }
        gspWaitForVBlank();
        
    }
}


void S9xAlertFailure(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0xC62828, 
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "A - OK");

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;

    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_A)
        {
            return;
        }
        gspWaitForVBlank();
        
    }
}


bool S9xConfirm(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0x00897B, 
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "START - Yes      B - No");


    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;


    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_START)
        {
            return true;
        }
        if (keysDown & KEY_B)
        {
            return false;
        }
        gspWaitForVBlank();
        
    }
    return false;

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