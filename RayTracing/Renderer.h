#pragma once
#include <memory>
#include <Walnut/Image.h>
#include "glm/glm.hpp"
#include "Ray.h"
#include "Camera.h"
#include "Scene.h"
class Renderer
{

private:
	struct HitPayLoad
	{
		float HitDistance;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;
		int ObjectIndex;
	};
public:
	struct Settings
	{
		bool Accumulate = true;
	};
private:
	std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;
	glm::vec4* m_AccumulationData = nullptr;
	uint32_t m_FrameIndex = 1;
	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;
	Settings m_settings;
	std::vector<uint32_t> m_ImageHorizontalIter, m_ImageVerticalIter;

public:
	Renderer() = default;
private:
	glm::vec4 PerPixel(uint32_t x,uint32_t y);

	HitPayLoad TraceRay(const Ray& ray);

	HitPayLoad ClosestHit(const Ray& ray, float hitDistance, int objectIndex);
	HitPayLoad Miss(const Ray& ray);
public:
	void Render(const Scene& scene,const Camera& camera);
	void OnResize(uint32_t width, uint32_t height);
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }
	bool FinalImageExist() const { return m_FinalImage!=nullptr; }
	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& GetSettings() { return m_settings; }
};