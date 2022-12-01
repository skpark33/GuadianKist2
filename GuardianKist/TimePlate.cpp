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
	dcMem.FillSolidRect(rect.left, rect.top, rect.Width(), rect.Height(), RGB(0xFB,0xFB,0xFC));


	CTime now = CTime::GetCurrentTime();
	CString hour = now.Format("%I:%M");
	CString ampm = now.Format("%p");

	CString weekday = now.Format("%a");
	if (weekday == "Mon") {
		weekday = " (��)";
	} else if (weekday == "Tue") {
		weekday = " (ȭ)";
	} else if (weekday == "Wed") {
		weekday = " (��)";
	} else if (weekday == "Thu") {
		weekday = " (��)";
	} else if (weekday == "Fri") {
		weekday = " (��)";
	} else if (weekday == "Sat") {
		weekday = " (��)";
	} else if (weekday == "Sun") {
		weekday = " (��)";
	}

	CString  date = now.Format("%Y�� %m�� %d��");
	date += weekday;

	int letterLeft = m_guardianDlg->m_videoLeft;

	WriteSingleLine(hour, &dcMem, m_guardianDlg->m_normalFont1, RGB(0x11, 0x11, 0x11), 0, 0, rect.Width(), rect.Height(), DT_LEFT, DT_TOP);
	WriteSingleLine(ampm, &dcMem, m_guardianDlg->m_alarmFont3, RGB(0xB5, 0xBB, 0xC7), 250, 0, rect.Width(), rect.Height(), DT_LEFT, DT_VCENTER);
	WriteSingleLine(date, &dcMem, m_guardianDlg->m_alarmFont4, RGB(0x66, 0x66, 0x66), 6, 0, rect.Width(), rect.Height(), DT_LEFT, DT_BOTTOM);

	//������ ���� �ڵ� start
	pDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &dcMem, 0, 0, SRCCOPY);
	dcMem.SelectObject(pOldBitmap);
	//������ ���� �ڵ� end
}

