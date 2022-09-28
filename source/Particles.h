#pragma once

#include "MathUtils.h"
#include "Components.h"
#include "ResourceHandles.h"
#include <random>

namespace FlexKit
{   /************************************************************************************************/
	// Forward Declarations
	class Context;
	class GBuffer;
	class FrameGraph;
	class ResourceHandler;

	float3		GetPositionW(NodeHandle node);
	Quaternion	GetOrientation(NodeHandle node);


	/************************************************************************************************/


	struct EmitterProperties
	{
		float maxLife = 10.0f;
		float minEmissionRate = 100;
		float maxEmissionRate = 200;
		float emissionVariance = 0.5f;
		float initialVelocity = 1.0f;
		float initialVariance = 1.0f;

		float emissionSpread = (float)(pi / 4);
		float emissionDirectionVariance = 0.3f;
	};

	class ParticleSystemInterface
	{
	public:
		virtual void Emit(double dT, EmitterProperties& properties, NodeHandle node) = 0;
	};

	struct ParticleEmitterData
	{
		ParticleSystemInterface*	parentSystem = nullptr;
		NodeHandle					node;

		EmitterProperties			properties;

		void Emit(double dT)
		{
			parentSystem->Emit(dT, properties, node);
		}
	};

	struct InitialParticleProperties
	{
		float3 position;
		float3 velocity;
		float  lifespan;
	};




	/************************************************************************************************/


	template<typename TY_ParticleData>
	class ParticleSystem : public ParticleSystemInterface
	{
	public:
		ParticleSystem(iAllocator& allocator) :
			particles{ &allocator } {}


		UpdateTask& Update(double dT, ThreadManager& threads, UpdateDispatcher& dispatcher)
		{
			struct _ {  };

			return dispatcher.Add<_>(
				[this](auto& builder, auto& data){},
				[&, &threads = threads, dT = dT](auto& data, auto& allocator)
				{
					ProfileFunction();

					Vector<TY_ParticleData> prev{ &allocator };
					prev = particles;
					particles.clear();


					for (auto& particle : prev)
						if(auto particleState = TY_ParticleData::Update(particle, dT); particleState)
							particles.push_back(particleState.value());
				});
		}


		void Emit(double dT, EmitterProperties& properties, NodeHandle node)
		{
			const auto minRate  = properties.minEmissionRate;
			const auto maxRate  = properties.maxEmissionRate;
			const auto variance = properties.emissionVariance;
			const float spread  = properties.emissionSpread;

			if (maxRate == minRate && minRate == 0.0f)
				return;

			std::random_device						generator;
			std::uniform_real_distribution<float>	thetaspreadDistro(0.0f, 1.0f);
			std::uniform_real_distribution<float>	gammaspreadDistro(std::lerp(1.0f, 0.0f, spread), 1.0f);
			std::normal_distribution<float>			realDistro(minRate, maxRate);
			std::uniform_real_distribution<float>	velocityDistro( properties.initialVelocity * (1.0f - properties.initialVariance),
																	properties.initialVelocity * (1.0f + properties.initialVariance));
			const auto emitterPOS			= GetPositionW(node);
			const auto emitterOrientation	= GetOrientation(node);
			const auto emissionCount		= realDistro(generator) * dT;

			for (size_t I = 0; I < emissionCount; I++)
			{
				const float theta		= 2 * float(pi) * thetaspreadDistro(generator);
				const float gamma		= lerp(-(float(pi) / 2.0f), (float(pi) / 2.0f), abs(gammaspreadDistro(generator)));

				const float sinTheta	= sin(theta);
				const float cosTheta	= cos(theta);
				const float cosGamma	= cos(gamma);


				const float y_velocity	= sin(gamma);
				const float x_velocity	= sinTheta * cosGamma;
				const float z_velocity	= cosTheta * cosGamma;


				particles.push_back(InitialParticleProperties{
						.position = emitterPOS,
						.velocity = (emitterOrientation * float3{ x_velocity, y_velocity, z_velocity }) * 10,
						.lifespan = properties.maxLife,
					});
			}
		}


		Vector<TY_ParticleData> particles;
	};

	inline static const ComponentID ParticleEmitterID = GetTypeGUID(ParticleEmitterComponent);


	using ParticleEmitterHandle     = Handle_t<32, ParticleEmitterID>;
	using ParticleEmitterComponent  = BasicComponent_t<ParticleEmitterData, ParticleEmitterHandle, ParticleEmitterID>;
	using ParticleEmitterView       = ParticleEmitterComponent::View;

	inline UpdateTask& UpdateParticleEmitters(UpdateDispatcher& dispatcher, double dT)
	{
		struct _ {};

		return dispatcher.Add<_>(
			[](auto&, auto& data){},
			[=](auto&, auto& threadMemory)
			{
				ProfileFunction();

				auto& particleEmitters = ParticleEmitterComponent::GetComponent();

				for (auto& emitter : particleEmitters)
					emitter.componentData.Emit(dT);
			});
	}
}


/**********************************************************************

Copyright (c) 2022 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
