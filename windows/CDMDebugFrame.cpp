#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "CDMDebugFrame.h"
#include "WinUtil.h"
#include "../client/File.h"

bool CDMDebugFrame::pause = false;

#define MAX_TEXT_LEN 131072

LRESULT CDMDebugFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlPad.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE);
	
	ctrlPad.LimitText(0);
	ctrlPad.SetFont(WinUtil::font);
	
	bHandled = FALSE;
	return 1;
}

LRESULT CDMDebugFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	
	bHandled = FALSE;
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
		if (frame->ctrlPad.GetWindowTextLength() > MAX_TEXT_LEN) {
			frame->ctrlPad.SetRedraw(FALSE);
			frame->ctrlPad.SetSel(0, frame->ctrlPad.LineIndex(frame->ctrlPad.LineFromChar(2000)), TRUE);
			frame->ctrlPad.ReplaceSel(_T(""));
			frame->ctrlPad.SetRedraw(TRUE);
		}
		BOOL noscroll = TRUE;
		POINT p = frame->ctrlPad.PosFromChar(frame->ctrlPad.GetWindowTextLength() - 1);
		CRect r;
		frame->ctrlPad.GetClientRect(r);
		
		if( r.PtInRect(p) || frame->MDIGetActive() != frame->m_hWnd)
			noscroll = FALSE;
		else {
			frame->ctrlPad.SetRedraw(FALSE); // Strange!! This disables the scrolling...????
		}
		frame->ctrlPad.AppendText(Text::toT(s + "\r\n").c_str());
		if(noscroll) {
		frame->ctrlPad.SetRedraw(TRUE);
		}
		frame->setDirty();
	}
}
void CDMDebugFrame::setPause(bool bPause) {
	pause = bPause;
}
