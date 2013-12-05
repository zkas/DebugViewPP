//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#include "stdafx.h"
#include <iomanip>
#include <array>
#include <regex>
#include "dbgstream.h"
#include "Utilities.h"
#include "Resource.h"
#include "LogView.h"

namespace gj {

SelectionInfo::SelectionInfo() :
	beginLine(0), endLine(0), count(0)
{
}

SelectionInfo::SelectionInfo(int beginLine, int endLine, int count) :
	beginLine(beginLine), endLine(endLine), count(count)
{
}

LogLine::LogLine(int line, COLORREF color) :
	line(line), color(color)
{
}

BEGIN_MSG_MAP_TRY(CLogView)
	MSG_WM_CREATE(OnCreate)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CLICK, OnClick)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CUSTOMDRAW, OnCustomDraw)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_GETDISPINFO, OnGetDispInfo)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODSTATECHANGED, OnOdStateChanged)
	DEFAULT_REFLECTION_HANDLER()
	CHAIN_MSG_MAP(COffscreenPaint<CLogView>)
END_MSG_MAP_CATCH(ExceptionHandler)

CLogView::CLogView(CMainFrame& mainFrame, LogFile& logFile) :
	m_mainFrame(mainFrame),
	m_logFile(logFile),
	m_clockTime(false),
	m_dirty(false)
{
}

void CLogView::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CLogView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

LRESULT CLogView::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	DefWindowProc();

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(0, L"Line", LVCFMT_RIGHT, 60, 0);
	InsertColumn(1, L"Time", LVCFMT_RIGHT, 90, 0);
	InsertColumn(2, L"PID", LVCFMT_RIGHT, 60, 0);
	InsertColumn(3, L"Process", LVCFMT_LEFT, 140, 0);
	InsertColumn(4, L"Log", LVCFMT_LEFT, 600, 0);

	ApplyFilters();
	return 0;
}

LRESULT CLogView::OnClick(NMHDR* pnmh)
{
	return 0;
}

LRESULT CLogView::OnCustomDraw(NMHDR* pnmh)
{
	auto pCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);

	int item = pCustomDraw->nmcd.dwItemSpec;
	switch (pCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		pCustomDraw->clrTextBk = m_logLines[item].color;
		return CDRF_DODEFAULT;
	}

	return CDRF_DODEFAULT;
}

void CopyItemText(const std::string& s, wchar_t* buf, size_t maxLen)
{
	for (auto it = s.begin(); maxLen > 1 && it != s.end(); ++it, ++buf, --maxLen)
		*buf = *it;
	*buf = '\0';
}

void CopyItemText(const std::wstring& s, wchar_t* buf, size_t maxLen)
{
	for (auto it = s.begin(); maxLen > 1 && it != s.end(); ++it, ++buf, --maxLen)
		*buf = *it;
	*buf = '\0';
}

std::string GetTimeText(double time)
{
	return stringbuilder() << std::fixed << std::setprecision(6) << time;
}

std::string GetTimeText(TickType abstime)
{
	std::string timeString = AccurateTime::GetLocalTimeString(abstime, "%H:%M:%S.%f");
	timeString.erase(timeString.end() - 3, timeString.end());
	return timeString;
}

LRESULT CLogView::OnGetDispInfo(NMHDR* pnmh)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);
	LVITEM& item = pDispInfo->item;
	if ((item.mask & LVIF_TEXT) == 0 || item.iItem >= static_cast<int>(m_logLines.size()))
		return 0;

	int line = m_logLines[item.iItem].line;
	const Message& msg = m_logFile[line];
	std::string timeString = m_clockTime ? GetTimeText(msg.rtctime) : GetTimeText(msg.time);

	switch (item.iSubItem)
	{
	case 0: CopyItemText(std::to_string(line + 1ULL), item.pszText, item.cchTextMax); break;
	case 1: CopyItemText(timeString, item.pszText, item.cchTextMax); break;
	case 2: CopyItemText(std::to_string(msg.processId + 0ULL), item.pszText, item.cchTextMax); break;
	case 3: CopyItemText(m_displayInfo.GetProcessName(msg.processId), item.pszText, item.cchTextMax); break;
	case 4: CopyItemText(msg.text, item.pszText, item.cchTextMax); break;
	}
	return 0;
}

SelectionInfo CLogView::GetSelectedRange() const
{
	int first = GetNextItem(-1, LVNI_SELECTED);
	if (first < 0)
		return SelectionInfo();

	int item = first;
	int last = item;
	do
	{
		last = item + 1;
		item = GetNextItem(item, LVNI_SELECTED);
	} while (item >= 0);

	return SelectionInfo(m_logLines[first].line, m_logLines[last].line, last - first);
}

LRESULT CLogView::OnOdStateChanged(NMHDR* pnmh)
{
	auto pStateChange = reinterpret_cast<NMLVODSTATECHANGE*>(pnmh);

	m_mainFrame.SetLineRange(GetSelectedRange());

	return 0;
}

void CLogView::DoPaint(CDCHandle dc, const RECT& rcClip)
{
	dc.FillSolidRect(&rcClip, GetSysColor(COLOR_WINDOW));
 
	DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
}

void CLogView::Clear()
{
	SetItemCount(0);
	m_dirty = false;
	m_logLines.clear();
}

void CLogView::Add(int line, const Message& msg)
{
	if (!IsIncluded(msg.text))
		return;

	m_dirty = true;
	m_logLines.push_back(LogLine(line, GetColor(msg.text)));
}

void CLogView::BeginUpdate()
{
	int focus = GetNextItem(0, LVNI_FOCUSED);
	m_autoScrollDown = focus < 0 || focus == GetItemCount() - 1;
}

void CLogView::EndUpdate()
{
	if (m_dirty)
	{
		SetItemCountEx(m_logLines.size(), LVSICF_NOSCROLL);
		if (m_autoScrollDown)
		{
			ScrollDown();
		}
		m_dirty = false;
	}
}

void CLogView::ScrollToIndex(int index, bool center)
{
	if (index < 0 || index >= static_cast<int>(m_logLines.size()))
		return;
	
	//todo: deselect any seletected items.
	
	EnsureVisible(index, false);
	SetItemState(index, LVIS_FOCUSED, LVIS_FOCUSED);

	if (center)
	{
		int maxExtraItems = GetCountPerPage() / 2;
		int maxBottomIndex = std::min<int>(m_logLines.size() - 1, index + maxExtraItems);
		EnsureVisible(maxBottomIndex, false);
	}
	//todo: make sure the listview control has focus
}

void CLogView::ScrollDown()
{
	ScrollToIndex(m_logLines.size() - 1, false);
}

bool CLogView::GetClockTime() const
{
	return m_clockTime;
}

void CLogView::SetClockTime(bool clockTime)
{
	m_clockTime = clockTime;
	Invalidate(false);
}

void CLogView::SelectAll()
{
	int lines = GetItemCount();
	for (int i = 0; i < lines; ++i)
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
}

std::string CLogView::GetItemText(int item, int subItem) const
{
	CComBSTR bstr;
	GetItemText(item, subItem, bstr.m_str);
	return std::string(bstr.m_str, bstr.m_str + bstr.Length());
}

struct GlobalAllocDeleter
{
	typedef HGLOBAL pointer;

	void operator()(pointer p) const
	{
		GlobalFree(p);
	}
};

typedef std::unique_ptr<void, GlobalAllocDeleter> HGlobal;

template <typename T>
class GlobalLock
{
public:
	explicit GlobalLock(const HGlobal& hg) :
		m_hg(hg.get()),
		m_ptr(::GlobalLock(m_hg))
	{
	}

	~GlobalLock()
	{
		::GlobalUnlock(m_hg);
	}

	T* Ptr() const
	{
		return static_cast<T*>(m_ptr);
	}

private:
	HGLOBAL m_hg;
	void* m_ptr;
};

void CLogView::Copy()
{
	std::ostringstream ss;

	int item = -1;
	while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
		ss <<
			GetItemText(item, 0) << "\t" <<
			GetItemText(item, 1) << "\t" <<
			GetItemText(item, 2) << "\t" <<
			GetItemText(item, 3) << "\t" <<
			GetItemText(item, 4) << "\n";
	const std::string& str = ss.str();

	HGlobal hdst(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, str.size() + 1));
	GlobalLock<char> lock(hdst);
	std::copy(str.begin(), str.end(), stdext::checked_array_iterator<char*>(lock.Ptr(), str.size()));
	lock.Ptr()[str.size()] = '\0';
	if (OpenClipboard())
	{
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hdst.release());
		CloseClipboard();
	}
}

bool CLogView::Find(const std::string& text, int direction)
{
	int begin = std::max(GetNextItem(-1, LVNI_FOCUSED), 0);
	int line = begin + direction;
	while (line != begin)
	{
		if (line < 0)
			line += m_logLines.size();
		if (line == m_logLines.size())
			line = 0;

		if (m_logFile[m_logLines[line].line].text.find(text) != std::string::npos)
		{
			EnsureVisible(line, true);
			SetItemState(line, LVIS_FOCUSED, LVIS_FOCUSED);
			SelectItem(line);
			return true;
		}
		line += direction;
	}
	return false;
}

bool CLogView::FindNext(const std::wstring& text)
{
	return Find(Str(text).str(), +1);
}

bool CLogView::FindPrevious(const std::wstring& text)
{
	return Find(Str(text).str(), -1);
}

void CLogView::LoadSettings(CRegKey& reg)
{
	std::array<wchar_t, 100> buf;
	DWORD len = buf.size();
	if (reg.QueryStringValue(L"ColWidths", buf.data(), &len) == ERROR_SUCCESS)
	{
		std::wistringstream ss(buf.data());
		int col = 0;
		int width;
		while (ss >> width)
		{
			SetColumnWidth(col, width);
			++col;
		}
	}
}

void CLogView::SaveSettings(CRegKey& reg)
{
	std::wostringstream ss;
	for (int i = 0; i < 5; ++i)
		ss << GetColumnWidth(i) << " ";
	reg.SetStringValue(L"ColWidths", ss.str().c_str());
}

std::vector<LogFilter> CLogView::GetFilters() const
{
	return m_filters;
}

void CLogView::SetFilters(std::vector<LogFilter> logFilters)
{
	m_filters.swap(logFilters);
	ApplyFilters();
}

void CLogView::ApplyFilters()
{
	m_logLines.clear();

	int count = m_logFile.Count();
	for (int i = 0; i < count; ++i)
	{
		if (IsIncluded(m_logFile[i].text))
			m_logLines.push_back(LogLine(i, GetColor(m_logFile[i].text)));
	}
	SetItemCount(m_logLines.size());
}

COLORREF CLogView::GetColor(const std::string& text) const
{
	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
	{
		if (it->type == FilterType::Highlight && std::regex_search(text, it->re))
			return it->color;
	}

	return RGB(255, 255, 255);
}

bool CLogView::IsIncluded(const std::string& text) const
{
	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
	{
		switch (it->type)
		{
		case FilterType::Include:
		case FilterType::Exclude:
			if (std::regex_search(text, it->re))
				return it->type == FilterType::Include;
			break;

		default:
			break;
		}
	}
	return true;
}

} // namespace gj