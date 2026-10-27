#include <vector>
#include "SettingsPage.hpp"
#include "../fs.hpp"
#include "../ViewFunctions.hpp"
#include "../SwitchThemesCommon/MyTypes.h"
#include <algorithm>

using namespace std;

namespace Settings {
	bool UseIcons = true;
	bool UseCommon = true;
	SwitchThemesCommon::LayoutCompatibilityOption HomeMenuCompat = SwitchThemesCommon::LayoutCompatibilityOption::Default;
};

namespace 
{
	const char* MarkerMagic = "THEME_SYMODULE:";

	int ExtractVersionFromSysmodule(span<const u8> data, int fallbackVersion)
	{
		std::basic_string<u8> marker = reinterpret_cast<const u8*>(MarkerMagic);
		auto found = std::search(data.begin(), data.end(), marker.begin(), marker.end());
		
		if (found == data.end())
			return fallbackVersion;

		auto marker_end = std::find(found, data.end(), '=');
		if (marker_end == data.end()) // Shouldn't happen, but just in case
			return fallbackVersion;

		std::string version_str(found + marker.size(), marker_end);
		return std::stoi(version_str);
	}
}

SettingsPage::SettingsPage()
{
	Name = "设置";
	ReloadSysmoduleInfo();

	if (sysmoduleInstalled && bundledSysmodule && *sysmoduleInstalled < *bundledSysmodule)
		NotificationIcon = true;
	else
		NotificationIcon = !fs::CheckFlagFile("sysmodule2");
}

#define SYSMODULE_ID "00FF007468656D65"

std::optional<int> SettingsPage::sysmoduleInstalled;
std::optional<int> SettingsPage::bundledSysmodule;

void SettingsPage::ReloadSysmoduleInfo()
{
	if (fs::Exists(fs::path::FsMitmFolder() + SYSMODULE_ID "/exefs.nsp"))
	{
		try 
		{
			// Original version was 1 and did not have the marker tag, assume this is the case if we don't find it
			sysmoduleInstalled = ExtractVersionFromSysmodule(fs::OpenFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/exefs.nsp"), 1);
		}
		catch (const std::exception& ex)
		{
			LOGf("读取系统模块版本时出错: %s", ex.what());
			sysmoduleInstalled = std::nullopt;
		}
	}
	else
	{
		sysmoduleInstalled = std::nullopt;
	}

	if (fs::Exists(ASSET("sysmodule/ThemeSysmodule.nsp")))
	{
		try
		{
			bundledSysmodule = ExtractVersionFromSysmodule(fs::OpenFile(ASSET("sysmodule/ThemeSysmodule.nsp")), -1);
			
			// We should always know the version bundled with the installer.
			if (bundledSysmodule == -1)
				bundledSysmodule = std::nullopt;
		}
		catch (const std::exception& ex)
		{
			LOGf("读取捆绑的系统模块版本时出错: %s", ex.what());
			bundledSysmodule = std::nullopt;
		}
	}
	else
	{
		bundledSysmodule = std::nullopt;
	}
}

bool SettingsPage::InstallSysmodule()
{
	if (!bundledSysmodule)
	{
		Dialog("系统模块安装失败。此主题安装程序版本未包含系统模块二进制文件。");
		return false;
	}

	try 
	{
		fs::CreateDirectory(fs::path::FsMitmFolder() + SYSMODULE_ID "/flags");
		fs::WriteFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/exefs.nsp", fs::OpenFile(ASSET("sysmodule/ThemeSysmodule.nsp")));
		fs::WriteFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/toolbox.json", fs::OpenFile(ASSET("sysmodule/toolbox.json")));
		fs::WriteFile(fs::path::FsMitmFolder() + SYSMODULE_ID "/flags/boot2.flag", std::vector<u8>{ 'a' });
		
		ReloadSysmoduleInfo();
		Dialog("系统模块已安装。重启您的主机以应用更改。");
		
		return true;
	}
	catch (const std::exception& ex)
	{
		Dialog("安装系统模块时出错: "s + ex.what());
		return false;
	}
}

bool SettingsPage::RemoveSysmodule(bool dialogs)
{
	auto path = fs::path::FsMitmFolder() + SYSMODULE_ID "/";

	try {
		if (fs::Exists(path))
			fs::DeleteDirectory(path);

		ReloadSysmoduleInfo();

		if (dialogs)
			Dialog("系统模块已卸载。重启您的主机以应用更改。");

		return true;
	}
	catch (const std::exception& ex)
	{
		Dialog("卸载系统模块时出错: "s + ex.what());
		return false;
	}
}

void SettingsPage::UISysmoduleAlreadyInstalled() 
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.65f, 0.60f, 1.00f)); // 青绿色
	ImGui::Text("系统模块 v%d 当前已安装。", *sysmoduleInstalled);
	ImGui::PopStyleColor();
	ImGui::SameLine();
	if (ImGui::Button("卸载"))
		PushFunction([]() { RemoveSysmodule(true); });
}

void SettingsPage::UISysmoduleUpdateAvailable() 
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.65f, 0.60f, 1.00f)); // 青绿色
	ImGui::Text("系统模块 v%d 当前已安装。有新版本可供使用。", *sysmoduleInstalled);
	ImGui::PopStyleColor();

	if (ImGui::Button("卸载"))
		PushFunction([]() { RemoveSysmodule(true); });
	
	// Cursor hack, this should be the first item
	UISetupFirstItem();

	ImGui::SameLine();

	// TODO: actually due to imgui handling events together as our tab manager using the dpan right input to switch to the settings page will focus this instead
	// It's an edge case and fixing it is probably more trouble than it's worth.
	if (ImGui::Button("立即更新"))
		PushFunction([]() { InstallSysmodule(); });

	focusItemPreventsLeft = ImGui::GetFocusID() == ImGui::GetCurrentWindow()->DC.LastItemId;
}

void SettingsPage::UISysmoduleBuildMissing()
{
	ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
	ImGui::TextWrapped("此主题安装程序版本不包含 sysmodule 二进制文件。");
	ImGui::PopStyleColor();
}

void SettingsPage::UISysmoduleNotInstalled() 
{
	ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
	ImGui::Text("系统模块当前未安装。");
	ImGui::PopStyleColor();

	ImGui::SameLine();
	if (ImGui::Button("立即安装"))
		PushFunction([]() { InstallSysmodule(); });
}

void SettingsPage::UISetupFirstItem()
{
	if (firstUiItem == 0)
		firstUiItem = ImGui::GetCurrentWindow()->DC.LastItemId;

	ImGui::SetItemDefaultFocus();
}

void SettingsPage::Render(int X, int Y)
{
	firstUiItem = 0;
	focusItemPreventsLeft = false;

	if (NotificationIcon)
	{
		NotificationIcon = false;
		fs::SetFlagFile("sysmodule2", true);
	}

	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font25);

	ImGui::PushFont(font30);
	ImGui::TextUnformatted("更新检测系统模块 v3");
	ImGui::PopFont();

	ImGui::TextWrapped("此系统模块会在系统固件更新时自动移除主题。这可以防止更新后出现异常，但更新后您仍需要从卸载页面删除残留文件。");
	if (sysmoduleInstalled)
	{
		if (!bundledSysmodule)
		{
			UISysmoduleBuildMissing();
			UISysmoduleAlreadyInstalled();
		}
		else if (*sysmoduleInstalled < *bundledSysmodule) 
		{
			UISysmoduleUpdateAvailable();
			ImGui::TextWrapped("此更新修复了用户报告的多个错误。");
		}
		else
			UISysmoduleAlreadyInstalled();
	}
	else if (bundledSysmodule)
		UISysmoduleNotInstalled();
	else
		UISysmoduleBuildMissing();
	
	UISetupFirstItem();

	PAGE_RESET_FOCUS_FOR(firstUiItem);

	// Fix scrolling: when the first item gets focused return at the top
	if (ImGui::GetScrollY() > 0 && ImGui::GetFocusID() == firstUiItem)
		ImGui::SetScrollY(0);

	ImGui::NewLine();

	ImGui::PushFont(font30);
	ImGui::TextUnformatted("NX主题设置");
	ImGui::PopFont();

	ImGui::TextWrapped("这些设置仅用于安装 nxthemes，不会被保存。每次启动此应用时，您都需要重新切换它们。");

	ImGui::Text("主页菜单布局兼容性选项");
	ImGui::RadioButton("自动决定 (默认)", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::Default);
	ImGui::RadioButton("强制使用原始主菜单小程序图标 (固件 <= 10.0)", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::Firmware10);
	ImGui::RadioButton("强制使用带NS在线图标的主菜单布局 (固件 11.0)", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::Firmware11);
	ImGui::RadioButton("不应用兼容性修复", (int*)&Settings::HomeMenuCompat, (int)SwitchThemesCommon::LayoutCompatibilityOption::DisableFixes);

	ImGui::NewLine();

	ImGui::Text("布局部件选项");
	ImGui::Checkbox("启用自定义图标", &Settings::UseIcons);
	ImGui::Checkbox("启用额外布局（例如：common.szs）", &Settings::UseCommon);

	ImGui::NewLine();

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void SettingsPage::Update()
{
	if (Utils::PageLeaveFocusInput(!focusItemPreventsLeft))
	{
		Parent->PageLeaveFocus(this);
		return;
	}
}









