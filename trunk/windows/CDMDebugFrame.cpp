#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "CDMDebugFrame.h"
#include "WinUtil.h"
#include "../client/File.h"

bool CDMDebugFrame::pause = false;

#define MAX_TEXT_LEN 32768

LRESULT CDMDebugFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL, WS_EX_CLIENTEDGE);
	
	ctrlPad.LimitText(0);
	ctrlPad.SetFont(WinUtil::font);
	
	bHandled = FALSE;
	return 1;
}

LRESULT CDMDebugFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	
	MDIDestroy(m_hWnd);
	return 0;
	
}

void CDMDebugFrame::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */)
{
	CRect rc;

	GetClientRect(rc);
	
	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;
	ctrlPad.MoveWindow(rc);
	
}

void CDMDebugFrame::addLine(const string& s) {
	if (pause)
		return;
	if (frame != NULL) {
		frame->ctrlPad.SetRedraw(FALSE);
		frame->ctrlPad.AppendText((s + "\r\n").c_str());
		if (frame->ctrlPad.GetWindowTextLength() > MAX_TEXT_LEN) {
			int i = frame->ctrlPad.GetWindowTextLength();
			AutoArray<char> buf(i + 1);
			frame->ctrlPad.GetWindowText(buf, i + 1);
			i -= MAX_TEXT_LEN;
			string s(buf + i + (MAX_TEXT_LEN / 8));
			frame->ctrlPad.SetWindowText(s.c_str());
		}
		frame->ctrlPad.SetRedraw(TRUE);
	}
}
void CDMDebugFrame::setPause(bool bPause) {
	pause = bPause;
}
