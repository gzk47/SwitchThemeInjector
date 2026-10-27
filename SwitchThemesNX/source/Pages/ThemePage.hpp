#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "../UI/UI.hpp"
#include "ThemeEntry/ThemeEntry.hpp"

class ThemesPage : public IPage
{
	public:
		static ThemesPage* Instance;

		ThemesPage();	
		~ThemesPage();
		
		void Render(int X, int Y) override;
		void Update() override;

		void SelectElementOnRescan(const std::string& path);
	private:
		void RefreshThemesList();
		void SelectElementByPath(const std::string& path);

		void SetDir(const std::string &dir);
		void SetPage(int num, int index = 0);
		void SelectCurrent();
		
		int PageItemsCount();
		
		std::vector<std::string> ThemeFiles;
		bool IsSelected(const std::string &fname);
		void ClearSelection();
		
		void UpdatePageText();
		void UpdateCommandText();

		std::string CurrentDir;
		std::vector<std::string> DirectoryFiles;
		
		std::vector<std::unique_ptr<ThemeEntry>> DisplayEntries;
		std::string lblPage;
		const char* lblCommands;
		int pageNum = -1;
		int pageCount = -1;
		
		//Will reset the scroll and force the selected item on the ui
		bool ResetScroll = false;
		int currentMenuIndex = 0;
		void setMenuIndex(int number);

		std::vector<std::string> SelectedFiles;
		
		const std::string CommandsTextNormal = "(A): 安装    (Y): 多选    (L)/(R): 上一页/下一页";
		const std::string CommandsTextFolder = "(A): 打开文件夹    (L)/(R): 上一页/下一页";
		const std::string CommandsTextWithOptions = "(A): 安装    (X): 选项    (Y): 多选    (L)/(R): 上一页/下一页";
		const std::string CommandsTextSelected = "(A): 添加/移除选中项    (Y): 清除选择   (+): 安装所选";

		int LimitLoad = 25;

		std::unordered_map<std::string, std::tuple<int,int>> CursorMemory;

		std::string SelectOnRescanTarget;
};