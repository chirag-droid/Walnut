#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "TriangleLayer.h"

Walnut::Application *Walnut::CreateApplication(int argc, char **argv) {
    Walnut::ApplicationSpecification spec;
    spec.Name = "Vulkan Triangle";

    Walnut::Application* app = new Walnut::Application(spec);
    app->PushLayer<TriangleLayer>();
    app->SetMenubarCallback([app]() {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                app->Close();
            }
            ImGui::EndMenu();
        }
        });
    return app;
}
