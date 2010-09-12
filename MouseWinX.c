// MouseWinX.c : Defines the entry point for the application.
// Copyright (C) 2005, 2010  Josh Stone
//
// This file is part of MouseWinX, and is free software: you can
// redistribute it and/or modify it under the terms of the GNU General
// Public License as published by the Free Software Foundation, either
// version 3 of the License, or (at your option) any later version.
//

#include "stdafx.h"
#include <shellapi.h>
#include <crtdbg.h>
#include <htmlhelp.h>
#include <strsafe.h>
#include "MouseWinX.h"

#define WM_APP_NOTIFYICON      (WM_APP + 1)
#define WM_APP_SETTINGCHANGE   (WM_APP + 2)

#define DBG0(format) _RPT0(_CRT_WARN, format)
#define DBG1(format, arg1) _RPT1(_CRT_WARN, format, arg1)
#define DBG2(format, arg1, arg2) _RPT2(_CRT_WARN, format, arg1, arg2)
#define DBG3(format, arg1, arg2, arg3) _RPT3(_CRT_WARN, format, arg1, arg2, arg3)
#define DBG4(format, arg1, arg2, arg3, arg4) _RPT4(_CRT_WARN, format, arg1, arg2, arg3, arg4)

// Global Variables:
HINSTANCE hInst;
struct { HICON big, small; } hIconHot, hIconCold, hIcon;
struct { HMENU root, popup; } hMenuNotify;

BOOL bSysTracking;
BOOL bSysZOrder;
DWORD dwSysDelay;

// Forward declarations of functions included in this code module:
BOOL                UpdateParameter(UINT uiAction, PVOID pvParam);
BOOL                SendNotifyIconMessage(HWND hWnd, DWORD dwMessage);
BOOL                LoadAllIcons();
BOOL                DestroyAllIcons();
BOOL                InitInstance();
BOOL                DestroyInstance();
BOOL                UpdateDialogApply(HWND hDlg);
BOOL                ApplyUserSettings(HWND hDlg);
HWND                ShowHelpPopup(LPHELPINFO lphi);
BOOL                SendMessageToPopups(HWND hWnd, UINT Msg,
                                        WPARAM wParam, LPARAM lParam);
BOOL                ShowSettingsDialog(HWND hWnd);
BOOL                ShowPopupMenu(HWND hWnd);
INT_PTR CALLBACK    DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
int APIENTRY        _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                              LPTSTR lpCmdLine, int nCmdShow);


BOOL
UpdateParameter(UINT uiAction, PVOID pvParam)
{
    // Don't use SPIF_SENDCHANGE, as that makes us wait for all windows to
    // receive it.  A manual SendNotifyMessage lets us move on right away.
    return SystemParametersInfo(uiAction, 0, pvParam, SPIF_UPDATEINIFILE)
        && SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, uiAction, 0);
}


BOOL 
SendNotifyIconMessage(HWND hWnd, DWORD dwMessage)
{
    LPCTSTR lpszApp = _T("MouseWinX");

    NOTIFYICONDATA nid;

    switch (dwMessage) {
        case NIM_ADD:
        case NIM_DELETE:
        case NIM_MODIFY:
            break;
        default:
            return FALSE;
    }

    nid.cbSize              = NOTIFYICONDATA_V1_SIZE;
    nid.hWnd                = hWnd;
    nid.uID                 = IDR_NOTIFYICON;
    nid.uFlags              = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage    = WM_APP_NOTIFYICON;
    nid.hIcon               = hIcon.small;
    StringCchCopy(nid.szTip, sizeof(nid.szTip), lpszApp);

    return Shell_NotifyIcon(dwMessage, &nid);
}

BOOL 
LoadAllIcons()
{
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    int cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    int cySmIcon = GetSystemMetrics(SM_CYSMICON);

	hIconHot.big = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_HOT),
        IMAGE_ICON, cxIcon, cyIcon, LR_DEFAULTCOLOR);
	hIconHot.small = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_HOT),
        IMAGE_ICON, cxSmIcon, cySmIcon, LR_DEFAULTCOLOR);

	hIconCold.big = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_COLD),
        IMAGE_ICON, cxIcon, cyIcon, LR_DEFAULTCOLOR);
    hIconCold.small = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_COLD),
        IMAGE_ICON, cxSmIcon, cySmIcon, LR_DEFAULTCOLOR);

    hIcon = hIconHot;
    return ((NULL != hIconHot.big) && (NULL != hIconHot.small) &&
        (NULL != hIconCold.big) && (NULL != hIconCold.small));
}

BOOL 
DestroyAllIcons()
{
    BOOL ret = TRUE;

    ret &= DestroyIcon(hIconHot.big);
    ret &= DestroyIcon(hIconHot.small);
    ret &= DestroyIcon(hIconCold.big);
    ret &= DestroyIcon(hIconCold.small);

    return ret;
}

BOOL 
InitInstance()
{
    LPCTSTR lpszApp = _T("MouseWinX");

    HWND hWnd;
    WNDCLASSEX wcx;

    // check for an existing session
    hWnd = FindWindow(lpszApp, lpszApp);
    if (hWnd != NULL) {
        PostMessage(hWnd, WM_COMMAND, ID_POPUP_SETTINGS, 0);
        return FALSE;
    }

    LoadAllIcons();

    hMenuNotify.root = LoadMenu(hInst, MAKEINTRESOURCE(IDR_NOTIFYICON));
    hMenuNotify.popup = GetSubMenu(hMenuNotify.root, 0);
    SetMenuDefaultItem(hMenuNotify.popup, ID_POPUP_ACTIVATE, FALSE);

    wcx.cbSize          = sizeof(WNDCLASSEX);
    wcx.style           = 0;
    wcx.lpfnWndProc     = (WNDPROC)WndProc;
    wcx.cbClsExtra      = 0;
    wcx.cbWndExtra      = 0;
    wcx.hInstance       = hInst;
    wcx.hIcon           = hIcon.big;
    wcx.hCursor         = NULL;
    wcx.hbrBackground   = NULL;
    wcx.lpszMenuName    = NULL;
    wcx.lpszClassName   = lpszApp;
    wcx.hIconSm         = hIcon.small;

    RegisterClassEx(&wcx);


    hWnd = CreateWindow(lpszApp, lpszApp, WS_MINIMIZE | WS_DISABLED | WS_SYSMENU,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInst, NULL);
    if (hWnd == NULL) {
        PostQuitMessage(1);
    }

    // initially trim the working set
    SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);

    return TRUE;
}

BOOL 
DestroyInstance()
{
    BOOL ret = TRUE;
    ret &= DestroyMenu(hMenuNotify.root);
    ret &= DestroyAllIcons();
    return ret;
}

BOOL
UpdateDialogApply(HWND hDlg)
{
    BOOL same = TRUE;
    BOOL bUserTracking;
    BOOL bUserZOrder;
    DWORD dwUserDelay;

    bUserTracking = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USERTRACKING));
    bUserZOrder = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USERZORDER));
    dwUserDelay = GetDlgItemInt(hDlg, IDC_USERDELAY, NULL, FALSE);

    same &= (bUserTracking == bSysTracking);
    same &= (bUserZOrder == bSysZOrder);
    same &= (dwUserDelay == dwSysDelay);
    return EnableWindow(GetDlgItem(hDlg, IDAPPLY), !same);
}

BOOL
ApplyUserSettings(HWND hDlg)
{
    BOOL bUserTracking;
    BOOL bUserZOrder;
    DWORD dwUserDelay;

    bUserTracking = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USERTRACKING));
    bUserZOrder = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_USERZORDER));
    dwUserDelay = GetDlgItemInt(hDlg, IDC_USERDELAY, NULL, FALSE);

    if (bUserTracking != bSysTracking) {
        DBG1("Setting activate to %s\n", bUserTracking ? "true" : "false");
        UpdateParameter(SPI_SETACTIVEWINDOWTRACKING, IntToPtr(bUserTracking));
    }
    if (bUserZOrder != bSysZOrder) {
        DBG1("Setting autoraise to %s\n", bUserZOrder ? "true" : "false");
        UpdateParameter(SPI_SETACTIVEWNDTRKZORDER, IntToPtr(bUserZOrder));
    }
    if (dwUserDelay != dwSysDelay) {
        DBG1("Setting delay to %d\n", dwUserDelay);
        UpdateParameter(SPI_SETACTIVEWNDTRKTIMEOUT, UIntToPtr(dwUserDelay));
    }
    return TRUE;
}

HWND
ShowHelpPopup(LPHELPINFO lphi)
{
    WINDOWINFO wiControl;
    HH_POPUP hhPopup;

    DBG1("\tiContextType: %i\n", lphi->iContextType);
    if (lphi->iContextType == HELPINFO_WINDOW) {
        DBG0("\t\t(HELPINFO_WINDOW)\n");
        DBG1("\tiCtrlId: %i\n", lphi->iCtrlId);
        switch(lphi->iCtrlId) {
            case IDOK:
                DBG0("\t\t(IDOK)\n");
                hhPopup.idString = IDS_HELP_OK;
                break;
            case IDCANCEL:
                DBG0("\t\t(IDCANCEL)\n");
                hhPopup.idString = IDS_HELP_CANCEL;
                break;
            case IDAPPLY:
                DBG0("\t\t(IDAPPLY)\n");
                hhPopup.idString = IDS_HELP_APPLY;
                break;
            case IDC_USERGROUP:
                DBG0("\t\t(IDC_USERGROUP)\n");
                hhPopup.idString = IDS_HELP_USERGROUP;
                break;
            case IDC_USERTRACKING:
                DBG0("\t\t(IDC_USERTRACKING)\n");
                hhPopup.idString = IDS_HELP_USERTRACKING;
                break;
            case IDC_USERZORDER:
                DBG0("\t\t(IDC_USERZORDER)\n");
                hhPopup.idString = IDS_HELP_USERZORDER;
                break;
            case IDC_USERDELAYLABEL:
                DBG0("\t\t(IDC_USERDELAYLABEL)->\n");
                // falls through to IDC_USERDELAY, but try
                // to redirect the item handle first
                lphi->hItemHandle = GetWindow(lphi->hItemHandle, GW_HWNDNEXT);
            case IDC_USERDELAY:
                DBG0("\t\t(IDC_USERDELAY)\n");
                hhPopup.idString = IDS_HELP_USERDELAY;
                break;
            case IDC_SYSGROUP:
                DBG0("\t\t(IDC_SYSGROUP)\n");
                hhPopup.idString = IDS_HELP_SYSGROUP;
                break;
            case IDC_SYSTRACKING:
                DBG0("\t\t(IDC_SYSTRACKING)\n");
                hhPopup.idString = IDS_HELP_SYSTRACKING;
                break;
            case IDC_SYSZORDER:
                DBG0("\t\t(IDC_SYSZORDER)\n");
                hhPopup.idString = IDS_HELP_SYSZORDER;
                break;
            case IDC_SYSDELAY:
                DBG0("\t\t(IDC_SYSDELAY)\n");
                hhPopup.idString = IDS_HELP_SYSDELAY;
                break;
            default:
                return NULL;
        }
    } else {
        return NULL;
    }

    wiControl.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(lphi->hItemHandle, &wiControl);

    hhPopup.cbStruct = sizeof(HH_POPUP);
    hhPopup.hinst = hInst;
    // hhPopup.idString was set individually
    hhPopup.pszText = NULL;
    hhPopup.pt.x = wiControl.rcWindow.left;
    hhPopup.pt.y = wiControl.rcWindow.bottom;
    hhPopup.clrForeground = -1;
    hhPopup.clrBackground = -1;
    hhPopup.rcMargins.left = -1;
    hhPopup.rcMargins.right = -1;
    hhPopup.rcMargins.top = -1;
    hhPopup.rcMargins.bottom = -1;
    hhPopup.pszFont = NULL;

    return HtmlHelp(lphi->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&hhPopup);
}

BOOL
SendMessageToPopups(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND hDlg = GetWindow(hWnd, GW_ENABLEDPOPUP);
    while (hDlg && (hWnd != hDlg)) {
        SendMessage(hDlg, Msg, wParam, lParam);
        hDlg = GetWindow(hDlg, GW_HWNDNEXT);
    }
    return TRUE;
}

BOOL
ShowSettingsDialog(HWND hWnd)
{
    HWND hDlg = GetWindow(hWnd, GW_ENABLEDPOPUP);
    if (hDlg && (hWnd != hDlg)) {
        FLASHWINFO fwi;

        fwi.cbSize      = sizeof(fwi);
        fwi.hwnd        = hDlg;
        fwi.dwFlags     = FLASHW_ALL;
        fwi.uCount      = 3;
        fwi.dwTimeout   = 0;

        SendMessage(hDlg, DM_REPOSITION, 0, 0);
        SetForegroundWindow(hDlg);
        FlashWindowEx(&fwi);
    } else {
        SetForegroundWindow(hWnd);
        DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, DlgProc);
    }
    return TRUE;
}

BOOL
ShowPopupMenu(HWND hWnd)
{
    POINT pos;
    GetCursorPos(&pos);
    SetForegroundWindow(hWnd);
    TrackPopupMenuEx(hMenuNotify.popup, TPM_LEFTBUTTON,
        pos.x, pos.y, hWnd, NULL);
    return TRUE;
}

INT_PTR CALLBACK 
DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
            DBG0("DlgProc got WM_INITDIALOG\n");
            SendDlgItemMessage(hDlg, IDC_USERDELAY, EM_SETLIMITTEXT, 4, 0);

            // trigger a read for all system settings
            SendMessage(hDlg, WM_APP_SETTINGCHANGE, SPI_SETACTIVEWINDOWTRACKING, 0);
            SendMessage(hDlg, WM_APP_SETTINGCHANGE, SPI_SETACTIVEWNDTRKZORDER, 0);
            SendMessage(hDlg, WM_APP_SETTINGCHANGE, SPI_SETACTIVEWNDTRKTIMEOUT, 0);
            
            // initialize the user settings
            CheckDlgButton(hDlg, IDC_USERTRACKING,
                bSysTracking ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_USERZORDER,
                bSysZOrder ? BST_CHECKED : BST_UNCHECKED);
            SetDlgItemInt(hDlg, IDC_USERDELAY, dwSysDelay, FALSE);

            return TRUE;
        case WM_APP_SETTINGCHANGE:
	        switch (wParam) {
		        case SPI_SETACTIVEWINDOWTRACKING:
                    DBG0("DlgProc got WM_APP_SETTINGCHANGE:SPI_SETACTIVEWINDOWTRACKING\n");
                    SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon.big);
                    SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon.small);
                    CheckDlgButton(hDlg, IDC_SYSTRACKING,
                        bSysTracking ? BST_CHECKED : BST_UNCHECKED);
                    UpdateDialogApply(hDlg);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			        return TRUE;
		        case SPI_SETACTIVEWNDTRKZORDER:
                    DBG0("DlgProc got WM_APP_SETTINGCHANGE:SPI_SETACTIVEWNDTRKZORDER\n");
                    CheckDlgButton(hDlg, IDC_SYSZORDER,
                        bSysZOrder ? BST_CHECKED : BST_UNCHECKED);
                    UpdateDialogApply(hDlg);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			        return TRUE;
		        case SPI_SETACTIVEWNDTRKTIMEOUT:
                    DBG0("DlgProc got WM_APP_SETTINGCHANGE:SPI_SETACTIVEWNDTRKTIMEOUT\n");
                    SetDlgItemInt(hDlg, IDC_SYSDELAY, dwSysDelay, FALSE);
                    UpdateDialogApply(hDlg);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			        return TRUE;
	        }
            break;
        case WM_HELP:
            DBG0("DlgProc got WM_HELP\n");
            if (ShowHelpPopup((LPHELPINFO)lParam)) {
                return TRUE;
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_USERTRACKING:
                    DBG0("DlgProc got WM_COMMAND:IDC_USERTRACKING\n");
                    UpdateDialogApply(hDlg);
                    return TRUE;
                case IDC_USERZORDER:
                    DBG0("DlgProc got WM_COMMAND:IDC_USERZORDER\n");
                    UpdateDialogApply(hDlg);
                    return TRUE;
                case IDC_USERDELAY:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        DBG0("DlgProc got WM_COMMAND:IDC_USERDELAY:EN_CHANGE\n");
                        UpdateDialogApply(hDlg);
                        return TRUE;
                    }
                    break;
                case IDAPPLY:
                    DBG0("DlgProc got WM_COMMAND:IDAPPLY\n");
                    ApplyUserSettings(hDlg);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
                    return TRUE;
                case IDOK:
                    DBG0("DlgProc got WM_COMMAND:IDOK\n");
                    ApplyUserSettings(hDlg);
                    EndDialog(hDlg, 0);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
                    return TRUE;
                case IDCANCEL:
                    DBG0("DlgProc got WM_COMMAND:IDCANCEL\n");
                    EndDialog(hDlg, 0);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT wmTaskbarCreated = WM_NULL;

    switch (uMsg) {
        case WM_CREATE:
            DBG0("WndProc got WM_CREATE\n");

            // trigger a read for all system settings
            SendMessage(hWnd, WM_SETTINGCHANGE, SPI_SETACTIVEWINDOWTRACKING, 0);
            SendMessage(hWnd, WM_SETTINGCHANGE, SPI_SETACTIVEWNDTRKTIMEOUT, 0);
            SendMessage(hWnd, WM_SETTINGCHANGE, SPI_SETACTIVEWNDTRKZORDER, 0);

            wmTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));

            return 0;
        case WM_DESTROY:
            DBG0("WndProc got WM_DESTROY\n");
            SendNotifyIconMessage(hWnd, NIM_DELETE);
            PostQuitMessage(0);
            return 0;
        case WM_SETTINGCHANGE:
	        switch (wParam) {
		        case SPI_SETACTIVEWINDOWTRACKING:
                    DBG0("WndProc got WM_SETTINGCHANGE:SPI_SETACTIVEWINDOWTRACKING\n");
                    SystemParametersInfo(SPI_GETACTIVEWINDOWTRACKING, 0, &bSysTracking, 0);
                    hIcon = bSysTracking ? hIconHot : hIconCold;
                    SendMessageToPopups(hWnd, WM_APP_SETTINGCHANGE, wParam, lParam);
                    SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon.big);
                    SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon.small);
                    SendNotifyIconMessage(hWnd,
                        (wmTaskbarCreated == WM_NULL) ? NIM_ADD : NIM_MODIFY);
                    CheckMenuItem(hMenuNotify.popup, ID_POPUP_ACTIVATE,
                        bSysTracking ? MF_CHECKED : MF_UNCHECKED);
			        return 0;
		        case SPI_SETACTIVEWNDTRKZORDER:
                    DBG0("WndProc got WM_SETTINGCHANGE:SPI_SETACTIVEWNDTRKZORDER\n");
                    SystemParametersInfo(SPI_GETACTIVEWNDTRKZORDER, 0, &bSysZOrder, 0);
                    SendMessageToPopups(hWnd, WM_APP_SETTINGCHANGE, wParam, lParam);
                    CheckMenuItem(hMenuNotify.popup, ID_POPUP_AUTORAISE,
                        bSysZOrder ? MF_CHECKED : MF_UNCHECKED);
			        return 0;
		        case SPI_SETACTIVEWNDTRKTIMEOUT:
                    DBG0("WndProc got WM_SETTINGCHANGE:SPI_SETACTIVEWNDTRKTIMEOUT\n");
                    SystemParametersInfo(SPI_GETACTIVEWNDTRKTIMEOUT, 0, &dwSysDelay, 0);
                    SendMessageToPopups(hWnd, WM_APP_SETTINGCHANGE, wParam, lParam);
			        return 0;
	        }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_POPUP_ACTIVATE:
                    DBG0("WndProc got WM_COMMAND:ID_POPUP_ACTIVATE\n");
                    UpdateParameter(SPI_SETACTIVEWINDOWTRACKING, IntToPtr(!bSysTracking));
                    return 0;
                case ID_POPUP_AUTORAISE:
                    DBG0("WndProc got WM_COMMAND:ID_POPUP_AUTORAISE\n");
                    UpdateParameter(SPI_SETACTIVEWNDTRKZORDER, IntToPtr(!bSysZOrder));
                    return 0;
                case ID_POPUP_SETTINGS:
                    DBG0("WndProc got WM_COMMAND:ID_POPUP_SETTINGS\n");
                    ShowSettingsDialog(hWnd);
                    return 0;
                case ID_POPUP_EXIT:
                    DBG0("WndProc got WM_COMMAND:ID_POPUP_EXIT\n");
                    return SendMessage(hWnd, WM_CLOSE, 0, 0);
            }
            break;
        case WM_APP_NOTIFYICON:
            if (wParam == IDR_NOTIFYICON) {
                switch (lParam) {
                    case WM_LBUTTONUP:
                        DBG0("WndProc got WM_APP_NOTIFYICON:IDR_NOTIFYICON:WM_LBUTTONUP\n");
                        return SendMessage(hWnd, WM_COMMAND, ID_POPUP_ACTIVATE, 0);
			        case WM_RBUTTONUP:
                        DBG0("WndProc got WM_APP_NOTIFYICON:IDR_NOTIFYICON:WM_RBUTTONUP\n");
                        ShowPopupMenu(hWnd);
                        return 0;
                }
            }
            break;
        default:
            if (uMsg == wmTaskbarCreated) {
                DBG0("WndProc got wmTaskbarCreated\n");
                SendNotifyIconMessage(hWnd, NIM_ADD);
            }
            break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


int APIENTRY 
_tWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR    lpCmdLine,
          int       nCmdShow)
{
    MSG msg;

    // Store instance handle in our global variable
    hInst = hInstance;

    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

    // Perform application initialization:
    if (!InitInstance()) {
        return FALSE;
    }

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyInstance();
    return (int) msg.wParam;
}
