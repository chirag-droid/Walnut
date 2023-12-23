#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"

class ImGuiDemoLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::ShowDemoWindow();
	}
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "ImGui Demo";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ImGuiDemoLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}
