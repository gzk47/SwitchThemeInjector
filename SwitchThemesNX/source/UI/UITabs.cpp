#include <algorithm>

#include "UI.hpp"
#include "../ViewFunctions.hpp"
#include "../Version.hpp"
#include "../Platform/Platform.hpp"

/*
18					18
--330  14  900		--
  ----|  |-----------

*/

const int BorderPadding = 20;

// X Y width height -> x y z w
const ImVec4 BottomRect = { 0, SCR_H - 67, SCR_W, 67 };
const ImVec4 TopRect = { 0, 0, SCR_W, 76 };
const ImVec4 SideRect = { 0, TopRect.w, 330, BottomRect.y };

#define TopLineLen 1200
#define SideLineLen 510

const ImVec2 TopLineSz = { TopLineLen,1 };
const ImVec2 SideLineSz = { 1,SideLineLen };

const ImVec2 TopLine = { SCR_W / 2 - TopLineLen / 2, TopRect.w + 1 };
const ImVec2 BottomLine = { SCR_W / 2 - TopLineLen / 2, BottomRect.y - 1 };
const ImVec2 SideLine = { SideRect.z + 1, (SCR_H) / 2 - SideLineLen / 2 };

void TabRenderer::Render(int X, int Y)
{
	ImGui::SetNextWindowSize(ImVec2(SideRect.z, SideRect.w));
	Utils::ImGuiSetupWin("TabRenderer", 0, 0, DefaultWinFlags);

	// 侧边栏浅色渐变背景（BGR顺序）
	ImGui::GetCurrentWindow()->DrawList->AddRectFilledMultiColor({ SideRect.x, SideRect.y }, { (SideRect.x + SideRect.z), (SideRect.y + SideRect.w) / 2 }, 0xfff3f3f3, 0xfff3f3f3, 0xffececec, 0xffececec);
	ImGui::GetCurrentWindow()->DrawList->AddRectFilledMultiColor({ SideRect.x, (SideRect.y + SideRect.w) / 2 }, { (SideRect.x + SideRect.z), (SideRect.y + SideRect.w) }, 0xffececec, 0xffececec, 0xfff3f3f3, 0xfff3f3f3);

	if (!ControlHasFocus && !ImGui::IsWindowFocused())
		ImGui::SetWindowFocus();

	ImGui::PushFont(font30);

	ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0,0.5 });
	const float BaseLabelY = TopRect.w + 15;
	const float BaseLabelX = TopLine.x + 5;
	ImGui::SetCursorPos({ BaseLabelX, BaseLabelY });
	int count = 0;
	static int selectedIndex;
	for (const IPage *page : Pages)
	{
		ImGui::SetCursorPosX(BaseLabelX);
		
		bool CurrentSelected = (page == CurrentControl);
		if (CurrentSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.65f, 0.60f, 1.00f)); // 选中左菜单字体青绿色
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 12);
		}

		auto cursor = ImGui::GetCursorPos();
		if (page->NotificationIcon)
		{
			auto len = ImGui::CalcTextSize(page->Name.c_str()).x;
			ImGui::GetCurrentWindow()->DrawList->AddCircleFilled({ cursor.x + len + 12, cursor.y + 12 }, 5, 0xffc90000);
		}
		
		if (ImGui::Button(page->Name.c_str(), ImVec2(250,0)))
		{
			SetFocused(count);
			selectedIndex = count;
		}

		if (ImGui::GetCurrentWindow()->Appearing && count == selectedIndex)
			Utils::ImGuiSelectItem();

		if (CurrentSelected)
		{
			ImGui::PopStyleColor();
			ImGui::GetCurrentWindow()->DrawList->AddRectFilled({ BaseLabelX, cursor.y }, { BaseLabelX + 4, ImGui::GetCursorPosY() - 10}, 0xff99a600); // 左侧边指示条青绿色
		}

		if (!ControlHasFocus && GImGui->NavId == ImGui::GetID(page->Name.c_str()))
			selectedIndex = count;

		++count;
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	
	if (!ControlHasFocus)
		CurrentControl = Pages[selectedIndex];

	auto dList = ImGui::GetOverlayDrawList();
	
	// 顶部标题改为黑色 0xff111111，字号30
	dList->AddText(font40, 30, { TopLine.x ,21 }, 0xff111111, Title.c_str());

	if (UseLowMemory) 
	{
		ImGui::PushFont(font40);
		auto titleSize = ImGui::CalcTextSize(Title.c_str());
		ImGui::PopFont();
	
		dList->AddText(font25, 30, { TopLine.x + titleSize.x + 20 , 30 }, 0xff2020ff, "Applet mode");
	}

	// 分割线改成浅灰色
	dList->AddRectFilled(TopLine, TopLine + TopLineSz, 0xffcccccc);
	dList->AddRectFilled(BottomLine, BottomLine + TopLineSz, 0xffcccccc);
	
	ImGui::PopFont();
	Utils::ImGuiCloseWin();

	if (ControlHasFocus && ImGui::IsWindowFocused())
		SetFocused(selectedIndex);

	if (CurrentControl)
		CurrentControl->Render((int)SideRect.z + 30, (int)TopRect.w + 14);
}

TabRenderer::TabRenderer() :
Title("NX主题安装程序 " + Version::Name)
{
	CurrentControl = nullptr;
}

void TabRenderer::AddPage(IPage* page) 
{
	page->Parent = this;
	page->FocusEvent.Reset();
	Pages.push_back(page);
	if (!CurrentControl)
		CurrentControl = page;
}

void TabRenderer::RemoveAt(int id)
{
	if (Pages[id] == CurrentControl)
		PageLeaveFocus(Pages[id]);
	Pages.erase(Pages.begin() + id);
}

IPage* TabRenderer::At(int id)
{
	return Pages[id];
}

void TabRenderer::PageLeaveFocus(IPage *page)
{
	ImGui::NavMoveRequestCancel();
	ControlHasFocus = false;
}

void TabRenderer::Update()
{
	if (CurrentControl && ControlHasFocus)
	{
		CurrentControl->Update();
		return;
	}
	if (Pages.size() == 0)
		return;
	else if (NAV_RIGHT)
	{
		auto res = std::find(Pages.begin(), Pages.end(), CurrentControl);
		if (res != Pages.end())
			SetFocused(res - Pages.begin());
	}
}

void TabRenderer::SetFocused(int id)
{
	if (CurrentControl)
		CurrentControl->FocusEvent.Reset();

	CurrentControl = Pages[id];
	ImGui::SetWindowFocus(CurrentControl->Name.c_str());
	CurrentControl->FocusEvent.Set();
	ControlHasFocus = true;
}

