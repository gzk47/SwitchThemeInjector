#include <algorithm>
#include <sstream>
#include "ThemePage.hpp"
#include "../fs.hpp"
#include "../ViewFunctions.hpp"
#include "../Platform/Platform.hpp"
#include "../UI/imgui/imgui_internal.h"

using namespace std;

ThemesPage* ThemesPage::Instance = nullptr;

ThemesPage::ThemesPage() : lblPage("")
{
	if (Instance)
		throw std::runtime_error("ThemePage::Instance should not be set");
	Instance = this;

	if (UseLowMemory)
		LimitLoad = 15;

	Name = "主题";
	lblCommands = CommandsTextNormal.c_str();

	RefreshThemesList();
}

void ThemesPage::RefreshThemesList()
{
	DisplayLoading("正在加载主题列表...");
	ClearSelection();
	SetPage(-1);
	CursorMemory.clear();

	ThemeFiles = std::move(fs::theme::ScanThemeFiles());
	std::sort(ThemeFiles.begin(), ThemeFiles.end());

	if (SelectOnRescanTarget != "")
	{
		SelectElementByPath(SelectOnRescanTarget);
		SelectOnRescanTarget = "";
	}
	else SetDir(fs::path::ThemesFolder);
}

void ThemesPage::SelectElementByPath(const std::string& path)
{	
	if (std::find(ThemeFiles.begin(), ThemeFiles.end(), path) == ThemeFiles.end())
		return; // File not in index

	SetDir(fs::GetPath(path));

	auto f = std::find(DirectoryFiles.begin(), DirectoryFiles.end(), path);
	if (f == DirectoryFiles.end())
		return; // Can this ever happen ?

	int index = (int)(f - DirectoryFiles.begin());

	// Find in which page is the file
	int page = 0;
	for (; page < pageCount; page++)
		if (index <= (page + 1) * LimitLoad) // Pages are 0-indexed
			break;

	int pageindex = index - (page * LimitLoad); // Start index of the current page

	SetPage(page, pageindex);
}

ThemesPage::~ThemesPage()
{
	Instance = nullptr;
	SetPage(-1);
}

void ThemesPage::SetDir(const string &dir)
{
	if (pageNum >= 0)
		CursorMemory[CurrentDir] = tuple<int,int>(pageNum, currentMenuIndex);

	CurrentDir = dir;
	if (!StrEndsWith(dir, "/"))
		CurrentDir += "/";
	
	DirectoryFiles.clear();
	for (auto f : ThemeFiles)
		if (fs::GetPath(f) == CurrentDir)
			DirectoryFiles.push_back(f);
	
	if (DirectoryFiles.size() == 0)
	{
		pageCount = 1;
	}
	else 
	{
		pageCount = DirectoryFiles.size() / LimitLoad + 1;
		if (DirectoryFiles.size() % LimitLoad == 0)
			pageCount--;
	}	

	pageNum = -1; //force setpage to reload the entries even if in the same page as the path changed
	if (CursorMemory.count(dir))
	{
		const auto& [num, index] = CursorMemory[dir];
		SetPage(num, index);
	}
	else SetPage(0);
}

/*
	Set page number.
	if num < 0 just clear current page and don't load anything
	if index < 0 select last item of the page that's being set
*/
void ThemesPage::SetPage(int num, int index)
{
	ImGui::NavMoveRequestCancel();

	// Reset scroll if we're changing the page or the menu index
	ResetScroll = num != pageNum || index != currentMenuIndex;

	if (num < 0) // Just clear the current page
	{
		DisplayEntries.clear();
		pageNum = num;
		lblPage = "";
	}
	else if (num >= 0 && num != pageNum) // If the page is valid and different than the current one load the new entries
	{
		DisplayEntries.clear();
		pageNum = num;

		if (!DirectoryFiles.size())
		{
			lblPage = CurrentDir + " 此文件夹中没有文件。按B键返回。";
			return;
		}
		else 
		{
			size_t baseIndex = num * LimitLoad;
			if (baseIndex >= DirectoryFiles.size())
			{
				// This shouldn't happen
				lblPage = "页面超出范围错误";
				return;
			}

			int imax = DirectoryFiles.size() - baseIndex;
			if (imax > LimitLoad) imax = LimitLoad;

			for (int i = 0; i < imax; i++)
				DisplayEntries.push_back(ThemeEntry::FromFile(DirectoryFiles[baseIndex + i]));
		}

		UpdatePageText();
	}

	// Set the menu index
	setMenuIndex(index < 0 ? PageItemsCount() - 1 : index);
}

void ThemesPage::setMenuIndex(int index)
{
	currentMenuIndex = index;
	UpdateCommandText();
}

void ThemesPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPageFullscreen("ThemesPageContainer", X, Y, DefaultWinFlags | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::PushFont(font25);

	if (ImGui::GetCurrentWindow()->Appearing && fs::theme::ShouldRescanThemeList())
		PushFunction([this]() { RefreshThemesList(); });

	if (DisplayEntries.size() == 0) {
		if (CurrentDir != fs::path::ThemesFolder)
		{
			ImGui::TextWrapped("这个文件夹是空的。");

			if (ImGui::Button("转到上一级文件夹"))
				SetDir(fs::GetParentDir(CurrentDir));
		}
		else {
			ImGui::TextWrapped(
			"此处没有任何内容，请将主题复制到sd上的themes文件夹中，然后重试。\n\n"
			"如果你的sd中确实有一个包含主题的主题文件夹，请确保名称全部小写，并检查sd是否损坏。\n\n"
			"你可以在themezer.net上找到更多主题"
			);
		}
	}

	ImGui::SetCursorPosY(600);
	Utils::ImGuiRightString(lblPage);
	
	if (DisplayEntries.size() == 0)
		goto QUIT_RENDERING;

	ImGui::SetCursorPosY(570);
	ImGui::TextUnformatted(lblCommands ? lblCommands : "");

	{
		Utils::ImGuiSetupPage("ThemesList", X, Y, DefaultWinFlags & ~ImGuiWindowFlags_NoScrollbar);
		int setNewMenuIndex = 0;
		if (ResetScroll || ImGui::GetCurrentWindow()->Appearing)
		{
			setNewMenuIndex = currentMenuIndex;
			ImGui::NavMoveRequestCancel();
			ImGui::SetScrollY(0);
			FocusEvent.Set();
			ResetScroll = false;
		}		
		{
			int count = 0;
			for (auto& e : DisplayEntries)
			{
				bool Selected = IsSelected(e->GetPath());
				if (Selected)
					ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x366e64ff);

				if (e->IsHighlighted() && currentMenuIndex != count)
					setMenuIndex(count);
				
				auto res = e->Render(Selected);

				if (Selected)
					ImGui::PopStyleColor();
				
				if (count == setNewMenuIndex && FocusEvent.Reset())
					Utils::ImGuiSelectItem(true);
				
				Utils::ImGuiDragWithLastElement();

				if (res == ThemeEntry::UserAction::Options)
					break;
				else if (res == ThemeEntry::UserAction::Enter)
					PushFunction([count, &e, this]()
						{
							if (e->IsFolder())
								SetDir(e->GetPath());
							else
							{
								if (SelectedFiles.size() == 0)
									e->Install();
								else
								{
									if (currentMenuIndex != count)
										setMenuIndex(count);
									SelectCurrent();
								}
							}
						});

				count++;
			}
		}

		//Here scrolling is handled by the individual theme entries, Utils::ImGuiSetWindowScrollable is not needed
		Utils::ImGuiCloseWin();
	}
QUIT_RENDERING:
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

int ThemesPage::PageItemsCount()
{
	if (pageNum < 0)
		return 0;

	int menuCount = DirectoryFiles.size() - pageNum * LimitLoad;

	return menuCount > LimitLoad ? LimitLoad : menuCount;
}

inline bool ThemesPage::IsSelected(const std::string &fname)
{
	return (std::find(SelectedFiles.begin(), SelectedFiles.end(), fname) != SelectedFiles.end());
}

void ThemesPage::ClearSelection()
{
	SelectedFiles.clear();
	UpdatePageText();
}

void ThemesPage::UpdatePageText()
{
	if (pageCount <= 0)
	{
		lblPage = "";
		lblCommands = "";
		return;
	}

	std::stringstream ss;

	if (SelectedFiles.size() != 0)
		ss << "(" << SelectedFiles.size() << " 已选择) ";

	ss << CurrentDir << " - 页 " << pageNum + 1 << "/" << pageCount;

	lblPage = ss.str();

	UpdateCommandText();
}

void ThemesPage::UpdateCommandText()
{
	if (SelectedFiles.size() != 0) 
		lblCommands = CommandsTextSelected.c_str();
	else if (currentMenuIndex >= (int)DisplayEntries.size())
		lblCommands = CommandsTextNormal.c_str();
	else 
	{
		if (DisplayEntries[currentMenuIndex]->IsFolder())
			lblCommands = CommandsTextFolder.c_str();
		else if (DisplayEntries[currentMenuIndex]->HasOptions())
			lblCommands = CommandsTextWithOptions.c_str();
		else 
			lblCommands = CommandsTextNormal.c_str();
	}
}

void ThemesPage::SelectCurrent()
{
	if (DisplayEntries[currentMenuIndex]->IsFolder() || !DisplayEntries[currentMenuIndex]->CanInstall()) return;

	auto fname = DisplayEntries[currentMenuIndex]->GetPath();
	auto position = std::find(SelectedFiles.begin(), SelectedFiles.end(), fname);

	if (position != SelectedFiles.end())
		SelectedFiles.erase(position);
	else 
		SelectedFiles.push_back(fname);
	
	UpdatePageText();
}

void ThemesPage::Update()
{
	int menuCount = PageItemsCount();	
	
	if (NAV_LEFT)
		Parent->PageLeaveFocus(this);
	if (KeyPressed(GLFW_GAMEPAD_BUTTON_B))
	{
		if (CurrentDir != fs::path::ThemesFolder)
			SetDir(fs::GetParentDir(CurrentDir));
		else 
			Parent->PageLeaveFocus(this);
	}
	
	if (menuCount <= 0 || pageNum < 0)
		return;

	if ((NAV_UP && currentMenuIndex <= 0) || KeyPressed(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))
	{
		if (pageNum != 0) // If not the first page go to the previous
			SetPage(pageNum - 1, -1);
		else if (pageCount > 1) // If the first page and we have more pages go to the last
			SetPage(pageCount - 1 , -1);
		else // If there's only one page go to the last entry
			SetPage(pageNum, -1);
	}
	else if ((NAV_DOWN && currentMenuIndex >= PageItemsCount() - 1) || KeyPressed(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
	{
		if (pageNum + 1 < pageCount) // If we can go to the next page 
			SetPage(pageNum + 1);
		else // Else go to the first page, this happens both if we're at the last one or at the end of the first and only page
			SetPage(0);
	}
	else if (KeyPressed(GLFW_GAMEPAD_BUTTON_Y))
	{
		if (SelectedFiles.size() == 0 && currentMenuIndex >= 0)
			SelectCurrent();
		else
			ClearSelection();
	}
	else if (KeyPressed(GLFW_GAMEPAD_BUTTON_START) && SelectedFiles.size() != 0)
	{
		std::string combinedInstallLog;
		for (string file : SelectedFiles)
		{
			ThemeEntry::DisplayInstallDialog(file);
			auto entry = ThemeEntry::FromFile(file);

			if (!entry->Install(false))
			{
				Dialog("安装主题失败，进程已取消");
				break;
			}

			if (!entry->InstallLog.empty())
			{
				if (combinedInstallLog.empty())
					combinedInstallLog = entry->InstallLog;
				else
					combinedInstallLog += "\n" + entry->InstallLog;
			}
		}

		if (!combinedInstallLog.empty())
			Dialog("主题已安装，但生成了以下警告：\n\n" + combinedInstallLog);

		ClearSelection();
		SetPage(pageNum);		
	}
}

void ThemesPage::SelectElementOnRescan(const std::string& path)
{
	SelectOnRescanTarget = path;
}
