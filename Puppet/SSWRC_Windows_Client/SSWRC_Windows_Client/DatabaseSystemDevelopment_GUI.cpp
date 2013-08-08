#include "MyStdAfx.h"
#include "Commctrl.h"
//#pragma comment(linker,"/SUBSYSTEM:Windows")

//#pragma comment(lib,"Comctl32.lib")
char szClassName[11 ] = "WindowsApp";
char *wndName="WRC_���ݿ�ϵͳ�γ���ƿͻ���_GUI";
HWND hwndButton = 0;
HWND hwndEdit = 0; 
HWND hwndLable_0 = 0;
HWND hwnd=0; 
HWND hwndCheck=0;
bool conn_status=false;
WNDPROC OldEditProc;
HINSTANCE hinst;
CSystemTray TrayIcon;
#define	WM_ICON_NOTIFY WM_APP+10
int WINAPI WinMain (HINSTANCE hThisInstance,HINSTANCE hPrevInstance,LPSTR lpszArgument,int nFunsterStil)
{ 
	MSG messages;
	WNDCLASSEX wincl;

	wincl.hInstance = hThisInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = WindowProcedure;
	wincl.style = CS_DBLCLKS;
	wincl.cbSize = sizeof (WNDCLASSEX);
	wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	wincl.hIconSm = (HICON)LoadImage(hThisInstance,MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = (HBRUSH) COLOR_MENU;

	//InitCommonControls();

	if (!RegisterClassEx (&wincl))
	{
		return 0;
	}

	hwnd = CreateWindowEx (NULL,szClassName,wndName, 
		WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,
		550,350,HWND_DESKTOP,NULL,hThisInstance,NULL);

	hwndButton=CreateWindowEx(NULL,TEXT("BUTTON"),TEXT("��¼"),
		WS_CHILD|WS_VISIBLE
		,360,20,50,20,hwnd,(struct HMENU__ *)ID_BTN_LOGIN,hThisInstance,NULL);

	hwndEdit=CreateWindowEx(NULL,TEXT("EDIT"),TEXT(""),
		WS_CHILD|WS_VISIBLE|WS_BORDER|BS_TEXT,
		140,20,200,20,hwnd,(struct HMENU__ *)0,hThisInstance,NULL);

	hwndLable_0=CreateWindowEx(NULL,TEXT("EDIT"),TEXT(""),
		WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY,
		20,70,495,230,hwnd,(struct HMENU__ *)0,hThisInstance,NULL);
	Edit_LimitText(hwndLable_0,0);

	hwndCheck = CreateWindow("BUTTON","��ס�û������Զ���¼",
		WS_VISIBLE | WS_CHILD |BS_CHECKBOX ,
		155,45,170,20,hwnd,(struct HMENU__ *)8899,hThisInstance,NULL);

	TrayIcon.Create(hThisInstance,hwnd,WM_ICON_NOTIFY,_T("WRC���ݿ�ϵͳ�γ���ƿͻ���"),
		(HICON)LoadImage(hThisInstance,MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR),
		ID_POPUP_MENU);

	if (InitialStruct())
	{
		Msg("��ʼ�����,�������û������е�¼!\r\n");
	}
	else
	{
		Msg("��ʼ��ʧ�ܣ�������缰����ǽ����!\r\n");
	}
	OldEditProc=(WNDPROC)SetWindowLong (hwndEdit, GWL_WNDPROC, (LONG)EditProcedure);
	ShowWindow (hwnd, nFunsterStil);
	hinst=hThisInstance;
	while (GetMessage (&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}
	return messages.wParam;
}


LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ICON_NOTIFY:
		return TrayIcon.OnTrayNotification(wParam, lParam);
		break;
	case WM_DESTROY:
		{
			if (conn_status)
			{
				Logout();
			}
			PostQuitMessage (0);
		}
		break;
	case WM_CLOSE:
		{
			if (conn_status)
			{
				Logout();
			}
			PostQuitMessage (0);
		}break;
	case WM_SYSCOMMAND:
		{
			if (LOWORD(wParam)==SC_MINIMIZE)
			{
				TrayIcon.MinimiseToTray(hwnd);
			}
			if (LOWORD(wParam)==SC_CLOSE)
			{
				DestroyWindow(hwnd);
			}
			return DefWindowProc (hwnd, message, wParam, lParam);
		}break;
	case WM_COMMAND:
		{ 
			if (LOWORD(wParam)==8899)
			{
				bool flag =SendMessage(hwndCheck,BM_GETCHECK,0,0) == BST_CHECKED ? false:true;
				SendMessage(hwndCheck,BM_SETCHECK,flag,0);
			}
			if (LOWORD(wParam)==IDM_EXIT)
			{
				DestroyWindow(hwnd);
			}
			if (LOWORD(wParam)==IDM_HIDE)
			{
				TrayIcon.MinimiseToTray(hwnd);
			}
			if (LOWORD(wParam)==IDM_SHOW)
			{
				TrayIcon.MaximiseFromTray(hwnd);
			}
			if (LOWORD(wParam)==ID_POPUP_SETTING)
			{
				TrayIcon.MaximiseFromTray(hwnd);
				DialogBox(hinst, (LPCTSTR)IDD_SETTING, hwnd, (DLGPROC)SetServerAddr);
			}
			
			if(LOWORD(wParam)==ID_BTN_LOGIN) 
			{
				//InitialDX();
				//DXScreenShot();
				//UnloadDx();
				
				if (!conn_status)
				{
					GetHostInfo(pHI);			
					if (GetThisUserName())
					{
						EnableWindow(hwndButton,false);
						_beginthread(Online,0,NULL);
					}
					else
					{
						return -1;
					}
				}
				else              //��ס���ﻹҪд�������ߵĴ��룡����
				{
					Logout();
					conn_status=false;
					if (CameraStatus)
					{
						CameraStatus=FALSE;
						StopCaptureDevice();
					}
					SetWindowText(hwndButton,TEXT("��¼"));
				}			
			}
		}
		break;
	case WM_ONLINE_MESSAGE:
		{
			int args=0;
			_beginthread(ReceiveCommand,0,(void*)args);
		}
		break;
	case WM_CMDRCVD_MESSAGE:
		{
			//			CMD_ID=88 : ����
			//			CMD_ID=0 : ������
			//			CMD_ID=1 : ��Ļ��ͼ
			//			CMD_ID=2 : ����ͷ��ͼ
			//			CMD_ID=3 : ��������ͷ
			//			CMD_ID=5 : �ر�����ͷ
			//			CMD_ID=4 : ����¼�
			//			CMD_ID=6 : ���̰���
			//			CMD_ID=7 : ���̵���
			//			CMD_ID=8 : ö���ļ��У������ļ�������
			//			CMD_ID=9 : ö���ļ������ļ����б�չʾ��
			//			CMD_ID=10 : ö�ٽ���
			//			CMD_ID=11 : �����ļ����������ƣ�ճ����ɾ������������
			//			CMD_ID=12 : �������̲�������������ģ�飬���̣߳�
			cmdinfo *cmd_rcvd=new cmdinfo;
			memcpy(cmd_rcvd,(cmdinfo*)wParam,sizeof(cmdinfo));
			switch(atoi(cmd_rcvd->cmd_type))
			{
			case 88:
				{
					Msg("����֪ͨ\r\n");
				}break;
			case 0:
				{
					_beginthread(RunCmd,0,(void*)cmd_rcvd);
				}break;
			case 1:
				{
					_beginthread(ScreenCapture,0,(void*)cmd_rcvd);
				}
				break;
			case 2:
				{
					_beginthread(CaptureCurrentFrame,0,(void*)cmd_rcvd);
				}
				break;
			case 3:
				{
					if (CameraStatus)
					{
						SubmitResult(cmd_rcvd,"1"); 
					}
					else if (StartCaptureDevice(hwnd))
					{
						//	Sleep(1500);//�ȴ�����ͷ���������ɹ�
						SubmitResult(cmd_rcvd,"1"); 
						CameraStatus=TRUE;
					}
					else
					{
						SubmitResult(cmd_rcvd,"0");
					}

				}
				break;
			case 5:
				{
					CameraStatus=FALSE;
					StopCaptureDevice();
					SubmitResult(cmd_rcvd,"1"); 
				}
				break;
			case 4:	//��������ʽ��<act>4</act><x>124</x><y>3534</y>
				{	//action=1:����
					//action=2:˫��
					//action=3:�һ�
					mousecommand mscmd;
					if (AnalyzeMouseCommand(cmd_rcvd->cmd_content,mscmd))
					{
						if (mscmd.action==1)
						{
							LeftClick(mscmd.x_pos,mscmd.y_pos);
						}else if (mscmd.action==2)
						{
							LeftDoubleClick(mscmd.x_pos,mscmd.y_pos);
						}else if (mscmd.action==3)
						{
							RightClick(mscmd.x_pos,mscmd.y_pos);
						}
					//	SubmitResult(cmd_rcvd->cmd_id,"0");
					}
					else
					{
						SubmitResult(cmd_rcvd,"����������");
					}
				}
				break;
			case 6:
				{
					KeyDown(atoi(cmd_rcvd->cmd_content.c_str()));
				}
				break;
			case 7:
				{
					KeyUp(atoi(cmd_rcvd->cmd_content.c_str()));
				}
				break;
			case 8:
				{
					_beginthread(Enum_Drectory,0,(void*)cmd_rcvd);
				}
				break;
			case 9:
				{
					_beginthread(Enum_All_File_in_Drectory,0,(void*)cmd_rcvd);
				}
				break;	
			case 10:
				{
					//EnumProcess();
					_beginthread(TaskList,0,(void*)cmd_rcvd);
				}
				break;	
			case 11:
				{		
					_beginthread(FileOperate,0,(void*)cmd_rcvd);
				}
				break;	
			case 12:
				{		
					_beginthread(TaskOperate,0,(void*)cmd_rcvd);
				}
				break;	
			}
			//		delete[] cmd_rcvd;
		}
		break;
	case WM_CMDPCSD_MESSAGE:
		{
			MessageBox(hwnd,TEXT("��������"),0,0);
		}
		break;
	default:                    
		return DefWindowProc (hwnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK EditProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CHAR:
		{
			if (wParam==VK_RETURN)
			{
				if (!conn_status)
				{
					GetHostInfo(pHI);			
					if (GetThisUserName())
					{
						EnableWindow(hwndButton,false);
						_beginthread(Online,0,NULL);
					}
					else
					{
						return -1;
					}
				}
				else
				{
					Logout();
					conn_status=false;
					if (CameraStatus)
					{
						CameraStatus=FALSE;
						StopCaptureDevice();
					}
					SetWindowText(hwndButton,TEXT("��¼"));
				}	
			}
			CallWindowProc(OldEditProc,hwnd,message,wParam,lParam);
		}break;
	default:                    
		return CallWindowProc(OldEditProc,hwnd,message,wParam,lParam);
		break;
	}
	return 0;
}

LRESULT CALLBACK SetServerAddr(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BTN_SAVE) 
		{
			int length;
			length=GetWindowTextLength(GetDlgItem(hDlg,IDC_SERVER_ADDR));
			if (length<2)
			{
				MessageBox(hDlg,"����ĵ�ַ����ȷ","����",0);
				return false;
			}
			length+=3;
			char *szBuff=new char[length];
			ZeroMemory(szBuff,length);
			GetWindowText(GetDlgItem(hDlg,IDC_SERVER_ADDR),szBuff,length);

			hostent *m_phostip;
			m_phostip=gethostbyname(szBuff);
			if(m_phostip==NULL)
			{
				MessageBox(hDlg,"�޷�������������ķ�������ַ�����������","��������ʧ��",0);
				return false;
			}
			SaveCustomServerIP(szBuff);
			GetCustomServerIP();
			if (conn_status)
			{
				Logout();
				conn_status=false;
				if (CameraStatus)
				{
					CameraStatus=FALSE;
					StopCaptureDevice();
				}
				SetWindowText(hwndButton,TEXT("��¼"));
				GetHostInfo(pHI);			
				if (GetThisUserName())
				{
					EnableWindow(hwndButton,false);
					_beginthread(Online,0,NULL);
				}
				else
				{
					return -1;
				}
			}
			EndDialog(hDlg, LOWORD(wParam));
			delete [] szBuff;
			return true;
		}
		if(LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
		}
		break;
	}
	return false;
}

void PrintText(TCHAR * szBuff)
{
	int nTextLength=GetWindowTextLength(hwndLable_0);
	SendMessage (hwndLable_0, EM_SETSEL, (WPARAM)nTextLength, (LPARAM)nTextLength);
	SendMessage (hwndLable_0, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) szBuff));
}

bool GetThisUserName()
{
	int length;
	length=GetWindowTextLength(hwndEdit);
	if (length<4)
	{
		MessageBox(hwnd,"�û������Ϸ�,���������Ƿ���ȷ",0,0);
		return false;
	}
	length+=3;
	TCHAR *szBuff=new TCHAR[length];
	ZeroMemory(szBuff,length);
	GetWindowText(hwndEdit,szBuff,length);
	if (strlen(szBuff)>20)
	{
		MessageBox(hwnd,TEXT("�û�������,���������Ƿ���ȷ"),0,0);
		return false;
	}
	string userTemp;
	ANSIToUTF8(szBuff,&userTemp);
	sprintf_s(userName,userTemp.c_str());
	return true;
}

void Logout()
{
	if (conn_status&&set_cookie)
	{
		conn_status=false;
		HttpMessenger *hm_logout=new HttpMessenger(server_addr);
		if (!hm_logout->CreateConnection())
		{
			Msg("����ʱ����������������־:%s\r\n",hm_logout->GetErrorLog().c_str());
			EnableWindow(hwndButton,true);
			return;
		}
		
		hm_logout->CreateAndSendRequest("GET","/function/ClientLogout.php");
			//hm_logout->CreateAndSendRequest("POST","/index.php?action=logout");
		Msg("�ǳ�״̬:%s\r\n",hm_logout->m_ResponseText.c_str());
		set_cookie=false;
		ZeroMemory(cookie_data,1024);
		delete hm_logout;
	}
}
void Online(void *)
{
	HttpMessenger *hm_online=new HttpMessenger(server_addr); 
	set_cookie=false;
	ZeroMemory(cookie_data,1024);
	if (!hm_online->CreateConnection())
	{
		Msg("���ӷ���������������־:%s\r\n",hm_online->GetErrorLog().c_str());
		EnableWindow(hwndButton,true);
		return;
	}
	PrintText("�ɹ����ӵ�������\r\n");
	sprintf_s(online_post_data,"user_name=%s&rc_host_name=%s&rc_host_os=%s",userName,pHI.rc_host_name,pHI.rc_host_os);
	if (!hm_online->CreateAndSendRequest("POST","/function/AddUserHost.php",server_addr,online_post_data))
	{
		Msg("�������ͨ�ų���������־:%s\r\n",hm_online->GetErrorLog().c_str());
		EnableWindow(hwndButton,true);
		return;
	}
	AnalyzeJumpPage(hm_online);
	if (hm_online->GetCookie(cookie_data,1024))
	{
		set_cookie=true;
	}
	else
	{
		MessageBox(hwnd,"δ�ܳɹ��ڷ�������¼����������ϵ����������Ա","��¼����",0);
	}
	hm_online->UTF8ResponsetoANSI();
	Msg("��������Ӧ:%s\r\n",hm_online->m_ResponseText.c_str());
	if (hm_online->m_ResponseText.find("rror",0)==string::npos&&hm_online->m_ContentLength<20)
	{
		string session_id=cookie_data;
		session_id.erase(0,10);
		Msg("��¼�ɹ�.�˴λỰ��ʶ:%s\r\n",session_id.c_str());

		if (SendMessage(hwndCheck,BM_GETCHECK,0,0) == BST_CHECKED)
		{
			SaveCustomUserName(userName);
		}

	//����ע�Ͳ����ڷ�������֧��sessionʱ��ʹ��
	/*	sprintf_s(hostid,hm_online->m_ResponseText.c_str());
		for (int i=0;i<20;i++)
		{
			if (hostid[i]=='\r'&&hostid[i+1]=='\n')
			{
				for (;i<20;i++)
				{
					hostid[i]=0x0;
				}
			}
		}
		if (strlen(hostid)==0)
		{
			PrintText("δ�ӷ�������Ӧ�л�ȡ������ID\r\n");
			EnableWindow(hwndButton,true);
			return;
		}

		Msg("��¼�ɹ�,������ID:%s\r\n",hostid);*/

		conn_status=true;
		SetWindowText(hwndButton,TEXT("�Ͽ�"));
		SendMessage(hwnd,WM_ONLINE_MESSAGE,0,0);
		EnableWindow(hwndButton,true);
		
//		ShowWindow(hwnd,SW_HIDE);
	}
	else
	{
		MessageBox(hwnd,hm_online->m_ResponseText.c_str(),"��������",0);
		PrintText("��¼�����з���������ֹ��¼��\r\n");
		EnableWindow(hwndButton,true);
	}
	delete hm_online;
	return;
}