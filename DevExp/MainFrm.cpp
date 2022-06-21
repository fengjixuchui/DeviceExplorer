// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "aboutdlg.h"
#include "DevNodeView.h"
#include "MainFrm.h"
#include "IconHelper.h"
#include "DevNodeListView.h"
#include "AppSettings.h"
#include "SecurityHelper.h"
#include "DeviceClassesView.h"
#include "DeviceInterfacesView.h"

const int WINDOW_MENU_POSITION = 6;

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	auto& settings = AppSettings::Get();
	settings.LoadFromKey(L"Software\\ScorpioSoftware\\DeviceExplorer");

	auto hMenu = GetMenu();
	if (SecurityHelper::IsRunningElevated()) {
		auto h = CMenuHandle(hMenu).GetSubMenu(0);
		h.DeleteMenu(0, MF_BYPOSITION);
		h.DeleteMenu(0, MF_BYPOSITION);
	}
	InitMenu();
	UIAddMenu(hMenu);
	AddMenu(hMenu);

	CToolBarCtrl tb;
	tb.Create(m_hWnd, nullptr, nullptr, ATL_SIMPLE_TOOLBAR_PANE_STYLE, 0, ATL_IDW_TOOLBAR);
	InitToolBar(tb, 24);
	UIAddToolBar(tb);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(tb);
	CreateSimpleStatusBar();

	m_view.m_bTabCloseButton = false;
	m_hWndClient = m_view.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	CImageList images;
	images.Create(16, 16, ILC_COLOR32 | ILC_MASK, 4, 4);
	UINT icons[] = { 
		IDI_TREE, IDI_LIST, IDI_DEVICES, IDI_INTERFACE, IDI_DRIVER,
	};
	for(auto icon : icons)
		images.AddIcon(AtlLoadIconImage(icon, 0, 16, 16));

	m_view.SetImageList(images);

	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	CReBarCtrl rb(m_hWndToolBar);
	rb.LockBands(true);

	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	if (SecurityHelper::IsRunningElevated()) {
		CString text;
		GetWindowText(text);
		SetWindowText(text + L" (Administrator)");
	}

	CMenuHandle menuMain = GetMenu();
	m_view.SetWindowMenu(menuMain.GetSubMenu(WINDOW_MENU_POSITION));

	UIEnable(ID_DEVICE_SCANFORHARDWARECHANGES, SecurityHelper::IsRunningElevated());

	PostMessage(WM_COMMAND, ID_EXPLORE_DEVICESBYCLASS);

	return 0;
}

LRESULT CMainFrame::OnExploreDeviceClasses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto pView = new CDeviceClassesView(this);
	pView->Create(m_view, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_view.AddPage(pView->m_hWnd, _T("Device Classes"), 2, pView);
	return 0;
}

LRESULT CMainFrame::OnExploreDeviceInterfaces(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto pView = new CDeviceInterfacesView(this);
	pView->Create(m_view, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_view.AddPage(pView->m_hWnd, _T("Device Interfaces"), 3, pView);
	return 0;
}

LRESULT CMainFrame::OnExploreDeviceTree(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto pView = new CDevNodeView(this);
	pView->Create(m_view, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_view.AddPage(pView->m_hWnd, _T("Device Tree"), 0, pView);
	return 0;
}

LRESULT CMainFrame::OnExploreDeviceList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto pView = new CDevNodeListView(this);
	pView->Create(m_view, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_view.AddPage(pView->m_hWnd, _T("Device List"), 1, pView);
	return 0;
}


LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	AppSettings::Get().Save();

	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	static bool bVisible = true;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	rebar.ShowBand(0, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) const {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnWindowClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (int nActivePage = m_view.GetActivePage(); nActivePage != -1) {
		m_view.RemovePage(nActivePage);
		if (m_view.GetPageCount() == 0)
			UpdateUI();
	}

	return 0;
}

LRESULT CMainFrame::OnWindowCloseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_view.RemoveAllPages();

	if (m_view.GetPageCount() == 0)
		UpdateUI();

	return 0;
}

void CMainFrame::UpdateUI() {
	int count = m_view.GetPageCount();
	if (count == 0) {
		UIEnable(ID_DEVICE_ENABLE, false);
		UIEnable(ID_DEVICE_PROPERTIES, false);
		UIEnable(ID_EDIT_COPY, false);
		UIEnable(ID_VIEW_REFRESH, false);
		UIEnable(ID_DEVICE_DISABLE, false);
	}
}

LRESULT CMainFrame::OnWindowActivate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int page = wID - ID_WINDOW_TABFIRST;
	m_view.SetActivePage(page);
	
	return 0;
}

void CMainFrame::InitToolBar(CToolBarCtrl& tb, int size) {
	CImageList tbImages;
	tbImages.Create(size, size, ILC_COLOR32, 8, 4);
	tb.SetImageList(tbImages);
	auto elevated = SecurityHelper::IsRunningElevated();

	const struct {
		UINT id;
		int image;
		WORD style = BTNS_BUTTON;
		PCWSTR text = nullptr;
	} buttons[] = {
		{ ID_VIEW_REFRESH, IDI_REFRESH },
		{ ID_EDIT_COPY, IDI_COPY },
		{ 0 },
		{ ID_EXPLORE_DEVICESBYCLASS, IDI_DEVICES },
		{ ID_EXPLORE_DEVICETREE, IDI_TREE },
		{ ID_EXPLORE_DEVICELIST, IDI_LIST },
		{ ID_EXPLORE_DEVICEINTERFACES, IDI_INTERFACE },
		{ ID_EXPLORE_DRIVERS, IDI_DRIVER },
		{ 0 },
		{ 0, 0, 0x8000 },
		{ ID_DEVICE_PROPERTIES, IDI_PROPS },
		{ ID_DEVICE_ENABLE, IDI_ENABLE_DEVICE, 0x8000 },
		{ ID_DEVICE_DISABLE, IDI_DISABLE_DEVICE, 0x8000 },
		{ 0, 0, 0x8000 },
		{ ID_DEVICE_SCANFORHARDWARECHANGES, IDI_RESCAN, 0x8000 },
	};
	for (auto& b : buttons) {
		if (!elevated && (b.style & 0x8000))
			continue;

		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			auto hIcon = AtlLoadIconImage(b.image, 0, size, size);
			ATLASSERT(hIcon);
			int image = tbImages.AddIcon(hIcon);
			tb.AddButton(b.id, (BYTE)b.style, TBSTATE_ENABLED, image, b.text, 0);
		}
	}
}

void CMainFrame::InitMenu() {
	struct {
		UINT id, icon;
		HICON hIcon = nullptr;
	} cmds[] = {
		{ ID_VIEW_REFRESH, IDI_REFRESH },
		{ ID_EDIT_COPY, IDI_COPY },
		{ ID_EDIT_DELETE, IDI_CANCEL },
		{ ID_DEVICE_PROPERTIES, IDI_PROPS },
		{ ID_FILE_RUNASADMINISTRATOR, 0, IconHelper::GetShieldIcon() },
		{ ID_DEVICE_SCANFORHARDWARECHANGES, IDI_RESCAN },
		{ ID_DEVICE_ENABLE, IDI_ENABLE_DEVICE },
		{ ID_DEVICE_DISABLE, IDI_DISABLE_DEVICE },
		{ ID_EXPLORE_DEVICEINTERFACES, IDI_INTERFACE },
		{ ID_EXPLORE_DEVICESBYCLASS, IDI_DEVICES },
		{ ID_EXPLORE_DEVICETREE, IDI_TREE },
		{ ID_EXPLORE_DEVICELIST, IDI_LIST },
		{ ID_EXPLORE_DRIVERS, IDI_DRIVER },
	};
	for (auto& cmd : cmds) {
		AddCommand(cmd.id, cmd.icon ? AtlLoadIconImage(cmd.icon, 0, 16, 16) : cmd.hIcon);
	}
}

HWND CMainFrame::GetHwnd() const {
	return m_hWnd;
}

BOOL CMainFrame::TrackPopupMenu(HMENU hMenu, DWORD flags, int x, int y) {
	return ShowContextMenu(hMenu, flags, x, y);
}

CUpdateUIBase& CMainFrame::GetUI() {
	return *this;
}

LRESULT CMainFrame::OnPageActivated(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	if (m_view.GetPageCount() > 0) {
		int page = m_view.GetActivePage();
		if (m_ActivePage >= 0 && m_ActivePage < m_view.GetPageCount())
			::PostMessage(m_view.GetPageHWND(m_ActivePage), WM_PAGE_ACTIVATED, 0, 0);
		m_ActivePage = page;
		if (page < m_view.GetPageCount())
			::PostMessage(m_view.GetPageHWND(page), WM_PAGE_ACTIVATED, 1, 0);
	}
	return 0;
}

LRESULT CMainFrame::OnRescanHardware(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	auto node = DeviceManager::GetRootDeviceNode();
	CWaitCursor wait;
	node.Rescan();
	PostMessageToAllTabs(WM_NEED_REFRESH);
	
	return 0;
}

void CMainFrame::PostMessageToAllTabs(UINT msg, WPARAM wp, LPARAM lp) const {
	auto count = m_view.GetPageCount();
	for (int i = 0; i < count; i++)
		::PostMessage(m_view.GetPageHWND(i), msg, wp, lp);
}

LRESULT CMainFrame::OnRunAsAdmin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	AppSettings::Get().Save();
	if (SecurityHelper::RunElevated())
		PostMessage(WM_CLOSE);
	return 0;
}

