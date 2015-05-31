// Disassemble.cpp : implementation file
//

#include "../stdafx.h"
#include "../resource.h"
#include "Disassemble.h"
#include "../VBA.h"

#include "../../gba/armdis.h"
#include "../../gba/GBA.h"
#include "../../gba/GBAGlobals.h"

extern void CPUUpdateCPSR();

/////////////////////////////////////////////////////////////////////////////
// Disassemble dialog

Disassemble::Disassemble(CWnd*pParent /*=NULL*/)
	: ResizeDlg(Disassemble::IDD, pParent)
{
	//{{AFX_DATA_INIT(Disassemble)
	m_c  = FALSE;
	m_f  = FALSE;
	m_i  = FALSE;
	m_n  = FALSE;
	m_t  = FALSE;
	m_v  = FALSE;
	m_z  = FALSE;
	//}}AFX_DATA_INIT
	autoUpdate      = false;
	address         = 0;
	count           = 1;
	mode            = 0;
	breakPointIndex = 0;
	jumpTraceIndex  = 0;
}

void Disassemble::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Disassemble)
	DDX_Control(pDX, IDC_ADDRESS, m_address);
	DDX_Control(pDX, IDC_DISASSEMBLE, m_list);
	DDX_Control(pDX, IDC_BREAKPOINTS, m_bp_list);
	DDX_Control(pDX, IDC_JUMPTRACE, m_jumptrace_list);
	DDX_Check(pDX, IDC_C, m_c);
	DDX_Check(pDX, IDC_F, m_f);
	DDX_Check(pDX, IDC_I, m_i);
	DDX_Check(pDX, IDC_N, m_n);
	DDX_Check(pDX, IDC_T, m_t);
	DDX_Check(pDX, IDC_V, m_v);
	DDX_Check(pDX, IDC_Z, m_z);
	DDX_Radio(pDX, IDC_AUTOMATIC, mode);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Disassemble, CDialog)
//{{AFX_MSG_MAP(Disassemble)
ON_BN_CLICKED(IDC_AUTO_UPDATE, OnAutoUpdate)
ON_BN_CLICKED(IDC_AUTOMATIC, OnAutomatic)
ON_BN_CLICKED(IDC_ARM, OnArm)
ON_BN_CLICKED(IDC_CLOSE, OnClose)
ON_BN_CLICKED(IDC_GO, OnGo)
ON_BN_CLICKED(IDC_GOPC, OnGopc)
ON_BN_CLICKED(IDC_NEXT, OnNext)
ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
ON_BN_CLICKED(IDC_THUMB, OnThumb)
ON_WM_VSCROLL()
//}}AFX_MSG_MAP
ON_BN_CLICKED(IDC_NEXT2, &Disassemble::OnNextFrame)
ON_BN_CLICKED(IDC_NEXT3, &Disassemble::OnContinue)
ON_WM_CONTEXTMENU()
ON_LBN_SELCHANGE(IDC_BREAKPOINTS, &Disassemble::OnLbnSelchangeBreakpoints)
ON_LBN_SELCHANGE(IDC_JUMPTRACE, &Disassemble::OnLbnSelchangeJumptrace)
ON_BN_CLICKED(IDC_COPY_JUMPTRACE, &Disassemble::OnBnClickedCopyJumptrace)
ON_BN_CLICKED(IDC_COPY_REGISTERS, &Disassemble::OnBnClickedCopyRegisters)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Disassemble message handlers

void Disassemble::OnAutoUpdate()
{
	autoUpdate = !autoUpdate;
	if (autoUpdate)
	{
		theApp.winAddUpdateListener(this);
	}
	else
	{
		theApp.winRemoveUpdateListener(this);
	}
}

void Disassemble::OnAutomatic()
{
	mode = 0;
	refresh();
	refreshBreakpoints();
}

void Disassemble::OnArm()
{
	mode = 1;
	refresh();
	refreshBreakpoints();
}

void Disassemble::OnClose()
{
	theApp.winRemoveUpdateListener(this);

	DestroyWindow();
}

void Disassemble::OnGo()
{
	CString buffer;
	m_address.GetWindowText(buffer);
	sscanf(buffer, "%x", &address);
	refresh();
}

void Disassemble::OnGopc()
{
	if (armState)
		address = armNextPC - 16;
	else
		address = armNextPC - 8;

	refresh();
}

void Disassemble::OnNext()
{
	//CPULoop(1);
	CPUExecuteOpcodes(1, 1);

	refreshPosition();
}

void Disassemble::OnRefresh()
{
	refresh();
}

void Disassemble::OnThumb()
{
	mode = 2;
	refresh();
	refreshBreakpoints();
}

void Disassemble::initScrollInfo(int res, int nMin, int nMax, int nPos)
{
	SCROLLINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	si.nMin = nMin;
	si.nMax = nMax;
	si.nPos = nPos;
	si.nPage = 10;
	GetDlgItem(res)->SetScrollInfo(SB_CTL, &si, TRUE);
}

BOOL Disassemble::OnInitDialog()
{
	CDialog::OnInitDialog();

	DIALOG_SIZER_START(sz)
	DIALOG_SIZER_ENTRY(IDC_DISASSEMBLE, DS_SizeY)
	DIALOG_SIZER_ENTRY(IDC_BREAKPOINTS, DS_SizeY | DS_SizeX)
	DIALOG_SIZER_ENTRY(IDC_BREAKPOINTS, )
	DIALOG_SIZER_ENTRY(IDC_JUMPTRACE, DS_SizeX)
	DIALOG_SIZER_ENTRY(IDC_REFRESH, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_CLOSE, DS_MoveY | DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_NEXT,  DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_NEXT2, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_NEXT3, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_AUTO_UPDATE, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_GOPC, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_VSCROLL, DS_SizeY)
	DIALOG_SIZER_ENTRY(IDC_VSCROLL2, DS_SizeY | DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_VSCROLL3, DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_COPY_JUMPTRACE, DS_MoveX)
	DIALOG_SIZER_END()
	SetData(sz,
	        TRUE,
	        HKEY_CURRENT_USER,
	        "Software\\Emulators\\VisualBoyAdvance\\Viewer\\DisassembleView",
	        NULL);

	CFont *font = CFont::FromHandle((HFONT)GetStockObject(SYSTEM_FIXED_FONT));
	m_list.SetFont(font, FALSE);
	m_bp_list.SetFont(font, FALSE);
	m_jumptrace_list.SetFont(font, FALSE);

	for (int i = 0; i < 17; i++)
		GetDlgItem(IDC_R0+i)->SetFont(font, FALSE);

	GetDlgItem(IDC_MODE)->SetFont(font, FALSE);

	m_address.LimitText(8);
	refresh();
	refreshBreakpoints();

	initScrollInfo(IDC_VSCROLL, 0, 100, 50);
	initScrollInfo(IDC_VSCROLL2, 0, nbBreakPoints, 0);
	initScrollInfo(IDC_VSCROLL3, 0, MAX_JUMPTRACE, 0);

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

bool Disassemble::isArm()
{
	return (mode == 1 || (mode == 0 && armState));
}

void Disassemble::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*pScrollBar)
{
	u32 inc = isArm() ? 4 : 2;

	// ScrollBar of the breakpoints list
	if (pScrollBar == GetDlgItem(IDC_VSCROLL2))
	{
		int  h = m_bp_list.GetItemHeight(0);
		RECT r;
		m_bp_list.GetClientRect(&r);
		count = min((r.bottom - r.top + 1) / h, (int)(nbBreakPoints - breakPointIndex));

		if ((nSBCode == SB_THUMBPOSITION || nSBCode == SB_THUMBTRACK))
		{
			breakPointIndex = nPos;
		}
		else if (nSBCode == SB_LINEDOWN && breakPointIndex + count < (int)nbBreakPoints)
		{
			++breakPointIndex;
		}
		else if (nSBCode == SB_LINEUP && breakPointIndex > 0)
		{
			--breakPointIndex;
		}

		refreshBreakpoints();
		initScrollInfo(IDC_VSCROLL2, 0, nbBreakPoints, breakPointIndex);
	} else if (pScrollBar == GetDlgItem(IDC_VSCROLL3))
	{
		// Scrollbar of the jumptrace
		if ((nSBCode == SB_THUMBPOSITION || nSBCode == SB_THUMBTRACK))
		{
			jumpTraceIndex = nPos;
		}
		else if (nSBCode == SB_LINEDOWN && jumpTraceIndex < m_jumptrace_list.GetCount())
		{
			++jumpTraceIndex;
		}
		else if (nSBCode == SB_LINEUP && jumpTraceIndex > 0)
		{
			--jumpTraceIndex;
		}

		refreshJumpTrace();
		initScrollInfo(IDC_VSCROLL3, 0, MAX_JUMPTRACE, jumpTraceIndex);
	}
	else 
	{
		// Scrollbar of the disassembler
		switch (nSBCode)
		{
		case SB_LINEDOWN:
			address += inc;
			break;
		case SB_LINEUP:
			address -= inc;
			break;
		case SB_PAGEDOWN:
			address += count * inc;
			break;
		case SB_PAGEUP:
			address -= count * inc;
			break;
		}
		refresh();
	}


	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void Disassemble::refreshBreakpoints()
{
	if (rom == NULL)
		return;

	int  h = m_bp_list.GetItemHeight(0);
	RECT r;
	m_bp_list.GetClientRect(&r);
	count = min((r.bottom - r.top + 1) / h, (int) (nbBreakPoints - breakPointIndex));

	m_bp_list.ResetContent();
	if (!systemIsEmulating() && systemCartridgeType == 0)
		return;

	char buffer[82];
	u32  addr = address;
	for (int i = 0; i < count; i++)
	{
		u32 itemAddr = breakPoints[i + breakPointIndex];

		if (isArm())
		{
			disArm(itemAddr, buffer, 3);
		}
		else
		{
			disThumb(itemAddr, buffer, 3);
		}
		int pos = m_bp_list.InsertString(-1, buffer);
		// Associate each item with his address
		m_bp_list.SetItemData(pos, (DWORD_PTR)itemAddr);
	}

	initScrollInfo(IDC_VSCROLL2, 0, nbBreakPoints, breakPointIndex);
}

void Disassemble::refreshJumpTrace()
{
	if (rom == NULL)
		return;

	int  h = m_jumptrace_list.GetItemHeight(0);
	RECT r;
	m_jumptrace_list.GetClientRect(&r);
	count = min((r.bottom - r.top + 1) / h, MAX_JUMPTRACE);

	m_jumptrace_list.ResetContent();
	if (!systemIsEmulating() && systemCartridgeType == 0)
		return;

	char buffer[102];
	u32  addr = address;
	for (int i = 1; i < count+1; ++i)
	{
		int idx = jumpTraceIdx - i - jumpTraceIndex;
		if (idx < 0)
		{
			idx += MAX_JUMPTRACE;
		}
		u32 itemAddr = jumpTrace[idx].addr; // (i + jumpTraceIdx) % MAX_JUMPTRACE
		if (itemAddr == 0)
		{
			continue;
		}

		if (jumpTrace[idx].armState)
		{
			disArm(itemAddr, buffer, 3);
		}
		else
		{
			disThumb(itemAddr, buffer, 3);
		}

		// Add the jump address
		int len = strlen(buffer);
		buffer[len] = buffer[len + 3] = ' ';
		buffer[len + 1] = '-';
		buffer[len + 2] = '>';

		addHex(&buffer[len + 4], 32, jumpTrace[idx].jmpAddr);
		buffer[len + 12] = 0;

		int pos = m_jumptrace_list.InsertString(-1, buffer);
		// Associate each item with his address
		m_jumptrace_list.SetItemData(pos, (DWORD_PTR)itemAddr);
	}

	initScrollInfo(IDC_VSCROLL2, 0, count, jumpTraceIdx);
}

void Disassemble::refresh(bool refreshSel)
{
	if (rom == NULL)
		return;

	bool8 arm = armState;

	if (mode != 0)
	{
		if (mode == 1)
			arm = true;
		else
			arm = false;
	}

	int  h = m_list.GetItemHeight(0);
	RECT r;
	m_list.GetClientRect(&r);
	count = (r.bottom - r.top+1)/h;

	m_list.ResetContent();
	if (!systemIsEmulating() && systemCartridgeType == 0)
		return;

	char buffer[82];
	u32  addr = address;
	int  i;
	int  sel = -1;
	for (i = 0; i < count; i++)
	{
		u32 itemAddr = addr;
		buffer[0] = buffer[1] =  ' ';

		// Show a '*' when there's a break point
		if (breakPointExist(addr))
		{
			buffer[0] = '*';
		}

		if (refreshSel && addr == armNextPC)
			sel = i;
		if (arm)
		{
			addr += disArm(addr, &buffer[2], 3);
		}
		else
		{
			addr += disThumb(addr, &buffer[2], 3);
		}
		int pos = m_list.InsertString(-1, buffer);
		// Associate each item with his address
		m_list.SetItemData(pos, (DWORD_PTR)itemAddr);
	}

	if (refreshSel && sel != -1)
		m_list.SetCurSel(sel);

	CPUUpdateCPSR();

	for (i = 0; i < 17; i++)
	{
		sprintf(buffer, "%08x", reg[i].I);
		GetDlgItem(IDC_R0+i)->SetWindowText(buffer);
	}

	m_n = (reg[16].I & 0x80000000) != 0;
	m_z = (reg[16].I & 0x40000000) != 0;
	m_c = (reg[16].I & 0x20000000) != 0;
	m_v = (reg[16].I & 0x10000000) != 0;
	m_i = (reg[16].I & 0x80) != 0;
	m_f = (reg[16].I & 0x40) != 0;
	m_t = (reg[16].I & 0x20) != 0;

	UpdateData(FALSE);

	int v = reg[16].I & 0x1f;
	sprintf(buffer, "%02x", v);
	GetDlgItem(IDC_MODE)->SetWindowText(buffer);

	// Disable/Enable the continue button
	updateContinueButton();

	refreshJumpTrace();
}

void Disassemble::update()
{
	OnGopc();
	refresh();
}

void Disassemble::PostNcDestroy()
{
	delete this;
}

void Disassemble::refreshPosition()
{

	if (armState)
	{
		u32 total = address + count * 4;
		if (!(armNextPC >= address && armNextPC < total))
		{
			OnGopc();
		}
	}
	else
	{
		u32 total = address + count * 2;
		if (!(armNextPC >= address && armNextPC < total))
		{
			OnGopc();
		}
	}
	refresh();
}


void Disassemble::OnNextFrame()
{
	CPULoop(1);
	refreshPosition();
}

void Disassemble::setClipboardText(char* text)
{
	OpenClipboard();
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, strlen(text) + 1);
	if (!hg){
		CloseClipboard();
		return;
	}
	memcpy(GlobalLock(hg), text, strlen(text) + 1);
	GlobalUnlock(hg);
	SetClipboardData(CF_TEXT, hg);
	CloseClipboard();
	GlobalFree(hg);
}

BOOL Disassemble::PreTranslateMessage(MSG* pMsg)
{
	// TODO : Use of accelerators
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_F5)
		{
			toggleBpAtSelection();
		}
		else if (pMsg->wParam == VK_F6)
		{
			OnNext();
		}
		else if (pMsg->wParam == VK_F7)
		{ 
			OnNextFrame();
		}
		else if (pMsg->wParam == VK_F8)
		{
			OnContinue();
		}
		else if (pMsg->wParam == 'C')
		{
			if (HIWORD(GetKeyState(VK_CONTROL)))
			{
				copySelectedLine();
			}
		}
	}
	return __super::PreTranslateMessage(pMsg);
}

void Disassemble::updateContinueButton()
{
	BOOL enable = (theApp.paused || !theApp.active) ? TRUE : FALSE;

	CWnd *continueButton = GetDlgItem(IDC_NEXT3);

	if (continueButton) {
		continueButton->EnableWindow(enable);
	}
}


void Disassemble::OnContinue()
{
	if (hasHitBP)
	{
		// If we're curently on a break point, we need to step over the instruction
		CPUExecuteOpcodes(1, 1);
	}
	hasHitBP = false;
	theApp.active = true;
	theApp.paused = false;
	updateContinueButton();
}

void Disassemble::toggleBpAtSelection()
{
	int idx = m_list.GetCurSel();
	if (idx != LB_ERR)
	{
		u32 addr = m_list.GetItemData(idx);
		if (addr >= 0)
		{
			// If there's already a breakpoint a this address, we remove it
			if (!removeBreakPoint(addr))
			{
				// If removing the breakpoint failed, then that's probably because
				// there's no breakpoint, so we can add one
				addBreakPoint(addr);
			}

			// Refresh (to update the breakpoint symbol)
			refresh(false);
			// Refresh the breakpoint list
			refreshBreakpoints();

			// To keep the selection
			m_list.SetCurSel(idx);
		}
	}
}

void Disassemble::copySelectedAddress()
{
	int idx = m_list.GetCurSel();
	if (idx != LB_ERR)
	{
		u32 addr = m_list.GetItemData(idx);
		if (addr >= 0)
		{
			char buffer[9];
			addHex(buffer, 32, addr);
			buffer[8] = 0;
			setClipboardText(buffer);
		}
	}
}

void Disassemble::copySelectedLine()
{
	int idx = m_list.GetCurSel();
	if (idx != LB_ERR)
	{
		CString str;
		int n = m_list.GetTextLen(idx);
		m_list.GetText(idx, str.GetBuffer(n));
		str.ReleaseBuffer();
		setClipboardText(&(str.GetBuffer(0))[2]);
	}
}


void Disassemble::OnContextMenu(CWnd* pWnd, CPoint point)
{

	if (&m_list == pWnd)
	{
		CMenu menu;
		CMenu* subMenu;

		menu.LoadMenu(IDR_DISASSEMBLE_MENU);
		subMenu = menu.GetSubMenu(0);

		int retVal = subMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, this);

		// Toggle Break Point
		if (retVal == ID_TOGGLEBREAKPOINT)
		{
			toggleBpAtSelection();
		}
		else if (retVal == ID_COPYADDRESS)
		{
			copySelectedAddress();
		}
		else if (retVal == ID_COPYLINE) 
		{
			copySelectedLine();
		}
	}
}


void Disassemble::OnLbnSelchangeBreakpoints()
{
	// When a breakpoint is selected
	int idx = m_bp_list.GetCurSel();
	if (idx != LB_ERR)
	{
		u32 addr = m_bp_list.GetItemData(idx);
		if (addr >= 0)
		{
			// We go to the selected bp
			address = addr;
			refresh();
		}
	}
}


void Disassemble::OnLbnSelchangeJumptrace()
{
	// When a line in the jumptrace is selected
	int idx = m_jumptrace_list.GetCurSel();
	if (idx != LB_ERR)
	{
		u32 addr = m_jumptrace_list.GetItemData(idx);
		if (addr >= 0)
		{
			// We go to the selected bp
			address = addr;
			refresh();
		}
	}
}


void Disassemble::OnBnClickedCopyJumptrace()
{
	// Copy all the jumptrace into the clipboard
	char line[82];
	CString result;
	for (int i = 1; i < MAX_JUMPTRACE + 1; ++i)
	{
		int idx = jumpTraceIdx - i;
		if (idx < 0)
		{
			idx += MAX_JUMPTRACE;
		}
		u32 itemAddr = jumpTrace[idx].addr;
		if (itemAddr == 0)
		{
			continue;
		}

		if (jumpTrace[idx].armState)
		{
			disArm(itemAddr, line, 3);
		}
		else
		{
			disThumb(itemAddr, line, 3);
		}

		// Add the jump address
		int len = strlen(line);
		line[len] = line[len + 3] = ' ';
		line[len + 1] = '-';
		line[len + 2] = '>';

		addHex(&line[len + 4], 32, jumpTrace[idx].jmpAddr);
		line[len + 12] = 0;

		result.Append(line);
		result.AppendChar('\r');
		result.AppendChar('\n');
	}
	result.ReleaseBuffer();
	setClipboardText(result.GetBuffer(0));
}


void Disassemble::OnBnClickedCopyRegisters()
{
	char buffer[17 * 20];
	int pos = 0;
	for (int i = 0; i < 17; i++)
	{
		pos += sprintf(&buffer[pos], "R%d:\t%08x\r\n", i, reg[i].I);
	}

	buffer[pos] = 0;

	setClipboardText(buffer);

}
