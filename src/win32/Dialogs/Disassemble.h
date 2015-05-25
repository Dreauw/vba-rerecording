#if !defined(AFX_DISASSEMBLE_H__CA10E857_7D76_4B19_A62B_D0677040FD0F__INCLUDED_)
#define AFX_DISASSEMBLE_H__CA10E857_7D76_4B19_A62B_D0677040FD0F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Disassemble.h : header file
//

#include "../IUpdate.h"
#include "ResizeDlg.h"

/////////////////////////////////////////////////////////////////////////////
// Disassemble dialog

class Disassemble : public ResizeDlg, IUpdateListener
{
	// Construction
public:
	virtual void update();
	void refresh();
	Disassemble(CWnd*pParent = NULL);  // standard constructor

	// Dialog Data
	//{{AFX_DATA(Disassemble)
	enum { IDD = IDD_DISASSEMBLE };
	CEdit    m_address;
	CListBox m_list;
	BOOL     m_c;
	BOOL     m_f;
	BOOL     m_i;
	BOOL     m_n;
	BOOL     m_t;
	BOOL     m_v;
	BOOL     m_z;
	//}}AFX_DATA
	bool autoUpdate;
	int  count;
	u32  address;
	int  mode;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Disassemble)
protected:
	virtual void DoDataExchange(CDataExchange*pDX);   // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(Disassemble)
	afx_msg void OnAutoUpdate();
	afx_msg void OnAutomatic();
	afx_msg void OnArm();
	afx_msg void OnClose();
	afx_msg void OnGo();
	afx_msg void OnNext();
	afx_msg void OnRefresh();
	afx_msg void OnThumb();
	virtual BOOL OnInitDialog();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnGopc();
	afx_msg void OnNextFrame();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
private:
	void refreshPosition();
	void updateContinueButton();

public:
	afx_msg void OnContinue();
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISASSEMBLE_H__CA10E857_7D76_4B19_A62B_D0677040FD0F__INCLUDED_)
