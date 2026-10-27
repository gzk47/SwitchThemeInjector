#include <string>
#include <exception>
#include "UninstallPage.hpp"
#include "SettingsPage.hpp"
#include "../ViewFunctions.hpp"
#include "../SwitchTools/PatchMng.hpp"
#include "../fs.hpp"

namespace
{
	bool RemoveTheme(bool full)
	{
		try
		{
			fs::theme::UninstallTheme(full);
		}
		catch (const std::exception& ex)
		{
			Dialog("卸载主题时出错: " + std::string(ex.what()));
			return false;
		}
		return true;
	}

	bool RemoveBootlogo()
	{
		try
		{
			if (fs::Exists(fs::path::BootlogoPath))
				fs::Delete(fs::path::BootlogoPath);
		}
		catch (const std::exception& ex)
		{
			Dialog("删除启动标志文件时出错: " + std::string(ex.what()));
			return false;
		}
		return true;
	}

	bool RemovePatches()
	{
		try
		{
			PatchMng::RemoveAll();
		}
		catch (const std::exception& ex)
		{
			Dialog("移除锁屏补丁时出错: " + std::string(ex.what()));
			return false;
		}
		return true;
	}
}

UninstallPage::UninstallPage()
{
	Name = "卸载主题";
}

void UninstallPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);
	ImGui::PushFont(font30);

	ImGui::TextWrapped("使用这些选项卸载当前已安装的主题。");

	ImGui::PushStyleColor(ImGuiCol_Button, u32(0x5B700009f));

	if (Utils::ImGuiCenterButton("移除当前主题", 550))
	{
		PushFunction([]() {
			if (!YesNoPage::Ask(
				"这将删除所有支持的主菜单部分的已安装主题。\n"
				"你想继续吗？"
			))
				return;

			DisplayLoading("加载中...");

			if (RemoveTheme(false))
				Dialog("已完成，所有已安装的主题均已移除，请重启您的控制台以应用更改");
		});
	}
	PAGE_RESET_FOCUS;

	if (Utils::ImGuiCenterButton("移除Hekate启动标志", 550))
	{
		PushFunction([]() {
			if (!YesNoPage::Ask(
				"这将移除自定义的hekate启动屏幕。\n"
				"你想继续吗？"
			))
				return;

			if (RemoveBootlogo())
				Dialog("hekate启动标志已移除，请重启您的主机以应用更改");
		});
	}

	ImGui::NewLine();
	ImGui::TextWrapped("如果您遇到意外报错，请使用以下选项删除与主题相关的一切内容，包括fsmitm文件夹、版本检查sysmodule和主题补丁。");

	if (Utils::ImGuiCenterButton("卸载所有内容", 550))
	{
		PushFunction([]() {
			if (!YesNoPage::Ask(
				"这将删除与自定义主题相关的一切内容，例如:\n"
				"- 当前已安装的主题\n"
				"- 应用主题补丁\n"
				"- 主题更新检查系统模块\n"
				"- 任何自定义Hekate启动标志\n\n"
				"你想继续吗？"
			))
				return;

			DisplayLoading("清除自定义主题数据...");

			bool success = SettingsPage::RemoveSysmodule(false);
			success &= RemoveTheme(true);
			success &= RemovePatches();
			success &= RemoveBootlogo();

			if (success)
			{
				Dialog(
					"已完成，所有与主题相关的内容均已移除，请重启控制台以应用更改。\n"
					"这还会移除所有主页菜单补丁，安装主题前你应该重新启动此应用。"
				);
			}
		});
	}

	ImGui::PopStyleColor();

	ImGui::PopFont();
	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void UninstallPage::Update()
{
	if (Utils::PageLeaveFocusInput()) {
		Parent->PageLeaveFocus(this);
	}
}




