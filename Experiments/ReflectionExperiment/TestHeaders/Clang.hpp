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
	PROPERTY("Min: 0.0f; Max: 0.0f") float	width;
	PROPERTY("Max: 0.0f; Max: 0.0f") float	height;
	PROPERTY("Max: 0; Max: 0") IMAINT INT;
};

using HelloWorldComponent = BasicComponent_t<HelloWorld, 1>;

struct HelloWorld2 : public ComponentBase<HelloWorld2>
{
	PROPERTYFIELDTYPE(HelloWorld);

	HelloWorld* data;
};

