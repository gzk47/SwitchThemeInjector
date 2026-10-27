#include "QlaunchPatchPage.hpp"
#include "RemoteInstall/Worker.hpp"
#include "../ViewFunctions.hpp"
#include "../fs.hpp"

class ThemeUpdateDownloader : public RemoteInstall::Worker::BaseWorker {
public:
	struct Result {
		std::string error;
		long httpCode;
		std::vector<u8> data;
	};

	ThemeUpdateDownloader(const std::string& url, Result& r) : BaseWorker({url}, true), OutResult(r) {
		appendUrlToError = false;
		SetLoadingLine("正在检查补丁更新...");
	}

protected:
	void OnComplete() {
		const auto& str = Errors.str();
		if (str.length())
			OutResult.error = str;
		else
			OutResult.data = Results.at(0);
	}

	bool OnFinished(uintptr_t index, long httpCode) override {
		OutResult.httpCode = httpCode;
		return true;
	}

	Result& OutResult;
};

QlaunchPatchPage::QlaunchPatchPage() : IPage("主题补丁") { }

void QlaunchPatchPage::Render(int X, int Y)
{
	Utils::ImGuiSetupPage(this, X, Y);

	ImGui::TextWrapped(
		"从固件9.0开始，主菜单的某些部分需要打补丁才能安装主题。\n"
		"如果您看到此界面，则表示您没有安装固件所需的补丁。"
	);	

	if (PatchMng::QlaunchBuildId() != "")
	{
		ImGui::Text("您的主菜单版本如下 (BuildID) :");
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
		Utils::ImGuiCenterString(PatchMng::QlaunchBuildId());
		ImGui::PopStyleColor();
	}
	else 
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
		ImGui::Text("错误：无法检测到您的主菜单版本");
		ImGui::PopStyleColor();
	}

	if (patchStatus == PatchMng::InstallResult::MissingIps) 
	{		
		ImGui::TextWrapped("此版本当前不受支持，新固件发布后补丁更新可能需要几天时间");
		ImGui::TextWrapped(
			"新补丁现在会在您启动此应用程序时自动从互联网下载。 "
			"如果需要，您也可以立即检查更新。"
		);
		
		if (ImGui::Button("检查更新"))
			PushFunction([this]() { CheckForUpdates(); });

		if (updateMessageString != "")
		{
			ImGui::SameLine();

			if (updateMessageIsError)
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
			else ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
			
			ImGui::TextWrapped(updateMessageString.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::TextWrapped(
			"如果您不想将主机连接到互联网，可以通过以下网址的说明手动下载补丁："
		);
		
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
		ImGui::Text("https://github.com/exelix11/theme-patches");
		ImGui::PopStyleColor();
	}
	else if (patchStatus == PatchMng::InstallResult::SDError)
	{
		ImGui::TextWrapped(
			"读取或写入SD卡文件时出错，这通常表示您的SD卡已损坏。\n"
			"请运行存档位修复工具，如果仍然无法工作，请格式化SD卡并重新设置。"
		);
	}
	else if (patchStatus == PatchMng::InstallResult::UnsupportedCFW)
	{
		ImGui::TextWrapped(
			"您的CFW似乎不受支持。\n"
			"如果您的CFW受支持但仍然看到此消息，可能是SD卡有问题，请重新安装您的CFW。"
		);
	}
	else if (patchStatus == PatchMng::InstallResult::Ok)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Highlight);
		ImGui::Text("更新成功，重启您的主机！");
		ImGui::PopStyleColor();
	}

	Utils::ImGuiSetWindowScrollable();
	Utils::ImGuiCloseWin();
}

void QlaunchPatchPage::Update()
{
	if (Utils::PageLeaveFocusInput())
		Parent->PageLeaveFocus(this);
}

void QlaunchPatchPage::CheckForUpdates() {
	ThemeUpdateDownloader::Result res;
	PushPageBlocking(new ThemeUpdateDownloader("https://exelix11.github.io/theme-patches/ips/" + PatchMng::QlaunchBuildId(), res));

	if (res.error != "")
	{
		updateMessageIsError = true;
		updateMessageString = res.error;
	}
	else if (res.httpCode == 404)
	{
		updateMessageIsError = false;
		updateMessageString = "未找到更新";
	}
	else if (res.httpCode != 200)
	{
		updateMessageIsError = true;
		updateMessageString = "HTTP错误：代码" + res.httpCode;
	}
	else
	{
		updateMessageIsError = false;
		fs::patches::WritePatchForBuild(PatchMng::QlaunchBuildId(), res.data);
		patchStatus = PatchMng::EnsureInstalled();
		updateMessageString = "更新成功，重启您的主机！";
	}
}

bool QlaunchPatchPage::ShouldShow()
{
	patchStatus = PatchMng::EnsureInstalled();

	if (patchStatus == PatchMng::InstallResult::Ok)
		return false;

	if (patchStatus == PatchMng::InstallResult::MissingIps)
	{
		CheckForUpdates();
		// Has anything changed ? 
		if (patchStatus == PatchMng::InstallResult::Ok)
			return false;
	}
	
	return true;
}
