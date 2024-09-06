#include <cstdint>
#include <vector>

#include "BuildSettings.hpp"
#include <Containers.hpp>

template<typename TY, int i = 0>
struct ComponentBase {};

template<typename TY, int i = 0>
struct BasicComponent_t {};

#define PROPERTY(AA)		__attribute__((annotate(##AA)))
#define PROPERTYFIELD		__attribute__((annotate("Field")))
#define PROPERTYFIELDTYPE	__attribute__((annotate("Field")))

struct HelloWorld
{   //asdf
	PROPERTY("Min: 0.0f; Max: 0.0f")	uint32_t			width;
	PROPERTY("Max: 0.0f; Max: 0.0f")	uint32_t			height;
	PROPERTY("MaxSize 1024")			std::vector<int>		items0;
	PROPERTY("MaxSize 1024")			FlexKit::Vector<int>	items1;
};

using HelloWorldComponent = BasicComponent_t<HelloWorld, 1>;

/*
#include "Header.hpp"

#define PROPERTY(AA) __attribute__((annotate("Property(" ##AA ")")))
#define PROPERTYFIELD __attribute__((annotate("Field")))
#define PROPERTYFIELDTYPE __attribute__((annotate("Field")))

template<typename TY, int i = 0>
struct ComponentBase {};

template<typename TY, int i = 0>
struct BasicComponent_t {};

struct HelloWorld
{   //asdf
	PROPERTY("Min: 0.0f; Max: 0.0f") uint32_t width;
	PROPERTY("Max: 0.0f; Max: 0.0f") uint32_t height;
	PROPERTY("Max: 0; Max: 0") IMAINT INT;
};

using HelloWorldComponent = BasicComponent_t<HelloWorld, 1>;

struct HelloWorld2 : public ComponentBase<HelloWorld2>
{
	PROPERTYFIELDTYPE(HelloWorld);

	HelloWorld* data;
};
*/


