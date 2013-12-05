//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <vector>
#include "OffscreenPaint.h"
#include "MainFrm.h"
#include "LogFile.h"
#include "FilterDlg.h"
#include "DisplayInfo.h"
#include "ProcessInfo.h"

namespace gj {

typedef CWinTraitsOR<LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS> CListViewTraits;

class CMainFrame;

struct SelectionInfo
{
	SelectionInfo();
	SelectionInfo(int beginLine, int endLine, int count);

	int beginLine;
	int endLine;
	int count;
};

struct LogLine
{
	LogLine(int line, COLORREF color);

	int line;
	COLORREF color;
};

class CLogView :
	public CWindowImpl<CLogView, CListViewCtrl, CListViewTraits>,
	public COffscreenPaint<CLogView>
{
public:
	CLogView(CMainFrame& mainFrame, LogFile& logFile);

	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();
	BOOL PreTranslateMessage(MSG* pMsg);

	void DoPaint(CDCHandle dc, const RECT& rcClip);

	void Clear();
	void Add(int line, const Message& msg);
	void BeginUpdate();
	void EndUpdate();

	void ScrollToIndex(int index, bool center);
	void ScrollDown();
	bool IsLastLineSelected();

	bool GetClockTime() const;
	void SetClockTime(bool clockTime);
	void SelectAll();
	void Copy();

	bool FindNext(const std::wstring& text);
	bool FindPrevious(const std::wstring& text);

	std::vector<LogFilter> GetFilters() const;
	void SetFilters(std::vector<LogFilter> filters);

	using CListViewCtrl::GetItemText;
	std::string GetItemText(int item, int subItem) const;

	void LoadSettings(CRegKey& reg);
	void SaveSettings(CRegKey& reg);

	SelectionInfo GetSelectedRange() const;

private:
	LRESULT OnCreate(const CREATESTRUCT* pCreate);
	LRESULT OnGetDispInfo(LPNMHDR pnmh);
	LRESULT OnClick(LPNMHDR pnmh);
	LRESULT OnCustomDraw(LPNMHDR pnmh);
	LRESULT OnOdStateChanged(LPNMHDR pnmh);

	bool Find(const std::string& text, int direction);
	void ApplyFilters();
	bool IsIncluded(const std::string& text) const;
	COLORREF GetColor(const std::string& text) const;

	CMainFrame& m_mainFrame;
	LogFile& m_logFile;
	std::vector<LogFilter> m_filters;
	std::vector<LogLine> m_logLines;
	bool m_clockTime;
	bool m_autoScrollDown;
	DisplayInfo m_displayInfo;
	ProcessInfo m_processInfo;
	bool m_dirty;
};

} // namespace gj