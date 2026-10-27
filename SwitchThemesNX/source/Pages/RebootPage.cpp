#include <string>
#include "../UI/UI.hpp"

#include "../Platform/Platform.hpp"
#include "../ViewFunctions.hpp"

class RebootPage : public IPage
{
	public:
		RebootPage()
		{
			Name = "重启";
		}
		
		void Render(int X, int Y) 
		{
			Utils::ImGuiSetupPage(this, X, Y);
			ImGui::PushFont(font30);
			ImGui::SetCursorPos({ 5, 10 });

			ImGui::TextUnformatted("重启您的游戏主机将应用您所做的更改。");
			ImGui::TextWrapped("这是系统重启选项的快捷方式。如果您的自定义固件不提供重启到payload功能，您将需要一种从RCM注入payload的方法。");
			if (ImGui::Button("重启"))
			{
				PlatformReboot();
			}
			PAGE_RESET_FOCUS;
			
			ImGui::PopFont();
			Utils::ImGuiCloseWin();
		}
		
		void Update() override
		{
			if (Utils::PageLeaveFocusInput())
				Parent->PageLeaveFocus(this);
		}
};