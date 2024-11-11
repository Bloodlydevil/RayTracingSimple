#include "Renderer.h"
#include "Walnut/Random.h"
#include <iostream>
#include <execution>
namespace Utils {
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) |r;

		return result;
	}

	static uint32_t PCG_Hash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}
	static float RandomFloat(uint32_t& seed)
	{
		seed = PCG_Hash(seed);
		return (float)seed / (float)UINT32_MAX;
	}
	static glm::vec3 InUnitSphere(uint32_t& seed)
	{
		return glm::normalize(glm::vec3(RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f,
			RandomFloat(seed) * 2.0f - 1.0f));
	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage )
	{
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;
			m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}
	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];
	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0;i < width;i++)
		m_ImageHorizontalIter[i] = i;
	for (uint32_t i = 0;i < height;i++)
		m_ImageVerticalIter[i] = i;
}
glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirection()[x + y * m_FinalImage->GetWidth()];

	glm::vec3 light(0.0f);
	glm::vec3 contribution(1.0f);

	uint32_t seed = x + y * m_FinalImage->GetWidth();
	seed *= m_FrameIndex;

	int bounces = 50;
	for (int i = 0;i < bounces;i++)
	{
		seed += i;
		Renderer::HitPayLoad payLoad = TraceRay(ray);

		if (payLoad.HitDistance < 0.0f)
		{
			glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
			//light += skyColor * contribution;
			break;
		}

		//glm::vec3 lightDirection = glm::normalize(glm::vec3(-1, -1, -1));
		//float lightIntensity = glm::max(glm::dot(payLoad.WorldNormal, -lightDirection), 0.0f);

		const Sphere& sphere = m_ActiveScene->Spheres[payLoad.ObjectIndex];
		const Material& material = m_ActiveScene->Materals[sphere.MatIndex];

		//glm::vec3 sphereColor = material.Albedo;
		//sphereColor *= lightIntensity;
		light += material.GetEmission()* material.Albedo;
		contribution *= material.Albedo;

		ray.Origin = payLoad.WorldPosition+payLoad.WorldNormal*0.0001f;
		//ray.Direction = glm::reflect(ray.Direction,
			//payLoad.WorldNormal + material.Roughness * Walnut::Random::Vec3(-0.5f,0.5f));
		ray.Direction =glm::normalize(payLoad.WorldNormal+ Utils::InUnitSphere(seed));
	}

	return glm::vec4(light, 1.0f);
}
Renderer::HitPayLoad Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayLoad payLoad;
	payLoad.HitDistance = hitDistance;
	payLoad.ObjectIndex = objectIndex;


	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];
	

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payLoad.WorldPosition = origin + ray.Direction * hitDistance;
	payLoad.WorldNormal = glm::normalize(payLoad.WorldPosition);

	payLoad.WorldPosition += closestSphere.Position;
	return payLoad;
}
Renderer::HitPayLoad Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayLoad payLoad;
	payLoad.HitDistance = -1.0f;
	return payLoad;
}
void Renderer::Render(const Scene& scene,const Camera& camera)
{
	m_ActiveCamera = &camera;
	m_ActiveScene = &scene;

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0,m_FinalImage->GetHeight()* m_FinalImage->GetWidth()*sizeof(glm::vec4));

#define MT 1
#if MT
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(),m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each( m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this,y](uint32_t x)
				{
					glm::vec4 color = PerPixel(x, y);
					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

					glm::vec4 accumlatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumlatedColor /= m_FrameIndex;
					//std::cout << accumlatedColor.r<<"," << accumlatedColor.g << "," << accumlatedColor.b<< "\n";
					accumlatedColor = glm::clamp(accumlatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumlatedColor);
				});
		});
#else
	for (uint32_t y = 0;y < m_FinalImage->GetHeight();y++)
	{
		for (uint32_t x = 0;x < m_FinalImage->GetWidth();x++)
		{
			glm::vec4 color = PerPixel(x,y);
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += color;

			glm::vec4 accumlatedColor = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
			accumlatedColor /= m_FrameIndex;
			//std::cout << accumlatedColor.r<<"," << accumlatedColor.g << "," << accumlatedColor.b<< "\n";
			accumlatedColor = glm::clamp(accumlatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(accumlatedColor);
		}
	}
#endif
	m_FinalImage->SetData(m_ImageData);
	if (m_settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}
Renderer::HitPayLoad Renderer::TraceRay(const Ray& ray)
{
	int closestSphere = -1;
	float hitDistance = FLT_MAX;
	for (size_t i=0;i< m_ActiveScene->Spheres.size();i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discrimenent = b * b - 4.0f * a * c;

		if (discrimenent < 0.0f)
			continue;
		//float t0 = (-b + glm::sqrt(discrimenent)) / (2.0f * a);
		//glm::vec3 h0 = rayOrigin + rayDirection * t0;
		float ClosestT = (-b - glm::sqrt(discrimenent)) / (2.0f * a);
		if (ClosestT>0.0f&& ClosestT < hitDistance)
		{
			hitDistance = ClosestT;
			closestSphere = i;
		}
	}

	if (closestSphere <0)
	{
		return Miss(ray);
	}
	
	return ClosestHit(ray, hitDistance, closestSphere);
}