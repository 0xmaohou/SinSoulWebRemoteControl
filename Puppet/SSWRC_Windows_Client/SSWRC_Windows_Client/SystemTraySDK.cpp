﻿/* ************************************
*《精通Windows API》 
* 示例代码
* SystmeTray.cpp
* 12.4 通知区域图标
**************************************/
/*  预编译声明　*/
#include "SystemTraySDK.h"
#include <assert.h>
#define _T(x)       (x)
#define ASSERT assert

#define TRAYICON_CLASS "TrayIconClass"

/*  全局变量　*/
CSystemTray* CSystemTray::m_pThis = NULL;
const UINT CSystemTray::m_nTimerID    = 4567;
UINT CSystemTray::m_nMaxTooltipLength  = 64;     
const UINT CSystemTray::m_nTaskbarCreatedMsg = ::RegisterWindowMessage("TaskbarCreated");
HWND  CSystemTray::m_hWndInvisible;

/* ************************************
* CSystemTray::CSystemTray()
* 功能	初始化，调用Initialise，暂不创建图标
**************************************/
CSystemTray::CSystemTray()
{
	Initialise();
}

/* ************************************
* CSystemTray::CSystemTray(...)
* 功能	初始化，调用Initialise，直接创建图标
**************************************/
CSystemTray::CSystemTray(HINSTANCE hInst,		//应用程序句柄
						 HWND hParent,				// 用于接收图标的notifications
						 UINT uCallbackMessage,     // 发送给父亲窗口的回调用消息
						 LPCTSTR szToolTip,		// tray icon tooltip
						 HICON icon,					// 图标的句柄
						 UINT uID,						// 图标的标识符
						 BOOL bHidden,				// 创建后是否隐藏    =FALSE            
						 LPCTSTR szBalloonTip,	// 气泡
						 LPCTSTR szBalloonTitle,	// 气泡的标题
						 DWORD dwBalloonIcon,	// 气泡的图标
						 UINT uBalloonTimeout)		// 气泡滞留时间
{
	Initialise();
	Create(hInst, hParent, uCallbackMessage, szToolTip, icon, uID, bHidden,
		szBalloonTip, szBalloonTitle, dwBalloonIcon, uBalloonTimeout);
}
/* ************************************
* void CSystemTray::Initialise()
* 功能	初始化，设置类成员变量等
**************************************/
void CSystemTray::Initialise()
{
	m_pThis = this;

	memset(&m_tnd, 0, sizeof(m_tnd));
	m_bEnabled = FALSE;
	m_bHidden  = TRUE;
	m_bRemoved = TRUE;

	m_DefaultMenuItemID    = 0;
	m_DefaultMenuItemByPos = TRUE;

	m_bShowIconPending = FALSE;

	m_uIDTimer   = 0;
	m_hSavedIcon = NULL;

	m_hTargetWnd = NULL;
	m_uCreationFlags = 0;

	// 系统版本判断
	OSVERSIONINFO os = { sizeof(os) };
	GetVersionEx(&os);
	m_bWin2K = ( VER_PLATFORM_WIN32_NT == os.dwPlatformId && os.dwMajorVersion >= 5 );
}
/* ************************************
* ATOM CSystemTray::RegisterClass(HINSTANCE hInstance)
* 功能	注册主窗口类
**************************************/
ATOM CSystemTray::RegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)WindowProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= 0;
	wcex.hCursor		= 0;
	wcex.hbrBackground	= 0;
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= TRAYICON_CLASS;
	wcex.hIconSm		= 0;

	return RegisterClassEx(&wcex);
}
/* ************************************
* BOOL CSystemTray::Create(...)
* 功能	创建通知图标窗口
**************************************/
BOOL CSystemTray::Create(HINSTANCE hInst, HWND hParent, UINT uCallbackMessage, 
						 LPCTSTR szToolTip, HICON icon, UINT uID, 
						 BOOL bHidden /*=FALSE*/,
						 LPCTSTR szBalloonTip, 
						 LPCTSTR szBalloonTitle ,  
						 DWORD dwBalloonIcon,
						 UINT uBalloonTimeout)
{
	// 判断系统版本 >= Windows 95 
	m_bEnabled = (GetVersion() & 0xff) >= 4;
	if (!m_bEnabled) 
	{
		ASSERT(FALSE);
		return FALSE;
	}

	m_nMaxTooltipLength = _countof(m_tnd.szTip);
	// 判断消息
	ASSERT(uCallbackMessage >= WM_APP);
	// 防止溢出
	ASSERT(strlen(szToolTip) <= m_nMaxTooltipLength);

	m_hInstance = hInst;
	RegisterClass(hInst);

	// 创建窗口
	m_hWnd = ::CreateWindow(TRAYICON_CLASS, _T(""), WS_POPUP, 
		CW_USEDEFAULT,CW_USEDEFAULT, 
		CW_USEDEFAULT,CW_USEDEFAULT, 
		NULL, 0,
		hInst, 0);

	// 填充 NOTIFYICONDATA 结构
	m_tnd.cbSize = sizeof(NOTIFYICONDATA);
	m_tnd.hWnd   = (hParent)? hParent : m_hWnd;
	m_tnd.uID    = uID;
	m_tnd.hIcon  = icon;
	m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_tnd.uCallbackMessage = uCallbackMessage;

	strncpy_s(m_tnd.szTip, szToolTip, m_nMaxTooltipLength);

	// 判断
	if (m_bWin2K && szBalloonTip)
	{
		ASSERT(lstrlen(szBalloonTip) < 256);
		if (szBalloonTitle)
		{
			ASSERT(lstrlen(szBalloonTitle) < 64);
		}

		ASSERT(NIIF_NONE == dwBalloonIcon    || NIIF_INFO == dwBalloonIcon ||
			NIIF_WARNING == dwBalloonIcon || NIIF_ERROR == dwBalloonIcon);

		ASSERT(uBalloonTimeout >= 10 && uBalloonTimeout <= 30);

		m_tnd.uFlags |= NIF_INFO;				// 气泡

		strncpy_s(m_tnd.szInfo, szBalloonTip, 255);
		if (szBalloonTitle)
			strncpy_s(m_tnd.szInfoTitle, szBalloonTitle, 63);
		else
			m_tnd.szInfoTitle[0] = _T('\0');
		m_tnd.uTimeout    = uBalloonTimeout * 1000; // ms
		m_tnd.dwInfoFlags = dwBalloonIcon;
	}

	m_bHidden = bHidden;
	m_hTargetWnd = m_tnd.hWnd;

	if (m_bWin2K && m_bHidden)
	{
		m_tnd.uFlags = NIF_STATE;
		m_tnd.dwState = NIS_HIDDEN;
		m_tnd.dwStateMask = NIS_HIDDEN;
	}

	m_uCreationFlags = m_tnd.uFlags;	

	BOOL bResult = TRUE;
	if (!m_bHidden || m_bWin2K)
	{
		// 以NIM_ADD 为参数调用Shell_NotifyIcon，增加通知区域图标
		bResult = Shell_NotifyIcon(NIM_ADD, &m_tnd);
		m_bShowIconPending = m_bHidden = m_bRemoved = !bResult;
	}

	if (m_bWin2K && szBalloonTip)
	{
		m_tnd.szInfo[0] = _T('\0');
	}

	return bResult;
}

/* ************************************
* CSystemTray::~CSystemTray()
**************************************/
CSystemTray::~CSystemTray()
{
	RemoveIcon();
	m_IconList.clear();
	if (m_hWnd)
		::DestroyWindow(m_hWnd);
}

/* ************************************
* void CSystemTray::SetFocus()
* 功能	设置焦点
**************************************/
void CSystemTray::SetFocus()
{
	Shell_NotifyIcon ( NIM_SETFOCUS, &m_tnd );
}

/* ************************************
* BOOL CSystemTray::MoveToRight()
* BOOL CSystemTray::AddIcon()
* BOOL CSystemTray::RemoveIcon()
* BOOL CSystemTray::HideIcon()
* BOOL CSystemTray::ShowIcon()
* BOOL CSystemTray::SetIcon(HICON hIcon)
* BOOL CSystemTray::SetIcon(HICON hIcon)
* BOOL CSystemTray::SetIcon(UINT nIDResource)
* BOOL CSystemTray::SetStandardIcon(LPCTSTR lpIconName)
* BOOL CSystemTray::SetStandardIcon(UINT nIDResource)
* HICON CSystemTray::GetIcon()
* BOOL CSystemTray::SetIconList(UINT uFirstIconID, UINT uLastIconID)
* BOOL CSystemTray::SetIconList(HICON* pHIconList, UINT nNumIcons)
* 功能	为通知区域图标ICON相关处理
**************************************/
BOOL CSystemTray::MoveToRight()
{
	RemoveIcon();
	return AddIcon();
}
BOOL CSystemTray::AddIcon()
{
	if (!m_bRemoved)
		RemoveIcon();

	if (m_bEnabled)
	{
		m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		if (!Shell_NotifyIcon(NIM_ADD, &m_tnd))
			m_bShowIconPending = TRUE;
		else
			m_bRemoved = m_bHidden = FALSE;
	}
	return (m_bRemoved == FALSE);
}
BOOL CSystemTray::RemoveIcon()
{
	m_bShowIconPending = FALSE;

	if (!m_bEnabled || m_bRemoved)
		return TRUE;

	m_tnd.uFlags = 0;
	if (Shell_NotifyIcon(NIM_DELETE, &m_tnd))
		m_bRemoved = m_bHidden = TRUE;

	return (m_bRemoved == TRUE);
}

BOOL CSystemTray::HideIcon()
{
	if (!m_bEnabled || m_bRemoved || m_bHidden)
		return TRUE;
	if (m_bWin2K)
	{
		m_tnd.uFlags = NIF_STATE;
		m_tnd.dwState = NIS_HIDDEN;
		m_tnd.dwStateMask = NIS_HIDDEN;

		m_bHidden = Shell_NotifyIcon( NIM_MODIFY, &m_tnd);
	}
	else
		RemoveIcon();

	return (m_bHidden == TRUE);
}

BOOL CSystemTray::ShowIcon()
{
	if (m_bRemoved)
		return AddIcon();

	if (!m_bHidden)
		return TRUE;
	if (m_bWin2K)
	{
		m_tnd.uFlags = NIF_STATE;
		m_tnd.dwState = 0;
		m_tnd.dwStateMask = NIS_HIDDEN;
		Shell_NotifyIcon ( NIM_MODIFY, &m_tnd );
	}
	else
		AddIcon();

	return (m_bHidden == FALSE);
}

BOOL CSystemTray::SetIcon(HICON hIcon)
{
	if (!m_bEnabled)
		return FALSE;

	m_tnd.uFlags = NIF_ICON;
	m_tnd.hIcon = hIcon;

	if (m_bHidden)
		return TRUE;
	else
		return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

BOOL CSystemTray::SetIcon(LPCTSTR lpszIconName)
{
	HICON hIcon = (HICON) ::LoadImage(m_hInstance, 
		lpszIconName,
		IMAGE_ICON, 
		0, 0,
		LR_LOADFROMFILE);

	if (!hIcon)
		return FALSE;
	BOOL returnCode = SetIcon(hIcon);
	::DestroyIcon(hIcon);
	return returnCode;
}

BOOL CSystemTray::SetIcon(UINT nIDResource)
{
	HICON hIcon = (HICON) ::LoadImage(m_hInstance, 
		MAKEINTRESOURCE(nIDResource),
		IMAGE_ICON, 
		0, 0,
		LR_DEFAULTCOLOR);

	BOOL returnCode = SetIcon(hIcon);
	::DestroyIcon(hIcon);
	return returnCode;
}

BOOL CSystemTray::SetStandardIcon(LPCTSTR lpIconName)
{
	HICON hIcon = ::LoadIcon(NULL, lpIconName);

	return SetIcon(hIcon);
}

BOOL CSystemTray::SetStandardIcon(UINT nIDResource)
{
	HICON hIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(nIDResource));

	return SetIcon(hIcon);
}

HICON CSystemTray::GetIcon() const
{
	return (m_bEnabled)? m_tnd.hIcon : NULL;
}

BOOL CSystemTray::SetIconList(UINT uFirstIconID, UINT uLastIconID) 
{
	if (uFirstIconID > uLastIconID)
		return FALSE;

	UINT uIconArraySize = uLastIconID - uFirstIconID + 1;

	m_IconList.clear();
	try 
	{
		for (UINT i = uFirstIconID; i <= uLastIconID; i++)
			m_IconList.push_back(::LoadIcon(m_hInstance, MAKEINTRESOURCE(i)));
	}
	catch (...)
	{
		m_IconList.clear();
		return FALSE;
	}

	return TRUE;
}

BOOL CSystemTray::SetIconList(HICON* pHIconList, UINT nNumIcons)
{
	m_IconList.clear();

	try {
		for (UINT i = 0; i <= nNumIcons; i++)
			m_IconList.push_back(pHIconList[i]);
	}
	catch (...)
	{
		m_IconList.clear();
		return FALSE;
	}

	return TRUE;
}
/* ************************************
*BOOL CSystemTray::Animate(UINT nDelayMilliSeconds, int nNumSeconds )
* 功能	使图标动作
**************************************/
BOOL CSystemTray::Animate(UINT nDelayMilliSeconds, int nNumSeconds /*=-1*/)
{
	if (m_IconList.empty())
		return FALSE;

	StopAnimation();

	m_nCurrentIcon = 0;
	time(&m_StartTime);
	m_nAnimationPeriod = nNumSeconds;
	m_hSavedIcon = GetIcon();

	// 计时器，注意实例中对计时器消息的处理
	m_uIDTimer = ::SetTimer(m_hWnd, m_nTimerID, nDelayMilliSeconds, NULL);
	return (m_uIDTimer != 0);
}
/* ************************************
*BOOL CSystemTray::StepAnimation()
* 功能	使图标动作
**************************************/
BOOL CSystemTray::StepAnimation()
{
	if (!m_IconList.size())
		return FALSE;

	m_nCurrentIcon++;
	if ((unsigned int)m_nCurrentIcon >= m_IconList.size())
		m_nCurrentIcon = 0;

	return SetIcon(m_IconList[m_nCurrentIcon]);
}
/* ************************************
*BOOL CSystemTray::StopAnimation()
* 功能	停止图标动作
**************************************/
BOOL CSystemTray::StopAnimation()
{
	BOOL bResult = FALSE;

	if (m_uIDTimer)
		bResult = ::KillTimer(m_hWnd, m_uIDTimer);
	m_uIDTimer = 0;

	if (m_hSavedIcon)
		SetIcon(m_hSavedIcon);
	m_hSavedIcon = NULL;

	return bResult;
}

/* ************************************
*BOOL CSystemTray::SetTooltipText(LPCTSTR pszTip)
*BOOL CSystemTray::SetTooltipText(UINT nID)
*LPTSTR CSystemTray::GetTooltipText() 
* 功能	设置（获取）图标ToolTip
**************************************/
BOOL CSystemTray::SetTooltipText(LPCTSTR pszTip)
{
	ASSERT(strlen(pszTip) < m_nMaxTooltipLength);

	if (!m_bEnabled)
		return FALSE;

	m_tnd.uFlags = NIF_TIP;
	strncpy_s(m_tnd.szTip, pszTip, m_nMaxTooltipLength-1);

	if (m_bHidden)
		return TRUE;
	else
		return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

BOOL CSystemTray::SetTooltipText(UINT nID)
{
	TCHAR strBuffer[1024];
	ASSERT(1024 >= m_nMaxTooltipLength);

	if (!LoadString(m_hInstance, nID, strBuffer, m_nMaxTooltipLength-1))
		return FALSE;

	return SetTooltipText(strBuffer);
}

LPTSTR CSystemTray::GetTooltipText() const
{
	if (!m_bEnabled)
		return FALSE;

	static TCHAR strBuffer[1024];
	ASSERT(1024 >= m_nMaxTooltipLength);

	strncpy_s(strBuffer, m_tnd.szTip, m_nMaxTooltipLength-1);
	return strBuffer;
}

/* ************************************
* CSystemTray::ShowBalloon
* 功能	显示气泡
**************************************/
BOOL CSystemTray::ShowBalloon(LPCTSTR szText,
							  LPCTSTR szTitle,
							  DWORD   dwIcon	/*=NIIF_NONE*/,
							  UINT    uTimeout	/*=10*/ )
{
	// Win2k以上
	if (!m_bWin2K)
		return FALSE;
	// 防止溢出
	ASSERT(lstrlen(szText) < 256);

	// 防止Title溢出
	if (szTitle)
	{
		ASSERT(lstrlen(szTitle) < 64);
	}
	// 判断Flag
	ASSERT(NIIF_NONE == dwIcon    || NIIF_INFO == dwIcon ||
		NIIF_WARNING == dwIcon || NIIF_ERROR == dwIcon);

	// 判断Timeout
//	ASSERT(uTimeout >= 10 && uTimeout <= 30);
	// 设置 NOTIFYICONDATA结构
	m_tnd.uFlags = NIF_INFO;		// 显示气泡信息
	strncpy_s(m_tnd.szInfo, szText, 256);
	if (szTitle)
		strncpy_s(m_tnd.szInfoTitle, szTitle, 64);
	else
		m_tnd.szInfoTitle[0] = _T('\0');
	m_tnd.dwInfoFlags = dwIcon;
	m_tnd.uTimeout = uTimeout * 1000;   // ms
	// 以 NIM_MODIFY 为参数，调用 Shell_NotifyIcon
	BOOL bSuccess = Shell_NotifyIcon (NIM_MODIFY, &m_tnd);

	// 清零
	m_tnd.szInfo[0] = _T('\0');
	return bSuccess;
}

/* ************************************
* BOOL CSystemTray::SetNotificationWnd(HWND hNotifyWnd)
* HWND CSystemTray::GetNotificationWnd() 
* BOOL CSystemTray::SetTargetWnd(HWND hTargetWnd)
* BOOL CSystemTray::SetCallbackMessage(UINT uCallbackMessage)
* UINT CSystemTray::GetCallbackMessage()
* BOOL CSystemTray::SetMenuDefaultItem(UINT uItem, BOOL bByPos)
* void CSystemTray::GetMenuDefaultItem(UINT& uItem, BOOL& bByPos)
* 功能	处理与主窗口（接收图标发送的消息的窗口）
*			消息、菜单等相关的内容
**************************************/
BOOL CSystemTray::SetNotificationWnd(HWND hNotifyWnd)
{
	if (!m_bEnabled)
		return FALSE;

	if (!hNotifyWnd || !::IsWindow(hNotifyWnd))
	{
		ASSERT(FALSE);
		return FALSE;
	}

	m_tnd.hWnd = hNotifyWnd;
	m_tnd.uFlags = 0;

	if (m_bHidden)
		return TRUE;
	else
		return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

HWND CSystemTray::GetNotificationWnd() const
{
	return m_tnd.hWnd;
}

BOOL CSystemTray::SetTargetWnd(HWND hTargetWnd)
{
	m_hTargetWnd = hTargetWnd;
	return TRUE;
} 

HWND CSystemTray::GetTargetWnd() const
{
	if (m_hTargetWnd)
		return m_hTargetWnd;
	else
		return m_tnd.hWnd;
} 


BOOL CSystemTray::SetCallbackMessage(UINT uCallbackMessage)
{
	if (!m_bEnabled)
		return FALSE;

	ASSERT(uCallbackMessage >= WM_APP);

	m_tnd.uCallbackMessage = uCallbackMessage;
	m_tnd.uFlags = NIF_MESSAGE;

	if (m_bHidden)
		return TRUE;
	else
		return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

UINT CSystemTray::GetCallbackMessage() const
{
	return m_tnd.uCallbackMessage;
}

BOOL CSystemTray::SetMenuDefaultItem(UINT uItem, BOOL bByPos)
{
	if ((m_DefaultMenuItemID == uItem) && (m_DefaultMenuItemByPos == bByPos)) 
		return TRUE;

	m_DefaultMenuItemID = uItem;
	m_DefaultMenuItemByPos = bByPos;   

	HMENU hMenu = ::LoadMenu(m_hInstance, MAKEINTRESOURCE(m_tnd.uID));
	if (!hMenu)
		return FALSE;

	HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
	if (!hSubMenu)
	{
		::DestroyMenu(hMenu);
		return FALSE;
	}

	::SetMenuDefaultItem(hSubMenu, m_DefaultMenuItemID, m_DefaultMenuItemByPos);

	::DestroyMenu(hSubMenu);
	::DestroyMenu(hMenu);

	return TRUE;
}

void CSystemTray::GetMenuDefaultItem(UINT& uItem, BOOL& bByPos)
{
	uItem = m_DefaultMenuItemID;
	bByPos = m_DefaultMenuItemByPos;
}

/////////////////////////////////////////////////////////////////////////////
/* ************************************
* CSystemTray 消息处理相关成员函数
**************************************/
/* ************************************
* 功能	处理计时器，图标动画
**************************************/
LRESULT CSystemTray::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent != m_uIDTimer)
	{
		ASSERT(FALSE);
		return 0L;
	}

	time_t CurrentTime;
	time(&CurrentTime);

	time_t period = CurrentTime - m_StartTime;
	if (m_nAnimationPeriod > 0 && m_nAnimationPeriod < period)
	{
		StopAnimation();
		return 0L;
	}

	StepAnimation();

	return 0L;
}

/* ************************************
* Pending
**************************************/
LRESULT CSystemTray::OnTaskbarCreated(WPARAM wParam, LPARAM lParam) 
{
	InstallIconPending();
	return 0L;
}
/* ************************************
* Pending
**************************************/
LRESULT CSystemTray::OnSettingChange(UINT uFlags, LPCTSTR lpszSection) 
{
	if (uFlags == SPI_SETWORKAREA)
		InstallIconPending();
	return 0L;
}

/* ************************************
* 处理Notification
**************************************/
LRESULT CSystemTray::OnTrayNotification(UINT wParam, LONG lParam) 
{
	if (wParam != m_tnd.uID)
		return 0L;

	HWND hTargetWnd = GetTargetWnd();
	if (!hTargetWnd)
		return 0L;

	if (LOWORD(lParam) == WM_RBUTTONUP)
	{    
		HMENU hMenu = ::LoadMenu(m_hInstance, MAKEINTRESOURCE(m_tnd.uID));
		if (!hMenu)
			return 0;

		HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
		if (!hSubMenu)
		{
			::DestroyMenu(hMenu);        
			return 0;
		}

		::SetMenuDefaultItem(hSubMenu, m_DefaultMenuItemID, m_DefaultMenuItemByPos);

		CustomizeMenu(hSubMenu);

		POINT pos;
		GetCursorPos(&pos);

		::SetForegroundWindow(m_tnd.hWnd);  
		::TrackPopupMenu(hSubMenu, 0, pos.x, pos.y, 0, hTargetWnd, NULL);

		::PostMessage(m_tnd.hWnd, WM_NULL, 0, 0);

		DestroyMenu(hMenu);
	} 
	else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) 
	{
		::SetForegroundWindow(m_tnd.hWnd);  

		UINT uItem;
		if (m_DefaultMenuItemByPos)
		{
			HMENU hMenu = ::LoadMenu(m_hInstance, MAKEINTRESOURCE(m_tnd.uID));
			if (!hMenu)
				return 0;

			HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
			if (!hSubMenu)
				return 0;
			uItem = ::GetMenuItemID(hSubMenu, m_DefaultMenuItemID);

			DestroyMenu(hMenu);
		}
		else
			uItem = m_DefaultMenuItemID;

		::PostMessage(hTargetWnd, WM_COMMAND, uItem, 0);
	}

	return 1;
}

/* ************************************
* 功能 Tray图标窗口消息处理函数
**************************************/
LRESULT PASCAL CSystemTray::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	CSystemTray* pTrayIcon = m_pThis;
	if (pTrayIcon->GetSafeHwnd() != hWnd)
		return ::DefWindowProc(hWnd, message, wParam, lParam);

	// Tray 被创建 此消息必须处理
	if (message == CSystemTray::m_nTaskbarCreatedMsg)
		return pTrayIcon->OnTaskbarCreated(wParam, lParam);

	// 图标动画计时器
	if (message == WM_TIMER && wParam == pTrayIcon->GetTimerID())
		return pTrayIcon->OnTimer(wParam);

	// 设置改变
	if (message == WM_SETTINGCHANGE && wParam == pTrayIcon->GetTimerID())
		return pTrayIcon->OnSettingChange(wParam, (LPCTSTR) lParam);

	// 是否是需要处理的消息
	if (message == pTrayIcon->GetCallbackMessage())
		return pTrayIcon->OnTrayNotification(wParam, lParam);

	return ::DefWindowProc(hWnd, message, wParam, lParam);
}

/* ************************************
* void CSystemTray::InstallIconPending()
**************************************/
void CSystemTray::InstallIconPending()
{
	// 是否pending
	if (!m_bShowIconPending || m_bHidden)
		return;

	// 重置标记
	m_tnd.uFlags = m_uCreationFlags;

	// 重新创建ICON
	m_bHidden = !Shell_NotifyIcon(NIM_ADD, &m_tnd);

	// 仍然... 下次再处理
	m_bShowIconPending = !m_bHidden;

	ASSERT(m_bHidden == FALSE);
}
/* ************************************
*	查找 Tray 窗口
* 最小化到通知区域，从通知区域恢复时需要调用
**************************************/
BOOL CALLBACK FindTrayWnd(HWND hwnd, LPARAM lParam)
{
	TCHAR szClassName[256];
	GetClassName(hwnd, szClassName, 255);

	if (strcmp(szClassName, _T("TrayNotifyWnd")) == 0)
	{
		LPRECT lpRect = (LPRECT) lParam;
		::GetWindowRect(hwnd, lpRect);
		return TRUE;
	}

	if (strcmp(szClassName, _T("TrayClockWClass")) == 0)
	{
		LPRECT lpRect = (LPRECT) lParam;
		RECT rectClock;
		::GetWindowRect(hwnd, &rectClock);

		if (rectClock.bottom < lpRect->bottom-5)
			lpRect->top = rectClock.bottom;
		else
			lpRect->right = rectClock.left;
		return FALSE;
	}

	return TRUE;
}

/* ************************************
*	获得Tray的RECT
**************************************/
void CSystemTray::GetTrayWndRect(LPRECT lprect)
{
#define DEFAULT_RECT_WIDTH 150
#define DEFAULT_RECT_HEIGHT 30

	HWND hShellTrayWnd = FindWindow(_T("Shell_TrayWnd"), NULL);
	if (hShellTrayWnd)
	{
		GetWindowRect(hShellTrayWnd, lprect);
		EnumChildWindows(hShellTrayWnd, FindTrayWnd, (LPARAM)lprect);
		return;
	}

	APPBARDATA appBarData;
	appBarData.cbSize=sizeof(appBarData);
	if (SHAppBarMessage(ABM_GETTASKBARPOS,&appBarData))
	{

		switch(appBarData.uEdge)
		{
		case ABE_LEFT:
		case ABE_RIGHT:

			lprect->top    = appBarData.rc.bottom-100;
			lprect->bottom = appBarData.rc.bottom-16;
			lprect->left   = appBarData.rc.left;
			lprect->right  = appBarData.rc.right;
			break;

		case ABE_TOP:
		case ABE_BOTTOM:

			lprect->top    = appBarData.rc.top;
			lprect->bottom = appBarData.rc.bottom;
			lprect->left   = appBarData.rc.right-100;
			lprect->right  = appBarData.rc.right-16;
			break;
		}
		return;
	}

	if (hShellTrayWnd)
	{
		::GetWindowRect(hShellTrayWnd, lprect);
		if (lprect->right - lprect->left > DEFAULT_RECT_WIDTH)
			lprect->left = lprect->right - DEFAULT_RECT_WIDTH;
		if (lprect->bottom - lprect->top > DEFAULT_RECT_HEIGHT)
			lprect->top = lprect->bottom - DEFAULT_RECT_HEIGHT;

		return;
	}

	SystemParametersInfo(SPI_GETWORKAREA,0,lprect, 0);
	lprect->left = lprect->right - DEFAULT_RECT_WIDTH;
	lprect->top  = lprect->bottom - DEFAULT_RECT_HEIGHT;
}

/* ************************************
*	MinAnimate?
**************************************/
BOOL CSystemTray::GetDoWndAnimation()
{
	ANIMATIONINFO ai;

	ai.cbSize=sizeof(ai);
	SystemParametersInfo(SPI_GETANIMATION,sizeof(ai),&ai,0);

	return ai.iMinAnimate?TRUE:FALSE;
}
/* ************************************
*	删除任务档图标
**************************************/
BOOL CSystemTray::RemoveTaskbarIcon(HWND hWnd)
{
	if (!::IsWindow(m_hWndInvisible))
	{
		m_hWndInvisible = CreateWindowEx(0, "Static", _T(""), WS_POPUP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, 0, NULL, 0);

		if (!m_hWndInvisible)
			return FALSE;
	}

	SetParent(hWnd, m_hWndInvisible);

	return TRUE;
}
/* ************************************
*	将窗口最小化到通知区域
**************************************/
void CSystemTray::MinimiseToTray(HWND hWnd)
{

	if (GetDoWndAnimation())
	{
		RECT rectFrom, rectTo;

		GetWindowRect(hWnd, &rectFrom);
		GetTrayWndRect(&rectTo);

		DrawAnimatedRects(hWnd, IDANI_CAPTION, &rectFrom, &rectTo);
	}

	RemoveTaskbarIcon(hWnd);
	SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) &~ WS_VISIBLE);

}
/* ************************************
*	从通知区域恢复大小
**************************************/
void CSystemTray::MaximiseFromTray(HWND hWnd)
{

	if (GetDoWndAnimation())
	{
		RECT rectTo;
		::GetWindowRect(hWnd, &rectTo);

		RECT rectFrom;
		GetTrayWndRect(&rectFrom);

		::SetParent(hWnd, NULL);
		DrawAnimatedRects(hWnd, IDANI_CAPTION, &rectFrom, &rectTo);
	}
	else
		::SetParent(hWnd, NULL);

	SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_VISIBLE);
	RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME |
		RDW_INVALIDATE | RDW_ERASE);

	// 焦点
	if (::IsWindow(m_hWndInvisible))
		SetActiveWindow(m_hWndInvisible);
	SetActiveWindow(hWnd);
	SetForegroundWindow(hWnd);
}
