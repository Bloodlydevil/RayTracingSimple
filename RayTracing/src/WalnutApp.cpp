#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"
#include "../Renderer.h"
#include "../Camera.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
private :
	Renderer m_renderer;
	Camera m_camera;
	Scene m_scene;
	uint32_t m_ViewportWidth=0;
	uint32_t m_ViewportHeight=0;
	float m_LastRenderTime = 0;
public:
	ExampleLayer()
		:m_camera(450.f, 0.1f, 100.0f)
	{
		Material& pinkSphere=m_scene.Materals.emplace_back();
		pinkSphere.Albedo = { 1.0f,0.0f,1.0f };
		pinkSphere.Roughness = 0;

		Material& blueSphere = m_scene.Materals.emplace_back();
		blueSphere.Albedo = { 0.2f,0.3f,1.0f };
		blueSphere.Roughness = 0;

		Material& orangeSphere = m_scene.Materals.emplace_back();
		orangeSphere.Albedo = { 0.8f,0.5f,0.2f };
		orangeSphere.Roughness = 0;
		orangeSphere.EmissionColor = orangeSphere.Albedo;
		orangeSphere.EmissionPower = 2;
		{
			Sphere sphere;
			sphere.Position = glm::vec3(0, 0, 0);
			sphere.Radius = 1;
			sphere.MatIndex = 0;
			m_scene.Spheres.push_back(sphere);
		}
		{
			Sphere sphere;
			sphere.Position = glm::vec3(2, 0, 0);
			sphere.Radius = 1;
			sphere.MatIndex = 2;
			m_scene.Spheres.push_back(sphere);
		}
		{
			Sphere sphere;
			sphere.Position = glm::vec3(0, -101, 0);
			sphere.Radius = 100;
			sphere.MatIndex = 1;

			m_scene.Spheres.push_back(sphere);
		}
	}
private:
	void Render()
	{
		Timer timer;
		
		m_renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_renderer.Render(m_scene,m_camera);
		m_LastRenderTime = timer.ElapsedMillis();
	}
public:
	virtual void OnUpdate(float ts) override
	{
		if (m_camera.OnUpdate(ts))
		{
			m_renderer.ResetFrameIndex();
		}
	}
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Last Render: %.3fms", m_LastRenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}

		ImGui::Checkbox("Accumulate", &m_renderer.GetSettings().Accumulate);

		if (ImGui::Button("Reset"))
			m_renderer.ResetFrameIndex();
		ImGui::End();

		ImGui::Begin("Scene");
		for (size_t i = 0;i < m_scene.Spheres.size();i++)
		{
			Sphere& sphere = m_scene.Spheres[i];
			ImGui::PushID(i);
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
			ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
			ImGui::DragInt("Material", &sphere.MatIndex, 1, 0, (int)m_scene.Materals.size()-1);
			ImGui::Separator();
			ImGui::PopID();
		}
		
		for (size_t i = 0;i < m_scene.Materals.size();i++)
		{
			ImGui::PushID(i);
			Material& material = m_scene.Materals[i];
			ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
			ImGui::DragFloat("Roughness", &material.Roughness, 0.001f, 0, 1);
			ImGui::DragFloat("Metallic", &material.Metallic, 0.001f, 0, 1);
			ImGui::ColorEdit3("Emission", glm::value_ptr(material.EmissionColor));
			ImGui::DragFloat("EmissionPower", &material.EmissionPower, 0.001f, 0,FLT_MAX);
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight=ImGui::GetContentRegionAvail().y;
		auto image = m_renderer.GetFinalImage();
		if(image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(),(float)image->GetWidth() },
				ImVec2(0,1),ImVec2(1,0));
		ImGui::End();
		ImGui::PopStyleVar();
		Render();
	}
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Walnut Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
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