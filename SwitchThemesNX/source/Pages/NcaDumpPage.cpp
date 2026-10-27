#include <string>

#include "NcaDumpPage.hpp"
#include "../ViewFunctions.hpp"

#include "../SwitchTools/hactool.hpp"
#include "../SwitchTools/RomfsCache.hpp"
#include "../fs.hpp"
#include "../SwitchThemesCommon/Common.hpp"

static void WriteExtracted(const std::string& basePath, const FileContainer& files)
{
	for (const auto& [name, data] : files)
	{
		auto finalPath = fs::path::SystemDataFolder + fs::JoinPath(basePath, name);
		auto parent = fs::GetParentDir(finalPath);
		fs::CreateDirectory(parent);
		fs::WriteFile(finalPath, data);
	}
}

static void WriteExtractedVersion()
{
	const auto version = std::to_string(hos::Version.major) + "." +
		std::to_string(hos::Version.minor) + "." +
		std::to_string(hos::Version.micro);
	const FileData data(version.begin(), version.end());

	fs::WriteFile(fs::path::SystemDataFolder + "extracted/ver.cfg", data);
}

NcaDumpPage::NcaDumpPage()
{
	Name = "提取主菜单";
}

void NcaDumpPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);

	ImGui::TextWrapped(
		"此页面允许提取主菜单的RomFS和相关标题。\n\n"
		"此选项允许将常见的szs文件 (例如，ResidentMenu.szs) 提取到SD卡中，以便您可以创建自定义布局。对于不需要此功能的简单安装主题，它会在安装过程中自动完成。"
	);

	if (ImGui::Button("提取主菜单"))
	{
		PushFunction([]() 
		{
			try 
			{
				if (fs::Exists(fs::path::SystemDataFolder + "已提取/"))
					fs::DeleteDirectory(fs::path::SystemDataFolder + "已提取/");

				DisplayLoading("提取 qlaunch...");
				WriteExtracted("已提取/qlaunch", RomfsCache::GetContent(ThemeTargetInfo::QlaunchID));

				DisplayLoading("正在提取玩家选择...");
				WriteExtracted("已提取/玩家选择", RomfsCache::GetContent(ThemeTargetInfo::PslID));

				DisplayLoading("正在提取我的页面...");
				WriteExtracted("已提取/我的页面", RomfsCache::GetContent(ThemeTargetInfo::UserPageID));

				WriteExtractedVersion();

				Dialog("文件已提取到你的SD卡 themes/systemData/extracted/ 的文件夹。");
			}
			catch (const std::exception& ex)
			{
				DialogBlocking("提取布局文件时出错: " + std::string(ex.what()));
			}
		});
	}

	ImGui::NewLine();

	ImGui::TextWrapped(
		"如果您需要主菜单的完整文件集，可以提取原始NCA。要提取此信息，您需要在电脑上安装hactool或类似软件"
	);
	if (ImGui::Button("提取原始NCA文件"))
	{
		PushFunction([]()
			{
				try
				{
					if (fs::Exists(fs::path::SystemDataFolder + "nca/"))
						fs::DeleteDirectory(fs::path::SystemDataFolder + "nca/");

					fs::CreateDirectory(fs::path::SystemDataFolder + "nca/");

					auto nca = hactool::GetNca(ThemeTargetInfo::QlaunchID);
					fs::WriteFile(fs::path::SystemDataFolder + "nca/" + ThemeTargetInfo::TitleIdToString(ThemeTargetInfo::QlaunchID) + ".nca", nca);
					
					nca = hactool::GetNca(ThemeTargetInfo::PslID);
					fs::WriteFile(fs::path::SystemDataFolder + "nca/" + ThemeTargetInfo::TitleIdToString(ThemeTargetInfo::PslID) + ".nca", nca);

					nca = hactool::GetNca(ThemeTargetInfo::UserPageID);
					fs::WriteFile(fs::path::SystemDataFolder + "nca/" + ThemeTargetInfo::TitleIdToString(ThemeTargetInfo::UserPageID) + ".nca", nca);

					Dialog("文件已提取到你的SD卡 themes/systemData/nca 的文件夹。");
				}
				catch (const std::exception& ex)
				{
					DialogBlocking("提取布局文件时出错: " + std::string(ex.what()));
				}
			});
	}
		
	ImGui::PopFont();
	Utils::ImGuiCloseWin();
}

void NcaDumpPage::Update()
{	
	if (Utils::PageLeaveFocusInput()){
		Parent->PageLeaveFocus(this);
	}
}
