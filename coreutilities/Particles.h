#include "MathUtils.h"
#include "Components.h"


namespace FlexKit
{
    class ParticleSystemInterface
    {
    public:
    };

    template<typename TY_ParticleData>
    class ParticleSystem : public ParticleSystemInterface
    {
    public:
        ParticleSystem(iAllocator& allocator) :
            particles{ &allocator } {}

        UpdateTask& Update(double dT, ThreadManager& threads, UpdateDispatcher& dispatcher)
        {
            struct _ {};

            return dispatcher.Add<_>(
                [](auto& builder, auto& data)
                {
                },
                [&threads = threads](auto& data, auto& allocator)
                {
                });

            Vector<TY_ParticleData> particles;
        }
    };


    inline static const ComponentID ParticleEmitterID = GetTypeGUID(SpellComponent);

    struct ParticleEmitterData
    {
    };

    using ParticleEmitterHandle     = Handle_t<32, ParticleEmitterID>;
    using ParticleEmitterComponent  = BasicComponent_t<ParticleEmitterData, ParticleEmitterHandle, ParticleEmitterID>;
    using ParticleEmitterView       = ParticleEmitterComponent::View;
}


/**********************************************************************

Copyright (c) 2021 Robert May

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
