#include "stdafx.h"
#include "TimePlate.h"
#include "GuardianDlg.h"

void
CTimePlate::OnPaint()
{
	CPaintDC dc(this);
	// ������ ���� �ڵ� start
	CRect rect;
	this->GetClientRect(&rect);
	CDC* pDC = (CDC*)&dc;
	CDC dcMem;
	CBitmap bitmap;
	dcMem.CreateCompatibleDC(pDC);
	bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
	CBitmap* pOldBitmap = dcMem.SelectObject(&bitmap);
	// ������ ���� �ڵ� end
	
	SetBkMode(dcMem.m_hDC, TRANSPARENT);
	dcMem.FillSolidRect(rect.left, rect.top, rect.Width(), rect.Height(), NORMAL_BG_COLOR);


	CTime now = CTime::GetCurrentTime();
	CString hour = now.Format("%I:%M");
	CString ampm = now.Format("%p");

	CString  date = now.Format("%Y�� %m�� %d�� (%a)");

	int letterLeft = m_guardianDlg->m_videoLeft;

	WriteSingleLine(date, &dcMem, m_guardianDlg->m_alarmFont4, RGB(0x90, 0x90, 0x90), 5, 5, rect.Width(), rect.Height(), DT_LEFT, DT_TOP);
	WriteSingleLine(hour, &dcMem, m_guardianDlg->m_normalFont1, RGB(0x00, 0x00, 0x00), 0, 0, rect.Width(), rect.Height(), DT_LEFT, DT_BOTTOM);
	WriteSingleLine(ampm, &dcMem, m_guardianDlg->m_normalFont3, RGB(0x90, 0x90, 0x90), 280, 25, rect.Width(), rect.Height(), DT_LEFT, DT_VCENTER);

	//������ ���� �ڵ� start
	pDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &dcMem, 0, 0, SRCCOPY);
	dcMem.SelectObject(pOldBitmap);
	//������ ���� �ڵ� end
}

